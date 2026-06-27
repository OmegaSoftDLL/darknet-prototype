# DARKNET — ARPG Futurista

> Inspirado em Enforcer + Diablo + Path of Exile + StarCraft + WarCraft
> C++17 + raylib 5.0. Universo 2047, IA KRONOS vs resistência NEXUS. Sem IP de filmes.

## Links Rápidos
- [[Sistemas do Jogo]]
- [[Inimigos e Bosses]]
- [[Controles e UI]]
- [[Roadmap e Pendências]]
- [[Sistemas de Engajamento]]
- [[Historia Completa]]

## Status (2026-06-27)

### ✅ Implementado
- **6 classes jogáveis** com seleção (Soldado, Guerreira, Robô, Mago, Bruxa, Homem-Fera) — visual e stats próprios
- **Mundo aberto 3×3** (7680×7680px) sem loading; minimapa; chão aberto (sem labirinto)
- **Zona Segura / Base**: refúgio sem inimigos, respawn ali, 5 NPCs de serviço, perímetro verde + aviso de fronteira
- **Motor de Evolução Infinita**: Nível de Ameaça crescente + 6 mutadores rotativos (nunca estagna)
- **Cenário com sprites pixel-art**: casas, lápides, prédios, lava, etc. em todas as regiões; tiles texturizados por bioma; 51 inimigos com sprite
- **~51 tipos de inimigos** com 5 papéis de IA + telegraphing de ataque + auto-evolução
- **Construção RTS**: 8 prédios, menu clicável, produção de unidades, **níveis 1-3 (tecla U evolui)**, seleção SHIFT+arrasto, ordem clique-direito
- **Arca**: respawn + aura de bônus + cura forte
- **Condição de vitória**: boss final NÚCLEO KRONOS no KronosNexus → tela de vitória
- **Trilha sonora oficial** procedural (tema Am-F-C-G + 11 temas de zona) + SFX punchy + grito de morte
- **Coleta automática + magnetismo**; quests genéricas que enchem as barras
- **Sempre inicia em FULLSCREEN**; mouse virtualizado; **ESC = menu de pause** (9 opções: salvar, dificuldade, mutes de áudio, reiniciar, sair)
- Narrativa: NPCs com histórias emocionais profundas (MAY, FERRO, CIPHER, ARIA, VANCE...)

### ✅ Implementado (continuação)
- **Coleta de recursos naturais**: madeira/pedra/ferro/prata/ouro (segurar **H**), HUD, respawn
- **Animais/vida selvagem**: veado, coelho, javali, lobo, pássaro — passivos fogem, lobo caça; caçáveis dão $/XP
- **Correr (SHIFT) e Pular (ESPAÇO)** — pulo cruza obstáculos indestrutíveis
- **Personagem "voando" → corrigido** (animação usa velocidade real)
- **Loja/Crafting clicáveis** + **avatares** na seleção + **fonte legível** (filtro POINT) + **ESC** abre menu
- **Chefes épicos** (3 fases, padrões telegrafados) + **loot com afixos** (Diablo-like)
- **Cenário VIVO**: fumaça, janelas piscando, vaga-lumes, pássaros, grama balançando
- **Sons de inimigo por facção**

### ⚡ Performance e Juice (auditoria — FEITO)
- **Frustum Culling** no mapa (só tiles visíveis) — FPS resolvido
- **BFS Pathfinding** do bot (contorna paredes, fim das travas em cantos)
- **Grid espacial** na separação de inimigos (fim do O(N²))
- **Hit-stop** no combate + **manchas de sangue/queimado** no chão

### 🌐 Infraestrutura (preparada — ver [[Infraestrutura e Distribuição]])
- **Cyber Station** (server/): containers Docker + Postgres, multiplayer realtime, loja de gems
- **Multiplayer LAN** (NetClient UDP) **LIGADO** — jogadores na mesma rede se veem
- **WSL** habilitado (falta reboot) · **Steam/Epic** scaffolding · **Companheiros** 4 tipos novos

### 🔄 Pendente
- Reboot p/ concluir WSL; subir containers (instalar Docker Desktop)
- Ligar NetClient (multiplayer LAN) + companheiros novos no build
- Wiring dos afixos de Item nos stats do Player
- Pagamento real (Stripe), deploy cloud, contas Steam/Epic — **dependem do usuário**
- Sprites do player por classe; achievements/meta-progressão

## Executável
```
C:\Users\ricar\darknet-prototype\build\Release\darknet.exe  (--autobot para bot de teste)
```
