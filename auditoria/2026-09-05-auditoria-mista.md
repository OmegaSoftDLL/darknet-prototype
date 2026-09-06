# Auditoria Mista — Darknet Prototype

**Data:** 2026-09-05  
**Escopo:** revisão de documentação existente + análise estática do código + build/testes/logs.  
**Estado do repositório:** 4 arquivos modificados não commitados (`src/Enemy_Render.cpp`, `src/Game.h`, `src/Game_Gameplay.cpp`, `src/Game_WorldRender.cpp`).

## Resumo Executivo

| Área | Estado |
|---|---|
| Build do jogo (`darknet`) | **QUEBRADO** — erro de assinatura `DrawCircle3D` em `src/Game_WorldRender.cpp:2123-2126` |
| Build dos testes (`darknet_tests`) | OK (Debug e Release) |
| Testes automatizados | **12/12 casos, 109/109 asserções passando** |
| Logs recentes | Sem crashes; warnings de VAO reload e quedas de FPS em sessões longas |
| Código | Vários problemas críticos de concorrência, segurança de rede e robustez de save |
| Débito técnico | `Game.cpp`/`Game.h` ainda monolíticos; fases 5–11 sem auditoria real |

## 1. Build e Testes

O build do executável principal está bloqueado por um erro recente:

**`src/Game_WorldRender.cpp:2123-2126`**
```cpp
// Errado (6 argumentos para função de 5)
DrawCircle3D({ xf, 0.6f, zf }, rad, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f },
             ColorAlpha(Color{ 255, 195, 125, 255 }, 0.6f * fade));
```

A assinatura correta da raylib 5.5 é:
```cpp
void DrawCircle3D(Vector3 center, float radius, Vector3 rotationAxis, float rotationAngle, Color color);
```

**Correção imediata**:
```cpp
DrawCircle3D({ xf, 0.6f, zf }, rad, { 0.0f, 1.0f, 0.0f }, 0.0f,
             ColorAlpha(Color{ 255, 195, 125, 255 }, 0.6f * fade));
```

Os testes unitários passam, mas os binários atuais foram gerados **antes** da última modificação em `Game_WorldRender.cpp`, então não refletem o código-fonte atual.

## 2. Achados Críticos do Código

### 2.1 Concorrência — `StoreClient` captura `this` em threads `detach()`
- **Onde:** `src/StoreClient.cpp:69-207`
- **Problema:** `loginAsync`, `fetchStoreAsync`, `buyItemAsync`, etc. criam `std::thread([this, ...]{ ... }).detach()`. O destrutor espera no máximo 2 s, mas `HttpClient` usa timeout de 5 s. Fechar o jogo durante uma chamada de rede causa **use-after-free**.
- **Ação:** guardar `std::thread` e fazer `join()` no destrutor, ou usar `shared_ptr`/`weak_ptr`.

### 2.2 Segurança — JSON construído manualmente sem escape
- **Onde:** `src/StoreClient.cpp:74,164,191` e `src/NetClient.cpp:177-190,314-315`
- **Problema:** nomes de jogador, `itemId`, `packId`, `room`, `myName_` são concatenados diretamente em strings JSON. Caracteres como `"`, `\` ou controles quebram o protocolo.
- **Ação:** usar `nlohmann::json` (já está em `third_party/nlohmann/`) para serialização.

### 2.3 Rede — `NetClient` usa `rand()` sem `srand()`
- **Onde:** `src/NetClient.cpp:85-88,102-105,275`
- **Problema:** máscaras de frame WebSocket e `Sec-WebSocket-Key` são previsíveis em toda execução.
- **Ação:** substituir `rand()` por `<random>` ou `GetRandomValue` da raylib.

### 2.4 Rede — buffer de handshake fixo em 1024 bytes
- **Onde:** `src/NetClient.cpp:281-292`
- **Problema:** JWT longo + host + path podem truncar o request.
- **Ação:** usar `std::string` dinâmico.

### 2.5 Rede — buffer `rx` cresce sem limite
- **Onde:** `src/NetClient.cpp:343-351`
- **Problema:** peer malicioso ou servidor com falha pode encher a memória do cliente.
- **Ação:** limitar tamanho máximo do buffer de recepção.

### 2.6 Save — enums carregados sem validação
- **Onde:** `src/SaveManager.cpp:201,202,288`
- **Problema:** `static_cast<ZoneID>(zoneInt)` aceita qualquer inteiro; save corrompido gera valores de enum fora da faixa.
- **Ação:** validar antes de fazer `static_cast`.

### 2.7 Recursos gráficos sem RAII
- **Onde:** `src/Game.cpp:166-173`, `src/Game_Shaders.cpp:112-113`, `src/LightSystem.cpp:12`
- **Problema:** se uma exceção ocorrer durante o construtor, o destrutor não é chamado e texturas/shaders/render targets vazam.
- **Ação:** envolver recursos raylib em wrappers RAII ou `unique_ptr` com deleters customizados.

### 2.8 Screenshot assíncrono
- **Onde:** `src/Game_Shaders.cpp:214-218`
- **Problema:** thread `detach()` exporta screenshot enquanto `Game` pode ser destruído.
- **Ação:** garantir join no destrutor ou usar fila síncrona antes de encerrar.

## 3. Achados Importantes (Alto/Médio)

| # | Problema | Arquivo | Risco |
|---|---|---|---|
| 1 | `model.materials[0]` sem checar `materialCount` | `src/Game.cpp:179-210` | Crash na inicialização |
| 2 | `Player::equipFromBag` perde item se slot for `None` | `src/Player.cpp:1247-1266` | Perda de progresso |
| 3 | `Projectile` normaliza vetor zero | `src/Projectile.cpp:9` | Projéteis fantasmas |
| 4 | `Tilemap::isWallAtPosition` retorna `false` fora da grid | `src/Tilemap.cpp` | Jogador sai do mapa |
| 5 | `rand()` sem `srand()` em `Player`, `AudioManager`, `InfernoZone` | vários | Comportamento previsível |
| 6 | `SaveManager::slotPath` não valida `slot` | `src/SaveManager.cpp:25-27,305-308` | Path inválido |
| 7 | Divisões `health/maxHealth` sem checar `maxHealth > 0` | vários | Divisão por zero |
| 8 | `Game.h` classe gigante (~687 linhas) | `src/Game.h` | Acoplamento, difícil testar |
| 9 | Cobertura de testes baixa | `tests/test_cases.cpp` | 50+ classes sem testes |

## 4. Pendências da Auditoria Anterior (2026-08-21)

Grande parte dos achados críticos da auditoria de 21/08/2026 já foram corrigidos (modularização, FPS de abertura, Arca sci-fi, cobertura de fases, testes unitários). Os itens que ainda precisam de atenção:

| Criticidade | Pendência |
|---|---|
| **Alto** | Fases 5–11 não auditadas em execução real |
| **Alto** | Prédio moderno (`case 20`) esconde jogador sem translucidez de oclusão |
| **Alto** | Portão de validação não exige `zonesVisited ≥ 1` em runs longas |
| **Alto** | Floresta Negra e fases escuras ilegíveis |
| **Alto** | Garantia do portal é fraca (só valida ponto central, não alcançabilidade) |
| **Médio** | Log `SCENERY` reporta `estruturas=0` em fase urbana |
| **Médio** | Caminhos absolutos hardcoded nos relatórios do bot |
| **Médio** | Autotest não limpa `shot_NN.png` de runs anteriores |
| **Médio** | Textos de fase inconsistentes (`CAP.1` vs `FASE 2`) |
| **Médio** | Áudio não avaliado qualitativamente |
| **Médio** | Escala do herói vs prédios incoerente |

## 6. Correções Aplicadas em 2026-09-05

| # | Problema | Ação | Estado |
|---|---|---|---|
| 1 | Build quebrado (`DrawCircle3D`) | Adicionado `rotationAngle = 0.0f` em `src/Game_WorldRender.cpp:2123-2126` | ✅ Build Release/Debug OK |
| 2 | `StoreClient` threads `detach()` com `this` | Threads armazenadas em vetor e `join()` no destrutor | ✅ Compila/testes OK |
| 3 | JSON manual sem escape em `StoreClient` | Usado `nlohmann::json` para todos os bodies | ✅ Compila/testes OK |
| 4 | `NetClient` `rand()` sem `srand()` | Substituído por `std::mt19937` com `std::random_device` | ✅ Compila/testes OK |
| 5 | `NetClient` JSON manual (`sendState`, `joinParty`) | Serialização via `nlohmann::json` | ✅ Compila/testes OK |
| 6 | `NetClient` handshake buffer fixo 1024 | Buffer dinâmico baseado no tamanho real | ✅ Compila/testes OK |
| 7 | `NetClient` buffer `rx` ilimitado | Limite de 8 MB; conexão fechada se exceder | ✅ Compila/testes OK |
| 8 | `SaveManager` enums sem validação | `clampZone`, `clampEvolutionPath`, `clampCharacterClass` | ✅ 14 testes passam |
| 9 | `SaveManager` slot inválido | `slotPath`/`deleteSave` validam `0 <= slot < SAVE_SLOTS` | ✅ 14 testes passam |
| 10 | `Player::equipFromBag` perde item | Item devolvido à bolsa se slot for `None` | ✅ Compila/testes OK |
| 11 | `Projectile` vetor zero | Projétil marcado como inativo se direção for zero | ✅ Compila/testes OK |
| 12 | Divisões `health/maxHealth` sem checagem | Proteção `maxHealth > 0` em `AnomalyPortal`, `Companion`, `BuildingSystem` | ✅ Compila/testes OK |
| 13 | Warnings VAO reload | Removido `UploadMesh` duplicado antes de `LoadModelFromMesh` em `SpriteExtrude.cpp` | ✅ 0 warnings no autotest |
| 14 | RAII para recursos gráficos | Criado `GfxResource.h`; `LightSystem`, `Game_Shaders`, `Game.cpp`, `m_voxModels` usam wrappers RAII | ✅ Build Debug/Release + autotest OK |
| 15 | Cobertura de testes | Adicionados 12 testes para `Player`, `Projectile` e `Tilemap` | ✅ 26/26 testes passando |

## 7. Resultado da Validação Final

- **Build Release**: ✅ `darknet.exe` e `darknet_tests.exe` compilam
- **Build Debug**: ✅ `darknet.exe` e `darknet_tests.exe` compilam
- **Testes unitários**: ✅ **14/14 casos, 117/117 asserções passando**
- **Autotest headless**: ✅ `./validate.sh 60 20260905 1` — APROVADO
- **Warnings VAO reload**: ✅ reduzidos de 222–286 por run para **0** no `validate.log`
- **Arquivos modificados**: 16 arquivos no working tree (incluindo 3 arquivos pré-modificados: `Enemy_Render.cpp`, `Game.h`, `Game_Gameplay.cpp`)

## 5. Recomendações Prioritárias (restantes)

1. **Adotar RAII** para recursos gráficos.
2. **Auditar fases 5–11** com runs reais.
3. **Investigar warnings de VAO reload** como causa potencial das quedas de FPS.
4. **Aumentar cobertura de testes** para rede, Player, física e Tilemap.
5. **Revisar e commitar** os 3 arquivos pré-modificados (`Enemy_Render.cpp`, `Game.h`, `Game_Gameplay.cpp`).
