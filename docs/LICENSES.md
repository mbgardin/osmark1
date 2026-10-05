# Pocket OS — Dependency and License Registry

This document records all third-party components used in Pocket OS, their upstream
projects, licenses, and how they are used.

Pocket OS original components (window manager, shell, desktop applications, branding,
build tooling, configuration, original artwork) are the work of Monte Gardiner and
are licensed under the MIT License (see LICENSE).

---

## Core System

| Component | Upstream | License | Use |
|-----------|----------|---------|-----|
| Linux kernel | Linus Torvalds et al. | GPL-2.0-only | OS kernel |
| Debian GNU/Linux | Debian Project | Various (per package) | Base distribution, package ecosystem |
| GNU C Library (glibc) | Free Software Foundation | LGPL-2.1+ | C runtime |
| systemd | systemd maintainers | LGPL-2.1+ | Init system, session management |
| D-Bus | freedesktop.org | GPL-2.0+ / AFL-2.1 | IPC |
| udev (systemd) | systemd maintainers | LGPL-2.1+ | Device management |

## Display

| Component | Upstream | License | Use |
|-----------|----------|---------|-----|
| X.Org Server | X.Org Foundation | MIT/various | Display server |
| libX11 | X.Org Foundation | MIT | X11 client library |
| libXft | X.Org Foundation | MIT | Font rendering for X |
| libXrandr | X.Org Foundation | MIT | Multi-monitor / resolution |
| libXinerama | X.Org Foundation | MIT | Multi-monitor geometry |
| libXcomposite | X.Org Foundation | MIT | Compositing extension |
| libXdamage | X.Org Foundation | MIT | Damage tracking |
| libXrender | X.Org Foundation | MIT | Render extension |
| Mesa | Mesa3D contributors | MIT | OpenGL / GPU acceleration |

## Desktop / Toolkit

| Component | Upstream | License | Use |
|-----------|----------|---------|-----|
| GTK 3 | GNOME Project | LGPL-2.1+ | GUI toolkit for Pocket applications |
| GLib | GNOME Project | LGPL-2.1+ | Core GLib library |
| GDK-Pixbuf | GNOME Project | LGPL-2.1+ | Image loading |
| Cairo | freedesktop.org | LGPL-2.1 / MPL-1.1 | 2D rendering |
| Pango | GNOME Project | LGPL-2.1+ | Text layout |
| ATK | GNOME Project | LGPL-2.1+ | Accessibility |
| VTE (libvte) | GNOME Project | LGPL-2.1+ | Terminal emulator widget |

## Networking

| Component | Upstream | License | Use |
|-----------|----------|---------|-----|
| NetworkManager | Red Hat / GNOME | GPL-2.0+ | Network management |
| iwd / wpa_supplicant | Intel / Jouni Malinen | GPL-2.0 / ISC | Wi-Fi |

## Audio

| Component | Upstream | License | Use |
|-----------|----------|---------|-----|
| PipeWire | Wim Taymans et al. | MIT / LGPL-2.1+ | Audio/video routing |
| WirePlumber | Collabora et al. | MIT | PipeWire session manager |

## Bluetooth

| Component | Upstream | License | Use |
|-----------|----------|---------|-----|
| BlueZ | Marcel Holtmann et al. | GPL-2.0 | Bluetooth stack |

## Fonts

| Component | Upstream | License | Use |
|-----------|----------|---------|-----|
| Liberation Fonts | Red Hat | SIL OFL 1.1 | Default UI font (metric-compatible with Arial/Times) |
| DejaVu Fonts | DejaVu Project | Bitstream Vera License | Fallback / monospace |

## Build Tooling

| Component | Upstream | License | Use |
|-----------|----------|---------|-----|
| live-build | Debian Live team | GPL-3.0+ | Live image build framework |
| debootstrap | Debian | GPL-2.0+ | Debian base system bootstrap |
| GRUB 2 | GNU Project | GPL-3.0+ | Bootloader (UEFI + BIOS) |
| xorriso | Thomas Schmitt | GPL-3.0+ | ISO image creation |
| mtools | Alain Knaff | GPL-3.0+ | FAT image manipulation |
| Docker | Docker Inc. | Apache-2.0 | Build environment container |

---

## Notes

1. This document is updated as new dependencies are added.
2. Debian package dependencies are not individually enumerated here; they are
   captured in the `config/package-lists/` used by live-build.
3. GPL-licensed components require that source code be made available. Pocket OS
   relies on Debian's infrastructure for this obligation regarding Debian packages.
   Pocket-original GPL code (if any) will be provided in this repository.
4. No Microsoft-proprietary assets (icons, sounds, fonts, artwork, themes) are
   included in Pocket OS.

---

*See also: `docs/DEPENDENCIES.md` for specific package versions.*
