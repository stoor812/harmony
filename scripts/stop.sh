#!/usr/bin/env bash
set -euo pipefail

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; NC='\033[0m'
info()  { echo -e "${GREEN}[stop]${NC} $*"; }
warn()  { echo -e "${YELLOW}[stop]${NC} $*"; }
error() { echo -e "${RED}[stop] ERROR:${NC} $*" >&2; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PIDFILE_DAEMON="$PROJECT_ROOT/.harmony-daemon.pid"
PIDFILE_SERVER="$PROJECT_ROOT/.harmony-server.pid"

stopped_any=false

# ── Stop via PID files (clean path) ───────────────────────────────────────────
stop_pid() {
    local pidfile="$1" label="$2"
    if [[ -f "$pidfile" ]]; then
        local pid
        pid=$(<"$pidfile")
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid"
            # Wait up to 5s for clean exit before SIGKILL
            for i in {1..10}; do
                kill -0 "$pid" 2>/dev/null || break
                sleep 0.5
            done
            if kill -0 "$pid" 2>/dev/null; then
                warn "$label (pid $pid) did not stop cleanly — sending SIGKILL"
                kill -9 "$pid" 2>/dev/null || true
            fi
            info "$label stopped ✓"
            stopped_any=true
        else
            warn "$label PID file found but process $pid is not running — cleaning up"
        fi
        rm -f "$pidfile"
    fi
}

stop_pid "$PIDFILE_DAEMON" "Daemon"
stop_pid "$PIDFILE_SERVER" "Frontend server"

# ── Fallback: kill by process name if PID files are missing ───────────────────
if pkill -f "daemon/build/harmony" 2>/dev/null; then
    info "Daemon stopped via process name ✓"
    stopped_any=true
fi
if pkill -f "http.server 5500" 2>/dev/null; then
    info "Frontend server stopped via process name ✓"
    stopped_any=true
fi

# ── Result ────────────────────────────────────────────────────────────────────
if [[ "$stopped_any" == true ]]; then
    echo -e "\n${GREEN}Harmony stopped cleanly.${NC}"
else
    warn "Nothing was running."
fi
