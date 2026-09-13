# Darknet Prototype — Especificação de Infraestrutura de Produção

> **Documento de handoff para o programador de infraestrutura.**
> Status do backend hoje: funciona em dev local (`ws://127.0.0.1:9000`), login é stub,
> sem TLS. Este documento define o alvo de produção e o caminho em fases.
> Contato técnico do jogo: ver `server/README.md` (tabela de endpoints) e `src/NetClient.*`.

---

## 1. Arquitetura atual (já existe no repo)

```
server/
├── game-server/        Node.js 20+ (Express + ws + Stripe + pg)
│   ├── src/index.js    API REST + WebSocket + webhook Stripe (~600 linhas)
│   ├── Dockerfile      imagem publicada no GHCR pelo CI
│   └── package.json
├── gateway/nginx.conf  nginx reverso (bloco HTTPS está COMENTADO — ativar)
├── db/init.sql         schema Postgres
└── docker-compose.yml  gateway + game-server + db(postgres:16-alpine) + volume
```

- Imagem de container: `ghcr.io/omegasoftdll/darknet-prototype/game-server`
  (CI `.github/workflows/docker-publish.yml` já publica a cada push em `server/game-server/**`).
- Protocolos do cliente: REST/JSON (HTTP) e WebSocket RFC6455 na rota `/ws`
  (implementação própria em Winsock no cliente — sem biblioteca).
- **Importante**: o servidor de jogo é um *relay* (salas/posições/chat), NÃO autoritativo.
  Isso é uma decisão aceita para o lançamento — não tente transformá-lo em
  servidor autoritativo sem requisito explícito do product owner.

## 2. Topologia alvo (Fase 1 — lançamento)

```
                 Internet
                     │
        ┌────────────┴────────────┐
        │  nginx (gateway)        │  TLS termina aqui (443/80→443)
        │  - /api/* → game-server │
        │  - /ws    → game-server │  (upgrade WebSocket, wss://)
        └───────┬────────────────┘
                │ rede docker interna
        ┌───────┴────────┐      ┌──────────────┐
        │ game-server    │──────│ Postgres 16  │
        │ (1 réplica)    │      │ (volume+backup│
        └────────────────┘      └──────────────┘
   Porta 9000 NUNCA exposta publicamente — só na rede interna.
```

## 3. Ambientes

| Ambiente | Finalidade | Stripe | Domínio sugerido |
|---|---|---|---|
| `staging` | testes de integração/deploy | chaves **test** | `staging-api.<dominio>` |
| `prod` | jogadores reais | chaves **live** | `api.<dominio>` |

- Mesma imagem de container nos dois ambientes; diferença só em variáveis de ambiente.

## 4. Computação e deploy (Fase 1)

- **1 VPS com docker-compose** é suficiente para o lançamento
  (referência: Hetzner CX22 / DigitalOcean 2vCPU-4GB — relay WS é leve, suporta
  centenas de conexões simultâneas).
- Deploy da imagem: pull do GHCR no host + `docker compose up -d`
  (Watchtower ou GitHub Action com SSH para automatizar).
- Reinício automático (`restart: unless-stopped`) em todos os serviços.
- Escalonamento horizontal (réplicas + Redis pub/sub para rotear salas WS) é **Fase 3**,
  somente quando CCU justificar.

## 5. DNS e domínio

- Comprar domínio dedicado (ex.: `<jogo>.com` — decisão de marca pendente;
  "Darknet" é nome genérico e concorrido, avaliar alternativa antes de registrar).
- Registros: `A api.<dominio>` → IP do VPS; idêntico para `staging-api`.
- Ativar proteção de privacidade WHOIS.

## 6. TLS / HTTPS / WSS — **obrigatório antes de qualquer jogador real**

- [ ] Certificado válido (Let's Encrypt via certbot, renovação automática).
- [ ] `server/gateway/nginx.conf`: **descomentar o bloco 443** (está comentado, linhas ~54-78),
      redirecionar 80→443, TLS 1.2+, HSTS.
- [ ] Rota `/ws` com upgrade headers (`Upgrade`, `Connection`) no nginx.
- [ ] Endpoint público de WS: `wss://api.<dominio>/ws`.
- [ ] Nota A+ no SSL Labs como critério de aceite.
- **Dependência de desenvolvimento (não-infra)**: o cliente C++ hoje **rejeita `wss://`**
  (`NetClient.cpp`). O time de jogo precisa adicionar TLS ao cliente (Schannel no Windows
  é o caminho natural) ou aceitar TLS só no nginx com proxy não-cifrado interno —
  definir responsável antes do go-live.

## 7. Banco de dados

- Postgres 16 (já no compose). Preferência: **managed** (Neon/Railway/AWS RDS free/low tier)
  para backups automáticos; se self-hosted no VPS, obrigatório:
- [ ] Volume dedicado + **backup automático diário** (`pg_dump` agendado) com retenção de 30 dias.
- [ ] **Teste de restore documentado e executado ao menos uma vez** (backup não testado = não existe).
- [ ] Credenciais por variável de ambiente, nunca no `init.sql` em claro.

## 8. Segredos (nenhum no git)

| Segredo | Uso | Onde |
|---|---|---|
| `JWT_SECRET` | assinatura de tokens | env do container (o compose tem default falha-segura — **substituir obrigatoriamente**) |
| `STRIPE_SECRET_KEY` | API Stripe | env, diferente por ambiente |
| `STRIPE_WEBHOOK_SECRET` | validação de assinatura do webhook | env |
| `DATABASE_URL` | conexão Postgres | env |
| `PUBLIC_URL` | URL pública da API (já usada no código) | env |

- Gerar com `openssl rand -hex 32`; armazenar em gerenciador de senhas do time
  (o dono do projeto usa KeePass2).

## 9. Stripe (monetização de gems)

- [ ] Webhook `checkout.session.completed` apontando para `https://api.<dominio>/webhook/stripe`
      (a validação de assinatura já está implementada no código — o webhook assinado é a
      única fonte de crédito, não o retorno do cliente).
- [ ] Chaves **test** no staging; validar fluxo de compra de gems ponta a ponta antes do live.
- [ ] O cliente só abre URLs `https://checkout.stripe.com/` / `https://buy.stripe.com/`
      (validação adicionada no código — manter).

## 10. Autenticação — **dependência de desenvolvimento crítica**

- `/auth/login` é **stub** hoje (qualquer nome gera token válido). Antes do go-live:
- [ ] Login real: email+senha com bcrypt (biblioteca já no projeto) ou OAuth (Steam/Google).
- [ ] Rate limit estrito em `/auth/*` (existe rate limiting geral — confirmar cobertura das rotas de auth).
- [ ] `/auth/register` restrito conforme política (hoje aberto — definir: aberto com
      verificação de email, ou convite).

## 11. Hardening do host

- [ ] Firewall: apenas 80/tcp, 443/tcp, 22/tcp (SSH **só com chave**, porta alternativa opcional).
- [ ] Fal2ban no SSH e no nginx.
- [ ] fail2ban + fail2ban-nginx ou equivalente.
- [ ] Usuário não-root para o docker; sudo apenas para manutenção.
- [ ] Atualizações de segurança automáticas do SO (unattended-upgrades).
- [ ] Scan de vulnerabilidades da imagem (Trivy) no CI — adicionar etapa ao workflow existente.
- [ ] CORS restrito aos domínios oficiais.

## 12. Observabilidade

- [ ] Endpoint de saúde: `GET /healthz` (responder 200 com versão do build).
- [ ] Uptime externo gratuito (Better Stack / UptimeRobot) monitorando `/healthz` e o WSS,
      alerta em <1 min de downtime.
- [ ] Logs estruturados (JSON) para stdout; rotação com retenção de 30 dias.
- [ ] Error tracking: Sentry (free tier) no game-server.
- [ ] Métricas mínimas no painel: conexões WS ativas, latência, erros 5xx, uso de CPU/RAM/disco.
- [ ] Runbook mínimo: como reiniciar, como rollback de versão, como restaurar backup.

## 13. LGPD (dados de brasileiros)

- [ ] Política de privacidade pública (dados coletados: email, nome, progresso).
- [ ] Endpoint de exclusão de conta/dados (`DELETE /account`) com remoção em cadastro.
- [ ] Consentimento no registro.

## 14. Estimativa de custo mensal (Fase 1)

| Item | Custo |
|---|---|
| VPS 2vCPU/4GB | US$ 5–20 |
| Domínio | US$ 1/ano (amortizado) |
| Managed Postgres (ou mesmo VPS) | US$ 0–19 |
| Backups (storage) | US$ 1–5 |
| Uptime/Sentry (free tiers) | US$ 0 |
| **Total** | **~US$ 10–45/mês** |

## 15. Critérios de aceite (Definition of Done)

1. `https://api.<dominio>/healthz` responde 200 com a versão do build em staging e prod.
2. SSL Labs grau A+ nos dois ambientes; HTTP redireciona para HTTPS.
3. Cliente de teste conecta em `wss://api.<dominio>/ws`, entra em sala e troca posições.
4. Compra de gems com cartão **de teste** credita gems (webhook validado).
5. Login inválido é recusado; token inválido não abre WS (401 no upgrade).
6. Backup diário configurado + restore testado e documentado.
7. Nenhum segredo em git/imagens (`docker inspect` limpo); `JWT_SECRET` != default.
8. Alerta de uptime disparando para o responsável; runbook entregue.
9. Porta 9000 inacessível externamente (testar do lado de fora).

## 16. Higiene imediata (antes de começar)

- [ ] Remover do repo: `server/game-server/server.log`, `server.log.err`,
      `bot_report.txt`, `saves/darknet_slot0.txt` (logs/saves não devem ser versionados —
      adicionar ao `.gitignore` e ao histórico com `git rm --cached`).
- [ ] Confirmar `.env`/segredos no `.gitignore` e nunca commitados.

## 17. Fora de escopo (não fazer sem requisito explícito)

- Servidor autoritativo de gameplay (anti-cheat de progresso).
- Kubernetes/managed complexo (Fase 3, quando CCU > ~1.000).
- Redis (só quando houver >1 réplica de game-server).
- Matchmaking ranqueado.

## 18. Caminho alternativo: Steamworks (decisão pendente do PO)

A Valve NÃO hospeda servidor de jogo, mas oferece **Steam Networking Sockets /
Steam Datagram Relay (SDR)**: conexão P2P roteada pelos relays da Valve — para
o co-op de sessões pequenas do Darknet isso ELIMINA o servidor dedicado na build Steam.

| Função | Build Steam | Build standalone/itch.io |
|---|---|---|
| Multiplayer | Steam lobbies + SDR (P2P com relay) | Servidor Node deste repo (relay WS) |
| Gems/pagamentos | **Steam Wallet (obrigatório — a Valve proíbe checkout externo)** | Stripe (webhook já implementado) |
| Auth | SteamID | JWT (após `/auth/login` virar real) |

Implicações:
- A build Steam exige reescrita da camada de rede para a Steamworks API
  (o stub `integration/steam/SteamIntegration.cpp` já existe no repo para isso).
- **Escopo desta spec se o PO escolher Steam-first**: reduz a DNS+TLS+segredos
  para a build standalone apenas; a build Steam não usa este servidor.
- **Decisão alinhada com o roadmap vigente**: o lançamento é single-player offline
  (nenhum servidor necessário); esta spec serve para o momento em que o co-op entrar.

---

*Versão 1.1 — 2026-09-13. Aprovado pelo product owner. Fase 1 alvo: lançamento itch.io.*
