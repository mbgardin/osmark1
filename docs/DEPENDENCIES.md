# Pocket OS — Dependency Versions

Specific version pins for Pocket OS build. Updated during each build cycle.

## Base Distribution

| Package | Version | Source |
|---------|---------|--------|
| Debian | 12 (Bookworm) | Official Debian archive |
| Linux kernel | 6.1.x (Bookworm default) | linux-image-amd64 |

## Build Tools (host Docker image)

| Package | Version |
|---------|---------|
| live-build | 20230612 |
| debootstrap | 1.0.128+ |
| xorriso | 1.5.4+ |
| grub-pc-bin | 2.06+ |
| grub-efi-amd64-bin | 2.06+ |
| mtools | 4.0.33+ |

*Versions resolved at build time from Debian Bookworm repos.*

---

*This file is auto-updated by build-tools/update-deps.sh (not yet implemented).*
