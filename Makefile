# Pocket OS — Makefile
# Convenience wrapper around build.sh and run-qemu.sh

.PHONY: all image run clean shell help wm shell-py

SHELL := /bin/bash
ISO := build/pocketos-amd64.iso

all: help

## image — Build the Pocket OS ISO
image:
	@./build.sh

## run — Run Pocket OS in QEMU (builds first if needed)
run: 
	@if [ ! -f "$(ISO)" ]; then \
		echo "ISO not found, building first..."; \
		$(MAKE) image; \
	fi
	@./run-qemu.sh

## run-bios — Run Pocket OS in QEMU with BIOS boot
run-bios:
	@./run-qemu.sh --bios

## clean — Remove all build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf build/work build/pocketos-amd64.iso
	@echo "Done."

## clean-all — Remove ALL build artifacts including VM disk
clean-all:
	@echo "Cleaning all artifacts..."
	@rm -rf build/
	@echo "Done."

## shell — Drop into the Docker build container shell
shell:
	@./build.sh --shell

## wm — Compile only the Pocket Window Manager
wm:
	@echo "Building pocket-wm..."
	@$(MAKE) -C desktop/pocket-wm

## help — Show this help message
help:
	@echo ""
	@echo "  Pocket OS Build System"
	@echo "  Created by Monte Gardiner"
	@echo ""
	@echo "  Targets:"
	@grep '## ' Makefile | grep -v grep | sed 's/## /    /' | sed 's/ — /\t/'
	@echo ""
