#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
BUILD_DIR="$ROOT/build/asm"
mkdir -p "$BUILD_DIR"

echo "Assembling collision harness..."
cd "$HERE"

nasm -f bin collision_harness.asm -o "$BUILD_DIR/collision_harness.com"

echo "Built: $BUILD_DIR/collision_harness.com"

# Optionally copy to build root for other tools
cp "$BUILD_DIR/collision_harness.com" "$ROOT/build/"

echo "Done."