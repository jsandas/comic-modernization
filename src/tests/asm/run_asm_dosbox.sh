#!/usr/bin/env bash
set -euo pipefail

# Usage: run_asm_dosbox.sh <mode:0|1> <x> <y>
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
BUILD_COM="$ROOT/build/asm/collision_harness.com"
PATCHED="$ROOT/build/asm/collision_harness_patched.com"

if [[ ! -f "$BUILD_COM" ]]; then
  "$HERE/build_asm.sh"
fi

python3 "$HERE/patch_com.py" "$BUILD_COM" "$PATCHED" --mode "$1" --x "$2" --y "$3"

# Run in DOSBox (mount build dir as C:)
# DOSBox will write result.bin to the build dir
DOSBOX_CMD=(dosbox -noconsole -c "mount c \"$ROOT/build\"" -c "c:" -c "asm/collision_harness_patched.com" -c "exit")

echo "Running patched harness in DOSBox..."
"${DOSBOX_CMD[@]}"

# Read result
if [[ -f "$ROOT/build/result.bin" ]]; then
  hexdump -v -e '1/1 "%u" "\n"' "$ROOT/build/result.bin"
else
  echo "No result file produced" >&2
  exit 2
fi
