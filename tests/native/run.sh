#!/usr/bin/env sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
BIN="$(mktemp)"
trap 'rm -f "$BIN"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
  -I"$ROOT/tests/native" -I"$ROOT/include" \
  "$ROOT/src/pet/PetBehavior.cpp" "$ROOT/tests/native/PetBehaviorTests.cpp" \
  -o "$BIN"
"$BIN"
