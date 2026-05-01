#!/usr/bin/env bash
set -euo pipefail

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; BOLD='\033[1m'; NC='\033[0m'
info()  { echo -e "${GREEN}[run]${NC} $*"; }
warn()  { echo -e "${YELLOW}[run]${NC} $*"; }
error() { echo -e "${RED}[run] ERROR:${NC} $*" >&2; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DAEMON_BIN="$PROJECT_ROOT/daemon/build/harmony"
PIDFILE_DAEMON="$PROJECT_ROOT/.harmony-daemon.pid"
PIDFILE_SERVER="$PROJECT_ROOT/.harmony-server.pid"
DAEMON_PORT=8080
FRONTEND_PORT=5500

# ── Preflight ─────────────────────────────────────────────────────────────────
if [[ ! -f "$DAEMON_BIN" ]]; then
    error "Daemon binary not found. Run setup first:"
    error "  ./scripts/setup.sh"
    exit 1
fi

if lsof -i ":$DAEMON_PORT" -sTCP:LISTEN -t &>/dev/null; then
    warn "Port $DAEMON_PORT already in use — daemon may already be running."
    warn "Run ./scripts/stop.sh first if you want a fresh start."
fi

# ── Start daemon ──────────────────────────────────────────────────────────────
info "Starting harmony daemon on port $DAEMON_PORT..."
cd "$PROJECT_ROOT/daemon/build"
"$DAEMON_BIN" &
DAEMON_PID=$!
echo "$DAEMON_PID" > "$PIDFILE_DAEMON"

# Wait up to 3s for daemon to be ready
for i in {1..6}; do
    if curl -sf "http://localhost:$DAEMON_PORT/health" &>/dev/null; then
        info "Daemon ready ✓  (pid $DAEMON_PID)"
        break
    fi
    sleep 0.5
    if [[ $i -eq 6 ]]; then
        error "Daemon did not respond on port $DAEMON_PORT after 3s."
        kill "$DAEMON_PID" 2>/dev/null || true
        exit 1
    fi
done

# ── Start frontend server ─────────────────────────────────────────────────────
info "Serving frontend on port $FRONTEND_PORT..."
python3 -m http.server "$FRONTEND_PORT" \
    --directory "$PROJECT_ROOT/frontend" \
    --bind 127.0.0.1 \
    &>/dev/null &
SERVER_PID=$!
echo "$SERVER_PID" > "$PIDFILE_SERVER"
info "Frontend ready ✓  (pid $SERVER_PID)"

# ── Open browser ──────────────────────────────────────────────────────────────
FRONTEND_URL="http://localhost:$FRONTEND_PORT"
if command -v wslview &>/dev/null; then
    wslview "$FRONTEND_URL" 2>/dev/null || true          # WSL2 → Windows browser
elif command -v open &>/dev/null; then
    open "$FRONTEND_URL" 2>/dev/null || true             # macOS
elif command -v xdg-open &>/dev/null; then
    xdg-open "$FRONTEND_URL" 2>/dev/null || true         # native Linux
else
    warn "Could not auto-open browser — navigate manually."
fi

# ── Running ───────────────────────────────────────────────────────────────────
echo ""
echo -e "${GREEN}${BOLD}Harmony is running${NC}"
echo -e "  Frontend → ${BOLD}$FRONTEND_URL${NC}"
echo -e "  Daemon   → ${BOLD}http://localhost:$DAEMON_PORT${NC}"
echo ""
echo -e "Press ${BOLD}Ctrl+C${NC} to stop, or run ${BOLD}./scripts/stop.sh${NC} from another terminal."

# ── Clean up on Ctrl+C or SIGTERM ─────────────────────────────────────────────
cleanup() {
    echo ""
    info "Shutting down..."
    kill "$DAEMON_PID" 2>/dev/null || true
    kill "$SERVER_PID" 2>/dev/null || true
    rm -f "$PIDFILE_DAEMON" "$PIDFILE_SERVER"
    info "Stopped."
}
trap cleanup SIGINT SIGTERM
wait
