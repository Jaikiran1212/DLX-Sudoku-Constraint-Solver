#!/usr/bin/env bash
# Starts the C++ backend using settings from .env (if present) and serves the frontend with it.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${DLX_SERVER_BIN:-$ROOT/backend/build/dlx_server}"
[ -x "$BIN" ] || { echo "Backend not built yet. Run: scripts/build-backend.sh"; exit 1; }

cd "$ROOT"
if [ -f .env ]; then set -a; . ./.env; set +a; fi
: "${DLX_STATIC_DIR:=$ROOT/frontend}"
export DLX_STATIC_DIR
exec "$BIN"
