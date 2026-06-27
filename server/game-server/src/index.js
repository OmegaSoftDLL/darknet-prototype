// ─────────────────────────────────────────────────────────────────────────────
// DARKNET — CYBER STATION (servidor do jogo)
// Auth (JWT) + loja premium (gems) + Stripe (pagamento real) + inventário +
// sincronização em tempo real (WebSocket) + matchmaking por salas.
//
// Pagamentos: a ÚNICA fonte de verdade para creditar gems é o WEBHOOK assinado do
// Stripe (checkout.session.completed / payment_intent.succeeded). O cliente NUNCA
// credita gems por conta própria.
//
// Configuração por ambiente (todas opcionais — sem elas roda em modo dev/local):
//   PORT                 porta HTTP/WS (default 9000)
//   JWT_SECRET           segredo dos tokens (default dev-secret)
//   STRIPE_SECRET_KEY    habilita Stripe real (Checkout Session)
//   STRIPE_WEBHOOK_SECRET valida a assinatura do webhook
//   PUBLIC_URL           base p/ success/cancel do Checkout (default http://localhost:9000)
//   DATABASE_URL         habilita persistência em Postgres (senão, memória)
//   ALLOW_DEV_GRANT=1    habilita POST /store/dev-grant-gems (APENAS testes locais)
// ─────────────────────────────────────────────────────────────────────────────
import express from "express";
import { WebSocketServer } from "ws";
import http from "http";
import jwt from "jsonwebtoken";

const PORT       = process.env.PORT || 9000;
const JWT_SECRET = process.env.JWT_SECRET || "dev-secret";
const PUBLIC_URL = process.env.PUBLIC_URL || `http://localhost:${PORT}`;

// ── Stripe (carregado dinamicamente só se houver chave) ──────────────────────
let stripe = null;
if (process.env.STRIPE_SECRET_KEY) {
  try {
    const Stripe = (await import("stripe")).default;
    stripe = new Stripe(process.env.STRIPE_SECRET_KEY);
    console.log("[stripe] habilitado (pagamento real)");
  } catch (e) {
    console.warn("[stripe] pacote ausente — rode 'npm install'. Pagamento real desabilitado.");
  }
} else {
  console.log("[stripe] STRIPE_SECRET_KEY ausente — modo dev (sem cobranca real)");
}

// ── Postgres opcional (senão, estado em memória) ─────────────────────────────
let pool = null;
if (process.env.DATABASE_URL) {
  try {
    const pg = await import("pg");
    pool = new pg.default.Pool({ connectionString: process.env.DATABASE_URL });
    // Esquema normalizado (consistente com server/db/init.sql) — accounts + inventory
    // + transactions. Nada de tabela "players" ad-hoc: usamos a arquitetura de producao.
    await pool.query(`CREATE TABLE IF NOT EXISTS accounts (
      id TEXT PRIMARY KEY, name TEXT NOT NULL, email TEXT UNIQUE, pass_hash TEXT,
      gems INTEGER NOT NULL DEFAULT 0, inv_slots INTEGER NOT NULL DEFAULT 40,
      created_at TIMESTAMPTZ NOT NULL DEFAULT now())`);
    await pool.query(`CREATE TABLE IF NOT EXISTS inventory (
      id BIGSERIAL PRIMARY KEY, account TEXT REFERENCES accounts(id),
      item_id TEXT NOT NULL, qty INTEGER NOT NULL DEFAULT 1)`);
    await pool.query(`CREATE TABLE IF NOT EXISTS transactions (
      id BIGSERIAL PRIMARY KEY, account TEXT REFERENCES accounts(id),
      provider TEXT NOT NULL, provider_ref TEXT NOT NULL, pack_id TEXT NOT NULL,
      amount_cents INTEGER NOT NULL, currency TEXT NOT NULL DEFAULT 'BRL',
      status TEXT NOT NULL DEFAULT 'pending', created_at TIMESTAMPTZ NOT NULL DEFAULT now())`);
    await pool.query(`CREATE TABLE IF NOT EXISTS progress (
      account TEXT PRIMARY KEY REFERENCES accounts(id),
      level INTEGER NOT NULL DEFAULT 1,
      credits INTEGER NOT NULL DEFAULT 0,
      char_class INTEGER NOT NULL DEFAULT 0,
      save_json JSONB)`);
    console.log("[db] Postgres conectado (esquema normalizado)");
  } catch (e) {
    console.warn("[db] falha ao conectar Postgres — usando memória:", e.message);
    pool = null;
  }
}

const memPlayers = new Map(); // fallback em memória

async function getPlayer(id, name = "Operador") {
  if (pool) {
    const acc = await pool.query("SELECT id,name,gems FROM accounts WHERE id=$1", [id]);
    if (!acc.rows.length) {
      await pool.query("INSERT INTO accounts(id,name,gems) VALUES($1,$2,0)", [id, name]);
      return { id, name, gems: 0, inventory: [] };
    }
    const inv = await pool.query("SELECT item_id FROM inventory WHERE account=$1", [id]);
    const p = acc.rows[0];
    return { id: p.id, name: p.name, gems: p.gems, inventory: inv.rows.map(r => r.item_id) };
  }
  if (!memPlayers.has(id)) memPlayers.set(id, { id, name, gems: 0, inventory: [] });
  return memPlayers.get(id);
}

async function savePlayer(p) {
  if (pool) {
    await pool.query("UPDATE accounts SET name=$2, gems=$3 WHERE id=$1", [p.id, p.name, p.gems]);
    // Sincroniza inventario (lista simples de item_ids) na tabela normalizada.
    await pool.query("DELETE FROM inventory WHERE account=$1", [p.id]);
    for (const itemId of p.inventory)
      await pool.query("INSERT INTO inventory(account,item_id,qty) VALUES($1,$2,1)", [p.id, itemId]);
  } else {
    memPlayers.set(p.id, p);
  }
}

const memProgress = new Map();

async function getPlayerProgress(accountId) {
  if (pool) {
    const res = await pool.query(
      "SELECT level, credits, char_class, save_json FROM progress WHERE account=$1",
      [accountId]
    );
    if (res.rows.length > 0) {
      const row = res.rows[0];
      return {
        account: accountId,
        level: row.level,
        credits: row.credits,
        char_class: row.char_class,
        save_json: row.save_json
      };
    }
  } else {
    if (memProgress.has(accountId)) {
      return memProgress.get(accountId);
    }
  }
  return {
    account: accountId,
    level: 1,
    credits: 0,
    char_class: 0,
    save_json: {}
  };
}

async function savePlayerProgress(accountId, data) {
  const level = data.level || 1;
  const credits = data.credits || 0;
  const charClass = data.char_class || 0;
  const saveJson = data.save_json || {};

  if (pool) {
    await pool.query(
      `INSERT INTO progress (account, level, credits, char_class, save_json)
       VALUES ($1, $2, $3, $4, $5)
       ON CONFLICT (account)
       DO UPDATE SET level = EXCLUDED.level, credits = EXCLUDED.credits,
                     char_class = EXCLUDED.char_class, save_json = EXCLUDED.save_json`,
      [accountId, level, credits, charClass, saveJson]
    );
  } else {
    memProgress.set(accountId, {
      account: accountId,
      level,
      credits,
      char_class: charClass,
      save_json: saveJson
    });
  }
}

async function creditGems(userId, amount) {
  const p = await getPlayer(userId);
  p.gems += amount;
  await savePlayer(p);
  console.log(`[gems] +${amount} -> ${userId} (total ${p.gems})`);
  return p.gems;
}

const app = express();

// ── Catálogo da loja ─────────────────────────────────────────────────────────
const STORE = {
  gemPacks: [
    { id: "gems_100",  gems: 100,  priceBRL: 4.90 },
    { id: "gems_550",  gems: 550,  priceBRL: 19.90 },
    { id: "gems_1200", gems: 1200, priceBRL: 39.90 },
    { id: "gems_3500", gems: 3500, priceBRL: 99.90 },
  ],
  items: [
    { id: "skin_neon",     name: "Skin Neon",           gems: 250, type: "cosmetic" },
    { id: "skin_dragon",   name: "Skin Dragao",         gems: 600, type: "cosmetic" },
    { id: "pet_drone",     name: "Drone de Estimacao",  gems: 400, type: "cosmetic" },
    { id: "boost_xp_7d",   name: "Boost XP 7 dias",     gems: 300, type: "boost" },
    { id: "inv_slots_20",  name: "+20 Slots Inventario",gems: 200, type: "account" },
  ],
};

// IMPORTANTE: o webhook do Stripe precisa do corpo BRUTO (raw) para validar a
// assinatura. Por isso ele é montado ANTES do express.json() global.
const server = http.createServer(app);

app.post("/store/webhook", express.raw({ type: "*/*" }), async (req, res) => {
  let event;
  if (stripe && process.env.STRIPE_WEBHOOK_SECRET) {
    try {
      event = stripe.webhooks.constructEvent(
        req.body, req.headers["stripe-signature"], process.env.STRIPE_WEBHOOK_SECRET);
    } catch (err) {
      console.warn("[webhook] assinatura invalida:", err.message);
      return res.status(400).send(`Webhook Error: ${err.message}`);
    }
  } else {
    // Sem Stripe configurado, recusa (não creditar sem verificação).
    return res.status(400).json({ error: "webhook nao configurado" });
  }

  try {
    if (event.type === "checkout.session.completed" ||
        event.type === "payment_intent.succeeded") {
      const obj  = event.data.object;
      const meta = obj.metadata || {};
      if (meta.userId && meta.gems) {
        await creditGems(meta.userId, parseInt(meta.gems, 10));
        // Registro de transacao (auditoria de pagamento) no esquema normalizado.
        if (pool) {
          try {
            await pool.query(
              `INSERT INTO transactions(account,provider,provider_ref,pack_id,amount_cents,currency,status)
               VALUES($1,'stripe',$2,$3,$4,$5,'paid')`,
              [meta.userId, obj.id || "", meta.packId || "",
               obj.amount_total || obj.amount || 0, (obj.currency || "brl").toUpperCase()]);
          } catch (e) { console.warn("[webhook] falha ao registrar transacao:", e.message); }
        }
      }
    }
  } catch (e) {
    console.error("[webhook] erro ao creditar:", e.message);
  }
  res.json({ received: true });
});

// A partir daqui, JSON normal.
app.use(express.json());

// ── Auth (stub — em produção: OAuth / e-mail+senha com hash) ─────────────────
app.post("/auth/login", async (req, res) => {
  const { name } = req.body || {};
  const id = "u_" + Math.random().toString(36).slice(2, 10);
  await getPlayer(id, name || "Operador");
  console.log(`[auth] login: ${name || "Operador"} -> ${id}`);
  const token = jwt.sign({ id }, JWT_SECRET, { expiresIn: "30d" });
  res.json({ token, id });
});

function auth(req, res, next) {
  try {
    const t = (req.headers.authorization || "").replace("Bearer ", "");
    req.user = jwt.verify(t, JWT_SECRET);
    next();
  } catch { res.status(401).json({ error: "nao autenticado" }); }
}

// ── Loja ──────────────────────────────────────────────────────────────────
app.get("/store", (_req, res) => res.json(STORE));

// Comprar GEMS com dinheiro real -> cria Stripe Checkout Session e devolve a URL.
// Os gems só são creditados pelo WEBHOOK quando o pagamento for confirmado.
app.post("/store/buy-gems", auth, async (req, res) => {
  const pack = STORE.gemPacks.find(p => p.id === (req.body || {}).packId);
  if (!pack) return res.status(400).json({ error: "pacote invalido" });

  if (!stripe) {
    return res.json({
      ok: false, needsPaymentProvider: true,
      message: "Configure STRIPE_SECRET_KEY no servidor para habilitar pagamento real.",
      pack,
    });
  }

  try {
    const session = await stripe.checkout.sessions.create({
      mode: "payment",
      line_items: [{
        quantity: 1,
        price_data: {
          currency: "brl",
          unit_amount: Math.round(pack.priceBRL * 100),
          product_data: { name: `${pack.gems} Gems — DARKNET` },
        },
      }],
      // O webhook usa estes metadados para saber quem e quanto creditar.
      metadata: { userId: req.user.id, gems: String(pack.gems), packId: pack.id },
      success_url: `${PUBLIC_URL}/store/success?session_id={CHECKOUT_SESSION_ID}`,
      cancel_url:  `${PUBLIC_URL}/store/cancel`,
    });
    res.json({ ok: true, url: session.url });
  } catch (e) {
    console.error("[stripe] erro ao criar checkout:", e.message);
    res.status(500).json({ error: "falha ao criar pagamento" });
  }
});

// Gastar gems em item — servidor valida saldo (NUNCA confiar no cliente).
app.post("/store/buy-item", auth, async (req, res) => {
  const p = await getPlayer(req.user.id);
  const item = STORE.items.find(i => i.id === (req.body || {}).itemId);
  if (!item) return res.status(400).json({ error: "item invalido" });
  if (p.gems < item.gems) return res.status(402).json({ error: "gems insuficientes" });
  p.gems -= item.gems;
  p.inventory.push(item.id);
  await savePlayer(p);
  res.json({ ok: true, gems: p.gems, inventory: p.inventory });
});

// DEV-ONLY: credita gems sem pagamento, para testar o fluxo localmente.
if (process.env.ALLOW_DEV_GRANT === "1") {
  app.post("/store/dev-grant-gems", auth, async (req, res) => {
    const amount = Math.max(0, parseInt((req.body || {}).amount, 10) || 0);
    const gems = await creditGems(req.user.id, amount);
    res.json({ ok: true, gems });
  });
  console.log("[dev] /store/dev-grant-gems HABILITADO (somente testes)");
}

app.get("/me", auth, async (req, res) => res.json(await getPlayer(req.user.id)));

app.get("/progress", auth, async (req, res) => {
  try {
    const prog = await getPlayerProgress(req.user.id);
    res.json(prog);
  } catch (err) {
    console.error("[progress] falha ao buscar progresso:", err.message);
    res.status(500).json({ error: "falha ao buscar progresso" });
  }
});

app.post("/progress", auth, async (req, res) => {
  try {
    const { level, credits, char_class, save_json } = req.body || {};
    await savePlayerProgress(req.user.id, { level, credits, char_class, save_json });
    res.json({ success: true });
  } catch (err) {
    console.error("[progress] falha ao salvar progresso:", err.message);
    res.status(500).json({ error: "falha ao salvar progresso" });
  }
});
app.get("/healthz", (_req, res) => res.json({
  ok: true, service: "cyber-station",
  stripe: !!stripe, db: pool ? "postgres" : "memory",
}));
app.get("/store/success", (_req, res) => res.send("Pagamento concluido! Volte ao jogo."));
app.get("/store/cancel",  (_req, res) => res.send("Pagamento cancelado."));

// ── Realtime: sincronização de jogadores + matchmaking por salas ─────────────
const wss = new WebSocketServer({ server, path: "/ws" });

wss.on("connection", (ws) => {
  ws.roomId = "lobby";
  console.log("[ws] cliente conectado");
  joinRoom(ws, "lobby");
  ws.on("message", (buf) => {
    let msg; try { msg = JSON.parse(buf.toString()); } catch { return; }
    switch (msg.t) {
      case "join":  joinRoom(ws, msg.room || "lobby"); break;
      case "state": // posição/ação -> retransmite p/ a sala (inclui classe e nome)
        broadcast(ws.roomId,
          { t: "peer", id: msg.id, x: msg.x, y: msg.y, a: msg.a, c: msg.c, n: msg.n }, ws);
        break;
      case "chat":
        broadcast(ws.roomId, { t: "chat", id: msg.id, text: String(msg.text).slice(0, 200) });
        break;
    }
  });
  ws.on("close", () => leaveRoom(ws));
});

const rooms = new Map();
function joinRoom(ws, roomId) {
  leaveRoom(ws);
  ws.roomId = roomId;
  if (!rooms.has(roomId)) rooms.set(roomId, new Set());
  rooms.get(roomId).add(ws);
  ws.send(JSON.stringify({ t: "joined", room: roomId, count: rooms.get(roomId).size }));
}
function leaveRoom(ws) {
  const r = rooms.get(ws.roomId);
  if (r) { r.delete(ws); if (r.size === 0) rooms.delete(ws.roomId); }
}
function broadcast(roomId, obj, except) {
  const r = rooms.get(roomId); if (!r) return;
  const data = JSON.stringify(obj);
  for (const c of r) if (c !== except && c.readyState === 1) c.send(data);
}

server.listen(PORT, () => console.log(`[CYBER STATION] online na porta ${PORT}`));
