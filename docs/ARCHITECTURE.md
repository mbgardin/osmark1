# Pocket OS — Architecture

**Document version:** 0.1  
**Status:** Living document — updated as design evolves

---

## 1. Overview

Pocket OS is a personal x86-64 Linux operating system built by layering original
Pocket-specific desktop and system components on top of a Debian Stable base.

The goal is a coherent desktop experience that feels like a modern, original
interpretation of early-2000s Windows desktop design — friendly, colorful, and
immediately familiar — while retaining the full power and security of a modern
Linux/Debian foundation.

---

## 2. Layer Model

```
┌────────────────────────────────────────────────────┐
│                    Pocket OS                       │
├────────────────────────────────────────────────────┤
│                  Pocket Desktop                    │
│                                                    │
│  Pocket Shell      – desktop, taskbar, start menu  │
│  Pocket WM         – X11 window manager            │
│  Pocket Explorer   – file manager                  │
│  Pocket Settings   – control panel                 │
│  Pocket Task Mgr   – process/resource viewer       │
│  Pocket Terminal   – terminal emulator             │
│  Pocket Notepad    – text editor                   │
├────────────────────────────────────────────────────┤
│               Pocket System Services               │
│   branding, session, theme, integration layer      │
├────────────────────────────────────────────────────┤
│                 Debian Userspace                   │
│   apt, dpkg, systemd, NetworkManager,              │
│   PipeWire, BlueZ, D-Bus, udev, Mesa               │
├────────────────────────────────────────────────────┤
│                  Linux Kernel                      │
├────────────────────────────────────────────────────┤
│                   Hardware                         │
└────────────────────────────────────────────────────┘
```

---

## 3. Display System

**Pocket OS 1.x: X11 / Xorg**

Rationale:
- Excellent compatibility with existing Linux GUI applications
- Permits a fully custom window manager without a compositor protocol
- Stable, well-documented, and supported on target hardware
- Available in Debian stable

Wayland is a future objective. The Pocket application layer will be designed
to minimize assumptions about the underlying display protocol, so a future
Wayland compositor can be investigated without rewriting all Pocket applications.

---

## 4. Pocket Window Manager (pocket-wm)

**Language:** C (with possible C++ extensions)  
**Protocol:** X11 / ICCCM / EWMH  
**Toolkit:** Direct Xlib/XCB — no DE toolkit dependency for the WM core

pocket-wm is a reparenting X11 window manager. It provides:

- Custom title bars and window borders (XP-inspired aesthetic)
- Window controls: close, minimize, maximize, restore
- Focus management (click-to-focus, with configurable options)
- Window stacking and raise/lower
- Move and resize via title bar / border drag
- Keyboard shortcuts (Alt+F4, Alt+Tab, Super/Win key, etc.)
- EWMH compliance for taskbar integration
- Multi-monitor awareness via Xinerama/RandR

Design principle: pocket-wm should be stable and correct before it is beautiful.
Stability and X11 compliance are non-negotiable for Phase 4.

---

## 5. Pocket Shell (pocket-shell)

**Language:** C++ or Python (TBD — Python for rapid prototyping, C++ for shipping)  
**Toolkit:** GTK 3 (styled with Pocket theme, overriding upstream GTK visuals)

pocket-shell provides:

- Desktop rendering (wallpaper, desktop icons)
- Taskbar (running application buttons, system tray, clock)
- Start-style Pocket menu (application launcher)
- Logout / lock / restart / shutdown controls
- Context menus on desktop
- Session management (starts pocket-wm, sets up environment)

The shell and WM are separate processes communicating via EWMH/X11 atoms
and optionally a lightweight IPC mechanism (D-Bus or Unix sockets).

---

## 6. Pocket Theme

GTK 3 CSS theme providing XP-inspired visual treatment:

- Title bars: strong blue gradient
- Window buttons: rounded, with highlight
- Controls: colorful, slightly beveled
- Menus: classic style
- Fonts: Liberation Sans (open-source, metric-compatible with Windows fonts)
- Icons: Pocket original icon set (SVG-sourced)

The theme does NOT copy Microsoft Luna theme assets.

---

## 7. Pocket Applications

| Application | Toolkit | Notes |
|-------------|---------|-------|
| pocket-explorer | GTK 3 | GIO/GVfs backend |
| pocket-terminal | VTE + GTK 3 | Wraps bash initially |
| pocket-notepad | GTK 3 | GtkTextView based |
| pocket-settings | GTK 3 | Frontends for Linux services |
| pocket-taskmgr | GTK 3 | /proc and D-Bus backend |

---

## 8. System Services Integration

Pocket does NOT reinvent these — it provides graphical frontends:

| Service | Pocket interface |
|---------|-----------------|
| NetworkManager | pocket-settings / tray applet |
| PipeWire | volume tray, pocket-settings audio page |
| BlueZ | pocket-settings Bluetooth page |
| systemd-logind | session/power actions |
| polkit | privilege escalation dialogs |
| udev | auto-mount, tray notifications |
| udisks2 | storage in pocket-explorer |

---

## 9. Build System

**Build host:** Docker container — `debian:bookworm` (amd64)  
**Why Docker:** Reproducible Debian build environment on any host OS, including
the developer's Apple Silicon macOS machine.

**Primary script:** `build.sh` — orchestrates Docker build, debootstrap, live-cd
image assembly, and ISO production.

**Output:** `build/pocketos-amd64.iso`

**VM testing:** `run-qemu.sh` — launches QEMU with the built ISO.

---

## 10. ISO / Live Image

**Format:** hybrid ISO-9660 / El Torito  
**Boot:** UEFI (GRUB 2) primary; legacy BIOS fallback  
**Framework:** live-build (Debian's official live system tooling)  
**Persistence:** casper-style persistence volume (Phase 13)

---

## 11. Package Management

Standard Debian: `apt` / `dpkg`

Pocket-specific packages will be `.deb` packages installed from a local
repo during image build. Eventually a signed Pocket package repository.

---

## 12. Language Decision Summary

| Component | Language | Rationale |
|-----------|----------|-----------|
| pocket-wm | C | Minimal runtime, direct Xlib/XCB, proven WM pattern |
| pocket-shell | Python → C++ | Python for prototyping speed; C++ for production |
| pocket-explorer | C / GTK 3 | GIO/GVfs for filesystem; GLib event loop |
| pocket-terminal | C / VTE | VTE is the natural fit for a GTK terminal |
| pocket-notepad | C / GTK 3 | Straightforward GtkTextView application |
| pocket-settings | Python + GTK 3 | Scripting D-Bus/NM from Python is ergonomic |
| pocket-taskmgr | C / GTK 3 | /proc reads benefit from C performance |
| Build tooling | Bash + Makefile | Universal on Debian, no extra dependency |
| Image assembly | Bash | Simple, auditable |

---

## 13. Security Design

- Desktop runs as a normal user, never root
- `polkit` gates privileged settings operations
- No GUI application has unrestricted sudo
- Standard Debian ASLR, NX, stack-smashing protection retained
- AppArmor profiles planned (Phase 16)
- Full-disk encryption planned (Phase 15 installer)
- Secure Boot planned (Phase 15+)
- Firewall: `nftables` / `ufw` default-enabled

---

## 14. Target Hardware

**Primary target:** x86-64 PCs (broad Debian hardware support)  
**Key test device:** Lenovo ThinkPad X1 Carbon (generation TBD)

Pocket should not be architected around one specific laptop. Pocket relies
on Debian's hardware support. ThinkPad-specific notes are tracked in
`docs/HARDWARE.md` only after Phase 12 (VM testing mature).

---

*Last updated: Phase 0*
