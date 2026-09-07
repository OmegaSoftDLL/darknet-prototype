// Testes automatizados do auth real (email + senha + bcrypt).
// Inicia o game-server em porta isolada, registra, loga e verifica falhas.
import { spawn } from "child_process";
import path from "path";
import { fileURLToPath } from "url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PORT = 19000 + Math.floor(Math.random() * 1000);
const BASE = `http://127.0.0.1:${PORT}`;

function post(route, body) {
  return fetch(BASE + route, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
}

function get(route, token) {
  return fetch(BASE + route, {
    headers: { Authorization: `Bearer ${token}` },
  });
}

async function run() {
  const server = spawn("node", [path.join(__dirname, "game-server/src/index.js")], {
    env: { ...process.env, PORT: String(PORT), JWT_SECRET: "test-secret-48-bytes-long-string-abc123" },
    stdio: "pipe",
  });

  // Aguarda servidor subir (polling em /store).
  await new Promise((resolve, reject) => {
    server.stdout.on("data", (d) => {});
    server.stderr.on("data", (d) => console.error("[server err]", d.toString().trim()));
    let attempts = 0;
    const tryReady = async () => {
      attempts++;
      try {
        const r = await fetch(BASE + "/store");
        if (r.status === 200) return resolve();
      } catch {}
      if (attempts > 50) return reject(new Error("timeout ao subir servidor"));
      setTimeout(tryReady, 200);
    };
    tryReady();
  });

  let passed = 0, failed = 0;
  const check = async (name, fn) => {
    try {
      await fn();
      console.log(`✅ ${name}`);
      passed++;
    } catch (e) {
      console.log(`❌ ${name}: ${e.message}`);
      failed++;
    }
  };

  const email = `test${Date.now()}@darknet.local`;
  const password = "senhaSegura123";
  let token = null;

  await check("registro com credenciais validas", async () => {
    const r = await post("/auth/register", { email, password, name: "Testador" });
    if (r.status !== 201) throw new Error(`status ${r.status}`);
    const j = await r.json();
    if (!j.token || !j.id) throw new Error("faltam token/id");
  });

  await check("login com credenciais validas", async () => {
    const r = await post("/auth/login", { email, password });
    if (r.status !== 200) throw new Error(`status ${r.status}`);
    const j = await r.json();
    if (!j.token || !j.id) throw new Error("faltam token/id");
    token = j.token;
  });

  await check("GET /me retorna dados sem pass_hash", async () => {
    const r = await get("/me", token);
    if (r.status !== 200) throw new Error(`status ${r.status}`);
    const j = await r.json();
    if (j.pass_hash) throw new Error("pass_hash exposto");
    if (j.email !== email.toLowerCase()) throw new Error("email incorreto");
  });

  await check("login com senha errada falha", async () => {
    const r = await post("/auth/login", { email, password: "errada" });
    if (r.status !== 401) throw new Error(`status ${r.status}`);
  });

  await check("registro de email duplicado falha", async () => {
    const r = await post("/auth/register", { email, password: "outraSenha123" });
    if (r.status !== 409) throw new Error(`status ${r.status}`);
  });

  await check("registro com email invalido falha", async () => {
    const r = await post("/auth/register", { email: "naoemail", password });
    if (r.status !== 400) throw new Error(`status ${r.status}`);
  });

  await check("registro com senha curta falha", async () => {
    const r = await post("/auth/register", { email: `outro${Date.now()}@darknet.local`, password: "123" });
    if (r.status !== 400) throw new Error(`status ${r.status}`);
  });

  await check("login com payload antigo (name) falha", async () => {
    const r = await post("/auth/login", { name: "Testador" });
    if (r.status !== 400) throw new Error(`status ${r.status}`);
  });

  await check("token invalido e rejeitado", async () => {
    const r = await get("/me", "token-invalido");
    if (r.status !== 401) throw new Error(`status ${r.status}`);
  });

  server.kill("SIGTERM");
  console.log(`\nResultado: ${passed} passaram, ${failed} falharam`);
  process.exit(failed > 0 ? 1 : 0);
}

run().catch((e) => { console.error(e); process.exit(1); });
