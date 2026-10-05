#!/usr/bin/env bash
# Pocket OS — Image build script (runs INSIDE Docker container)
# This script runs inside the build container on linux/amd64.
# It configures and runs live-build to produce the Pocket OS ISO.

set -euo pipefail

log()  { echo "[POCKET-BUILD] $*"; }
ok()   { echo "[POCKET-BUILD] ✓ $*"; }
err()  { echo "[POCKET-BUILD] ERROR: $*" >&2; }

REPO_ROOT="/pocket-os"
BUILD_WORK="$REPO_ROOT/build/work"
OUTPUT_ISO="$REPO_ROOT/build/pocketos-amd64.iso"

mkdir -p "$BUILD_WORK"
cd "$BUILD_WORK"

log "Setting up live-build configuration..."

# Initialize live-build config
lb config \
    --architectures amd64 \
    --distribution bookworm \
    --debian-installer none \
    --archive-areas "main contrib non-free non-free-firmware" \
    --apt-options "--yes --allow-downgrades" \
    --bootappend-live "boot=live components hostname=pocketos username=pocket quiet splash" \
    --bootloaders "grub-efi,syslinux" \
    --binary-images iso-hybrid \
    --iso-application "Pocket OS" \
    --iso-publisher "Monte Gardiner" \
    --iso-volume "POCKETOS_AMD64" \
    --image-name "pocketos-amd64" \
    2>&1

ok "live-build config initialized."

# ── Package lists ─────────────────────────────────────────────────────────

log "Installing package lists..."

mkdir -p config/package-lists

# Base system packages
cat > config/package-lists/base.list.chroot << 'EOF'
# Pocket OS — Base System Packages
# These are installed into the live image chroot.

# Core utilities
bash
coreutils
util-linux
procps
psmisc
less
grep
sed
gawk
findutils
diffutils
file
tar
gzip
bzip2
xz-utils
zip
unzip
pciutils
usbutils
lshw
lsof
iproute2
iputils-ping
curl
wget
ca-certificates
sudo
nano
vim-tiny

# systemd
systemd
systemd-sysv
dbus
dbus-user-session
polkit

# Networking
network-manager
network-manager-gnome
wpasupplicant
iw
rfkill

# Audio (PipeWire stack)
pipewire
pipewire-audio
pipewire-alsa
pipewire-pulse
pipewire-jack
wireplumber
alsa-utils
pulseaudio-utils

# Bluetooth
bluez
blueman

# Firmware
firmware-linux
firmware-linux-nonfree
firmware-misc-nonfree
firmware-iwlwifi
firmware-realtek
firmware-atheros
firmware-brcm80211

# Display / X11
xserver-xorg
xserver-xorg-core
xserver-xorg-video-all
xserver-xorg-input-all
x11-xserver-utils
x11-utils
x11-apps
xinit
xauth
xterm

# GTK stack (for Pocket applications)
libgtk-3-0
librsvg2-common
adwaita-icon-theme

# Fonts
fonts-liberation
fonts-dejavu
fonts-noto-core

# Power management
acpi
acpid
tlp
tlp-rdw
powertop

# Bluetooth
bluetooth

# Hardware support
upower
udisks2
gvfs
gvfs-backends

# VTE (for Pocket Terminal)
libvte-2.91-0

# Python 3 (for Pocket Shell prototype)
python3
python3-gi
python3-gi-cairo
gir1.2-gtk-3.0
gir1.2-gdkpixbuf-2.0
gir1.2-pango-1.0
gir1.2-nm-1.0
gir1.2-vte-2.91
gir1.2-upower-glib-1.0
gir1.2-udisks-2.0

# Storage tools
dosfstools
ntfs-3g
exfatprogs
gparted

# Archive manager
file-roller

# Image viewer (minimal)
eog

# PDF viewer
evince

# Firefox ESR (web browser — not building our own engine)
firefox-esr

# LibreOffice (optional, add in Phase 9)
# libreoffice

# Development tools (present in live image for now)
build-essential
git
EOF

# Pocket OS custom packages (will be added in Phase 2+)
cat > config/package-lists/pocket.list.chroot << 'EOF'
# Pocket OS — Pocket-specific packages
# (populated as Pocket packages are built)
EOF

ok "Package lists created."

# ── Hooks ────────────────────────────────────────────────────────────────────

log "Installing build hooks..."
mkdir -p config/hooks/normal

# Hook: Set up os-release and hostname
cat > config/hooks/normal/0010-pocket-identity.hook.chroot << 'HOOKEOF'
#!/bin/bash
# Pocket OS identity hook — sets system identification

set -e

# /etc/os-release (LSB / systemd standard)
cat > /etc/os-release << 'EOF'
NAME="Pocket OS"
VERSION="0.1 (Alpha)"
ID=pocketos
ID_LIKE=debian
PRETTY_NAME="Pocket OS 0.1 (Alpha)"
VERSION_ID="0.1"
HOME_URL="https://github.com/montegardiner/pocketos"
SUPPORT_URL="https://github.com/montegardiner/pocketos/issues"
BUG_REPORT_URL="https://github.com/montegardiner/pocketos/issues"
PRIVACY_POLICY_URL=""
VERSION_CODENAME=alpha
DEBIAN_CODENAME=bookworm
BUILD_ID="$(date -u +%Y%m%d%H%M%S)"
EOF

# /etc/lsb-release (compatibility)
cat > /etc/lsb-release << 'EOF'
DISTRIB_ID=PocketOS
DISTRIB_RELEASE=0.1
DISTRIB_CODENAME=alpha
DISTRIB_DESCRIPTION="Pocket OS 0.1 (Alpha)"
EOF

# /etc/issue (login banner)
cat > /etc/issue << 'EOF'
Pocket OS 0.1 — Created by Monte Gardiner
Built on Debian GNU/Linux
\n \l

EOF

cat > /etc/issue.net << 'EOF'
Pocket OS 0.1 — Created by Monte Gardiner
Built on Debian GNU/Linux
EOF

# Hostname
echo "pocketos" > /etc/hostname

# /etc/hosts
cat > /etc/hosts << 'EOF'
127.0.0.1   localhost
127.0.1.1   pocketos
::1         localhost ip6-localhost ip6-loopback
ff02::1     ip6-allnodes
ff02::2     ip6-allrouters
EOF

echo "Pocket OS identity configured."
HOOKEOF

chmod +x config/hooks/normal/0010-pocket-identity.hook.chroot

# Hook: Configure default user
cat > config/hooks/normal/0020-pocket-user.hook.chroot << 'HOOKEOF'
#!/bin/bash
# Set up the default 'pocket' user for the live session

set -e

# The live-build 'pocket' user is created by casper/live-config
# Configure sudo access (live session)
if ! grep -q "^pocket" /etc/sudoers.d/pocket 2>/dev/null; then
    echo "pocket ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/pocket
    chmod 440 /etc/sudoers.d/pocket
fi

echo "Pocket user configured."
HOOKEOF

chmod +x config/hooks/normal/0020-pocket-user.hook.chroot

# Hook: Set up Pocket session
cat > config/hooks/normal/0030-pocket-session.hook.chroot << 'HOOKEOF'
#!/bin/bash
# Configure the Pocket OS X11 session

set -e

# Create pocket X11 session desktop entry
mkdir -p /usr/share/xsessions

cat > /usr/share/xsessions/pocket.desktop << 'EOF'
[Desktop Entry]
Name=Pocket OS
Comment=Pocket OS Desktop Environment
Exec=/usr/bin/pocket-session
Type=Application
DesktopNames=PocketOS
EOF

# Create pocket-session startup script
cat > /usr/bin/pocket-session << 'EOF'
#!/bin/bash
# Pocket OS Session Startup
# Starts pocket-wm (window manager) and pocket-shell (desktop)

export XDG_CURRENT_DESKTOP=PocketOS
export XDG_SESSION_TYPE=x11

# Set wallpaper path
export POCKET_WALLPAPER="/usr/share/pocket/wallpapers/pocket-default.png"

# Start D-Bus session if not already running
if [ -z "$DBUS_SESSION_BUS_ADDRESS" ]; then
    eval "$(dbus-launch --sh-syntax --exit-with-session)"
fi

# Start NetworkManager applet (tray)
nm-applet &

# Start PipeWire audio
/usr/bin/pipewire &
/usr/bin/pipewire-pulse &
/usr/bin/wireplumber &

# Launch Pocket WM
if [ -x /usr/bin/pocket-wm ]; then
    /usr/bin/pocket-wm &
    POCKET_WM_PID=$!
fi

# Launch Pocket Shell (desktop + taskbar)
if [ -x /usr/bin/pocket-shell ]; then
    exec /usr/bin/pocket-shell
elif [ -x /usr/bin/pocket-shell.py ]; then
    exec python3 /usr/bin/pocket-shell.py
else
    # Fallback: minimal session with xterm
    xterm &
    wait
fi
EOF

chmod +x /usr/bin/pocket-session

echo "Pocket session configured."
HOOKEOF

chmod +x config/hooks/normal/0030-pocket-session.hook.chroot

# Hook: Auto-login to Pocket session (live environment)
cat > config/hooks/normal/0040-pocket-autologin.hook.chroot << 'HOOKEOF'
#!/bin/bash
# Configure LightDM for auto-login into Pocket session

set -e

# Install LightDM if not present (it should be from package list)
apt-get install -y --no-install-recommends lightdm lightdm-gtk-greeter 2>/dev/null || true

# Configure LightDM
mkdir -p /etc/lightdm/lightdm.conf.d

cat > /etc/lightdm/lightdm.conf.d/50-pocket.conf << 'EOF'
[Seat:*]
autologin-user=pocket
autologin-user-timeout=0
user-session=pocket
greeter-session=lightdm-gtk-greeter
EOF

# Enable LightDM
systemctl enable lightdm 2>/dev/null || true

echo "LightDM auto-login configured."
HOOKEOF

chmod +x config/hooks/normal/0040-pocket-autologin.hook.chroot

# Add LightDM to packages
echo "lightdm" >> config/package-lists/base.list.chroot
echo "lightdm-gtk-greeter" >> config/package-lists/base.list.chroot

ok "Hooks created."

# ── Live config ───────────────────────────────────────────────────────────────

log "Copying Pocket assets into build..."

# Create placeholder directories in chroot overlay
mkdir -p config/includes.chroot/usr/share/pocket/{wallpapers,icons,themes,sounds}
mkdir -p config/includes.chroot/usr/share/pocket/desktop
mkdir -p config/includes.chroot/usr/bin

# Copy Pocket components from repo if they exist
POCKET_DESKTOP_SRC="$REPO_ROOT/desktop"
POCKET_ASSETS_SRC="$REPO_ROOT/assets"
POCKET_SYSTEM_SRC="$REPO_ROOT/system"
POCKET_THEMES_SRC="$REPO_ROOT/themes"

# Assets
if [ -d "$POCKET_ASSETS_SRC/wallpapers" ]; then
    cp -v "$POCKET_ASSETS_SRC/wallpapers/"* \
        config/includes.chroot/usr/share/pocket/wallpapers/ 2>/dev/null || true
fi

if [ -d "$POCKET_ASSETS_SRC/icons" ]; then
    cp -rv "$POCKET_ASSETS_SRC/icons/"* \
        config/includes.chroot/usr/share/pocket/icons/ 2>/dev/null || true
fi

# System overlays
if [ -d "$POCKET_SYSTEM_SRC/etc" ]; then
    mkdir -p config/includes.chroot/etc
    cp -rv "$POCKET_SYSTEM_SRC/etc/"* \
        config/includes.chroot/etc/ 2>/dev/null || true
fi

# Pocket shell (Python prototype)
if [ -f "$POCKET_DESKTOP_SRC/pocket-shell/pocket-shell.py" ]; then
    cp "$POCKET_DESKTOP_SRC/pocket-shell/pocket-shell.py" \
       config/includes.chroot/usr/bin/pocket-shell.py
    chmod +x config/includes.chroot/usr/bin/pocket-shell.py
fi

# Pocket WM binary (if compiled)
if [ -f "$POCKET_DESKTOP_SRC/pocket-wm/pocket-wm" ]; then
    cp "$POCKET_DESKTOP_SRC/pocket-wm/pocket-wm" \
       config/includes.chroot/usr/bin/pocket-wm
    chmod +x config/includes.chroot/usr/bin/pocket-wm
fi

ok "Assets staged."

# ── Build ─────────────────────────────────────────────────────────────────────

log "Running lb build..."
log "This will download Debian packages. First run may take 20-60 minutes."

lb build 2>&1

ok "lb build complete."

# ── Move ISO to output ────────────────────────────────────────────────────────

ISO_SOURCE="$BUILD_WORK/pocketos-amd64.hybrid.iso"
ISO_FALLBACK="$BUILD_WORK/live-image-amd64.hybrid.iso"

if [ -f "$ISO_SOURCE" ]; then
    mv "$ISO_SOURCE" "$OUTPUT_ISO"
elif [ -f "$ISO_FALLBACK" ]; then
    mv "$ISO_FALLBACK" "$OUTPUT_ISO"
else
    # Find any iso
    FOUND=$(find "$BUILD_WORK" -maxdepth 2 -name "*.iso" | head -1)
    if [ -n "$FOUND" ]; then
        mv "$FOUND" "$OUTPUT_ISO"
    else
        err "No ISO found in $BUILD_WORK"
        ls -la "$BUILD_WORK/"
        exit 1
    fi
fi

ok "ISO moved to: $OUTPUT_ISO"
ls -lh "$OUTPUT_ISO"
