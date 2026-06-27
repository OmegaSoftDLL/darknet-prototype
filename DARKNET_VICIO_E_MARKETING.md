# DARKNET — Guia de Engajamento, Vício Saudável e Estratégia Comercial

> Documento criado por Agente Especialista em Game Design Psicológico  
> Baseado em análise de Diablo, Hades, Path of Exile, Vampire Survivors, Dead Cells

---

## PARTE 1 — OS 10 PILARES PSICOLÓGICOS DO VÍCIO SAUDÁVEL

### Pilar 1: LOOP DE RECOMPENSA VARIÁVEL (Dopamina)
**Como funciona:** O cérebro libera dopamina com mais intensidade quando a recompensa é *imprevisível*. Um baú que sempre dá 10 moedas é menos viciante que um que às vezes dá 10 e às vezes dá 1000.  
**Exemplo:** Diablo — você nunca sabe se o próximo drop vai ser lixo ou lendário.  
**No DARKNET:** Loot em 6 tiers com drop rates variáveis. O OMEGA drop (0.1%) deve ser espetacular o suficiente para o jogador contar para amigos.

### Pilar 2: PROGRESSÃO VISÍVEL + SENSAÇÃO DE PODER
**Como funciona:** O jogador precisa sentir que ficou mais forte desde a última sessão. A progressão deve ser *perceptível*, não só numérica.  
**Exemplo:** Path of Exile — a cada liga o personagem começa do zero mas o jogador já sabe exatamente o que quer construir.  
**No DARKNET:** Visual do personagem muda por caminho de evolução (Cyborg/Hacker/Executor). O mapa tem marcas dos inimigos derrotados.

### Pilar 3: "JUST ONE MORE RUN" — O GANCHO DE SESSÃO
**Como funciona:** O jogo sempre termina com o jogador *a beira* de algo — quase subiu de nível, quase fechou todos os portais, quase craftou o item.  
**Exemplo:** Hades — você sempre morre com algum recurso que vai desbloquear algo novo.  
**No DARKNET:** Timer de desafio diário visível, boss lendário que só aparece por 10 minutos, streak de kills quase ativando loot épico.

### Pilar 4: MASTERY + CURVA DE APRENDIZADO JUSTA
**Como funciona:** O jogador deve sentir que sua morte foi *culpa dele*, não do jogo. Isso cria motivação para tentar de novo.  
**Exemplo:** Dead Cells — cada morte ensina algo. O jogo é difícil mas sempre justo.  
**No DARKNET:** Dificuldade HISTORIA remove a barreira de entrada. APOCALIPSE recompensa maestria com loot exclusivo.

### Pilar 5: IDENTIDADE DO PERSONAGEM
**Como funciona:** Quando o jogador se identifica com o personagem, ele *investe emocionalmente*. Customização amplifica isso.  
**Exemplo:** Path of Exile — builds únicas fazem cada personagem ser "o meu".  
**No DARKNET:** 3 caminhos de evolução com visual único + sistema de títulos + skins de implante.

### Pilar 6: TENSÃO → ALÍVIO (Loop Emocional)
**Como funciona:** Tensão sem alívio é estresse. Alívio sem tensão é tédio. O "flow" está no meio.  
**Exemplo:** Vampire Survivors — a densidade de inimigos escala até ser quase impossível, o power-up seguinte alivia.  
**No DARKNET:** Portais de anomalia criam tensão crescente, fechar o último portal é o alívio catártico.

### Pilar 7: METAS DE CURTO, MÉDIO E LONGO PRAZO
**Como funciona:** O jogador precisa de algo para fazer em 5 minutos, em 1 hora e em 10 horas ao mesmo tempo.  
**Exemplo:** Hades — curto=run atual, médio=desbloquear nova arma, longo=completar a história.  
**No DARKNET:** Curto=desafio diário, médio=evolução de classe, longo=encerrar os 5 atos.

### Pilar 8: SURPRESA E DESCOBERTA
**Como funciona:** Encontrar algo inesperado — item secreto, sala oculta, diálogo inédito — cria memórias positivas fortes.  
**Exemplo:** Diablo — itens únicos com lore próprio que revelam histórias.  
**No DARKNET:** 15 documentos de lore escondidos, conquistas secretas, boss lendário que aparece aleatoriamente.

### Pilar 9: SOCIAL PROOF E COMPARAÇÃO
**Como funciona:** Saber que outros jogadores também estão progredindo (ou que você está à frente) motiva.  
**Exemplo:** Path of Exile — Ladder de personagens mais poderosos.  
**No DARKNET:** Hall of Fame local (sua melhor run), sistema de títulos visíveis, conquistas compartilháveis.

### Pilar 10: SENSO DE PROPRIEDADE E PERDA POTENCIAL
**Como funciona:** "Você construiu uma base, equipou companions, tem um item lendário" — perder isso dói.  
**Exemplo:** Minecraft — você construiu algo que não quer ver destruído.  
**No DARKNET:** Base com Arks e construções que persiste, companions com equipamento único, save de progresso automático.

---

## PARTE 2 — SISTEMAS CONCRETOS A IMPLEMENTAR

### A) SISTEMA DE RARIDADE DE LOOT — 6 TIERS

```
TIER 1 — COMUM (cinza)
  Drop rate: 60%
  Efeito visual: drop simples, sem partículas
  Exemplo de itens: Fragmento de Metal, Chip Danificado, Bateria Gasta
  Stat range: +1 a +5 em qualquer atributo
  Som: click simples

TIER 2 — INCOMUM (verde)
  Drop rate: 25%
  Efeito visual: brilho verde ao pousar no chão
  Exemplo: Lâmina Reforçada, Colete de Fibra, Chip de Processamento
  Stat range: +6 a +15, 1 atributo bônus
  Som: ping suave

TIER 3 — RARO (azul)
  Drop rate: 10%
  Efeito visual: feixe de luz azul subindo do chão, partículas
  Exemplo: Rifle de Precisão MK-II, Armadura Nanofiber, Implante Neural
  Stat range: +16 a +35, 2 atributos bônus
  Som: chime duplo

TIER 4 — ÉPICO (roxo)
  Drop rate: 4%
  Efeito visual: explosão de partículas roxas, anel de energia no chão, câmera leve zoom
  Exemplo: Canhão de Plasma, Exoesqueleto Titan, Núcleo de Fusão
  Stat range: +36 a +60, 3 atributos bônus + 1 efeito especial passivo
  Som: chord musical de 3 notas

TIER 5 — LENDÁRIO (laranja/dourado)
  Drop rate: 0.9%
  Efeito visual: FREEZE FRAME 0.3s, raio de luz dourada do céu, partículas douradas em espiral,
                 nome do item aparece em texto grande no centro da tela, borda dourada pulsante
  Exemplo: Lâmina da Resistência, Armadura NEXUS Elite, Implante KRONOS Capturado
  Stat range: +61 a +100, 4 atributos + 2 efeitos especiais únicos
  Lore: cada lendário tem 2 linhas de história própria
  Som: fanfarra de 5 notas, inconfundível

TIER 6 — OMEGA (vermelho + partículas especiais)
  Drop rate: 0.1% (APENAS OmegaBoss ou Bosses Nomeados Lendários)
  Efeito visual: SLOW-MO 0.1x por 2 segundos, explosão de partículas vermelhas/douradas,
                 "!! ITEM OMEGA !!" em texto gigante vermelho pulsante,
                 ondas de choque circulares, música especial toca
  Exemplo: ESPADA DO EXECUTOR (herdado do lore), ARMADURA OMEGA-VANCE, NÚCLEO DE KRONOS
  Stats: únicos, efeitos quebrados que definem builds inteiras
  Lore: revelação sobre a história do universo
  Son: música de boss + fanfarra épica
```

**REGRA DE OURO:** Sistema de "pity timer" — a cada 30 inimigos sem drop épico+, chance de épico aumenta 2%. Reseta ao dropar. Jogador de 10 minutos SEMPRE encontra algo memorável.

---

### B) "JUST ONE MORE RUN" — SISTEMAS DE MOMENTUM

#### 1. Kill Streak com Multiplicador de Loot
```
5  kills seguidos: Loot Streak x1.2 (barra aparece no HUD)
10 kills seguidos: Loot Streak x1.5 (barra fica amarela)
20 kills seguidos: Loot Streak x2.0 (barra fica laranja, partículas)
35 kills seguidos: Loot Streak x3.0 (barra fica vermelha, "IMPARÁVEL!")
50+ kills:         Loot Streak x4.0 + chance de Boss Nomeado aparecer
```
Levar dano reseta o streak. Isso cria tensão: "não quero levar dano agora!"

#### 2. Boss Nomeado Aleatório — "LENDA DESPERTA"
- A cada 15 minutos de jogo ativo, rola 20% de chance de um Boss Nomeado aparecer
- Boss Nomeado = boss normal com prefixo gerado: "IRON-VIII CORRUPTO — O IMPIEDOSO"
- Tem skin única (cor diferente + partículas especiais)
- Loot garantido: mínimo ÉPICO + 30% de LENDÁRIO
- Timer de 10 minutos: "Boss Nomeado disponível: 9:47" no HUD
- Se jogador não matar no tempo: boss some, oportunidade perdida

#### 3. Eventos Raros de Sessão (1 por sessão, aleatório)
```
AURORA DE LOOT    — todos os drops +1 tier por 3 minutos
ONDAS DO KRONOS   — inimigos infinitos por 2 minutos, loot 3x
PORTAL DOURADO    — portal especial leva a sala secreta com baú OMEGA
BENÇÃO DO NEXUS   — HP cheio + todas skills prontas por 1 minuto
CHUVA DE FRAGMENTOS — 50 Fragmentos de KRONOS (meta-moeda) caem do céu
```

---

### C) PROGRESSÃO VISÍVEL E CINEMATOGRÁFICA

#### Level Up — Efeito Cinematográfico
```
1. FREEZE FRAME 0.5 segundos (tudo para, só partículas continuam)
2. Onda de energia branca/dourada expande do player
3. "NÍVEL X" aparece em texto grande com animação de scale
4. Partículas douradas chovem por 2 segundos
5. Stats do novo nível aparecem em floating text (+HP, +Dano, etc.)
6. Som: fanfarra ascendente de 4 notas
7. Player speech: "Estou ficando mais forte." ou variante
```

#### Evolução de Classe (níveis 5, 10, 15, 20)
```
1. Tela escurece 80%
2. Silhueta do player no centro
3. Raios de energia do tipo da classe escolhida envolvem a silhueta
4. Flash de luz: player aparece com visual novo da classe
5. "VANCE RIOS — CYBORG SOLDIER" em texto grande
6. Câmera zoom-in no player por 1.5 segundos
```

#### Milestone de Kills — Molduras e Títulos
```
100 kills:  Badge "CAÇADOR" aparece no HUD + moldura prata no portrait
500 kills:  Badge "VETERANO" + moldura dourada
1000 kills: Badge "LENDA" + moldura vermelha pulsante + título permanente
2500 kills: Badge "EXECUTOR" + efeito especial no trail do player
5000 kills: Badge "IMPARÁVEL" + skin exclusiva do portrait desbloqueada
```

#### Hall of Fame — Tela de Estatísticas no Menu
```
┌─────────────────────────────────────────┐
│       SUA MELHOR RUN                    │
│  Kills: 347    Nível: 12    Tempo: 2h14 │
│  Maior Streak: 42    Bosses: 8          │
│  Item mais raro: [LENDÁRIO] Rifle Elite │
│  Conquistas esta sessão: 3              │
│                                         │
│       ESTATÍSTICAS TOTAIS               │
│  Total de kills: 2.847                  │
│  Total de horas: 18h32                  │
│  Lendários encontrados: 4               │
└─────────────────────────────────────────┘
```

---

### D) 30 CONQUISTAS COMPLETAS

#### COMBATE (10)
| ID | Título | Descrição | Ícone | Recompensa |
|---|---|---|---|---|
| C01 | Primeiro Sangue | Matar o primeiro inimigo | Gota vermelha | 100 XP |
| C02 | Imparável | 20 kills consecutivos sem tomar dano | Chama dourada | Título [IMPARÁVEL] |
| C03 | Caçador de Bosses | Matar 10 bosses no total | Caveira com coroa | 1000 XP |
| C04 | Arsenal Completo | Usar as 6 skills no mesmo combate | Grade 3x2 de ícones | Skin de skill ciano |
| C05 | Ceifador | Atingir 1000 kills no total | Foice estilizada | Moldura dourada |
| C06 | Predador | Matar 5 inimigos em 3 segundos | Raio vermelho | Trail de chamas |
| C07 | Duelo de Bosses | Matar um boss sem usar skills | Espada simples | Título [DUELO] |
| C08 | Intocável | Completar uma zona sem tomar dano | Escudo perfeito | +10% escudo permanente |
| C09 | Caça ao OMEGA | Matar o OmegaBoss | Símbolo Ω vermelho | Item OMEGA garantido |
| C10 | Exterminador | Matar um de cada tipo de inimigo | Grade de silhuetas | Título [EXTERMINADOR] |

#### EXPLORAÇÃO (8)
| ID | Título | Descrição | Ícone | Recompensa |
|---|---|---|---|---|
| E01 | Desbravador | Visitar todas as zonas | Mapa com pins | 500 XP |
| E02 | Investigador | Encontrar 10 documentos de lore | Pergaminho aberto | Título [ARQUEÓLOGO] |
| E03 | Explorador das Trevas | Visitar todas as zonas sombrias | Lua crescente | Trail de sombra |
| E04 | Caçador de Segredos | Encontrar 3 salas secretas | Ponto de interrogação | Mapa com locais ocultos |
| E05 | Sobrevivente | Escapar da Mansão Abandonada | Casa assombrada | Skin de player fantasma |
| E06 | Pioneiro | Visitar toda zona em modo APOCALIPSE | Crânio + bandeira | Título [PIONEIRO] |
| E07 | Conhecedor | Ler todos os 15 documentos de lore | Livro aberto | Cena bônus de história |
| E08 | Fantasma nas Trevas | Atravessar o Cemitério sem matar nada | Fantasma pacífico | Item cosmético raro |

#### CONSTRUÇÃO (6)
| ID | Título | Descrição | Ícone | Recompensa |
|---|---|---|---|---|
| B01 | Arquiteto | Construir 10 estruturas em uma sessão | Martelo e planta | 300 créditos |
| B02 | Fortaleza | Ter Ark + 3 Torres + Quartel simultâneos | Castelo estilizado | Título [ARQUITETO] |
| B03 | Industrialista | Construir uma Fábrica de Tanques | Engrenagem | Tanque começa com +50% HP |
| B04 | Médico de Campo | Construir 3 Baías Médicas | Cruz vermelha | Cura passiva +2 HP/s |
| B05 | Senhor da Guerra | Ter 5 Tanques aliados ativos | Tanque + coroa | Título [SENHOR DA GUERRA] |
| B06 | O Bunker | Construir todas as 8 estruturas diferentes | Grade completa | Desbloqueio de estrutura secreta |

#### CONQUISTAS SECRETAS (6) — Dicas crípticas, descobertas por exploração
| ID | Título | Dica | Condição Real | Recompensa |
|---|---|---|---|---|
| S01 | "Ele Não Estava Sozinho" | "Às vezes os mortos voltam..." | Matar UndeadTerminator quando ele ressuscita pela segunda vez (glitch na IA) | Item lendário único |
| S02 | "A Última Transmissão" | "KRONOS disse algo antes do fim..." | Encontrar todos os 3 logs de transmissão interceptada do KRONOS | Cena de lore oculta |
| S03 | "Protetor" | "Nenhum aliado caiu sob seu comando" | Completar um ato inteiro sem nenhum companion morrer | Trail dourado permanente |
| S04 | "Diálogo com a Máquina" | "E se você pudesse conversar?" | Deixar o player parado ao lado de STEEL por 60 segundos sem se mover | Diálogo secreto de STEEL |
| S05 | "O Verdadeiro Teste" | "Prove que você não precisa de nada" | Chegar ao nível 10 sem usar nenhum item do inventário | Título [ASCETA] |
| S06 | "Kronos Estava Certo?" | "Leia as palavras do inimigo com cuidado" | Encontrar os 3 logs do KRONOS E os 3 diários de DR. CHEN | Ending alternativo desbloqueado |

---

### E) DESAFIOS DIÁRIOS E SEMANAIS

#### Pool de 20 Desafios Diários (3 sorteados por dia)
```
1.  Feche 3 portais de anomalia
2.  Mate 50 inimigos comuns
3.  Construa uma Arca
4.  Use cada skill pelo menos 5 vezes
5.  Mate 3 inimigos sem tomar dano cada
6.  Colete 10 itens raros+
7.  Sobreviva 10 minutos em GUERREIRO+
8.  Mate um boss sem companions
9.  Alcance kill streak de 15
10. Visite 3 zonas diferentes
11. Faça 1 item via crafting
12. Feche um portal tier 3
13. Mate 20 inimigos sobrenaturais
14. Construa 5 estruturas
15. Use STEEL como companion
16. Sobreviva onda de Zerglings
17. Mate boss sem usar skills
18. Colete 500 créditos em uma sessão
19. Mate inimigo com kill streak x3 ativo
20. Encontre 1 documento de lore
```
**Recompensa diária:** 200-500 créditos + chance 15% de drop épico garantido

#### Desafios Semanais (1 por semana)
```
Semana A: "A Grande Purga" — 500 kills em uma única sessão
           Recompensa: Item LENDÁRIO garantido + Título [PURGA]

Semana B: "Arquiteto da Resistência" — Construir base completa (Ark + Quartel + 5 Torres)
           Recompensa: Skin especial de construção + 1000 créditos

Semana C: "Caçador das Sombras" — Matar todos os tipos de inimigos sobrenaturais
           Recompensa: Trail de sombra permanente + 500 XP bônus

Semana D: "Sobrevivente do Apocalipse" — Completar 1 zona em dificuldade APOCALIPSE
           Recompensa: Moldura exclusiva APOCALIPSE + Título [SOBREVIVENTE]
```

#### Eventos Sazonais
```
NOITE DAS ANOMALIAS (1x por mês, 48h)
- 3x mais portais de anomalia ativos
- Loot de portais = +1 tier
- Boss especial "PORTAL OMEGA" que só existe neste evento
- Ranking de portais fechados (top 10 exibido no menu)

INVASÃO KRONOS (1x por mês, 72h)
- Ondas infinitas de inimigos com escalada de dificuldade
- Cada 100 kills = recompensa automática
- Score final salvo como recorde pessoal
- Item especial "Fragmento da Invasão" desbloqueado

SEMANA DA RESISTÊNCIA (1x a cada 2 meses)
- XP +50% em todas as atividades
- Novos diálogos dos companions sobre o evento
- Missão especial única com boss de evento
```

---

### F) SISTEMA DE TÍTULOS E COSMÉTICOS

#### Títulos Desbloqueáveis (aparecem antes do nome no HUD)
```
[RECRUTA]       — padrão inicial
[CAÇADOR]       — 100 kills
[VETERANO]      — 500 kills + 3 bosses
[ARQUITETO]     — construir todas as estruturas
[EXECUTOR]      — usar cada skill 50 vezes
[LENDA]         — 1000 kills
[SOBREVIVENTE]  — completar modo APOCALIPSE
[PURGA]         — desafio semanal especial
[ASCETA]        — conquista secreta
[IMPARÁVEL]     — streak de 35+ kills
[PIONEIRO]      — explorar tudo no APOCALIPSE
[SENHOR DA GUERRA] — 5 tanques ativos
```

#### Skins de Implante (cor do brilho dos implantes do player)
```
AZUL NEXUS    — padrão
VERMELHO KRONOS — desbloquear derrotando IRON-VIII
DOURADO OMEGA   — desbloquear item OMEGA
VERDE ALIEN     — matar 200 inimigos alienígenas
ROXO SOMBRA     — completar todas as zonas sombrias
BRANCO GHOST    — conquista secreta S04
```

#### Trails de Partículas (aparecem ao correr)
```
SEM TRAIL       — padrão
CHAMAS          — kill streak x3 ativo / conquista C06
ELÉTRICO        — caminho Hacker Fantasma + nível 10
SOMBRA          — conquista E03 ou S01
DOURADO         — conquista S03
OMEGA (vermelho)— Título [LENDA] + item OMEGA equipado
```

---

### G) META-PROGRESSÃO — FRAGMENTOS DE KRONOS

Moeda meta que persiste entre sessões, colecionada ao:
- Morrer (5-20 fragmentos dependendo de quanto durou a run)
- Completar desafio diário (+30 fragmentos)
- Fechar portal de anomalia (+5 fragmentos)
- Matar boss (+15-50 fragmentos)

**Árvore de Talentos Permanentes (20 nós, 3 caminhos):**

```
CAMINHO SOBREVIVÊNCIA (verde):
├─ Resistência I:    +10% HP máximo        (50 fragmentos)
├─ Resistência II:   +20% HP máximo        (100 fragmentos)
├─ Regeneração:      +1 HP/s passivo       (80 fragmentos)
├─ Escudo Reforçado: +25% duração escudo   (120 fragmentos)
└─ Lenda Viva:       Ressuscita 1x por sessão (300 fragmentos)

CAMINHO PODER (vermelho):
├─ Precisão I:       +10% dano             (50 fragmentos)
├─ Precisão II:      +25% dano             (100 fragmentos)
├─ Velocidade:       +15% velocidade ataque (80 fragmentos)
├─ Destruição:       +20% AoE das skills   (120 fragmentos)
└─ EXECUTOR:         Skills custam -30% cooldown (300 fragmentos)

CAMINHO NEXUS (azul):
├─ Comerciante:      Itens 15% mais baratos (60 fragmentos)
├─ Coletor:          +20% drop rate        (100 fragmentos)
├─ Construtor:       Construções -25% custo (80 fragmentos)
├─ Companion Bond:   Companions +30% HP    (120 fragmentos)
└─ Fragmento Vivo:   +50% ganho de fragmentos (200 fragmentos)
```

---

### H) SISTEMA DE TENSÃO E ALÍVIO

#### Escalada de Tensão (visual + sonora)
```
0-3 inimigos na tela:    HUD azul, música calma
4-8 inimigos:            HUD muda para amarelo suave, ritmo musical aumenta
9-15 inimigos:           HUD laranja, música tensa, câmera leve zoom-out
16+ inimigos:            HUD VERMELHO pulsante, música de urgência, vinheta escurecendo
Portal tier 3 ativo:     Overlay de tempestade + relâmpagos + "ALERTA: ANOMALIA CRÍTICA"
HP < 20%:                Tela em preto-e-branco parcial + batida de coração + "PERIGO"
```

#### Alívio Catártico (momentos de recompensa máxima)
```
Último portal fechado:
  - FREEZE FRAME 0.5s
  - Explosão de luz do último portal
  - CHUVA DE LOOT — todos os drops do portal caem de uma vez
  - Música de vitória toca
  - "ZONA SEGURA" em texto verde
  - Player speech: "Todas anomalias fechadas! Zona segura."

Boss morto:
  - SLOW-MO 0.25x por 1 segundo
  - Explosão visual do boss com partículas específicas
  - Loot pousa com efeito de impacto
  - "BOSS ELIMINADO — +[XP] XP" em texto grande
  - Camera shake suave (satisfatório, não irritante)

Level up:
  - FREEZE 0.5s + onda de energia
  - Todos os inimigos próximos recuam (knockback de celebração)
  - Tempo lento 0.5s enquanto stats sobem em floating text
```

---

### I) GANCHOS DE RETORNO

#### No Menu Principal (ao abrir o jogo)
```
┌──────────────────────────────────────────┐
│  BEM-VINDO DE VOLTA, VANCE               │
│                                          │
│  Você ficou ausente por 18 horas.        │
│  BÔNUS DE RETORNO: +50% XP por 30 min   │
│                                          │
│  DESAFIO DIÁRIO:                         │
│  ✗ Fechar 3 portais                      │
│  ✗ Matar 50 inimigos                     │
│  ✓ Construir 1 Arca (COMPLETO!)          │
│                                          │
│  PRÓXIMO BOSS NOMEADO: 47 minutos        │
│  EVENTO: Noite das Anomalias — 2h 18min  │
└──────────────────────────────────────────┘
```

#### Sistema "Base Offline" (construção continua)
- Construções em andamento mostram timer no menu
- "Sua Arca completou construção!" ao entrar
- Baía Médica acumula cura offline (cap de 5 minutos)
- Quartel: gerou 1 companion de recurso enquanto você estava fora

---

### J) ESTRATÉGIA COMERCIAL — STEAM E MARKETING

#### Preço e Posicionamento
```
Early Access:   R$ 19,90 / USD 6,99 (30% desconto de lançamento)
Versão Full:    R$ 29,90 / USD 9,99
DLC Cosméticos: R$ 9,90 / USD 3,99 (skins, trails, trilha sonora)
Bundle:         R$ 34,90 / USD 11,99 (jogo + DLC)
```

**Por que USD 9,99?** Sweet spot de impulso de compra. Acima de USD 14,99 o jogador pesquisa mais. Abaixo de USD 7 parece "jogo de celular".

#### Trailer de Impacto — Primeiros 30 Segundos (CRUCIAIS)
```
00:00-00:05 — Silêncio. Texto: "2047. A IA KRONOS destruiu o mundo."
00:05-00:10 — Portal de anomalia rasga o céu com relâmpagos. Zoom dramático.
00:10-00:15 — VANCE RIOS entra em combate. Kill streak 15x. Partículas voam.
00:15-00:20 — Boss IRON-VIII lança projéteis. VANCE desvia, usa skill EMP, explosão.
00:20-00:25 — Item LENDÁRIO cai. Raio de luz dourada. Música épica.
00:25-00:30 — Tela divide: Ark sendo construída, tanques aliados marchando, portal fechando.
00:30 — LOGO DARKNET. "Disponível em Early Access — Steam"
```

#### Screenshots de Marketing (os 6 mais impactantes)
```
1. Portal de anomalia com tempestade de relâmpagos (atmosfera = Solo Leveling)
2. Kill streak x35 com IMPARÁVEL em texto + partículas (poder = satisfatório)
3. Tela de evolução de classe — VANCE virando Cyborg (progressão visível)
4. Item LENDÁRIO caindo com raio de luz dourada (ganância saudável)
5. Base construída: Ark + Torres + Tanques aliados (RTS dentro do ARPG)
6. Cemitério com iluminação de tochas e fantasmas (atmosfera sombria)
```

#### Steam Tags Recomendadas
```
Principal: Action RPG, ARPG, Top-Down Shooter, Roguelite
Gênero:    Dark Fantasy, Sci-Fi, Dystopian, Post-Apocalyptic
Mecânica:  Base Building, Crafting, Loot, Skill-Based
Atmosfera: Dark, Gore, Atmospheric, Story Rich
Comparação: "Like Diablo meets StarCraft"
```

#### Demo Gratuita — O Que Incluir
```
Conteúdo: Ato 1 completo (2-3 horas)
Zonas: Ruinas de Avalon + Bunker Nexus
Bosses: 1 boss de ato (IRON-VIII Comandante)
Loot: Até raridade ÉPICO disponível
Limite: Não deixa passar do Ato 1
Call to Action no final da demo: "Continue a resistência — Early Access disponível"
```

#### O Que Faz Alguém Recomendar para Amigos
```
1. Drop de item OMEGA — visual tão espetacular que dá vontade de filmar e postar
2. Kill streak IMPARÁVEL — sensação de poder que você quer mostrar
3. "Olha o visual do meu personagem após a evolução" — identidade visual
4. "Não acredita no que aconteceu quando fechei o último portal" — momento cinematográfico
5. Comparação: "É tipo Diablo mas com portais do Solo Leveling e base do StarCraft"
```

---

## PARTE 3 — TOP 5 IMPLEMENTAÇÕES PRIORITÁRIAS

### #1 — SISTEMA DE RARIDADE 6 TIERS COM EFEITOS VISUAIS
**Impacto:** MÁXIMO — cada sessão tem momento memorável  
**Complexidade:** Baixa — modificar tabela de drop existente + adicionar efeitos visuais por tier  
**Retorno:** Jogador imediatamente sente "recompensa" mais frequente e mais emocionante  
**Implementação:** 3-4 horas de código  

### #2 — KILL STREAK COM MULTIPLICADOR DE LOOT
**Impacto:** MUITO ALTO — cria tensão e objetivo em cada momento de combate  
**Complexidade:** Baixa — contador de kills consecutivos sem dano, multiplicador simples  
**Retorno:** "Não posso levar dano agora" = engajamento ativo constante  
**Implementação:** 1-2 horas de código  

### #3 — SISTEMA DE CONQUISTAS (30 iniciais)
**Impacto:** ALTO — metas de curto e longo prazo simultâneas  
**Complexidade:** Média — struct Achievement + checagem de condições nos eventos existentes  
**Retorno:** Jogador sempre tem "próximo objetivo" mesmo sem missão ativa  
**Implementação:** 6-8 horas de código  

### #4 — HALL OF FAME E ESTATÍSTICAS PERSISTENTES
**Impacto:** ALTO — cria orgulho e motiva melhorar  
**Complexidade:** Baixa — salvar stats no SaveManager já existente + tela de exibição  
**Retorno:** Jogador volta para "bater seu próprio recorde"  
**Implementação:** 2-3 horas de código  

### #5 — DESAFIOS DIÁRIOS (3 por dia)
**Impacto:** ALTO — razão para abrir o jogo todos os dias  
**Complexidade:** Média — pool de desafios + sistema de geração procedural + timer de reset  
**Retorno:** Retenção diária — o mais valioso para reviews e recomendações no Steam  
**Implementação:** 4-5 horas de código  

---

## CONCLUSÃO — A FEATURE #1 PARA MÁXIMO VÍCIO COM MÍNIMO ESFORÇO

**Sistema de Raridade 6 Tiers com Efeito Visual de Drop**

Por quê? Porque o efeito de ver um raio de luz dourada pousar no chão com partículas ativa dopamina imediata. É o mecanismo central de Diablo, Path of Exile, e todos os ARPGs de sucesso. Com o sistema existente de drops, é só uma questão de:
1. Adicionar o campo `ItemRarity` ao item
2. Modificar o roll de drop para usar a tabela de 6 tiers
3. Adicionar o efeito visual/sonoro específico por tier

**Custo:** ~3 horas de implementação  
**Retorno:** O jogador sempre tem esperança de que o próximo inimigo vai dropar algo épico.  
Essa esperança é o coração de todo ARPG viciante.

---

*Documento gerado pelo Agente de Game Design Psicológico do DARKNET*  
*Baseado em análise de: Diablo 2/3/4, Path of Exile, Hades, Dead Cells, Vampire Survivors, Hollow Knight*
