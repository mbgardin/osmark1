#!/usr/bin/env bash
# Pocket OS — USB Flash Script
# Writes the Pocket OS ISO to a USB drive.
#
# SAFETY: This script NEVER automatically selects a disk.
#         It requires explicit user confirmation and device specification.
#
# Usage:
#   ./flash-usb.sh /dev/sdX
#
# Replace /dev/sdX with the actual USB device (NOT a partition).
# WARNING: ALL DATA ON THE DEVICE WILL BE ERASED.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ISO="$SCRIPT_DIR/build/pocketos-amd64.iso"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log()  { echo -e "${BLUE}[POCKET-USB]${NC} $*"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $*"; }
err()  { echo -e "${RED}[ERROR]${NC} $*" >&2; }
ok()   { echo -e "${GREEN}[OK]${NC} $*"; }

TARGET="${1:-}"

if [[ -z "$TARGET" ]]; then
    err "No target device specified."
    echo ""
    echo "Usage: ./flash-usb.sh /dev/sdX"
    echo ""
    echo "⚠️  ALL DATA ON THE TARGET DEVICE WILL BE ERASED."
    echo "    Never run this against a system disk."
    echo ""
    # Show available block devices for reference
    if command -v lsblk &>/dev/null; then
        echo "Available block devices:"
        lsblk -d -o NAME,SIZE,TYPE,MODEL,TRAN
    elif command -v diskutil &>/dev/null; then
        echo "Available disks (macOS):"
        diskutil list
    fi
    exit 1
fi

# Validate ISO
if [[ ! -f "$ISO" ]]; then
    err "ISO not found: $ISO"
    err "Run ./build.sh first."
    exit 1
fi

# Validate device exists
if [[ ! -b "$TARGET" ]]; then
    err "Device not found or not a block device: $TARGET"
    exit 1
fi

# Safety: refuse if target looks like an internal disk
case "$TARGET" in
    /dev/sda|/dev/nvme0n1|/dev/nvme1n1|/dev/disk0|/dev/disk1)
        err "REFUSING: $TARGET appears to be an internal system disk."
        err "Specify the actual USB device (e.g. /dev/sdb, /dev/disk2)."
        exit 1
        ;;
esac

ISO_SIZE=$(du -sh "$ISO" | cut -f1)
DEVICE_INFO=""
if command -v lsblk &>/dev/null; then
    DEVICE_INFO=$(lsblk -d -o NAME,SIZE,MODEL "$TARGET" 2>/dev/null | tail -1)
elif command -v diskutil &>/dev/null; then
    DEVICE_INFO=$(diskutil info "$TARGET" 2>/dev/null | grep "Device / Media Name" | awk -F: '{print $2}')
fi

echo ""
echo -e "${RED}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${RED}║  ⚠️   DESTRUCTIVE OPERATION — READ CAREFULLY                ║${NC}"
echo -e "${RED}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "  ISO:    $ISO ($ISO_SIZE)"
echo "  Target: $TARGET $DEVICE_INFO"
echo ""
echo -e "${RED}  ALL DATA ON $TARGET WILL BE PERMANENTLY ERASED.${NC}"
echo ""
warn "Are you ABSOLUTELY SURE you want to write to $TARGET?"
warn "Type the device path to confirm (or Ctrl-C to cancel):"
echo ""
read -p "  Confirm device: " CONFIRM

if [[ "$CONFIRM" != "$TARGET" ]]; then
    log "Confirmation did not match. Aborting."
    exit 0
fi

echo ""
log "Unmounting $TARGET partitions..."

# Unmount all partitions
if command -v diskutil &>/dev/null; then
    diskutil unmountDisk "$TARGET" || true
else
    for part in "$TARGET"[0-9]*; do
        umount "$part" 2>/dev/null || true
    done
fi

log "Writing $ISO to $TARGET..."
log "This may take several minutes..."

if command -v dd &>/dev/null; then
    sudo dd if="$ISO" of="$TARGET" bs=4M status=progress oflag=sync
elif command -v cp &>/dev/null; then
    sudo cp "$ISO" "$TARGET"
fi

sync

echo ""
ok "Done. $ISO written to $TARGET."
ok "Safely remove the USB drive and boot from it."
