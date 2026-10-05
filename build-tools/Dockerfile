# Pocket OS Build Environment
# Uses debian:bookworm as the build host for reproducible x86-64 image creation.
# Works on any Docker host including Apple Silicon (via Docker's x86-64 emulation).

FROM --platform=linux/amd64 debian:bookworm

LABEL maintainer="Monte Gardiner"
LABEL description="Pocket OS build environment"

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8

# Install build tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    # Live build toolchain
    live-build \
    debootstrap \
    # ISO creation
    xorriso \
    isolinux \
    syslinux-common \
    # EFI / GRUB
    grub-pc-bin \
    grub-efi-amd64-bin \
    grub-efi-amd64-signed \
    grub-common \
    # FAT image tools (for EFI)
    mtools \
    dosfstools \
    # Dev essentials
    git \
    make \
    build-essential \
    pkg-config \
    # GTK / X11 development (for compiling Pocket components)
    libgtk-3-dev \
    libglib2.0-dev \
    libx11-dev \
    libxft-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcomposite-dev \
    libxdamage-dev \
    libxrender-dev \
    libxext-dev \
    libxfixes-dev \
    libxcb1-dev \
    libvte-2.91-dev \
    # Cairo / Pango (for rendering in WM)
    libcairo2-dev \
    libpango1.0-dev \
    # Python (for shell prototype)
    python3 \
    python3-gi \
    python3-gi-cairo \
    gir1.2-gtk-3.0 \
    # Utilities
    rsync \
    curl \
    wget \
    squashfs-tools \
    genisoimage \
    cpio \
    pigz \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Create workspace
RUN mkdir -p /pocket-os/build /pocket-os/config /pocket-os/output

WORKDIR /pocket-os

# Default: build shell
CMD ["/bin/bash"]
