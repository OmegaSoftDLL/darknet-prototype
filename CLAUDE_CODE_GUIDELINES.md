# Diretrizes de Handoff para Claude Code

## Objetivo
Este documento orienta o próximo programador a continuar o desenvolvimento do projeto Darknet Prototype de forma consistente, segura e alinhada com a visão do jogo.

## Contexto do projeto
- Projeto: Darknet Prototype
- Tipo: ARPG 2D top-down em C++ com raylib
- Foco atual: protótipo avançado com vários sistemas implementados, mas ainda sem consolidação total em produção
- Arquitetura principal: cliente C++ em [src](src) + backend em [server](server) + integração opcional em [integration](integration)

## Visão de produto
O projeto busca ser um ARPG de ação com identidade forte, narrativa sombria, combate fluido, progressão, loot, sistemas de zona, companions, crafting e possibilidade de expansão para multiplayer e monetização.

## O que já existe
- Loop principal de jogo com player, inimigos, projéteis, drops e XP
- NPCs, quests, tutorial, achievements e save/load
- Sistemas de construção, crafting, portais, dark world, inferno zone e áudio
- Backend com estrutura inicial em Docker para multiplayer e loja
- Documentação estratégica completa em [README.md](README.md), [DARKNET_STORY.md](DARKNET_STORY.md), [GAME_DESIGN.md](GAME_DESIGN.md) e [DISTRIBUTION.md](DISTRIBUTION.md)

## O que deve ser priorizado
A ordem de trabalho recomendada é:
1. Estabilizar build e ambiente local
2. Corrigir qualquer regressão de compilação ou integração
3. Implementar features de alto impacto para o loop de jogo
4. Reduzir acoplamento do fluxo principal
5. Integrar backend e monetização de forma gradual

## Regras de trabalho
### 1. Respeitar a arquitetura existente
- Não reescrever o projeto do zero.
- Trabalhar por módulos e integrar com o código já existente.
- Preferir pequenas mudanças incrementais.

### 2. Manter compatibilidade com o gameplay atual
- Evitar quebrar movimento do jogador, combate, save, UI e zonas.
- Qualquer nova feature deve ser adicionada sem impedir a execução atual.

### 3. Priorizar a estabilidade antes da novidade
- Features bonitas sem estabilidade não devem ser prioridade inicial.
- Se houver dúvida entre “novo sistema” e “corrigir o core”, corrigir o core primeiro.

### 4. Usar C++17 e padrões do projeto
- Manter compatibilidade com CMake e raylib 5.0.
- Evitar dependências novas sem necessidade.
- Se adicionar uma dependência externa, documentar por que ela é necessária.

### 5. Não editar arquivos gerados automaticamente
- Evitar mudanças em conteúdo gerado em [build](build), salvo necessidade técnica explícita e documentada.

## Diretrizes de implementação
### Para novas features
- Entender primeiro onde a feature se encaixa no fluxo atual.
- Procurar os módulos relevantes antes de editar.
- Integrar em [src/Game.cpp](src/Game.cpp) apenas quando for inevitável; preferir separar lógica em classes menores.
- Adicionar UI, lógica e dados de forma modular.

### Para correções de bugs
- Reproduzir o problema antes de mexer no código.
- Identificar a causa raiz antes de aplicar uma correção.
- Se possível, criar um cenário mínimo de repro antes da mudança.
- Verificar se o ajuste não introduziu regressão em outras partes.

### Para refactors
- Fazer em etapas pequenas.
- Manter o comportamento externo igual enquanto a estrutura interna melhora.
- Preferir extração de responsabilidade a reescrita ampla.

## Arquivos centrais para conhecer primeiro
- [CMakeLists.txt](CMakeLists.txt)
- [src/Game.h](src/Game.h)
- [src/Game.cpp](src/Game.cpp)
- [src/Player.h](src/Player.h)
- [src/Player.cpp](src/Player.cpp)
- [src/Enemy.h](src/Enemy.h)
- [src/Enemy.cpp](src/Enemy.cpp)
- [src/Item.h](src/Item.h)
- [src/Item.cpp](src/Item.cpp)
- [src/NPC.h](src/NPC.h)
- [src/NPC.cpp](src/NPC.cpp)
- [src/Companion.h](src/Companion.h)
- [src/Companion.cpp](src/Companion.cpp)
- [src/ShopSystem.h](src/ShopSystem.h)
- [src/ShopSystem.cpp](src/ShopSystem.cpp)
- [src/SaveManager.h](src/SaveManager.h)
- [src/SaveManager.cpp](src/SaveManager.cpp)
- [server/README.md](server/README.md)

## Priorização de features de alto valor
### Alta prioridade
- Companion / aliado com poderes
- Sistema de loja/vendor NPC
- Visualização de itens únicos e mais legível
- Crafting com materiais e economia mais clara
- Melhor integração com save/load e progression loop

### Média prioridade
- Mais tipos de inimigos e variações de zona
- Melhor feedback visual geral
- Ajustes de balanceamento
- Polimento de UI

### Baixa prioridade no momento
- Grande expansão de lore sem gameplay funcional
- Backend premium complexo sem cliente integrado
- Features de distribuição e publish sem build estável

## Regras de qualidade
- Código deve ser legível e bem nomeado.
- Evitar lógica duplicada.
- Manter consistência com nomes em português/inglês já usados no projeto.
- Comentar apenas o que realmente ajuda a entender a intenção.
- Não introduzir “dead code” sem necessidade.

## Processo de validação
Sempre que fizer uma alteração:
1. Revisar o impacto local.
2. Compilar ou validar o projeto se possível.
3. Confirmar que o comportamento relevante não foi quebrado.
4. Registrar o que foi alterado e por que.

### Validação mínima obrigatória
- O projeto deve continuar sem erros óbvios de compilação.
- O fluxo principal do jogo deve continuar rodando sem regressão crítica.
- Save/load e sistemas principais devem continuar funcionando.

## Regras específicas para Windows
- O projeto é desenvolvido com foco em Windows, então priorizar compatibilidade com Visual Studio e CMake.
- Evitar caminhos hardcoded ou dependentes de ambiente local.
- Respeitar o uso de bibliotecas como raylib e Winsock quando aplicável.

## O que evitar
- Substituir a arquitetura atual por um modelo novo sem necessidade.
- Implementar sistemas complexos sem integração com o gameplay existente.
- Deixar TODOs grandes sem plano de execução.
- Criar assets ou sistemas que não tenham impacto claro no loop principal.
- Trabalhar em múltiplos escopos grandes ao mesmo tempo sem validar incrementalmente.

## Critérios de conclusão para uma tarefa
Uma tarefa só pode ser considerada concluída se:
- a mudança foi implementada
- o código compila ou não apresenta erro claro de build
- o efeito esperado foi validado de forma objetiva
- não há regressão evidente no fluxo principal
- a alteração foi documentada se relevante

## Prompt pronto para passar ao Claude Code
Use este texto como handoff direto:

“Continue o desenvolvimento do Darknet Prototype como um protótipo avançado de ARPG 2D em C++ com raylib. Foque em estabilidade, integração de sistemas existentes e implementação de features de alto impacto para o loop de jogo. Preserve o estado atual do gameplay, evite reescrever o projeto do zero, trabalhe por módulos e valide cada mudança. Priorize: estabilizar build, corrigir regressões, implementar companion/loja/visual de itens e melhorar a integração entre gameplay, save e UI. Use C++17, CMake e a arquitetura já existente. Documente mudanças relevantes e não altere arquivos gerados automaticamente.”

## Resumo executivo
O trabalho ideal para o próximo programador é continuar evoluindo o projeto de forma incremental, com foco em estabilidade, integração e impacto real no jogador, em vez de expandir demais sem consolidar a base existente.
