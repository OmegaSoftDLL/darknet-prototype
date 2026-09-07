#!/usr/bin/env bash
# ------------------------------------------------------------------------------
# Darknet validation gate.
# Builds both configurations (the game must run in Release; Debug-only proves
# nothing) and runs the bot with a fixed seed. Exits non-zero if the build is
# not playable.
#   usage: ./validate.sh [seconds] [seed] [headless]
#   headless=1 runs without window/GPU (same mode as CI)
# ------------------------------------------------------------------------------
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
    echo "  (no relevant build output)"
  fi
  if [ ! -f "$ROOT/build/$CFG/darknet.exe" ]; then
    echo "FAILED: $CFG did not produce executable"; FAIL=1; continue
  fi
done

echo "== playable test (Release, ${SECS}s, seed $SEED) $EXTRA =="
( cd "$ROOT/build/Release" && ./darknet.exe --autotest --test-seconds="$SECS" --seed="$SEED" $EXTRA > validate.log 2>&1 )
CODE=$?
grep -E "VALIDACAO|FASES:|POSTFX|WORLDLIT" "$ROOT/build/Release/validate.log" | head -20
if [ "$CODE" -ne 0 ]; then
  echo "REPROVED (exit $CODE) - reasons above"; FAIL=1
else
  echo "APPROVED"
fi
exit $FAIL
