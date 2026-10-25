#!/usr/bin/env bash
# Starts the built backend on a free port and runs the jsdom UI smoke test against it.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${DLX_SERVER_BIN:-$ROOT/backend/build/dlx_server}"
[ -x "$BIN" ] || { echo "Backend not built: $BIN (see README: Build)"; exit 1; }
[ -d "$ROOT/node_modules/jsdom" ] || { echo "Run 'npm install' in $ROOT first (installs jsdom)."; exit 1; }

PORT="${TEST_PORT:-$(node -e "const s=require('net').createServer().listen(0,()=>{console.log(s.address().port);s.close()})")}"
LOG="$(mktemp)"
DLX_PORT="$PORT" "$BIN" >"$LOG" 2>&1 &
PID=$!
trap 'kill $PID 2>/dev/null || true; rm -f "$LOG"' EXIT
for _ in $(seq 1 50); do
  curl -fsS "http://127.0.0.1:$PORT/api/health" >/dev/null 2>&1 && break
  kill -0 $PID 2>/dev/null || { echo "Server exited early:"; cat "$LOG"; exit 1; }
  sleep 0.1
done
API_BASE="http://127.0.0.1:$PORT" node "$ROOT/tests/ui/ui-smoke.test.js"
