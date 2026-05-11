# =============================================================================
# MCSOS Unified Build System — M0 / M1 / M2
# Target arsitektur : x86_64
# Host              : WSL 2 Linux (Ubuntu/Debian)
# Compiler          : Clang/LLVM + LLD
# =============================================================================

SHELL := /usr/bin/env bash
.DEFAULT_GOAL := help

# -----------------------------------------------------------------------------
# Direktori
# -----------------------------------------------------------------------------
BUILD_DIR  := build
SMOKE_DIR  := smoke
META_DIR   := $(BUILD_DIR)/meta
KERNEL_DIR := kernel
ISO_ROOT   := iso_root

# -----------------------------------------------------------------------------
# Toolchain
# -----------------------------------------------------------------------------
CC      := clang
LD      := ld.lld
OBJDUMP := objdump
READELF := readelf
NM      := nm

# -----------------------------------------------------------------------------
# Flags M2 (kernel build)
# -----------------------------------------------------------------------------
CFLAGS := \
	--target=x86_64-unknown-none-elf \
	-std=c17 \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-pic \
	-fno-pie \
	-fno-lto \
	-m64 \
	-march=x86-64 \
	-mabi=sysv \
	-mno-red-zone \
	-mno-mmx \
	-mno-sse \
	-mno-sse2 \
	-mcmodel=kernel \
	-Wall \
	-Wextra \
	-Werror \
	-Ikernel/arch/x86_64/include \
	-Ikernel/include

LDFLAGS := \
	-nostdlib \
	-static \
	-z max-page-size=0x1000 \
	-T linker.ld \
	-Map=$(BUILD_DIR)/kernel.map

PANIC_CFLAGS := $(CFLAGS) -DMCSOS_M3_TRIGGER_PANIC=1

PANIC_KERNEL := $(BUILD_DIR)/kernel.panic.elf
PANIC_MAP    := $(BUILD_DIR)/kernel.panic.map

# -----------------------------------------------------------------------------
# Source & object M2
# -----------------------------------------------------------------------------
KERNEL := $(BUILD_DIR)/kernel.elf
MAP    := $(BUILD_DIR)/kernel.map
SRC_C  := $(shell find $(KERNEL_DIR) -name '*.c' 2>/dev/null | LC_ALL=C sort)
OBJ    := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC_C))
PANIC_OBJ    := $(patsubst %.c,$(BUILD_DIR)/panic/%.o,$(SRC_C))

# -----------------------------------------------------------------------------
# Phony targets
# -----------------------------------------------------------------------------
.PHONY: help \
        meta check smoke qemu-version tree \
        proof qemu-probe repro \
        test \
        check-prev check-src check-scripts \
        build inspect image run debug \
	panic audit \
        grade \
        clean distclean

# =============================================================================
# HELP
# =============================================================================
help:
	@echo ""
	@echo "MCSOS Build System — M0 / M1 / M2"
	@echo "======================================"
	@echo ""
	@echo "── M0/M1: Environment & Toolchain ──"
	@echo "  make meta          Collect toolchain metadata"
	@echo "  make check         Verify environment & toolchain"
	@echo "  make smoke         Build freestanding smoke object (M0)"
	@echo "  make proof         Build freestanding x86_64 ELF proof (M1)"
	@echo "  make qemu-version  Check QEMU availability (M0)"
	@echo "  make qemu-probe    Verify QEMU machine & OVMF (M1)"
	@echo "  make repro         Run reproducibility audit (M1)"
	@echo "  make test          Run all M0/M1 validation checks"
	@echo "  make tree          Display repository structure"
	@echo ""
	@echo "── M2: Kernel Build ──"
	@echo "  make check-prev    M1 preflight gate sebelum M2"
	@echo "  make check-src     Verify toolchain & source structure M2"
	@echo "  make check-scripts Lint semua shell script di tools/scripts/"
	@echo "  make build         Compile kernel ELF ($(KERNEL))"
	@echo "  make inspect       Inspect kernel ELF (readelf, objdump, nm)"
	@echo "  make image         Build bootable ISO image"
	@echo "  make run           Run kernel di QEMU"
	@echo "  make debug         Run kernel di QEMU dengan GDB stub"
	@echo "  make grade         Full M2 build + inspect + run + grade check"
	@echo ""
	@echo "── Cleanup ──"
	@echo "  make clean         Remove generated artifacts (proof, smoke, kernel)"
	@echo "  make distclean     Purge all build outputs"
	@echo ""

# =============================================================================
# M0 — Environment Baseline
# =============================================================================
meta:
	@mkdir -p $(META_DIR)
	@bash tools/check_env.sh

check:
	@bash tools/check_env.sh
	@shellcheck --severity=warning tools/check_env.sh

smoke:
	@mkdir -p $(BUILD_DIR)/smoke
	@clang --target=x86_64-unknown-none \
		-ffreestanding \
		-fno-stack-protector \
		-fno-pic \
		-mno-red-zone \
		-mno-mmx -mno-sse -mno-sse2 \
		-Wall -Wextra -Werror \
		-std=c17 \
		-c $(SMOKE_DIR)/freestanding.c \
		-o $(BUILD_DIR)/smoke/freestanding.o
	@readelf -h $(BUILD_DIR)/smoke/freestanding.o | tee $(BUILD_DIR)/smoke/readelf-smoke.txt
	@file $(BUILD_DIR)/smoke/freestanding.o       | tee $(BUILD_DIR)/smoke/file-smoke.txt

qemu-version:
	@qemu-system-x86_64 --version
	@echo "QEMU exists. M0 does not boot a kernel image."

tree:
	@tree -a -L 3

# Jalankan semua gate M0/M1
test: meta check smoke proof qemu-probe repro
	@echo "OK: All M0/M1 environment and smoke tests passed."

# =============================================================================
# M1 — Reproducible Toolchain Proof
# =============================================================================
proof:
	@./tools/scripts/proof_compile.sh

qemu-probe:
	@./tools/scripts/qemu_probe.sh

repro:
	@./tools/scripts/repro_check.sh

# =============================================================================
# M2 — Kernel Build
# =============================================================================

# Gate: pastikan M1 sudah lulus sebelum membangun kernel
check-prev:
	@./tools/scripts/m2_preflight.sh

# Verifikasi toolchain dan struktur source M2
check-src:
	@$(CC) --version | head -n 1
	@$(LD) --version | head -n 1
	@test -f linker.ld \
		|| { echo "ERROR: linker.ld tidak ditemukan"; exit 1; }
	@test -d kernel/core \
		|| { echo "ERROR: kernel/core tidak ditemukan"; exit 1; }
	@test -d kernel/lib \
		|| { echo "ERROR: kernel/lib tidak ditemukan"; exit 1; }
	@test -d kernel/arch/x86_64/include \
		|| { echo "ERROR: kernel/arch/x86_64/include tidak ditemukan"; exit 1; }

# Lint semua shell script — syntax check dulu, lalu shellcheck
check-scripts:
	@find tools/scripts -name '*.sh' -exec bash -n {} \;
	@if command -v shellcheck >/dev/null 2>&1; then \
		find tools/scripts -name '*.sh' \
			-exec shellcheck --severity=warning {} \;; \
	else \
		echo "WARN: shellcheck tidak tersedia"; \
	fi

# Compile seluruh source kernel
build: check-prev check-src check-scripts $(KERNEL)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJ) linker.ld
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

# Inspect kernel ELF
inspect: $(KERNEL)
	@./tools/scripts/inspect_kernel.sh

# Build bootable ISO
image: $(KERNEL)
	@./tools/scripts/make_iso.sh

# Jalankan kernel di QEMU (normal)
run: image
	@./tools/scripts/run_qemu.sh

# Jalankan kernel di QEMU dengan GDB stub
debug: image
	@./tools/scripts/run_qemu_debug.sh

# Build varian intentional panic
panic: check-prev check-src $(PANIC_KERNEL)

$(BUILD_DIR)/panic/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(PANIC_CFLAGS) -c $< -o $@

$(PANIC_KERNEL): $(PANIC_OBJ) linker.ld
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -Map=$(PANIC_MAP) -o $@ $(PANIC_OBJ)

# Audit ELF dan disassembly
audit: inspect panic
	@./tools/scripts/m3_audit_elf.sh build/kernel.elf

# Full M3 pipeline
grade: check-src check-scripts build inspect image run
	@./tools/scripts/grade_m3.sh
# =============================================================================
# Cleanup
# =============================================================================
clean:
	@rm -rf \
		$(BUILD_DIR)/smoke \
		$(BUILD_DIR)/proof \
		$(BUILD_DIR)/repro \
		$(BUILD_DIR)/panic \
		$(BUILD_DIR)/kernel \
		$(BUILD_DIR)/*.elf \
		$(BUILD_DIR)/*.map \
		$(BUILD_DIR)/inspect
	@echo "OK: Cleaned generated artifacts."

distclean:
	@rm -rf $(BUILD_DIR) $(ISO_ROOT)
	@echo "OK: Removed all build outputs."
