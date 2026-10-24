#!/usr/bin/env sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
BIN="$(mktemp)"
ACH_BIN="$(mktemp)"
trap 'rm -f "$BIN" "$ACH_BIN"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
  -I"$ROOT/tests/native" -I"$ROOT/include" \
  "$ROOT/src/pet/PetBehavior.cpp" "$ROOT/tests/native/PetBehaviorTests.cpp" \
  -o "$BIN"
"$BIN"

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
  "$ROOT/tests/native/AchievementProgressTests.cpp" \
  -o "$ACH_BIN"
"$ACH_BIN"

TIMER_BIN="$(mktemp)"
trap 'rm -f "$BIN" "$ACH_BIN" "$TIMER_BIN"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
  "$ROOT/tests/native/PomodoroTimerTests.cpp" \
  -o "$TIMER_BIN"
"$TIMER_BIN"
