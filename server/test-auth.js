// Tests automatizados of the auth real (email + password + bcrypt).
// Inicia the game-server in door isolated, registra, loga and checks failures.
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

  // Aguarda server go up (polling in /store).
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
      if (attempts > 50) return reject(new Error("timeout to the go up server"));
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
    } catch (and) {
      console.log(`❌ ${name}: ${and.message}`);
      failed++;
    }
  };

  const email = `test${Date.now()}@darknet.local`;
  const password = "senhaSegura123";
  let token = null;

  await check("register with credenciais validas", async () => {
    const r = await post("/auth/register", { email, password, name: "Testador" });
    if (r.status !== 201) throw new Error(`status ${r.status}`);
    const j = await r.json();
    if (!j.token || !j.id) throw new Error("faltam token/id");
  });

  await check("login with credenciais validas", async () => {
    const r = await post("/auth/login", { email, password });
    if (r.status !== 200) throw new Error(`status ${r.status}`);
    const j = await r.json();
    if (!j.token || !j.id) throw new Error("faltam token/id");
    token = j.token;
  });

  await check("GET /me returns data without pass_hash", async () => {
    const r = await get("/me", token);
    if (r.status !== 200) throw new Error(`status ${r.status}`);
    const j = await r.json();
    if (j.pass_hash) throw new Error("pass_hash exposto");
    if (j.email !== email.toLowerCase()) throw new Error("email incorreto");
  });

  await check("login with password errada failure", async () => {
    const r = await post("/auth/login", { email, password: "errada" });
    if (r.status !== 401) throw new Error(`status ${r.status}`);
  });

  await check("register of email duplicado failure", async () => {
    const r = await post("/auth/register", { email, password: "outraSenha123" });
    if (r.status !== 409) throw new Error(`status ${r.status}`);
  });

  await check("register with email invalid failure", async () => {
    const r = await post("/auth/register", { email: "naoemail", password });
    if (r.status !== 400) throw new Error(`status ${r.status}`);
  });

  await check("register with password curta failure", async () => {
    const r = await post("/auth/register", { email: `other${Date.now()}@darknet.local`, password: "123" });
    if (r.status !== 400) throw new Error(`status ${r.status}`);
  });

  await check("login with payload antigo (name) failure", async () => {
    const r = await post("/auth/login", { name: "Testador" });
    if (r.status !== 400) throw new Error(`status ${r.status}`);
  });

  await check("token invalid and rejeitado", async () => {
    const r = await get("/me", "token-invalid");
    if (r.status !== 401) throw new Error(`status ${r.status}`);
  });

  server.kill("SIGTERM");
  console.log(`\nResultado: ${passed} passaram, ${failed} falharam`);
  process.exit(failed > 0 ? 1 : 0);
}

run().catch((and) => { console.error(and); process.exit(1); });
