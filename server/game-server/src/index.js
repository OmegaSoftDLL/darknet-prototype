// ─────────────────────────────────────────────────────────────────────────────
// DARKNET — CYBER STATION (game server)
// Auth (JWT) + shop premium (gems) + Stripe (payment real) + inventory +
// synchronization in time real (WebSocket authenticated) + matchmaking by rooms.
//
// Payments: the UNICA fonte of verdade to creditar gems is the WEBHOOK assinado of the
// Stripe (checkout.session.completed / payment_intent.succeeded). O client NUNCA
// credita gems by account own. O webhook is idempotent (provider_ref single).
//
// Configuration by environment:
//   PORT                 door HTTP/WS (default 9000)
//   JWT_SECRET           segredo of the tokens (FAILURE in docker-compose without ele via
//                        ${JWT_SECRET:?}; without env roda in dev with segredo random
//                        by boot — the tokens existentes are invalidados in the restart)
//   STRIPE_SECRET_KEY    habilita Stripe real (Checkout Session)
//   STRIPE_WEBHOOK_SECRET valid the assinatura of the webhook
//   PUBLIC_URL           base p/ success/cancel of the Checkout (default http://localhost:8080,
//                        door publica of the gateway nginx — atras of proxy, SEM ela the URLs
//                        of return of the Stripe apontam for the door interna errada)
//   DATABASE_URL         habilita persistence in Postgres (otherwise, memory)
//   ALLOW_DEV_GRANT=1    habilita POST /store/dev-grant-gems (APENAS tests locais)
//
// WebSocket: the upgrade only is accepted with JWT valid in the header Authorization
// (`Authorization: Bearer <token>`), sent in the handshake pelo client. A
// identidade (peer/chat) VEM DO TOKEN, never of the body of the message — not ha spoof
// of id. Besides: heartbeat 30s (expulsa clientes mortos), rate limit by
// connection (120 msgs/s) and bodies of message limitados (4 KiB).
// ─────────────────────────────────────────────────────────────────────────────
import express from "express";
import { WebSocketServer } from "ws";
import http from "http";
import jwt from "jsonwebtoken";
import crypto from "crypto";
import bcrypt from "bcryptjs";

const PORT       = process.env.PORT || 9000;
// Sem JWT_SECRET, generates um segredo random by boot. Nunca um default fixed:
// um "dev-secret" conhecido permitiria FORJAR tokens of qualquer player.
// Em production the docker-compose failure before go up without JWT_SECRET definido.
const JWT_SECRET = process.env.JWT_SECRET || crypto.randomBytes(48).toString("hex");
if (!process.env.JWT_SECRET) {
  console.warn("[auth] JWT_SECRET not definido — segredo random generated to ESTA execution. " +
               "Restart the server invalid the tokens atuais. Defina JWT_SECRET in production.");
}
// Default: door publica of the gateway nginx (8080), NOT the door interna of the app (9000).
// Without the proxy (dev direct in the Node), defina PUBLIC_URL=http://localhost:9000.
// Em production atras of TLS in the gateway, use https:// and ajuste the door.
const PUBLIC_URL = process.env.PUBLIC_URL || "http://localhost:8080";

// ── Saneamento of input (never confie in the client) ────────────────────────────
const cleanName = (s) =>
  String(s ?? "").replace(/[^\x20-\x7e]/g, "").trim().slice(0, 24) || "Operador";

const clamp = (n, lo, hi) => Math.min(hi, Math.max(lo, n));

// Position of world: finita and inside of um intervalo plausivel of the map.
const coord = (v) => {
  const n = Number(v);
  return Number.isFinite(n) ? clamp(Math.round(n * 10) / 10, -100000, 100000) : 0;
};
const uint = (v, lo = 0, hi = 0xffffffff) => {
  const n = Number.isInteger(v) ? v : parseInt(String(v), 10);
  return (Number.isInteger(n) && n >= lo && n <= hi) ? n : lo;
};

// ── Rate limit simple in memory (window fixa per IP+route) ──────────────────
const rateHits = new Map();
function rateLimit(ms, max) {
  return (req, res, next) => {
    const key = (req.ip || req.socket.remoteAddress || "?") + req.path;
    const now = Date.now();
    let and = rateHits.get(key);
    if (!and || now - and.t > ms) { and = { t: now, n: 0 }; rateHits.set(key, and); }
    and.n++;
    if (and.n > max) return res.status(429).json({ error: "many requests (limit of taxa)" });
    next();
  };
}
setInterval(() => rateHits.clear(), 60_000).unref();

// ── Stripe (loaded dinamicamente only if houver chave) ──────────────────────
let stripe = null;
if (process.env.STRIPE_SECRET_KEY) {
  try {
    const Stripe = (await import("stripe")).default;
    stripe = new Stripe(process.env.STRIPE_SECRET_KEY);
    console.log("[stripe] habilitado (payment real)");
  } catch (and) {
    console.warn("[stripe] packet ausente — rode 'npm install'. Payment real desabilitado.");
  }
} else {
  console.log("[stripe] STRIPE_SECRET_KEY ausente — modo dev (without cobranca real)");
}

// ── Postgres opcional (otherwise, state in memory) ─────────────────────────────
let pool = null;
if (process.env.DATABASE_URL) {
  try {
    const pg = await import("pg");
    pool = new pg.default.Pool({ connectionString: process.env.DATABASE_URL });
    // Esquema normalizado — FONTE UNICA DE VERDADE of the DDL (CREATE TABLE IF NOT EXISTS
    // ensures the schema same without the server/db/init.sql, that only roda in the 1o boot of the volume).
    await pool.query(`CREATE TABLE IF NOT EXISTS accounts (
      id TEXT PRIMARY KEY, name TEXT NOT NULL, email TEXT NOT NULL UNIQUE, pass_hash TEXT NOT NULL,
      gems INTEGER NOT NULL DEFAULT 0, inv_slots INTEGER NOT NULL DEFAULT 40,
      created_at TIMESTAMPTZ NOT NULL DEFAULT now())`);
    // Migracao: removes accounts orfas of the stub antigo (without email/password real).
    await pool.query(`DELETE FROM accounts WHERE email IS NULL OR pass_hash IS NULL`);
    await pool.query(`CREATE TABLE IF NOT EXISTS inventory (
      id BIGSERIAL PRIMARY KEY, account TEXT REFERENCES accounts(id),
      item_id TEXT NOT NULL, qty INTEGER NOT NULL DEFAULT 1)`);
    // Migracao idempotent: bancos antigos guardavam 1 line by item (qty=1).
    // Consolida duplicatas in qty to power create the indice single (account,item_id).
    await pool.query(`UPDATE inventory i SET qty = s.total FROM (
      SELECT account, item_id, COUNT(*)::int AS total
      FROM inventory GROUP BY account, item_id) s
      WHERE i.account = s.account AND i.item_id = s.item_id`);
    await pool.query(`DELETE FROM inventory the USING inventory b
      WHERE the.account = b.account AND the.item_id = b.item_id AND the.id > b.id`);
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
  } catch (and) {
    console.warn("[db] failure to the connect Postgres — usando memory:", and.message);
    pool = null;
  }
}

const memPlayers = new Map(); // fallback in memory

async function getPlayer(id, name = "Operador") {
  if (pool) {
    const acc = await pool.query("SELECT id,name,gems FROM accounts WHERE id=$1", [id]);
    if (!acc.rows.length) {
      await pool.query("INSERT INTO accounts(id,name,gems) VALUES($1,$2,0)", [id, name]);
      return { id, name, gems: 0, inventory: [] };
    }
    // Expande qty: the inventory in memory is uma list plana of item_ids.
    const inv = await pool.query("SELECT item_id, qty FROM inventory WHERE account=$1", [id]);
    const p = acc.rows[0];
    const inventory = [];
    for (const r of inv.rows) for (let i = 0; i < r.qty; i++) inventory.push(r.item_id);
    return { id: p.id, name: p.name, gems: p.gems, inventory };
  }
  if (!memPlayers.has(id)) memPlayers.set(id, { id, name, gems: 0, inventory: [] });
  const p = memPlayers.get(id);
  // Nunca expoe hash of password in the respostas autenticadas.
  return { id: p.id, name: p.name, email: p.email, gems: p.gems, inventory: p.inventory || [] };
}

async function savePlayer(p) {
  if (pool) {
    await pool.query("UPDATE accounts SET name=$2, gems=$3 WHERE id=$1", [p.id, p.name, p.gems]);
    // Sincroniza inventory agregando the list plana of item_ids in the coluna qty:
    // 1 line by (account,item_id) — without DELETE+re-INSERT of tudo the cada purchase.
    const counts = {};
    for (const itemId of p.inventory) counts[itemId] = (counts[itemId] || 0) + 1;
    const itemIds = Object.keys(counts);
    // Removes items that sairam of the inventory (with list vazia, apaga tudo of the account).
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
    try { saveJson = JSON.parse(data.save_json); } catch { /* tag invalid vira {} */ }
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
// Atras of the gateway nginx, usa X-Forwarded-For p/ rate limit per IP real.
app.set("trust proxy", true);

// ── Catalogo of the shop ─────────────────────────────────────────────────────────
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
    { id: "pet_drone",     name: "Drone of Estimacao",  gems: 400, type: "cosmetic" },
    { id: "boost_xp_7d",   name: "Boost XP 7 dias",     gems: 300, type: "boost" },
    { id: "inv_slots_20",  name: "+20 Slots Inventory",gems: 200, type: "account" },
  ],
};

// IMPORTANTE: the webhook of the Stripe precisa of the body BRUTO (raw) to validate the
// assinatura. Por isso ele is montado ANTES of the express.json() global.
const server = http.createServer(app);

// Idempotencia of the webhook: same provider_ref processed only credita UMA vez
// (Stripe reenvia eventos; some can chegar duplicados by retry).
const webhookSeen = new Set(); // dedupe in memory (modo dev without Postgres)

app.post("/store/webhook", express.raw({ type: "*/*" }), async (req, res) => {
  let event;
  if (stripe && process.env.STRIPE_WEBHOOK_SECRET) {
    try {
      event = stripe.webhooks.constructEvent(
        req.body, req.headers["stripe-signature"], process.env.STRIPE_WEBHOOK_SECRET);
    } catch (err) {
      console.warn("[webhook] assinatura invalid:", err.message);
      return res.status(400).send(`Webhook Error: ${err.message}`);
    }
  } else {
    // Sem Stripe configurado, recusa (not creditar without verificacao).
    return res.status(400).json({ error: "webhook not configurado" });
  }

  try {
    if (event.type === "checkout.session.completed" ||
        event.type === "payment_intent.succeeded") {
      const obj  = event.data.object;
      const meta = obj.metadata || {};
      const ref  = obj.id || "";
      if (meta.userId && meta.gems && ref) {
        // Guards anti-duplicidade: checks if the provider_ref already went processed.
        if (pool) {
          const dup = await pool.query(
            "SELECT 1 FROM transactions WHERE provider='stripe' AND provider_ref=$1", [ref]);
          if (dup.rows.length) {
            console.log(`[webhook] ${ref} already processed — ignorando duplicata`);
            return res.json({ received: true, duplicate: true });
          }
        } else if (webhookSeen.has(ref)) {
          console.log(`[webhook] ${ref} already processed (memoria) — ignorando duplicata`);
          return res.json({ received: true, duplicate: true });
        }
        webhookSeen.add(ref);

        const gems = Math.min(parseInt(meta.gems, 10) || 0, 1000000000);
        await creditGems(meta.userId, gems);
        // Transaction record (payment audit) in the normalized schema.
        if (pool) {
          try {
            await pool.query(
              `INSERT INTO transactions(account,provider,provider_ref,pack_id,amount_cents,currency,status)
               VALUES($1,'stripe',$2,$3,$4,$5,'paid')`,
              [meta.userId, ref, meta.packId || "",
               obj.amount_total || obj.amount || 0, (obj.currency || "brl").toUpperCase()]);
          } catch (and) { console.warn("[webhook] failure to the register transacao:", and.message); }
        }
      }
    }
  } catch (and) {
    console.error("[webhook] error to the creditar:", and.message);
  }
  res.json({ received: true });
});

// A partir daqui, JSON normal.
app.use(express.json());

// ── Auth real: and-mail + password + bcrypt ──────────────────────────────────────
// Rate limit: 20 tentativas/min per IP (mitiga strength bruta and spam of cadastro).
const BCRYPT_ROUNDS = 12;

function isValidEmail(s) {
  return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(String(s));
}

async function findAccountByEmail(email) {
  const and = String(email).toLowerCase().trim();
  if (pool) {
    const r = await pool.query("SELECT id,name,pass_hash FROM accounts WHERE email=$1", [and]);
    return r.rows[0] || null;
  }
  for (const p of memPlayers.values()) if (p.email === and) return p;
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
    if (!isValidEmail(email)) return res.status(400).json({ error: "and-mail invalid" });
    if (password.length < 8) return res.status(400).json({ error: "password very curta (minimum 8 caracteres)" });
    const existing = await findAccountByEmail(email);
    if (existing) return res.status(409).json({ error: "and-mail already cadastrado" });
    const acc = await createAccount(name, email, password);
    const token = jwt.sign({ id: acc.id, name: acc.name }, JWT_SECRET, { expiresIn: "30d" });
    console.log(`[auth] register: ${acc.email} -> ${acc.id}`);
    res.status(201).json({ token, id: acc.id, name: acc.name });
  } catch (and) {
    console.error("[auth] register error:", and.message);
    res.status(500).json({ error: "error internal" });
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
  } catch (and) {
    console.error("[auth] login error:", and.message);
    res.status(500).json({ error: "error internal" });
  }
});

function auth(req, res, next) {
  try {
    const t = (req.headers.authorization || "").replace("Bearer ", "");
    req.user = jwt.verify(t, JWT_SECRET);
    next();
  } catch { res.status(401).json({ error: "not authenticated" }); }
}

// ── Shop ──────────────────────────────────────────────────────────────────
app.get("/store", (_req, res) => res.json(STORE));

// Buy GEMS with money real -> creates Stripe Checkout Session and devolve the URL.
// Os gems only are creditados pelo WEBHOOK when the payment for confirmed.
app.post("/store/buy-gems", rateLimit(60_000, 30), auth, async (req, res) => {
  const pack = STORE.gemPacks.find(p => p.id === (req.body || {}).packId);
  if (!pack) return res.status(400).json({ error: "packet invalid" });

  if (!stripe) {
    return res.json({
      ok: false, needsPaymentProvider: true,
      message: "Configure STRIPE_SECRET_KEY in the server to habilitar payment real.",
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
      // O webhook usa estes metadados to saber quem and the creditar.
      metadata: { userId: req.user.id, gems: String(pack.gems), packId: pack.id },
      success_url: `${PUBLIC_URL}/store/success?session_id={CHECKOUT_SESSION_ID}`,
      cancel_url:  `${PUBLIC_URL}/store/cancel`,
    });
    res.json({ ok: true, url: session.url });
  } catch (and) {
    console.error("[stripe] error to the create checkout:", and.message);
    res.status(500).json({ error: "failure to the create payment" });
  }
});

// Gastar gems in item — server valid saldo (NUNCA confiar in the client).
app.post("/store/buy-item", rateLimit(60_000, 30), auth, async (req, res) => {
  const p = await getPlayer(req.user.id);
  const item = STORE.items.find(i => i.id === (req.body || {}).itemId);
  if (!item) return res.status(400).json({ error: "item invalid" });
  if (p.gems < item.gems) return res.status(402).json({ error: "gems insuficientes" });
  p.gems -= item.gems;
  p.inventory.push(item.id);
  await savePlayer(p);
  res.json({ ok: true, gems: p.gems, inventory: p.inventory });
});

// DEV-ONLY: credita gems without payment, to testar the fluxo localmente.
if (process.env.ALLOW_DEV_GRANT === "1") {
  app.post("/store/dev-grant-gems", rateLimit(60_000, 10), auth, async (req, res) => {
    const amount = uint(req.body && req.body.amount, 0, 1000000);
    const gems = await creditGems(req.user.id, amount);
    res.json({ ok: true, gems });
  });
  console.log("[dev] /store/dev-grant-gems HABILITADO (somente tests)");
}

app.get("/me", auth, async (req, res) => res.json(await getPlayer(req.user.id)));

app.get("/progress", auth, async (req, res) => {
  try {
    const prog = await getPlayerProgress(req.user.id);
    res.json(prog);
  } catch (err) {
    console.error("[progress] failure to the buscar progress:", err.message);
    res.status(500).json({ error: "failure to the buscar progress" });
  }
});

app.post("/progress", rateLimit(60_000, 60), auth, async (req, res) => {
  try {
    const { level, credits, char_class, save_json } = req.body || {};
    await savePlayerProgress(req.user.id, { level, credits, char_class, save_json });
    res.json({ success: true });
  } catch (err) {
    console.error("[progress] failure to the save progress:", err.message);
    res.status(500).json({ error: "failure to the save progress" });
  }
});

app.get("/healthz", (_req, res) => res.json({
  ok: true, service: "cyber-station",
  stripe: !!stripe, db: pool ? "postgres" : "memory",
}));
app.get("/store/success", (_req, res) => res.send("Payment concluido! Volte to the game."));
app.get("/store/cancel",  (_req, res) => res.send("Payment canceled."));

// ── Realtime: synchronization of players + matchmaking by rooms ─────────────
// Authenticated in the UPGRADE (header Authorization). Com noServer, all upgrade that
// not pass in the jwt.verify is refused with 401 before same of open the socket.
const wss = new WebSocketServer({ noServer: true, maxPayload: 1024 * 1024 });

const WS_HEARTBEAT_MS     = 30_000; // the cada 30s sends ping; without pong -> expulsa
const WS_MAX_MSG_PER_SEC  = 120;    // above disso the message is descartada (dropa)

server.on("upgrade", (req, socket, head) => {
  if (!req.url || !req.url.startsWith("/ws")) { socket.destroy(); return; }
  const token = (req.headers.authorization || "").replace(/^Bearer\s+/i, "");
  let user = null;
  if (token) {
    try { user = jwt.verify(token, JWT_SECRET); } catch { /* token invalid/vencido */ }
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
  ws.user      = user;   // identidade of the JWT — fonte of the verdade p/ peer/chat
  ws.roomId    = "lobby";
  ws.msgWindow = Date.now();
  ws.msgCount  = 0;
  console.log(`[ws] cliente conectado (${user.id})`);

  joinRoom(ws, "lobby");

  ws.on("pong", () => { ws.isAlive = true; });
  ws.on("error", () => {}); // error of mesh TCP not derruba the process

  ws.on("message", (buf) => {
    if (buf.length > 4096) return;                    // body alem of the saneamento
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
      case "state": // position/acao -> retransmite p/ the room (id comes of the TOKEN)
        broadcast(ws.roomId,
          { t: "peer", id: ws.user.id, x: coord(msg.x), y: coord(msg.y),
            the: String(msg.the || "i").slice(0, 4), c: uint(msg.c, 0, 5),
            n: ws.user.name || ws.user.id }, ws);
        break;
      case "chat":
        const text = String(msg.text || "").slice(0, 200).replace(/[\r\n]/g, " ");
        if (text.trim()) broadcast(ws.roomId, { t: "chat", id: ws.user.id, text }, ws);
        break;
      case "edeath":  // enemy defeated -> retransmite p/ the room (evita "fantasmas")
        // Cooperativo LAN: qualquer player authenticated is host of the entidades that
        // saw. id = id of the ENTIDADE of the enemy (not is identidade of player).
        broadcast(ws.roomId, { t: "edeath", id: uint(msg.id) }, ws);
        break;
      case "espawn":  // enemy spawnado (host autoritativo) -> demais clientes espelham
        broadcast(ws.roomId,
          { t: "espawn", id: uint(msg.id), et: uint(msg.et), x: coord(msg.x), y: coord(msg.y) }, ws);
        break;
    }
  });
  ws.on("close", () => leaveRoom(ws));
});

// Heartbeat of the server: client that not responde pong in ~30s is expulso.
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

server.listen(PORT, () => console.log(`[CYBER STATION] online in the porta ${PORT}`));