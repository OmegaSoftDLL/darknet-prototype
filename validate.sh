#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# PORTAO DE VALIDACAO do Darknet.
# Compila as DUAS configuracoes (o jogo e aberto pelo Release: Debug passando nao
# prova nada) e roda o bot com seed fixa. Sai != 0 se a build nao ficou jogavel.
#   uso: ./validate.sh [segundos] [seed] [headless]
#   headless=1 roda sem janela/GPU (mesmo modo da CI)
# ─────────────────────────────────────────────────────────────────────────────
set -u
SECS="${1:-100}"
SEED="${2:-20260821}"
EXTRA=""
if [ "${3:-}" = "1" ]; then EXTRA="--headless"; fi
CMAKE="/c/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
ROOT="$(cd "$(dirname "$0")" && pwd)"
FAIL=0

for CFG in Debug Release; do
  echo "== build $CFG =="
  if ! "$CMAKE" --build "$ROOT/build" --config "$CFG" 2>&1 | grep -E "error C|error LNK|darknet.vcxproj ->"; then
    echo "  (sem saida relevante do build)"
  fi
  if [ ! -f "$ROOT/build/$CFG/darknet.exe" ]; then
    echo "FALHOU: $CFG nao gerou executavel"; FAIL=1; continue
  fi
done

echo "== teste jogavel (Release, ${SECS}s, seed $SEED) $EXTRA =="
( cd "$ROOT/build/Release" && ./darknet.exe --autotest --test-seconds="$SECS" --seed="$SEED" $EXTRA > validate.log 2>&1 )
CODE=$?
grep -E "VALIDACAO|FASES:|POSTFX|WORLDLIT" "$ROOT/build/Release/validate.log" | head -20
if [ "$CODE" -ne 0 ]; then
  echo "REPROVADO (exit $CODE) - motivos acima"; FAIL=1
else
  echo "APROVADO"
fi
exit $FAIL
