# --- Unified Makefile MCSOS M0/M1 ---
SHELL := /usr/bin/env bash
.DEFAULT_GOAL := help

# Variabel Direktori
BUILD_DIR := build
SMOKE_DIR := smoke
META_DIR  := $(BUILD_DIR)/meta

# Gabungan Phony Targets dari M0 dan M1
.PHONY: help meta check smoke proof qemu-version qemu-probe repro test tree clean distclean

help:
	@echo "MCSOS Build System Targets:"
	@echo "  make meta         - Collect toolchain metadata "
	@echo "  make check        - Verify environment & tools "
	@echo "  make smoke        - Build freestanding smoke test (M0 standard) "
	@echo "  make proof        - Build x86_64 ELF proof via script"
	@echo "  make qemu-version - Check QEMU availability "
	@echo "  make qemu-probe   - Verify QEMU machine & OVMF path"
	@echo "  make repro        - Run reproducibility audit"
	@echo "  make test         - Run all M0/M1 validation suites"
	@echo "  make tree         - Display repository structure"
	@echo "  make clean        - Remove generated artifacts"
	@echo "  make distclean    - Purge all build outputs"

# --- Target M0 (Sesuai Panduan PDF) ---

meta:
	@mkdir -p $(META_DIR)
	@bash tools/check_env.sh

check:
	@bash tools/check_env.sh
	@shellcheck tools/check_env.sh

smoke:
	@mkdir -p $(BUILD_DIR)/smoke
	clang --target=x86_64-unknown-none \
		-ffreestanding \
		-fno-stack-protector \
		-fno-pic \
		-mno-red-zone \
		-mno-mmx -mno-sse -mno-sse2 \
		-Wall -Wextra -Werror \
		-std=c17 \
		-c $(SMOKE_DIR)/freestanding.c \
		-o $(BUILD_DIR)/smoke/freestanding.o
	readelf -h $(BUILD_DIR)/smoke/freestanding.o | tee $(BUILD_DIR)/smoke/readelf-header.txt
	file $(BUILD_DIR)/smoke/freestanding.o | tee $(BUILD_DIR)/smoke/file.txt

qemu-version:
	@qemu-system-x86_64 --version
	@echo "QEMU exists. M0 does not boot a kernel image."

tree:
	@tree -a -L 3

# --- Target M1 (Otomasi Script Anda) ---

proof:
	@./tools/scripts/proof_compile.sh

qemu-probe:
	@./tools/scripts/qemu_probe.sh

repro:
	@./tools/scripts/repro_check.sh

test: meta check smoke proof qemu-probe repro
	@echo "OK: All M0/M1 environment and smoke tests passed."

# --- Cleanup ---

clean:
	rm -rf $(BUILD_DIR)/smoke build/proof build/repro
	@echo "OK: Cleaned generated artifacts."

distclean:
	rm -rf $(BUILD_DIR)
	@echo "OK: Removed all build outputs."
