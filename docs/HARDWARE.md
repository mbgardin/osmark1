# Pocket OS — Hardware Compatibility Notes

Target hardware is any conventional x86-64 PC supported by Debian Bookworm.

## ThinkPad X1 Carbon (Primary Test Device)

Generation and exact model to be confirmed.

Testing will begin in Phase 14, after VM testing is mature.

### Planned Test Matrix

| Feature | Status | Notes |
|---------|--------|-------|
| Boot (UEFI) | ⬜ Untested | |
| Boot (BIOS) | ⬜ Untested | |
| Graphics | ⬜ Untested | Intel iGPU expected |
| Keyboard | ⬜ Untested | |
| TrackPad / TrackPoint | ⬜ Untested | |
| Wi-Fi | ⬜ Untested | Intel Wi-Fi expected |
| Bluetooth | ⬜ Untested | |
| Audio | ⬜ Untested | |
| Battery / Power | ⬜ Untested | |
| Suspend / Resume | ⬜ Untested | |
| USB | ⬜ Untested | |
| Internal Storage | ⬜ Untested | NVMe expected |
| Display Brightness | ⬜ Untested | |
| Camera | ⬜ Untested | |
| Fingerprint Reader | ⬜ Untested | |
| Thunderbolt | ⬜ Untested | |

## VM Testing (QEMU)

| VM Configuration | Status |
|-----------------|--------|
| QEMU/KVM amd64 UEFI | ⬜ Pending Phase 1 |
| QEMU/KVM amd64 BIOS | ⬜ Pending Phase 1 |

---

*Updated as hardware testing is completed.*
