#!/usr/bin/env bash
set -euo pipefail

# ── Color helpers ──────────────────────────────────────────────────────────────
GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; BOLD='\033[1m'; NC='\033[0m'
info()    { echo -e "${GREEN}[install-service]${NC} $*"; }
warn()    { echo -e "${YELLOW}[install-service]${NC} $*"; }
error()   { echo -e "${RED}[install-service] ERROR:${NC} $*" >&2; }
section() { echo -e "\n${BOLD}── $* ──────────────────────────────────────────────${NC}"; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
INSTALL_PREFIX="/opt/harmony"

# ── Preflight ─────────────────────────────────────────────────────────────────
section "Preflight"
if [[ "$(uname -s)" != "Linux" ]]; then
    error "systemd is Linux-only — this script does not run on macOS/WSL host."
    exit 1
fi
if ! command -v systemctl &>/dev/null; then
    error "systemctl not found — this system does not use systemd."
    exit 1
fi
if [[ ! -x "$PROJECT_ROOT/daemon/build/harmony" ]]; then
    error "Daemon binary not found. Build first:"
    error "  ./scripts/setup.sh"
    exit 1
fi
info "Preflight OK ✓"

# ── 1. Create dedicated low-privilege user ────────────────────────────────────
section "Creating system user 'harmony'"
if id -u harmony &>/dev/null; then
    warn "User 'harmony' already exists — skipping"
else
    sudo useradd --system --no-create-home --shell /usr/sbin/nologin harmony
    info "User created ✓"
fi

# ── 2. Install build artifacts to /opt/harmony ────────────────────────────────
section "Installing to $INSTALL_PREFIX"
sudo mkdir -p "$INSTALL_PREFIX/daemon/build"
sudo cp    "$PROJECT_ROOT/daemon/build/harmony" "$INSTALL_PREFIX/daemon/build/"
sudo cp -r "$PROJECT_ROOT/daemon/build/data"    "$INSTALL_PREFIX/daemon/build/"
sudo chown -R harmony:harmony "$INSTALL_PREFIX"
info "Files installed ✓"

# ── 3. Install systemd unit ───────────────────────────────────────────────────
section "Installing systemd unit"
sudo cp "$PROJECT_ROOT/daemon/deploy/harmony.service" /etc/systemd/system/
sudo systemctl daemon-reload
info "Unit installed ✓"

# ── 4. Enable + start ─────────────────────────────────────────────────────────
section "Enabling and starting service"
sudo systemctl enable harmony
sudo systemctl restart harmony
sleep 1
sudo systemctl status harmony --no-pager || true

# ── Done ──────────────────────────────────────────────────────────────────────
echo ""
echo -e "${GREEN}${BOLD}Service installed and running.${NC}"
echo -e "  Logs:      ${BOLD}journalctl -u harmony -f${NC}"
echo -e "  Stop:      ${BOLD}sudo systemctl stop harmony${NC}"
echo -e "  Uninstall: ${BOLD}sudo systemctl disable --now harmony && sudo rm /etc/systemd/system/harmony.service${NC}"
