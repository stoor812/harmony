#!/usr/bin/env bash
set -euo pipefail

# ── Color helpers ──────────────────────────────────────────────────────────────
GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; BOLD='\033[1m'; NC='\033[0m'
info()    { echo -e "${GREEN}[setup]${NC} $*"; }
warn()    { echo -e "${YELLOW}[setup]${NC} $*"; }
error()   { echo -e "${RED}[setup] ERROR:${NC} $*" >&2; }
section() { echo -e "\n${BOLD}── $* ──────────────────────────────────────────────${NC}"; }

# ── Project root ───────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# ── 1. Platform check ─────────────────────────────────────────────────────────
section "Platform check"
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    error "Run this script inside WSL2 (Ubuntu), not PowerShell or Git Bash."
    error "Open a WSL terminal: wsl -d Ubuntu"
    exit 1
fi

OS="$(uname -s)"
if [[ "$OS" == "Linux" ]]; then
    info "Running on Linux ✓"
elif [[ "$OS" == "Darwin" ]]; then
    info "Running on macOS ✓"
else
    error "Unsupported platform: $OS"
    exit 1
fi

# ── 2. Dependencies ───────────────────────────────────────────────────────────
section "Installing dependencies"
if [[ "$OS" == "Linux" ]]; then
    sudo apt-get update -qq
    sudo apt-get install -y \
        cmake \
        build-essential \
        libssl-dev \
        libcurl4-openssl-dev \
        python3 \
        git \
        curl
elif [[ "$OS" == "Darwin" ]]; then
    if ! command -v brew &>/dev/null; then
        error "Homebrew is required on macOS but was not found."
        error "Install it from: https://brew.sh"
        exit 1
    fi
    brew install cmake openssl curl git python3
fi
info "System packages installed ✓"

# ── 3. cpr library ────────────────────────────────────────────────────────────
section "Installing cpr (C++ HTTP client)"
if pkg-config --exists libcpr 2>/dev/null; then
    warn "cpr already installed — skipping"
else
    CPR_BUILD_DIR="$(mktemp -d)"
    git clone --depth=1 https://github.com/libcpr/cpr.git "$CPR_BUILD_DIR/cpr"
    cmake -B "$CPR_BUILD_DIR/build" -S "$CPR_BUILD_DIR/cpr" -DCPR_USE_SYSTEM_CURL=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build "$CPR_BUILD_DIR/build" --parallel
    sudo cmake --install "$CPR_BUILD_DIR/build"
    rm -rf "$CPR_BUILD_DIR"
    info "cpr installed ✓"
fi

# ── 4. Build daemon ───────────────────────────────────────────────────────────
section "Building daemon"
cmake -B "$PROJECT_ROOT/daemon/build" -S "$PROJECT_ROOT/daemon" -DCMAKE_BUILD_TYPE=Release
cmake --build "$PROJECT_ROOT/daemon/build" --parallel
info "Build complete ✓"

# ── 5. Run unit tests ─────────────────────────────────────────────────────────
section "Running unit tests"
if "$PROJECT_ROOT/daemon/build/test_harmony" --no-intro 2>/dev/null; then
    info "All tests passed ✓"
else
    error "Tests failed. Build may be broken."
    exit 1
fi

# ── Done ──────────────────────────────────────────────────────────────────────
echo -e "\n${GREEN}${BOLD}Setup complete!${NC}"
echo -e "Run the demo with: ${BOLD}./scripts/run.sh${NC}"
