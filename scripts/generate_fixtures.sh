#!/usr/bin/env bash
set -euo pipefail

echo "Generating fixtures (assets/extraction)..."
make -C "${PWD%/scripts}"/assets/extraction

echo "Done."