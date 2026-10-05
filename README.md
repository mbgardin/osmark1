# Pocket OS

**Pocket OS** is a personal x86-64 Linux operating system created by **Monte Gardiner**.

It is built on a Debian Stable foundation and provides an original desktop experience
inspired by the friendly, colorful aesthetic of the early-2000s Windows era — with its
own distinct Pocket identity, branding, and engineering.

---

## What is Pocket OS?

Pocket OS is not a simple reskin of Debian. It is a complete desktop operating system
that layers Pocket-specific components on top of the mature Debian/Linux base:

```
┌────────────────────────────────────┐
│             Pocket OS              │
├────────────────────────────────────┤
│          Pocket Desktop            │
│                                    │
│ Pocket Shell                       │
│ Pocket Window Manager              │
│ Pocket Explorer                    │
│ Pocket Settings                    │
│ Pocket Task Manager                │
│ Pocket Terminal                    │
├────────────────────────────────────┤
│       Pocket System Services       │
├────────────────────────────────────┤
│          Debian Userspace          │
├────────────────────────────────────┤
│           Linux Kernel             │
├────────────────────────────────────┤
│             Hardware               │
└────────────────────────────────────┘
```

---

## Project Status

| Phase | Description | Status |
|-------|-------------|--------|
| 0 | Repository + Linux Build Environment | 🟡 In Progress |
| 1 | Minimal Pocket Linux Image | ⬜ Pending |
| 2 | Pocket Branding | ⬜ Pending |
| 3 | Graphical Foundation (Xorg) | ⬜ Pending |
| 4 | Pocket Window Manager Prototype | ⬜ Pending |
| 5 | XP-Inspired Window Experience | ⬜ Pending |
| 6 | Pocket Shell | ⬜ Pending |
| 7 | Pocket Theme | ⬜ Pending |
| 8 | Pocket Explorer | ⬜ Pending |
| 9 | System Applications | ⬜ Pending |
| 10 | Hardware Integration | ⬜ Pending |
| 11 | Application Management | ⬜ Pending |
| 12 | Live ISO | ⬜ Pending |
| 13 | Persistent USB | ⬜ Pending |
| 14 | ThinkPad Testing | ⬜ Pending |
| 15 | Installer | ⬜ Pending |
| 16 | Polish / 1.0 | ⬜ Pending |

---

## Quick Start (Developer)

### Requirements

- Docker (running, x86-64/amd64 image support)
- macOS or Linux host (Apple Silicon supported via Docker buildx)
- ~10 GB free disk space for build artifacts

### Build

```bash
# Build the Pocket OS ISO
./build.sh

# Run in QEMU (after building)
./run-qemu.sh
```

The ISO will be produced at `build/pocketos-amd64.iso`.

---

## Architecture

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for full design decisions.

## Dependencies & Licenses

See [`docs/LICENSES.md`](docs/LICENSES.md) and [`docs/DEPENDENCIES.md`](docs/DEPENDENCIES.md).

---

## Attribution

Pocket OS is built on:

- **Linux kernel** — various authors, GPLv2
- **Debian GNU/Linux** — Debian Project, various licenses
- **Xorg / X.Org** — X.Org Foundation, MIT/various
- **systemd** — various authors, LGPLv2.1+
- **NetworkManager** — Red Hat/GNOME Project, GPLv2+

Pocket-specific components (window manager, shell, desktop, branding, configuration,
build tooling, system integration, and original artwork) are the work of the Pocket OS
project, created by Monte Gardiner.

---

*Pocket OS — A personal operating system.*
*Created by Monte Gardiner.*
