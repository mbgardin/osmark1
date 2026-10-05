#!/usr/bin/env bash
# Pocket OS — QEMU VM Test Runner
# Boots the Pocket OS ISO in QEMU for testing.
#
# Usage:
#   ./run-qemu.sh [OPTIONS]
#
# Options:
#   --bios     Boot in legacy BIOS mode (default: UEFI)
#   --ram N    RAM in MB (default: 2048)
#   --cores N  CPU cores (default: 2)
#   --vnc      Use VNC instead of SDL window
#
# Requirements:
#   QEMU: brew install qemu (macOS)
#         apt install qemu-system-x86 (Debian/Ubuntu)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ISO="$SCRIPT_DIR/build/pocketos-amd64.iso"

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

log()  { echo -e "${BLUE}[POCKET-VM]${NC} $*"; }
err()  { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# Defaults
BOOT_MODE="uefi"
RAM_MB=2048
CPU_CORES=2
DISPLAY_TYPE="sdl"

for arg in "$@"; do
    case "$arg" in
        --bios)   BOOT_MODE="bios" ;;
        --vnc)    DISPLAY_TYPE="vnc,127.0.0.1:1" ;;
        --ram)    shift; RAM_MB="$1" ;;
        --cores)  shift; CPU_CORES="$1" ;;
        --help|-h)
            grep '^#' "$0" | grep -v '^#!/' | sed 's/^# //' | sed 's/^#//'
            exit 0
            ;;
    esac
done

# Check QEMU
if ! command -v qemu-system-x86_64 &>/dev/null; then
    err "qemu-system-x86_64 not found."
    echo ""
    echo "Install QEMU:"
    echo "  macOS:  brew install qemu"
    echo "  Debian: sudo apt install qemu-system-x86"
    echo ""
    echo "After installing QEMU, run this script again."
    exit 1
fi

# Check ISO
if [[ ! -f "$ISO" ]]; then
    err "Pocket OS ISO not found at: $ISO"
    err "Run ./build.sh first to build the ISO."
    exit 1
fi

ISO_SIZE=$(du -sh "$ISO" | cut -f1)
log "Booting: $ISO ($ISO_SIZE)"
log "Mode: $BOOT_MODE | RAM: ${RAM_MB}MB | Cores: $CPU_CORES"
echo ""

# Create a small persistent disk (for future persistence support)
DISK="$SCRIPT_DIR/build/pocket-vm-disk.qcow2"
if [[ ! -f "$DISK" ]]; then
    log "Creating VM disk: $DISK (8GB)"
    qemu-img create -f qcow2 "$DISK" 8G
fi

# QEMU command construction
QEMU_ARGS=(
    -name "Pocket OS"
    -machine type=q35,accel=tcg
    -cpu qemu64
    -smp "$CPU_CORES"
    -m "$RAM_MB"
    -cdrom "$ISO"
    -drive "file=$DISK,format=qcow2,if=virtio"
    -boot order=dc
    -vga std
    -display "$DISPLAY_TYPE"
    -device virtio-net-pci,netdev=net0
    -netdev user,id=net0
    -device intel-hda
    -device hda-duplex
    -rtc base=localtime
    -no-reboot
)

# UEFI boot
if [[ "$BOOT_MODE" == "uefi" ]]; then
    # Find OVMF firmware
    OVMF_CODE=""
    OVMF_VARS=""

    for search_path in \
        "/usr/local/share/qemu/edk2-x86_64-code.fd" \
        "/usr/share/qemu/OVMF.fd" \
        "/usr/share/ovmf/OVMF.fd" \
        "/opt/homebrew/share/qemu/edk2-x86_64-code.fd" \
        "$(brew --prefix 2>/dev/null)/share/qemu/edk2-x86_64-code.fd"; do
        if [[ -f "$search_path" ]]; then
            OVMF_CODE="$search_path"
            break
        fi
    done

    if [[ -n "$OVMF_CODE" ]]; then
        log "UEFI firmware: $OVMF_CODE"
        QEMU_ARGS+=(
            -drive "if=pflash,format=raw,readonly=on,file=$OVMF_CODE"
        )
    else
        warn "OVMF not found, falling back to BIOS mode."
        log "Install: brew install qemu (includes OVMF)"
        BOOT_MODE="bios"
    fi
fi

if [[ "$DISPLAY_TYPE" == vnc* ]]; then
    log "VNC: connect to 127.0.0.1:5901"
fi

log "Starting QEMU..."
echo ""

qemu-system-x86_64 "${QEMU_ARGS[@]}"
