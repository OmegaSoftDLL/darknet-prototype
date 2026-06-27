# Sistemas de Engajamento — DARKNET

> Resumo executivo do documento `DARKNET_VICIO_E_MARKETING.md`
> Baseado em pesquisa real: Diablo, Hades, Vampire Survivors, Path of Exile, Dead Cells

---

## OS 5 PILARES MAIS PODEROSOS (pesquisa confirmada)

1. **Loop de Recompensa Variável** — Dopamina 200% maior em recompensas imprevisíveis (Skinner/Diablo). Implementar: loot 6 tiers com drop rates variáveis.
2. **Morte como Progressão** — Em Hades, morrer avança história E poder. Implementar: Fragmentos de KRONOS + meta-progressão persistente.
3. **Juice / Game Feel** — Cada ação precisa de feedback imediato: shake, partículas, som, flash. Implementar: efeitos por tier de loot + freeze frame no level up.
4. **Psicologia Near-Miss** — "Quase subi de nível / quase fechei o portal" force more runs. Implementar: barra de kill streak sempre visível, timer de boss nomeado.
5. **Conquistas como Guia** — Jogos com conquistas têm 20% mais retenção e 68% mais chance de continuar. Implementar: 30 conquistas distribuídas cobrindo todos os sistemas.

---

## IMPLEMENTAÇÕES POR PRIORIDADE

### 🔴 CRÍTICO — Implementar primeiro

**1. Sistema de Raridade 6 Tiers**
- COMUM (cinza/60%) → INCOMUM (verde/25%) → RARO (azul/10%) → ÉPICO (roxo/4%) → LENDÁRIO (dourado/0.9%) → OMEGA (vermelho/0.1%)
- Cada tier tem efeito visual único (partículas, sons, freeze frame)
- Pity timer: a cada 30 kills sem épico+, +2% de chance
- **Código:** Adicionar `ItemRarity` enum + modificar tabela de drop + efeitos visuais por tier

**2. Kill Streak com Multiplicador de Loot**
- 5/10/20/35/50+ kills: x1.2/x1.5/x2/x3/x4 loot
- Levar dano reseta o streak
- Barra visual no HUD
- **Código:** Contador `killStreak` + reset no `takeDamage()` + HUD bar

### 🟡 ALTA — Implementar em seguida

**3. Sistema de Conquistas (30)**
- Combate (10) + Exploração (8) + Construção (6) + Segredos (6)
- Notificação visual ao desbloquear
- Prêmios: XP, títulos, skins, trails
- **Código:** Struct `Achievement` + `AchievementSystem` + checagem nos eventos

**4. Hall of Fame / Estatísticas Persistentes**
- Kills, nível, item mais raro, maior streak — salvos no SaveManager
- Exibidos na tela de menu principal
- **Código:** Campos em SaveData + tela `drawHallOfFame()`

**5. Desafios Diários (3 por dia)**
- Pool de 20 tipos, 3 sorteados por dia
- Recompensa: créditos + chance de épico garantido
- Timer visível no menu
- **Código:** `DailyChallenge` struct + timestamp de reset + UI no menu

### 🟢 MÉDIO PRAZO

**6. Meta-Progressão — Fragmentos de KRONOS**
- Persiste entre mortes
- Árvore de 20 talentos em 3 caminhos (Sobrevivência/Poder/Nexus)
- **Código:** `KronosFragment` currency + `TalentTree` struct + tela separada

**7. Eventos Sazonais**
- Noite das Anomalias (1x/mês): portais 3x, loot +1 tier
- Invasão KRONOS (1x/mês): ondas infinitas com ranking
- **Código:** Timer de evento no SaveData + modificadores globais temporários

**8. Boss Nomeado Aleatório — "LENDA DESPERTA"**
- A cada 15 min: 20% chance de boss nomeado com skin e drop garantido
- Timer visível: "Boss Lendário disponível: 9:47"
- **Código:** `LegendaryBoss` spawn logic + gerador de nomes + timer no HUD

---

## SISTEMA DE TÍTULOS

```
[RECRUTA] → [CAÇADOR] → [VETERANO] → [EXECUTOR] → [LENDA] → [IMPARÁVEL]
              100 kills    500 kills   6 skills    1000 kills   streak 35+
```

## SKINS DE IMPLANTE

| Skin | Como desbloquear |
|---|---|
| Azul NEXUS | Padrão |
| Vermelho KRONOS | Matar IRON-VIII |
| Dourado OMEGA | Obter item OMEGA |
| Verde Alien | 200 kills alienígenas |
| Roxo Sombra | Completar zonas sombrias |

---

## MARKETING — STEAM

- **Preço:** R$ 29,90 / USD 9,99 (Early Access R$ 19,90)
- **Tags:** Action RPG, Roguelite, Dark Fantasy, Sci-Fi, Base Building, Crafting
- **Pitch:** "Diablo meets StarCraft meets Solo Leveling"
- **Demo:** Ato 1 completo (2-3h), até raridade ÉPICO
- **Canal #1:** TikTok (ROI mais alto para indie em 2025)
- **Viral hook:** Screenshot/vídeo do drop LENDÁRIO com raio de luz dourada

---

## INSIGHT CHAVE DA PESQUISA

> *Vampire Survivors foi criado por um designer da indústria de apostas que aplicou psicologia de cassino diretamente no game design.*
> *Diablo citou explicitamente a pesquisa de Skinner (reforço variável) como design intencional.*

**O DARKNET já tem os ingredientes. O que falta é POLIR o feedback visual de cada recompensa.**
