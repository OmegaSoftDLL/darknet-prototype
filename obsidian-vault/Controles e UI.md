# Controles e UI

## Controles do Jogador

| Tecla | Ação |
|-------|------|
| WASD / Setas | Mover o personagem |
| Mouse clique esq | Atirar / Mover para |
| 1 | Skill: Laser (pierce, 3 feixes) |
| 2 | Skill: EMP (AoE elétrico) |
| 3 | Skill: Granada Plasma (AoE massivo) |
| 4 | Skill: Sobrecarga (buff dano+vel 8s) |
| 5 | Skill: Barreira (imunidade 3s) |
| 6 | Skill: Rajada (8 projéteis em leque) |
| E | Interagir NPC / Pegar equip / Abrir loja |
| I | Inventário full-screen |
| C | Crafting interface |
| F2 | Spawn companion MARCO VEIL |
| F3 | Spawn companion STEEL |
| F4 | Spawn companion RoboDog |
| F5 | Salvar jogo |
| F11 | Fullscreen / Janela |
| F12 | Toggle bot autotest |
| ESC | Pausar / Fechar menus |

## HUD Elements

```
┌─────────────────────────────────────────────────────┐
│ [HP bar] [XP bar] [Shield]   [Minimap] [Zone name]  │
│ [Stats: dmg/def/lvl]                                │
│                                                     │
│              [GAMEPLAY AREA]                        │
│                  ☐                                  │
│              (player + speech bubble)               │
│                                                     │
│ [Skills 1-6 com cooldown]    [Quest log]            │
└─────────────────────────────────────────────────────┘
```

## Telas do Jogo

### Menu Principal
- Enforcer skull animado à esquerda
- Grid ciano animado
- Botões com mouse support
- Trilha sonora de menu (Enforcer-style, 25s loop)

### Inventário (I)
- Coluna esquerda: stats do personagem
- Centro: 3 slots de equipamento (Arma/Armadura/Implante) com preview
- Coluna direita: bolsa de itens com navegação

### Crafting (C)
- Lista de receitas à esquerda
- Seus materiais à direita com contagem
- Verde = pode craftar, vermelho = falta material

### Loja / Vendor (E)
- Lista de itens com preço em créditos
- Painel direito: descrição e efeitos
- "Créditos: XXX" no topo

## Feedback Visual
- **Speech bubble:** aparece acima da cabeça com tail apontando para o personagem
- **Combo:** número grande no centro ao acumular kills rápidos
- **Story banner:** barra central com título do capítulo e subtítulo
- **Floating numbers:** dano em vermelho, cura em verde, XP em azul, créditos em amarelo
- **Screen shake:** ao matar boss, receber dano crítico
- **Slow-mo:** 0.25× velocidade por 0.8s ao matar boss

