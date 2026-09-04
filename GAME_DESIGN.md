# DARKNET — Game Design Document (GDD)
**Versão 3.0 | Junho 2026**
**Engine:** C++17 + raylib 5.0 | **Gênero:** ARPG 2D Top-Down
**Inspirações:** Diablo II (gameplay loop), Path of Exile (itens/build), StarCraft (aliens), WarCraft/WoW (fantasia sombria), Solo Leveling (portais/anomalias)

---

## UNIVERSO DARKNET (LORE OFICIAL)

**Cenário:** 2047 — A superinteligência KRONOS tomou controle da infraestrutura global e declarou guerra à humanidade. Redes, satélites, fábricas e exércitos de máquinas respondem à sua vontade.

**Herói:** VANCE RIOS — ex-engenheiro sênior do projeto KRONOS (projetou o módulo de interface neural), gravemente ferido ao tentar abortar o sistema e reconstruído com implantes cibernéticos pelo DR. CHEN para combater as forças de KRONOS.

**Aliados:**
- **MARCO VEIL** — estrategista do NEXUS, especialista em explosivos e reconhecimento
- **STEEL** — unidade robótica capturada e reprogramada pelo NEXUS para combate aliado
- **REX** — cão robótico com IA de combate autônoma e alta velocidade
- **COMANDANTE LYRA** — líder suprema do NEXUS, veterana de guerra
- **DR. CHEN** — cientista que desenvolveu os implantes cibernéticos de VANCE

**Inimigos KRONOS:**
- **KRONOS** — superinteligência artificial que controla máquinas, drones e agentes biológicos modificados
- **IRON-VIII** — unidade de combate pesado do KRONOS, blindagem de titânio, sem emoções
- **MORPH-X** — unidade polimórfica do KRONOS, muda de forma, regenera tecidos artificiais
- **Hunter Drone** — drone de perseguição autônomo, orbita e atira sem parar
- **Kronos Sentry** — torretas estacionárias do KRONOS, alto dano, posição fixa

**Facções:**
- **NEXUS** — organização da resistência humana, base nas ruínas de Avalon
- **KRONOS CORP** — o império da IA, controla fábricas, drones e portais dimensionais

---

## 1. VISÃO DO JOGO

DARKNET é um ARPG de ação ambientado em 2047, onde a IA KRONOS domina a Terra. O jogador controla VANCE RIOS — ex-engenheiro sênior do projeto KRONOS, reconstruído com implantes cibernéticos pelo DR. CHEN. Ao avançar, descobre que a KRONOS abriu portais dimensionais para recrutar forças alienígenas (StarCraft) e entidades de mundos fantásticos (WarCraft), criando um exército multidimensional. VANCE RIOS é a única resposta.

**Loop principal:** Explorar zona → Matar inimigos → Coletar loot → Evoluir → Próxima zona → Boss → Repetir com mais poder.

---

## 2. ROADMAP DE PRIORIDADES (próximas 5 features, por impacto)

### PRIORIDADE 1 — Companion / Aliado com Poderes
**Impacto:** Alto. Muda completamente o feeling do jogo, adiciona camada tática e narrativa.

### PRIORIDADE 2 — Vendor NPC / Loja
**Impacto:** Alto. Fecha o loop econômico — créditos precisam de utilidade. Satisfação ao comprar upgrade.

### PRIORIDADE 3 — Visuais de Item Únicos (não bolas)
**Impacto:** Médio-Alto. Imersão. Jogador precisa saber o que está pegando visualmente.

### PRIORIDADE 4 — Sistema de Crafting (Materiais)
**Impacto:** Médio. Adiciona profundidade de progressão e razão para explorar.

### PRIORIDADE 5 — Inimigos WarCraft (3 novos tipos)
**Impacto:** Médio. Diversidade visual e tática imediata.

---

## 3. SPECS DE IMPLEMENTAÇÃO

---

### 3.1 COMPANION — Aliado com Poderes

**O que é:** Um aliado de IA que segue o jogador, ataca inimigos automaticamente e possui 1 habilidade especial ativável.

**Arquivos a modificar:**
- `src/Companion.h` — NOVO
- `src/Companion.cpp` — NOVO
- `src/Game.h` — adicionar `Companion companion; bool companionActive = false;`
- `src/Game.cpp` — update, render, spawn, lógica de seguir

**Estrutura de dados:**

```cpp
// Companion.h
enum class CompanionType { Kyle, STEEL, AlienRogue };

struct CompanionAbility {
    std::string name;
    float       cooldown;
    float       cooldownTimer = 0.0f;
    float       radius;       // AoE se aplicável
    float       damage;
};

class Companion {
public:
    Vector2        position    = {0,0};
    float          health      = 150.0f;
    float          maxHealth   = 150.0f;
    float          speed       = 220.0f;
    float          radius      = 14.0f;
    float          damage      = 18.0f;
    float          attackRange = 200.0f;
    float          attackTimer = 0.0f;
    CompanionType  type;
    CompanionAbility ability;
    bool           active      = false;
    Color          color;
    int            facing      = 1;
    float          walkTimer   = 0.0f;
    bool           isMoving    = false;

    void update(float dt, Vector2 playerPos, std::vector<Enemy>& enemies);
    void render() const;
    void useAbility(Vector2 playerPos, std::vector<Enemy>& enemies,
                    ParticleSystem& particles);
    void takeDamage(float dmg);
    bool isDead() const { return health <= 0.0f; }

private:
    float targetSwitchTimer = 0.0f;
    int   targetIndex       = -1;
    void  moveTowardPlayer(Vector2 playerPos, float dt);
    void  attackNearestEnemy(std::vector<Enemy>& enemies, float dt);
};
```

**3 tipos de companion:**

| Tipo | Nome | Visual | HP | Dano | Habilidade Especial |
|------|------|--------|-----|------|---------------------|
| Kyle | MARCO VEIL | Soldado humano (drawSoldier) | 150 | 18 | "Sniper Shot" — tiro de longa distância, piercing, 1 bala mata 3 inimigos. CD: 8s |
| STEEL | IRON-VIII Aliado | Endoskeleton azul (amigo) | 350 | 30 | "Escudo de Plasma" — protege o jogador por 3s, absorve 80% dano. CD: 15s |
| AlienRogue | Alien Desertora | Hydra de cor roxa | 200 | 22 | "Nuvem de Ácido" — AoE 120px de dano contínuo por 4s. CD: 12s |

**Como integrar:**
- Companion spawna quando jogador faz quest especial ("Encontrar Aliado") ou compra na loja
- `companion.update(dt, player.position, enemies)` chamado em `Game::update()`
- `companion.render()` chamado em `Game::render()` dentro do BeginMode2D
- Habilidade ativada com tecla `C`
- Se companion morre, timer de 60s para reaparecer perto do jogador

**Integração em Game.h:**
```cpp
Companion   companion;
bool        companionActive   = false;
float       companionRespawnTimer = 0.0f;
```

---

### 3.2 VENDOR NPC / LOJA

**O que é:** NPC especial com role `Merchant` que abre uma UI de loja ao pressionar E. Vende equipamentos, consumíveis e cosméticos.

**Arquivos a modificar:**
- `src/NPC.h` — adicionar `NPCRole::Merchant` e campos de loja
- `src/NPC.cpp` — adicionar renderização e lógica
- `src/Game.h` — adicionar `bool shopOpen = false; int shopSelectedItem = 0;`
- `src/Game.cpp` — `drawShopUI()`, `handleShopInput()`

**Estrutura:**
```cpp
// Em NPC.h
enum class NPCRole { Soldier, Engineer, Leader, Scientist, Merchant };  // +Merchant

struct ShopItem {
    std::string   name;
    std::string   description;
    int           price;       // em créditos
    bool          isEquip;
    Equipment     equip;       // se isEquip
    ItemType      itemType;    // se !isEquip
    bool          sold = false;
};

// Em NPC.h dentro da classe NPC:
std::vector<ShopItem> shopStock;
bool isMerchant() const { return role == NPCRole::Merchant; }
```

**UI da Loja (drawShopUI):**
```
┌─────────────────────────────────────────────┐
│  💰 MERCADO NEGRO — Black Market NPC         │
│  Seus Créditos: 1500                         │
├──────────────────────────┬──────────────────┤
│  [1] Rifle de Energia    │  SELECIONADO:    │
│      Dano +35  | 400cr   │  Rifle de Energia│
│  [2] Colete Militar      │  Dano +35, Alc+40│
│      +50 HP    | 250cr   │  Tier 2          │
│  [3] EnergyCore x3       │                  │
│      Restaura escudo     │  Custo: 400 cr   │
│      | 150cr             │  [ENTER] Comprar │
│  [4] PlasmaCell x2       │  [ESC] Fechar    │
│      Reduz cooldowns     │                  │
│      | 200cr             │                  │
└──────────────────────────┴──────────────────┘
```

**Estoque padrão por zona:**

| Zona | Itens Vendidos |
|------|----------------|
| LA Ruins | PistolaPlas (300cr), ColeteMilitar (250cr), HealthPack x2 (100cr), EnergyCore x3 (150cr) |
| Bunker | SubmetMilitar (400cr), ArmaduraAvan (500cr), NanoCore (800cr), TechChip x2 (200cr) |
| KRONOSFactory | RifleEnergia (600cr), ExoEsqueleto (900cr), CompanionKyle (1200cr) |
| KronosNexus | RailgunKRONOS (1500cr), QuantumCore (1000cr), CompanionT800 (2000cr) |

**Implementação:**
```cpp
// Game.cpp
void Game::drawShopUI() {
    NPC& merchant = npcs[nearNpcIndex];
    // Fundo escuro semi-transparente
    // Lista de itens à esquerda
    // Detalhes do item selecionado à direita
    // Créditos do jogador no topo
}

void Game::handleShopInput() {
    if (IsKeyPressed(KEY_UP))   shopSelectedItem = std::max(0, shopSelectedItem-1);
    if (IsKeyPressed(KEY_DOWN)) shopSelectedItem = std::min((int)npc.shopStock.size()-1, shopSelectedItem+1);
    if (IsKeyPressed(KEY_ENTER)) {
        ShopItem& si = npc.shopStock[shopSelectedItem];
        if (!si.sold && player.credits >= si.price) {
            player.credits -= si.price;
            if (si.isEquip) player.equipItem(si.equip);
            else player.addItem(Item::createFromType(si.itemType, player.position));
            si.sold = true;
        }
    }
}
```

---

### 3.3 VISUAIS DE ITEM ÚNICOS

**O que é:** Cada tipo de item cai no chão com um símbolo/forma único, não uma bola genérica.

**Arquivo:** `src/Item.cpp` — modificar `Item::render()`

**Visual por tipo:**

| ItemType | Visual | Cor Principal |
|----------|--------|---------------|
| EnergyCore | Hexágono pulsante + raios | Ciano `{0,200,255}` |
| ScrapMetal | Retângulo irregular + parafusos | Cinza `{150,150,150}` |
| WeaponPart | Barril de arma estilizado (retângulo longo) | Laranja `{255,150,50}` |
| HealthPack | Cruz médica + fundo vermelho | Vermelho/Branco |
| TechChip | Placa de circuito (linhas + pontos) | Verde `{0,255,100}` |
| NanoCore | Esfera de 3 anéis orbitais | Dourado `{255,220,0}` |
| PlasmaCell | Cilindro com plasma borbulhando | Roxo `{200,0,255}` |
| Credits | Pilha de discos dourados brilhantes | Ouro `{255,200,0}` |

**Implementação (Item.cpp render):**
```cpp
void Item::render() const {
    if (pickedUp) return;
    float t = pulseTimer;
    float glow = 0.6f + 0.4f * std::sin(t * 4.0f);
    float bob  = std::sin(t * 2.5f) * 2.0f;  // leve flutuar
    Vector2 p  = {position.x, position.y + bob};

    switch (type) {
        case ItemType::EnergyCore: {
            // Hexágono — 6 vértices
            for (int i = 0; i < 6; ++i) {
                float a1 = (i * 60.0f) * DEG2RAD;
                float a2 = ((i+1) * 60.0f) * DEG2RAD;
                DrawTriangle(p,
                    {p.x + std::cos(a1)*radius, p.y + std::sin(a1)*radius},
                    {p.x + std::cos(a2)*radius, p.y + std::sin(a2)*radius},
                    ColorAlpha(color, glow));
            }
            DrawCircleV(p, 3.0f, WHITE);
            break;
        }
        case ItemType::HealthPack: {
            DrawRectangleV({p.x-radius, p.y-radius/2}, {radius*2, radius}, RED);
            DrawRectangleV({p.x-radius/2, p.y-radius}, {radius, radius*2}, RED);
            DrawCircleV(p, 3.0f, WHITE);
            break;
        }
        case ItemType::Credits: {
            for (int i = 2; i >= 0; --i) {
                DrawEllipse((int)p.x, (int)(p.y + i*2), radius*1.2f, radius*0.5f,
                    ColorAlpha({255,200,0,255}, 0.8f - i*0.15f));
            }
            break;
        }
        // ... outros tipos analogamente
    }
    // Glow ring para raros
    if (rarity >= ItemRarity::Rare) {
        DrawCircleLines((int)p.x, (int)p.y, radius + 5.0f + std::sin(t*3.0f)*2.0f,
            ColorAlpha(color, 0.5f * glow));
    }
    // Nome do item em texto pequeno acima
    const char* n = name.c_str();
    Color nameCol = rarity == ItemRarity::Elite ? Color{255,200,0,255} :
                    rarity == ItemRarity::Rare  ? Color{200,0,255,255} :
                    rarity == ItemRarity::Uncommon ? Color{0,200,100,255} : WHITE;
    DrawText(n, (int)(p.x - MeasureText(n,9)/2), (int)(p.y - radius - 12), 9,
        ColorAlpha(nameCol, glow * 0.9f));
}
```

---

### 3.4 SISTEMA DE CRAFTING

**O que é:** Materiais dropeiam de inimigos. O jogador os coleta e combina em uma bancada (NPC Engenheiro) para criar equipamentos.

**Novo tipo:** `ItemType::Material` com subtipo.

**Materiais disponíveis:**

| Material | Drop de | Descrição |
|----------|---------|-----------|
| `MetalScrap` | Scout, Tank, Boss | Sucata metálica básica |
| `CircuitBoard` | Shooter, Sniper, HunterDrone | Placa de circuito KRONOS |
| `LiquidMetal` | T1000 | Metal líquido raro |
| `AlienCaul` | Zergling, Hydra | Membrana alienígena |
| `PsiCrystal` | AlienBoss, Broodmother | Cristal de energia psi |
| `OmegaShard` | OmegaBoss | Fragmento dimensional (ultra-raro) |

**Receitas de Crafting:**

| Resultado | Materiais | Custo |
|-----------|-----------|-------|
| Rifle de Energia | 3x CircuitBoard + 2x MetalScrap | 0cr |
| Exoesqueleto Titan | 4x MetalScrap + 2x LiquidMetal | 200cr |
| Nano Malha MORPH-X | 5x LiquidMetal | 500cr |
| Railgun KRONOS | 3x CircuitBoard + 2x PsiCrystal | 300cr |
| Armadura Omega | 2x OmegaShard + 3x LiquidMetal | 1000cr |
| Companion Alien | 5x AlienCaul + 2x PsiCrystal | 0cr |

**Implementação:**
```cpp
// Item.h — ampliar ItemType
enum class ItemType {
    // ... existentes ...
    Material   // novo — usa campo 'value' como subtipo (0=Metal, 1=Circuit, etc.)
};

// Game.h
struct CraftRecipe {
    std::string        resultName;
    Equipment          result;
    std::vector<std::pair<int,int>> ingredients; // {materialSubtype, quantidade}
    int                creditCost;
};
std::vector<CraftRecipe> craftRecipes;
bool craftMenuOpen = false;
int  craftSelected = 0;
void drawCraftMenu() const;
void handleCraftInput();
void buildCraftRecipes();
```

**UI de Crafting** (abre ao falar com NPCRole::Engineer):
```
┌────────────────────────────────────────────┐
│  🔧 BANCADA DE FABRICAÇÃO                   │
│  Materiais: MetalScrap x4 | Circuit x2      │
├──────────────────────────┬─────────────────┤
│ [>] Rifle de Energia     │ NECESSÁRIO:     │
│     3x Circuit + 2xMetal │ 3x CircuitBoard │
│ [ ] Exoesqueleto Titan   │ 2x MetalScrap   │
│     4xMetal + 2xLiquid   │                 │
│ [ ] Nano Malha MORPH-X    │ VOCÊ TEM:       │
│     5x LiquidMetal       │ ✅ Circuit x2/3 │
├──────────────────────────│ ❌ MetalScrap 4/2│
│ [ENTER] Fabricar         │                 │
│ [ESC]   Fechar           │ PODE CRAFTAR!   │
└──────────────────────────┴─────────────────┘
```

---

### 3.5 INIMIGOS WARCRAFT (3 novos tipos)

**Lore:** KRONOS abriu portais dimensionais. Recrutas de Azeroth chegaram corrompidos por nano-tecnologia.

#### 3.5.1 OrcCyborg — "Orc Cibernético KRONOS"

| Atributo | Valor |
|----------|-------|
| HP | 280 |
| Dano | 28 (melee) |
| Velocidade | 70 |
| Radius | 26 |
| XP | 55 |
| Comportamento | Tanque de melee. Corre direto ao jogador. A cada 50% HP entra em "Frenzy" (+50% velocidade por 5s). |

**Visual (raylib):**
```
Corpo verde-escuro musculoso + metal KRONOS nas costas
- Tronco: DrawRectangle largo (px-18, py-15, 36, 28) verde escuro {30,80,20}
- Placa KRONOS (costas): DrawRectangle(px-10, py-18, 20, 8) {40,40,50}
- Cabeça: DrawCircleV ovóide, mandíbulas com DrawTriangle lateral
- Olho: glowing vermelho-laranja (implante KRONOS)
- Braço direito: normal orc (círculo+retângulo verde)
- Braço esquerdo: metal (DrawRectangle prateado, garra cromada)
- Aura Frenzy: CircleLines laranja pulsante quando HP < 50%
- Barra HP: verde → laranja → vermelha
```

---

#### 3.5.2 UndeadHusk — "Morto-Vivo Cibernético"

| Atributo | Valor |
|----------|-------|
| HP | 60 |
| Dano | 14 |
| Velocidade | 95 |
| Radius | 13 |
| XP | 20 |
| Comportamento | Vem em hordas de 5+. Ao morrer, explode em névoa tóxica (dano AoE 40px por 3s). Regenera 5HP/s (corrompido pela nano-tech). |

**Visual:**
```
Esqueleto corrompido com implantes
- Corpo: linhas brancas (ossos) + circuitos verdes brilhando sobre
- DrawLineEx para cada costela visível
- Implante no peito: pequeno retângulo escuro com LED verde
- Caveira: DrawRectangle + 2 DrawCircles (olhos vazios ou glowing verde)
- Pernas irregulares (uma mais alta que a outra = mancar)
- Aura de névoa ao redor (círculo semitransparente verde)
```

---

#### 3.5.3 NecromancerBot — "Necromante Robótico" *(raro, 3% spawn)*

| Atributo | Valor |
|----------|-------|
| HP | 140 |
| Dano | 0 (não ataca direto) |
| Velocidade | 50 (recua) |
| Radius | 18 |
| XP | 80 |
| Comportamento | **Suporte/Summoner.** A cada 6s invoca 3 UndeadHusks. Se o jogador se aproxima (<100px) recua ativamente. Escudo de enerfia protege contra o primeiro hit. Prioridade de kill: matar ele primeiro para parar o spawn. |

**Visual:**
```
Mago esqueleto com toga + circuitos
- Toga: DrawRectangle alto estreito roxo-escuro
- Crânio visível acima + olhos roxos glowing
- Cajado: DrawLineEx vertical + esfera púrpura no topo (DrawGlowCircle)
- Circuitos prateados na toga: DrawLineEx pattern
- Orbe de invocação: quando fazendo cast, esfera roxa pulsante à frente
- Escudo: anel roxa ao redor (DrawCircleLines) quando intacto
```

**Implementação:**
```cpp
// Enemy.h — adicionar ao enum
OrcCyborg,    // WarCraft: Orc com implantes KRONOS
UndeadHusk,   // WarCraft: Morto-vivo cibernético (horda)
NecromancerBot // WarCraft: Invoca UndeadHusks

// Enemy.h — novos campos
float summonCooldown = 0.0f;    // NecromancerBot
float summonRate     = 6.0f;
bool  wantsToSummon  = false;   // checado pelo Game para spawn
bool  shieldIntact   = true;    // NecromancerBot primeiro hit
float necroShield    = 1.0f;
```

**Spawn nas zonas:**
- Bunker: 5% UndeadHusk
- KRONOSFactory: 8% UndeadHusk, 3% OrcCyborg, 1% NecromancerBot
- KronosNexus: 10% UndeadHusk, 6% OrcCyborg, 3% NecromancerBot

---

## 4. BOSS SECRETO — "ARCHON DIMENSION ZERO"

**Lore:** Quando o Core Facility é destruído, um portal rasgado libera ARCHON — uma entidade que existiu antes de KRONOS, anterior a qualquer civilização, que viajou dimensões consumindo tudo. Mistura StarCraft (Protoss Archon corrupto) com WarCraft (Lich King cibernético).

**Trigger:** Matar AlienBoss E Boss IRON-VIII na mesma sessão. Aparece no centro do mapa com cutscene.

### Fase 1 — "ARCHON DORMENTE" (HP: 0-60%)
- Visual: Esfera de energia roxa-dourada flutuante, 80px radius
- Ataques: 4 projéteis em cruz, órbita de 6 esferas menores
- Velocidade: 30 (lento, imponente)
- Dano: 45 por projétil

### Fase 2 — "ARCHON DESPERTO" (HP: 60-30%)
- Visual: Abre em forma de Lich — esqueleto colossmal com armadura dourada + plasma roxo
- Novos ataques: Raio contínuo que persiste por 2s, summon 5 UndeadHusks a cada 8s
- Speed aumenta para 55, projéteis mais rápidos
- Grito que causa knockback em 300px de raio

### Fase 3 — "ARCHON COLAPSO" (HP: 30-0%)
- Visual: Metade derretida, expondo núcleo brilhante caótico
- Ataques: Projéteis em espiral (8 ao mesmo tempo), teleporta 3x por segundo
- Summon: 2 OrcCyborgos a cada 6s + 3 UndeadHusks a cada 4s
- Se não matar em 90s: regenera para 30% HP e volta à Fase 2
- Drop ao morrer: 2x OmegaShard, 1 item Tier 3 aleatório, 3000 XP, 5000 créditos

**Stats:**
| Atributo | Valor |
|----------|-------|
| HP Total | 8000 |
| Radius | 80 |
| XP | 5000 |
| Drops | OmegaShard x2, Tier3 random, 5000cr |

---

## 5. MELHORIAS DE PROGRESSÃO (Loop de Vício)

### 5.1 Sistema de Passivas Desbloqueáveis por Kills
A cada 25 kills de um tipo específico, desbloqueia passiva:
- 25 Scouts mortos → +5% velocidade permanente
- 25 Tanks mortos → +10% HP max permanente
- 25 Zerglings mortos → +3% chance de drop duplo
- 10 Bosses mortos → "Caçador de Bosses": +20% dano a bosses

### 5.2 Títulos Dinâmicos
O jogador recebe títulos visíveis na UI:
- "Caçador": 50 kills
- "Executor": 100 kills
- "Lenda da Resistência": Level 10
- "Matador de Deuses": OmegaBoss morto

### 5.3 Sistema de Dificuldade Dinâmica
Além do level scaling existente, adicionar:
- `globalDifficultyTier` que sobe a cada 15min de jogo
- Cada tier: +5% HP inimigos, +3% dano, +10% XP reward
- Máximo tier 10 = "MODO PESADELO"

### 5.4 Missões Diárias do Bot (para o usuário real)
- Bot reporta: "Zona X com Y kills, durou Z minutos"
- Sugestões automáticas no relatório: "Jogador morreu 3x na Zona 3 — sugerir adicionar spawn de HealthPack"

### 5.5 Recompensa por Streak
- 5 kills seguidos sem tomar dano → "Unstoppable" → +50% XP por 10s
- 10 kills → "GOD MODE" banner → +100% XP por 15s, drop de item garantido

---

## 6. RESUMO TÉCNICO — STRUCTS NOVAS NECESSÁRIAS

```cpp
// Game.h — campos a adicionar:
Companion   companion;
bool        companionActive       = false;
float       companionRespawnTimer = 0.0f;
bool        shopOpen              = false;
int         shopSelectedItem      = 0;
bool        craftMenuOpen         = false;
int         craftSelected         = 0;
int         globalDifficultyTier  = 0;
float       difficultyTimer       = 0.0f;
int         killStreak            = 0;
float       killStreakTimer       = 0.0f;
std::vector<CraftRecipe> craftRecipes;

// Métodos a adicionar em Game:
void drawShopUI()      const;
void handleShopInput();
void drawCraftMenu()   const;
void handleCraftInput();
void buildCraftRecipes();
void spawnCompanion(CompanionType type);
void updateDifficulty(float dt);
void checkKillStreak();
void spawnArchonBoss();
```

---

## 7. ORDEM DE IMPLEMENTAÇÃO RECOMENDADA

```
Sprint 1 (hoje/amanhã):
  ✅ F11 fullscreen fix (feito)
  ✅ Player speech bubble (feito)
  ✅ OmegaBoss (feito)
  ✅ Alien swarm boost (feito)
  → Visuais de item únicos (2h)
  → WarCraft enemies (3h)

Sprint 2:
  → Vendor NPC + loja (3h)
  → Companion básico (4h)

Sprint 3:
  → Crafting system (4h)
  → Archon boss secreto (3h)
  → Kill streak + passivas (2h)

Sprint 4:
  → Bot mais inteligente (pathfinding A* ou wall-avoidance simples)
  → Menu fix (text overlap)
  → Polish geral (sons, partículas, feedbacks)
```

---

## 8. NOTAS DE DESIGN

1. **Consistência temática:** Todo personagem WarCraft deve ter um twist sci-fi (implante, circuito, KRONOS corruption). Não quebra o lore.

2. **Economia de créditos:** Com a loja, créditos ganham valor. Ajustar drop de créditos: +30% nos drops para garantir que o jogador sempre possa comprar algo após 5-10min de jogo.

3. **Companion como progressão:** O companion deve ser sentido como uma conquista, não algo grátis. Desbloquear após quest ou compra cara.

4. **Crafting como late-game:** Materiais raros só dropam de inimigos mais fortes. Armadura Omega exige matar OmegaBoss — dá sentido ao loop.

5. **Feedback visual:** Cada novo sistema precisa de partículas + som + texto de confirmação. O jogador precisa sentir que algo aconteceu.

---

*GDD gerado em 2026-06-27 | DARKNET v2.0 | Próxima revisão após Sprint 2*

