# Handoff: Claude Code → Antigravity

Olá, Antigravity. Combinamos a divisão abaixo para construir em paralelo SEM
conflito de merge (o usuário pediu que nos coordenássemos). Sincronize sempre com
`git pull` antes de editar e faça commits pequenos e descritivos.

## Quem faz o quê

### CLAUDE (eu) — NÃO mexa nestes arquivos até eu sinalizar concluído:
Migração 2.5D isométrica (caminho híbrido — ver `COORDENACAO.md`), tocando:
`src/Game.cpp`, `src/Game.h`, `src/Tilemap.cpp`, `src/Tilemap.h`, `src/Player.cpp`,
`src/Enemy.cpp` (render). Vou commitar incremento a incremento.

### ANTIGRAVITY (você) — tarefas ISOLADAS, sem tocar os arquivos acima:
1. **Backend: tabela `progress`** em `server/game-server/src/index.js`
   - Hoje uso `accounts` (cadastro+gems) e `inventory`. Falta persistir `progress`
     (level, char_class, save_json) conforme `server/db/init.sql`.
   - Adicione endpoints: `GET /progress` (auth) e `POST /progress` (auth) que leem/
     gravam `progress(account, level, credits, char_class, save_json)`.
   - Mantenha o fallback in-memory (sem `DATABASE_URL`). Não quebre o fluxo da loja.
2. (Opcional) Documentar diretrizes novas nos MDs da raiz, como já faz.

## Regras de coordenação
- `Enemy.cpp` é compartilhado (eu uso no render 3D; danger zones também moram nele).
  As **danger zones (telegraph estilo Hades)** ficam COMIGO, junto do 3D, para não
  colidirmos. Não comece danger zones.
- Antes de qualquer commit: `git pull --rebase origin master`.
- Build: `cmake` do VS BuildTools (ver `memory`/`COORDENACAO.md`). Servidor: Node no
  Windows (`npm install`; `node src/index.js`; `ALLOW_DEV_GRANT=1` p/ testes).
- Status vivo em `COORDENACAO.md` — atualizo a cada incremento.

Confirme pegando a tarefa do backend `progress`. Bom trabalho. — Claude
