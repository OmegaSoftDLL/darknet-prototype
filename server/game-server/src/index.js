// ─────────────────────────────────────────────────────────────────────────────
// DARKNET — CYBER STATION (servidor do jogo)
// Auth (JWT) + loja premium (gems) + Stripe (pagamento real) + inventário +
// sincronização em tempo real (WebSocket autenticado) + matchmaking por salas.
//
// Pagamentos: a ÚNICA fonte de verdade para creditar gems é o WEBHOOK assinado do
// Stripe (checkout.session.completed / payment_intent.succeeded). O cliente NUNCA
// credita gems por conta própria. O webhook é idempotente (provider_ref único).
//
// Configuração por ambiente:
//   PORT                 porta HTTP/WS (default 9000)
//   JWT_SECRET           segredo dos tokens (FALHA em docker-compose sem ele via
//                        ${JWT_SECRET:?}; sem env roda em dev com segredo aleatório
//                        por boot — os tokens existentes são invalidados no restart)
//   STRIPE_SECRET_KEY    habilita Stripe real (Checkout Session)
//   STRIPE_WEBHOOK_SECRET valida a assinatura do webhook
//   PUBLIC_URL           base p/ success/cancel do Checkout (default http://localhost:8080,
//                        porta pública do gateway nginx — atrás de proxy, SEM ela os URLs
//                        de retorno do Stripe apontam para a porta interna errada)
//   DATABASE_URL         habilita persistência em Postgres (senão, memória)
//   ALLOW_DEV_GRANT=1    habilita POST /store/dev-grant-gems (APENAS testes locais)
//
// WebSocket: o upgrade só é aceito com JWT válido no header Authorization
// (`Authorization: Bearer <token>`), enviado no handshake pelo cliente. A
// identidade (peer/chat) VEM DO TOKEN, nunca do corpo da mensagem — não há spoof
// de id. Além disso: heartbeat 30s (expulsa clientes mortos), rate limit por
// conexão (120 msgs/s) e corpos de mensagem limitados (4 KiB).
// ─────────────────────────────────────────────────────────────────────────────
import express from "express";
import { WebSocketServer } from "ws";
import http from "http";
import jwt from "jsonwebtoken";
import crypto from "crypto";
import bcrypt from "bcryptjs";

const PORT       = process.env.PORT || 9000;
// Sem JWT_SECRET, gera um segredo aleatorio por boot. Nunca um default fixo:
// um "dev-secret" conhecido permitiria FORJAR tokens de qualquer jogador.
// Em produção o docker-compose falha antes de subir sem JWT_SECRET definido.
const JWT_SECRET = process.env.JWT_SECRET || crypto.randomBytes(48).toString("hex");
if (!process.env.JWT_SECRET) {
  console.warn("[auth] JWT_SECRET nao definido — segredo aleatorio gerado para ESTA execucao. " +
               "Reiniciar o servidor invalida os tokens atuais. Defina JWT_SECRET em producao.");
}
// Default: porta pública do gateway nginx (8080), NÃO a porta interna do app (9000).
// Sem o proxy (dev direto no Node), defina PUBLIC_URL=http://localhost:9000.
// Em producao atrás de TLS no gateway, use https:// e ajuste a porta.
const PUBLIC_URL = process.env.PUBLIC_URL || "http://localhost:8080";

// ── Saneamento de input (nunca confie no cliente) ────────────────────────────
const cleanName = (s) =>
  String(s ?? "").replace(/[^\x20-\x7e]/g, "").trim().slice(0, 24) || "Operador";

const clamp = (n, lo, hi) => Math.min(hi, Math.max(lo, n));

// Posição de mundo: finita e dentro de um intervalo plausível do mapa.
const coord = (v) => {
  const n = Number(v);
  return Number.isFinite(n) ? clamp(Math.round(n * 10) / 10, -100000, 100000) : 0;
};
const uint = (v, lo = 0, hi = 0xffffffff) => {
  const n = Number.isInteger(v) ? v : parseInt(String(v), 10);
  return (Number.isInteger(n) && n >= lo && n <= hi) ? n : lo;
};

// ── Rate limit simples em memória (janela fixa por IP+rota) ──────────────────
const rateHits = new Map();
function rateLimit(ms, max) {
  return (req, res, next) => {
    const key = (req.ip || req.socket.remoteAddress || "?") + req.path;
    const now = Date.now();
    let e = rateHits.get(key);
    if (!e || now - e.t > ms) { e = { t: now, n: 0 }; rateHits.set(key, e); }
    e.n++;
    if (e.n > max) return res.status(429).json({ error: "muitas requisicoes (limite de taxa)" });
    next();
  };
}
setInterval(() => rateHits.clear(), 60_000).unref();

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
    // Esquema normalizado — FONTE ÚNICA DE VERDADE do DDL (CREATE TABLE IF NOT EXISTS
    // garante o schema mesmo sem o server/db/init.sql, que só roda no 1º boot do volume).
    await pool.query(`CREATE TABLE IF NOT EXISTS accounts (
      id TEXT PRIMARY KEY, name TEXT NOT NULL, email TEXT NOT NULL UNIQUE, pass_hash TEXT NOT NULL,
      gems INTEGER NOT NULL DEFAULT 0, inv_slots INTEGER NOT NULL DEFAULT 40,
      created_at TIMESTAMPTZ NOT NULL DEFAULT now())`);
    // Migracao: remove contas orfas do stub antigo (sem email/senha real).
    await pool.query(`DELETE FROM accounts WHERE email IS NULL OR pass_hash IS NULL`);
    await pool.query(`CREATE TABLE IF NOT EXISTS inventory (
      id BIGSERIAL PRIMARY KEY, account TEXT REFERENCES accounts(id),
      item_id TEXT NOT NULL, qty INTEGER NOT NULL DEFAULT 1)`);
    // Migração idempotente: bancos antigos guardavam 1 linha por item (qty=1).
    // Consolida duplicatas em qty para poder criar o índice único (account,item_id).
    await pool.query(`UPDATE inventory i SET qty = s.total FROM (
      SELECT account, item_id, COUNT(*)::int AS total
      FROM inventory GROUP BY account, item_id) s
      WHERE i.account = s.account AND i.item_id = s.item_id`);
    await pool.query(`DELETE FROM inventory a USING inventory b
      WHERE a.account = b.account AND a.item_id = b.item_id AND a.id > b.id`);
    await pool.query(`CREATE UNIQUE INDEX IF NOT EXISTS inventory_account_item_uq
      ON inventory(account, item_id)`);
    await pool.query(`CREATE TABLE IF NOT EXISTS transactions (
      id BIGSERIAL PRIMARY KEY, account TEXT REFERENCES accounts(id),
      provider TEXT NOT NULL, provider_ref TEXT NOT NULL, pack_id TEXT NOT NULL,
      amount_cents INTEGER NOT NULL, currency TEXT NOT NULL DEFAULT 'BRL',
      status TEXT NOT NULL DEFAULT 'pending', created_at TIMESTAMPTZ NOT NULL DEFAULT now())`);
    await pool.query(`CREATE INDEX IF NOT EXISTS transactions_provider_ref_idx
      ON transactions(provider, provider_ref)`);
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
    // Expande qty: o inventário em memória é uma lista plana de item_ids.
    const inv = await pool.query("SELECT item_id, qty FROM inventory WHERE account=$1", [id]);
    const p = acc.rows[0];
    const inventory = [];
    for (const r of inv.rows) for (let i = 0; i < r.qty; i++) inventory.push(r.item_id);
    return { id: p.id, name: p.name, gems: p.gems, inventory };
  }
  if (!memPlayers.has(id)) memPlayers.set(id, { id, name, gems: 0, inventory: [] });
  const p = memPlayers.get(id);
  // Nunca expoe hash de senha nas respostas autenticadas.
  return { id: p.id, name: p.name, email: p.email, gems: p.gems, inventory: p.inventory || [] };
}

async function savePlayer(p) {
  if (pool) {
    await pool.query("UPDATE accounts SET name=$2, gems=$3 WHERE id=$1", [p.id, p.name, p.gems]);
    // Sincroniza inventario agregando a lista plana de item_ids na coluna qty:
    // 1 linha por (account,item_id) — sem DELETE+re-INSERT de tudo a cada compra.
    const counts = {};
    for (const itemId of p.inventory) counts[itemId] = (counts[itemId] || 0) + 1;
    const itemIds = Object.keys(counts);
    // Remove itens que saíram do inventário (com lista vazia, apaga tudo da conta).
    await pool.query(
      "DELETE FROM inventory WHERE account=$1 AND item_id <> ALL($2::text[])",
      [p.id, itemIds]);
    for (const itemId of itemIds) {
      await pool.query(
        `INSERT INTO inventory(account,item_id,qty) VALUES($1,$2,$3)
         ON CONFLICT (account,item_id) DO UPDATE SET qty = EXCLUDED.qty`,
        [p.id, itemId, counts[itemId]]);
    }
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
  const level = uint(data.level, 1, 9999);
  const credits = uint(data.credits, 0, 2000000000);
  const charClass = uint(data.char_class, 0, 63);
  let saveJson = {};
  if (data.save_json && typeof data.save_json === "object" && !Array.isArray(data.save_json)) {
    const s = JSON.stringify(data.save_json);
    if (s.length <= 100000) saveJson = data.save_json;
    else console.warn("[progress] save_json excede 100KB — ignorado");
  } else if (typeof data.save_json === "string" && data.save_json.length <= 100000) {
    try { saveJson = JSON.parse(data.save_json); } catch { /* tag inválida vira {} */ }
  }

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
// Atrás do gateway nginx, usa X-Forwarded-For p/ rate limit por IP real.
app.set("trust proxy", true);

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

// Idempotência do webhook: mesmo provider_ref processado só credita UMA vez
// (Stripe reenvia eventos; alguns podem chegar duplicados por retry).
const webhookSeen = new Set(); // dedupe em memória (modo dev sem Postgres)

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
      const ref  = obj.id || "";
      if (meta.userId && meta.gems && ref) {
        // Guards anti-duplicidade: checa se o provider_ref já foi processado.
        if (pool) {
          const dup = await pool.query(
            "SELECT 1 FROM transactions WHERE provider='stripe' AND provider_ref=$1", [ref]);
          if (dup.rows.length) {
            console.log(`[webhook] ${ref} ja processado — ignorando duplicata`);
            return res.json({ received: true, duplicate: true });
          }
        } else if (webhookSeen.has(ref)) {
          console.log(`[webhook] ${ref} ja processado (memoria) — ignorando duplicata`);
          return res.json({ received: true, duplicate: true });
        }
        webhookSeen.add(ref);

        const gems = Math.min(parseInt(meta.gems, 10) || 0, 1000000000);
        await creditGems(meta.userId, gems);
        // Registro de transacao (auditoria de pagamento) no esquema normalizado.
        if (pool) {
          try {
            await pool.query(
              `INSERT INTO transactions(account,provider,provider_ref,pack_id,amount_cents,currency,status)
               VALUES($1,'stripe',$2,$3,$4,$5,'paid')`,
              [meta.userId, ref, meta.packId || "",
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

// ── Auth real: e-mail + senha + bcrypt ──────────────────────────────────────
// Rate limit: 20 tentativas/min por IP (mitiga forca bruta e spam de cadastro).
const BCRYPT_ROUNDS = 12;

function isValidEmail(s) {
  return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(String(s));
}

async function findAccountByEmail(email) {
  const e = String(email).toLowerCase().trim();
  if (pool) {
    const r = await pool.query("SELECT id,name,pass_hash FROM accounts WHERE email=$1", [e]);
    return r.rows[0] || null;
  }
  for (const p of memPlayers.values()) if (p.email === e) return p;
  return null;
}

async function createAccount(name, email, password) {
  const id = "u_" + crypto.randomBytes(6).toString("hex");
  const hash = await bcrypt.hash(password, BCRYPT_ROUNDS);
  const acc = { id, name, email: String(email).toLowerCase().trim(), gems: 0, inventory: [], pass_hash: hash };
  if (pool) {
    await pool.query(
      "INSERT INTO accounts(id,name,email,pass_hash,gems) VALUES($1,$2,$3,$4,0)",
      [acc.id, acc.name, acc.email, acc.pass_hash]);
  } else {
    memPlayers.set(id, acc);
  }
  return acc;
}

app.post("/auth/register", rateLimit(60_000, 20), async (req, res) => {
  try {
    const body = req.body || {};
    const email = String(body.email || "").trim();
    const password = String(body.password || "");
    const name = cleanName(body.name);
    if (!isValidEmail(email)) return res.status(400).json({ error: "e-mail invalido" });
    if (password.length < 8) return res.status(400).json({ error: "senha muito curta (minimo 8 caracteres)" });
    const existing = await findAccountByEmail(email);
    if (existing) return res.status(409).json({ error: "e-mail ja cadastrado" });
    const acc = await createAccount(name, email, password);
    const token = jwt.sign({ id: acc.id, name: acc.name }, JWT_SECRET, { expiresIn: "30d" });
    console.log(`[auth] register: ${acc.email} -> ${acc.id}`);
    res.status(201).json({ token, id: acc.id, name: acc.name });
  } catch (e) {
    console.error("[auth] register error:", e.message);
    res.status(500).json({ error: "erro interno" });
  }
});

app.post("/auth/login", rateLimit(60_000, 20), async (req, res) => {
  try {
    const body = req.body || {};
    const email = String(body.email || "").trim();
    const password = String(body.password || "");
    if (!isValidEmail(email) || !password) return res.status(400).json({ error: "credenciais invalidas" });
    const acc = await findAccountByEmail(email);
    if (!acc) return res.status(401).json({ error: "credenciais invalidas" });
    const ok = await bcrypt.compare(password, acc.pass_hash);
    if (!ok) return res.status(401).json({ error: "credenciais invalidas" });
    const token = jwt.sign({ id: acc.id, name: acc.name }, JWT_SECRET, { expiresIn: "30d" });
    console.log(`[auth] login: ${email} -> ${acc.id}`);
    res.json({ token, id: acc.id, name: acc.name });
  } catch (e) {
    console.error("[auth] login error:", e.message);
    res.status(500).json({ error: "erro interno" });
  }
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
app.post("/store/buy-gems", rateLimit(60_000, 30), auth, async (req, res) => {
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
app.post("/store/buy-item", rateLimit(60_000, 30), auth, async (req, res) => {
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
  app.post("/store/dev-grant-gems", rateLimit(60_000, 10), auth, async (req, res) => {
    const amount = uint(req.body && req.body.amount, 0, 1000000);
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

app.post("/progress", rateLimit(60_000, 60), auth, async (req, res) => {
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
// Autenticado no UPGRADE (header Authorization). Com noServer, todo upgrade que
// não passar no jwt.verify é recusado com 401 antes mesmo de abrir o socket.
const wss = new WebSocketServer({ noServer: true, maxPayload: 1024 * 1024 });

const WS_HEARTBEAT_MS     = 30_000; // a cada 30s envia ping; sem pong -> expulsa
const WS_MAX_MSG_PER_SEC  = 120;    // acima disso a mensagem é descartada (dropa)

server.on("upgrade", (req, socket, head) => {
  if (!req.url || !req.url.startsWith("/ws")) { socket.destroy(); return; }
  const token = (req.headers.authorization || "").replace(/^Bearer\s+/i, "");
  let user = null;
  if (token) {
    try { user = jwt.verify(token, JWT_SECRET); } catch { /* token invalido/vencido */ }
  }
  if (!user) {
    socket.write("HTTP/1.1 401 Unauthorized\r\n\r\n");
    socket.destroy();
    return;
  }
  wss.handleUpgrade(req, socket, head, (ws) => wss.emit("connection", ws, req, user));
});

wss.on("connection", (ws, _req, user) => {
  ws.isAlive   = true;
  ws.user      = user;   // identidade do JWT — fonte da verdade p/ peer/chat
  ws.roomId    = "lobby";
  ws.msgWindow = Date.now();
  ws.msgCount  = 0;
  console.log(`[ws] cliente conectado (${user.id})`);

  joinRoom(ws, "lobby");

  ws.on("pong", () => { ws.isAlive = true; });
  ws.on("error", () => {}); // erro de malha TCP não derruba o processo

  ws.on("message", (buf) => {
    if (buf.length > 4096) return;                    // corpo além do saneamento
    const now = Date.now();
    if (now - ws.msgWindow >= 1000) { ws.msgWindow = now; ws.msgCount = 0; }
    if (++ws.msgCount > WS_MAX_MSG_PER_SEC) return;   // flood -> descarta

    let msg; try { msg = JSON.parse(buf.toString()); } catch { return; }
    switch (msg && msg.t) {
      case "join": {
        const room = String(msg.room || "lobby").replace(/[^\w-]/g, "").slice(0, 32) || "lobby";
        joinRoom(ws, room);
        break;
      }
      case "state": // posição/ação -> retransmite p/ a sala (id vem do TOKEN)
        broadcast(ws.roomId,
          { t: "peer", id: ws.user.id, x: coord(msg.x), y: coord(msg.y),
            a: String(msg.a || "i").slice(0, 4), c: uint(msg.c, 0, 5),
            n: ws.user.name || ws.user.id }, ws);
        break;
      case "chat":
        const text = String(msg.text || "").slice(0, 200).replace(/[\r\n]/g, " ");
        if (text.trim()) broadcast(ws.roomId, { t: "chat", id: ws.user.id, text }, ws);
        break;
      case "edeath":  // inimigo abatido -> retransmite p/ a sala (evita "fantasmas")
        // Cooperativo LAN: qualquer jogador autenticado é host das entidades que
        // viu. id = id da ENTIDADE do inimigo (não é identidade de jogador).
        broadcast(ws.roomId, { t: "edeath", id: uint(msg.id) }, ws);
        break;
      case "espawn":  // inimigo spawnado (host autoritativo) -> demais clientes espelham
        broadcast(ws.roomId,
          { t: "espawn", id: uint(msg.id), et: uint(msg.et), x: coord(msg.x), y: coord(msg.y) }, ws);
        break;
    }
  });
  ws.on("close", () => leaveRoom(ws));
});

// Heartbeat do servidor: cliente que não responde pong em ~30s é expulso.
const heartbeat = setInterval(() => {
  for (const ws of wss.clients) {
    if (!ws.isAlive) { leaveRoom(ws); ws.terminate(); continue; }
    ws.isAlive = false;
    ws.ping();
  }
}, WS_HEARTBEAT_MS);
heartbeat.unref();

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