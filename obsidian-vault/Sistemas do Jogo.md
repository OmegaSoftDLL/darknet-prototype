# Sistemas do Jogo

## Combate
- **Projéteis:** player e inimigos, colisão com knockback
- **Melee:** range configurável, swing animation
- **Skills (6):** Laser • EMP AoE • Granada Plasma • Sobrecarga buff • Barreira escudo • Rajada 8-shot
- **Combo:** multiplicador 1 + min(combo,10) × 0.15 — reset após 3s sem kill
- **Elite System:** 15% chance de spawn elite (Berserker / Armado / Volátil)
- **Screen shake:** triggerShake(intensity, duration)
- **Slow-mo:** kill de boss ativa slow-mo 0.25× por 0.8s

## Inventário / Build
| Tecla | Ação |
|-------|------|
| I | Abre inventário full-screen |
| 1/2/3 | Seleciona slot (Arma/Armadura/Implante) |
| U | Upgrade do slot (custa créditos, máx 3★) |
| Q/E | Navega itens na bolsa |
| F | Usa item selecionado |
| R | Funde 3 iguais → bônus permanente |

### Fusões
- 3× NanoCore → +60 MaxHP permanente
- 3× TechChip → +200 XP
- 3× WeaponPart → +12 Dano, +15 Alcance

## Crafting (NOVO)
| Tecla | Ação |
|-------|------|
| C | Abre/fecha interface de crafting |

### Materiais (drop dos inimigos)
| Material | Drop de |
|----------|---------|
| MetalScrap | Scout, Tank (40%) |
| AlienCarapace | Zergling, Hydra (50%) |
| PlasmaCore | HunterDrone (35%) |
| NanoFiber | MORPH-X (45%) |
| OmegaEssence | OmegaBoss (100%) |

### Receitas
| Resultado | Ingredientes |
|-----------|-------------|
| Faca de Combate | 3× MetalScrap |
| Armadura Híbrida | 5× MetalScrap + 2× AlienCarapace |
| Lançador Ácido | 3× AlienCarapace + 1× PlasmaCore |
| Nano-Armadura | 4× NanoFiber |
| Rifle de Plasma | 2× PlasmaCore + 2× NanoFiber |
| **ESPADA DO Executor** ⭐ | 1× OmegaEssence + 3× MetalScrap + 3× AlienCarapace |

## Companion System (NOVO)
| Tecla | Companion |
|-------|-----------|
| F2 | MARCO VEIL — atira, skill: rajada 3 tiros |
| F3 | STEEL — tanque melee, skill: pisão AoE |
| F4 | REX — rush, skill: flanqueia em alta velocidade |

## Loja / Vendor (NOVO)
- Tecla **E** ao lado de NPC Merchant/WeaponDealer/ArmorSmith
- 1 vendedor por zona
- Itens: armas, armaduras, implantes, consumíveis
- Cosméticos: paletas de cor para o personagem

## Progressão
- **XP → Level:** enemy scaling +12% HP/nível, +8% dano/nível por nível do player
- **OmegaBoss:** a cada 50 kills — +1500 XP, loot raro
- **Quests:** VANCE RIOS, COMANDANTE LYRA, MARCO VEIL, DR. CHEN

