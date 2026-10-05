#!/usr/bin/env bash
# Pocket OS — Main Build Script
# Builds the Pocket OS x86-64 ISO using a Docker-based Debian build environment.
#
# Usage:
#   ./build.sh [OPTIONS]
#
# Options:
#   --clean     Clean previous build artifacts before building
#   --shell     Drop into the build container shell (for debugging)
#   --no-cache  Rebuild Docker image without cache
#
# Output:
#   build/pocketos-amd64.iso

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$SCRIPT_DIR"
BUILD_DIR="$REPO_ROOT/build"
DOCKER_IMAGE="pocketos-builder:latest"
DOCKERFILE="$REPO_ROOT/build-tools/Dockerfile"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

log()  { echo -e "${BLUE}[POCKET]${NC} $*"; }
ok()   { echo -e "${GREEN}[OK]${NC} $*"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $*"; }
err()  { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# Parse arguments
CLEAN=false
SHELL_MODE=false
NO_CACHE=""

for arg in "$@"; do
    case "$arg" in
        --clean)    CLEAN=true ;;
        --shell)    SHELL_MODE=true ;;
        --no-cache) NO_CACHE="--no-cache" ;;
        --help|-h)
            grep '^#' "$0" | grep -v '^#!/' | sed 's/^# //' | sed 's/^#//'
            exit 0
            ;;
        *)
            err "Unknown argument: $arg"
            exit 1
            ;;
    esac
done

echo ""
echo -e "${CYAN}╔══════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║          P O C K E T   O S              ║${NC}"
echo -e "${CYAN}║       Build System v0.1                  ║${NC}"
echo -e "${CYAN}║       Created by Monte Gardiner          ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════╝${NC}"
echo ""

# ── Prerequisites check ─────────────────────────────────────────────────────

log "Checking prerequisites..."

if ! command -v docker &>/dev/null; then
    err "Docker is not installed or not in PATH."
    err "Install Docker Desktop: https://docs.docker.com/get-docker/"
    exit 1
fi

if ! docker info &>/dev/null; then
    err "Docker daemon is not running. Please start Docker Desktop."
    exit 1
fi

ok "Docker is available."

# ── Clean ────────────────────────────────────────────────────────────────────

if [[ "$CLEAN" == "true" ]]; then
    log "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    ok "Build directory cleaned."
fi

mkdir -p "$BUILD_DIR"

# ── Build Docker image ───────────────────────────────────────────────────────

log "Building Docker build environment..."
log "  Image: $DOCKER_IMAGE"
log "  Platform: linux/amd64"

docker build \
    --platform linux/amd64 \
    $NO_CACHE \
    -t "$DOCKER_IMAGE" \
    -f "$DOCKERFILE" \
    "$REPO_ROOT/build-tools" \
    2>&1 | sed 's/^/  /'

ok "Build environment ready."

# ── Shell mode ───────────────────────────────────────────────────────────────

if [[ "$SHELL_MODE" == "true" ]]; then
    log "Dropping into build container shell..."
    docker run \
        --platform linux/amd64 \
        --rm \
        -it \
        --privileged \
        -v "$REPO_ROOT:/pocket-os" \
        -w /pocket-os \
        "$DOCKER_IMAGE" \
        /bin/bash
    exit 0
fi

# ── Run live-build inside Docker ─────────────────────────────────────────────

log "Starting Pocket OS image build inside Docker..."
log "  This may take 20-60 minutes on first run (downloads Debian packages)."
echo ""

docker run \
    --platform linux/amd64 \
    --rm \
    --privileged \
    -v "$REPO_ROOT:/pocket-os" \
    -w /pocket-os \
    "$DOCKER_IMAGE" \
    /bin/bash /pocket-os/build-tools/build-image.sh

# ── Verify output ────────────────────────────────────────────────────────────

ISO_PATH="$BUILD_DIR/pocketos-amd64.iso"

if [[ -f "$ISO_PATH" ]]; then
    ISO_SIZE=$(du -sh "$ISO_PATH" | cut -f1)
    echo ""
    echo -e "${GREEN}╔══════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║       BUILD SUCCESSFUL                       ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════╝${NC}"
    echo ""
    ok "ISO: $ISO_PATH ($ISO_SIZE)"
    echo ""
    log "To test in QEMU:  ./run-qemu.sh"
    log "To flash to USB:  ./flash-usb.sh /dev/sdX  (requires confirmation)"
    echo ""
else
    err "Build completed but ISO not found at: $ISO_PATH"
    err "Check the build log above for errors."
    exit 1
fi
