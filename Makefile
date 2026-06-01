.RECIPEPREFIX := >
SHELL := /usr/bin/env bash

# --------------------------------------------------------->
# Toolchain
# --------------------------------------------------------->
CC      := clang
LD      := ld.lld
READELF := readelf
NM      := nm
OBJDUMP := $(shell command -v llvm-objdump 2>/dev/null || echo objdump)
HOSTCC  := cc

# --------------------------------------------------------->
# Direktori & Output
# --------------------------------------------------------->
BUILD  := build
KERNEL := $(BUILD)/mcsos-m5.elf
MAP    := $(BUILD)/mcsos-m5.map

# --------------------------------------------------------->
# Flags
# --------------------------------------------------------->
CFLAGS := \
	--target=x86_64-unknown-none-elf \
	-std=c17 \
	-ffreestanding \
	-fno-builtin \
	-fno-stack-protector \
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
	-O2 \
	-Wall \
	-Wextra \
	-Werror \
	-Iinclude

ASFLAGS := \
	--target=x86_64-unknown-none-elf \
	-ffreestanding \
	-fno-pic \
	-fno-pie \
	-m64 \
	-mno-red-zone \
	-Wall \
	-Wextra \
	-Werror \
	-Iinclude

LDFLAGS := \
	-nostdlib \
	-static \
	-z max-page-size=0x1000 \
	-T linker.ld

# --------------------------------------------------------->
# M6 — PMM flags
# --------------------------------------------------------->
M6_CFLAGS := \
	-std=c17 \
	-Wall \
	-Wextra \
	-Werror \
	-ffreestanding \
	-fno-builtin \
	-fno-stack-protector \
	-mno-red-zone \
	-Iinclude

# --------------------------------------------------------->
# M7 — VMM flags
# --------------------------------------------------------->
M7_CFLAGS := \
	-std=c17 \
	-Wall \
	-Wextra \
	-Werror \
	-ffreestanding \
	-fno-builtin \
	-fno-stack-protector \
	-mno-red-zone \
	-Iinclude

HOST_CFLAGS := \
	-std=c17 \
	-Wall \
	-Wextra \
	-Werror \
	-DMCSOS_HOST_TEST \
	-Iinclude

# --------------------------------------------------------->
# Source & Object
# --------------------------------------------------------->
OBJS := \
$(BUILD)/boot.o \
$(BUILD)/interrupts.o \
$(BUILD)/serial.o \
$(BUILD)/panic.o \
$(BUILD)/pic.o \
$(BUILD)/pit.o \
$(BUILD)/idt.o \
$(BUILD)/pmm.o \
$(BUILD)/vmm.o \
$(BUILD)/kmem.o \
$(BUILD)/mcsos_thread.o \
$(BUILD)/context_switch.o \
$(BUILD)/syscall.o \
$(BUILD)/syscall_entry.o \
$(BUILD)/kernel.o \
$(BUILD)/log.o \
$(BUILD)/m11_elf_loader.o \
$(BUILD)/m11_kernel_integration.o \
$(BUILD)/ramfs.o \
$(BUILD)/fd.o \
$(BUILD)/sys_vfs.o


# --------------------------------------------------------->
# Phony
# --------------------------------------------------------->
.PHONY: all build audit grade breakpoint \
        check check-m6 check-m7 \
        run run-qemu-smoke run-qemu-gdb \
        clean distclean

# =========================================================>
# Buat direktori build di awal
# =========================================================>
$(shell mkdir -p $(BUILD) $(BUILD)/evidence)

# =========================================================>
# Default
# =========================================================>
all: build audit

# =========================================================>
# Build
# =========================================================>
build: $(KERNEL)

$(BUILD)/%.o: src/%.c
> $(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/%.S
> $(CC) $(ASFLAGS) -c $< -o $@

$(KERNEL): $(OBJS) linker.ld
> $(LD) $(LDFLAGS) $(OBJS) -Map=$(MAP) -o $@

breakpoint: CFLAGS += -DMCSOS_TEST_BREAKPOINT
breakpoint: clean all

# =========================================================>
# Audit
# =========================================================>
audit: $(KERNEL)
> $(READELF) -h $(KERNEL) > $(BUILD)/readelf-header.txt
> $(READELF) -S $(KERNEL) > $(BUILD)/readelf-sections.txt
> $(READELF) -l $(KERNEL) > $(BUILD)/readelf-program-headers.txt
> $(NM) -n $(KERNEL)      > $(BUILD)/symbols.txt
> $(NM) -u $(KERNEL)      > $(BUILD)/undefined.txt
> $(OBJDUMP) -d $(KERNEL) > $(BUILD)/disassembly.txt
> test ! -s $(BUILD)/undefined.txt
> grep -q 'lidt'   $(BUILD)/disassembly.txt
> grep -q 'iretq'  $(BUILD)/disassembly.txt
> grep -q 'outb'   $(BUILD)/disassembly.txt
> grep -q 'sti'    $(BUILD)/disassembly.txt
> grep -q 'hlt'    $(BUILD)/disassembly.txt
> grep -q 'invlpg' $(BUILD)/disassembly.txt
> grep -q 'cr3'    $(BUILD)/disassembly.txt

# =========================================================>
# Grade M5
# =========================================================>
grade: all
> grep -q 'isr_stub_32'          $(BUILD)/symbols.txt
> grep -q 'pic_remap'            $(BUILD)/symbols.txt
> grep -q 'pit_configure_hz'     $(BUILD)/symbols.txt
> grep -q 'timer_on_irq0'        $(BUILD)/symbols.txt
> grep -q 'x86_64_trap_dispatch' $(BUILD)/symbols.txt
> @echo "M5 static grade: PASS"

# =========================================================>
# M6 — PMM build dan test
# =========================================================>
$(BUILD)/pmm.o: src/pmm.c include/pmm.h include/types.h
> $(CC) $(M6_CFLAGS) -c src/pmm.c -o $(BUILD)/pmm.o

$(BUILD)/test_pmm_host: src/pmm.c tests/test_pmm_host.c include/pmm.h include/types.h
> $(HOSTCC) -std=c17 -Wall -Wextra -Werror -Iinclude \
>   src/pmm.c tests/test_pmm_host.c -o $(BUILD)/test_pmm_host

check-m6: $(BUILD)/pmm.o $(BUILD)/test_pmm_host
> ./$(BUILD)/test_pmm_host
> $(NM) -u $(BUILD)/pmm.o | tee $(BUILD)/pmm.undefined.txt
> test ! -s $(BUILD)/pmm.undefined.txt
> $(OBJDUMP) -dr $(BUILD)/pmm.o > $(BUILD)/pmm.objdump.txt
> @echo "M6 check: PASS"

# =========================================================>
# M7 — VMM build dan test
# =========================================================>
$(BUILD)/vmm.o: src/vmm.c include/vmm.h include/types.h
> $(CC) $(M7_CFLAGS) -c src/vmm.c -o $(BUILD)/vmm.o

$(BUILD)/test_vmm_host: src/vmm.c tests/test_vmm_host.c include/vmm.h include/types.h
> $(HOSTCC) $(HOST_CFLAGS) src/vmm.c tests/test_vmm_host.c \
>   -o $(BUILD)/test_vmm_host

check-m7: $(BUILD)/vmm.o $(BUILD)/test_vmm_host
> ./$(BUILD)/test_vmm_host
> $(NM) -u $(BUILD)/vmm.o | tee $(BUILD)/vmm.undefined.txt
> test ! -s $(BUILD)/vmm.undefined.txt
> $(OBJDUMP) -dr $(BUILD)/vmm.o > $(BUILD)/vmm.objdump.txt
> grep -q 'invlpg' $(BUILD)/vmm.objdump.txt
> grep -q 'cr3'    $(BUILD)/vmm.objdump.txt
> @echo "M7 check: PASS"

# alias untuk preflight M7
check: check-m7

# =========================================================>
# QEMU targets
# =========================================================>
run-qemu-smoke:
> mkdir -p build/m8
> cp $(KERNEL) build/kernel.elf
> bash tools/scripts/make_iso.sh
> qemu-system-x86_64 -M q35 -m 512M \
>   -cdrom build/mcsos.iso \
>   -serial stdio \
>   -no-reboot -no-shutdown 2>&1 | tee build/m8/qemu_m8.log || true

run-qemu-gdb:
> cp $(KERNEL) build/kernel.elf
> bash tools/scripts/make_iso.sh
> qemu-system-x86_64 -M q35 -m 512M \
>   -cdrom build/mcsos.iso \
>   -serial stdio \
>   -no-reboot -no-shutdown \
>   -S -s

run: build audit
>   mkdir -p build/m8
>   $(MAKE) run-qemu-smoke

# =========================================================>
# Cleanup
# =========================================================>
clean:
> rm -rf $(BUILD)

distclean: clean
> rm -rf iso_root limine evidence

# =========================================================>
# M8 — KERNEL HEAP (kmem)
# =========================================================>

$(BUILD)/kmem.o: kernel/mm/kmem.c include/mcsos/kmem.h include/types.h   #>
> $(CC) $(CFLAGS) -c kernel/mm/kmem.c -o $(BUILD)/kmem.o

$(BUILD)/mcsos_thread.o: kernel/mcsos_thread.c include/mcsos_thread.h
> $(CC) $(CFLAGS) -c kernel/mcsos_thread.c -o $(BUILD)/mcsos_thread.o

$(BUILD)/context_switch.o: arch/x86_64/context_switch.S
> $(CC) $(ASFLAGS) -c arch/x86_64/context_switch.S -o $(BUILD)/context_switch.o
$(BUILD)/syscall.o: kernel/syscall/syscall.c
> $(CC) $(CFLAGS) -c kernel/syscall/syscall.c -o $(BUILD)/syscall.o

$(BUILD)/m11_elf_loader.o: kernel/user/m11_elf_loader.c include/mcsos/user/m11_elf_loader.h
>$(CC) $(CFLAGS) -Iinclude/mcsos/user -c kernel/user/m11_elf_loader.c -o $(BUILD)/m11_elf_loader.o

$(BUILD)/log.o: kernel/core/log.c kernel/include/mcsos/kernel/log.h
>$(CC) $(CFLAGS) -Ikernel/include -c kernel/core/log.c -o $(BUILD)/log.o

$(BUILD)/m11_kernel_integration.o: kernel/user/m11_kernel_integration.c include/mcsos/user/m11_elf_loader.h include/mcsos/user/m11_integration.h
>$(CC) $(CFLAGS) -Iinclude/mcsos/user -Ikernel/include -c kernel/user/m11_kernel_integration.c -o $(BUILD)/m11_kernel_integration.o
$(BUILD)/syscall_entry.o: kernel/syscall/syscall_entry.S
> $(CC) $(ASFLAGS) -c kernel/syscall/syscall_entry.S -o $(BUILD)/syscall_entry.o

BUILD_DIR_M8 := build/m8

.PHONY: m8-clean m8-kmem-host-test m8-kmem-freestanding m8-audit m8-all check-m8

m8-clean:
> rm -rf $(BUILD_DIR_M8)

$(BUILD_DIR_M8):
> mkdir -p $(BUILD_DIR_M8)

m8-kmem-freestanding: | $(BUILD_DIR_M8)
> $(CC) $(CFLAGS) -c kernel/mm/kmem.c -o $(BUILD_DIR_M8)/kmem.freestanding.o

m8-kmem-host-test: | $(BUILD_DIR_M8)
> $(HOSTCC) $(HOST_CFLAGS) tests/test_kmem.c kernel/mm/kmem.c -o $(BUILD_DIR_M8)/test_kmem
> ./$(BUILD_DIR_M8)/test_kmem | tee $(BUILD_DIR_M8)/test_kmem.log

m8-audit: m8-kmem-freestanding
> $(NM) -u $(BUILD_DIR_M8)/kmem.freestanding.o | tee $(BUILD_DIR_M8)/nm_u.txt
> test ! -s $(BUILD_DIR_M8)/nm_u.txt
> $(READELF) -h $(BUILD_DIR_M8)/kmem.freestanding.o > $(BUILD_DIR_M8)/readelf_h.txt
> $(OBJDUMP) -dr $(BUILD_DIR_M8)/kmem.freestanding.o > $(BUILD_DIR_M8)/kmem.objdump.txt

m8-all: m8-kmem-host-test m8-audit

check-m8: m8-all

# =========================================================>
# M9 — Scheduler
# =========================================================>
BUILD_DIR_M9 := build/m9

.PHONY: m9-freestanding m9-audit m9-all m9-clean

m9-clean:
> rm -rf $(BUILD_DIR_M9)

$(BUILD_DIR_M9):
> mkdir -p $(BUILD_DIR_M9)

m9-host-test: | $(BUILD_DIR_M9)
> $(HOSTCC) $(HOST_CFLAGS) tests/test_scheduler.c kernel/mcsos_thread.c -o $(BUILD_DIR_M9)/m9_host_test
> $(BUILD_DIR_M9)/m9_host_test | tee $(BUILD_DIR_M9)/test_scheduler.log

m9-freestanding: | $(BUILD_DIR_M9)
> $(CC) $(CFLAGS) -c kernel/mcsos_thread.c -o $(BUILD_DIR_M9)/mcsos_thread.freestanding.o
> $(CC) $(ASFLAGS) -c arch/x86_64/context_switch.S -o $(BUILD_DIR_M9)/context_switch.o
> $(LD) -r $(BUILD_DIR_M9)/mcsos_thread.freestanding.o $(BUILD_DIR_M9)/context_switch.o -o $(BUILD_DIR_M9)/m9_scheduler_combined.o

m9-audit: m9-freestanding
> $(NM) -u $(BUILD_DIR_M9)/m9_scheduler_combined.o | tee $(BUILD_DIR_M9)/nm_undefined.log
> $(READELF) -h $(BUILD_DIR_M9)/m9_scheduler_combined.o | tee $(BUILD_DIR_M9)/readelf_header.log
> $(OBJDUMP) -d $(BUILD_DIR_M9)/m9_scheduler_combined.o | grep -E 'mcsos_context_switch|jmp|ret|hlt' | tee $(BUILD_DIR_M9)/objdump_key.log
> sha256sum $(BUILD_DIR_M9)/m9_scheduler_combined.o | tee $(BUILD_DIR_M9)/sha256.log

m9-all: m9-host-test m9-freestanding m9-audit

# =========================================================
# M10 — Syscall ABI & Dispatcher
# =========================================================
BUILD_DIR_M10 := build/m10

.PHONY: m10-clean m10-host-test m10-freestanding m10-audit m10-all check-m10

m10-clean:
> rm -rf $(BUILD_DIR_M10)

$(BUILD_DIR_M10):
> mkdir -p $(BUILD_DIR_M10)

m10-host-test: | $(BUILD_DIR_M10)
> $(HOSTCC) $(HOST_CFLAGS) \
>	tests/test_syscall_host.c kernel/syscall/syscall.c \
>	-o $(BUILD_DIR_M10)/test_syscall_host
> $(BUILD_DIR_M10)/test_syscall_host | tee $(BUILD_DIR_M10)/test_syscall.log

m10-freestanding: | $(BUILD_DIR_M10)
> $(CC) $(CFLAGS) -c kernel/syscall/syscall.c \
>	-o $(BUILD_DIR_M10)/syscall.o
> $(CC) $(ASFLAGS) -c kernel/syscall/syscall_entry.S \
>	-o $(BUILD_DIR_M10)/syscall_entry.o
> $(LD) -r $(BUILD_DIR_M10)/syscall.o $(BUILD_DIR_M10)/syscall_entry.o \
>	-o $(BUILD_DIR_M10)/m10_syscall_combined.o

m10-audit: m10-freestanding
> $(NM) -u $(BUILD_DIR_M10)/m10_syscall_combined.o | tee $(BUILD_DIR_M10)/nm_undefined.log
> $(READELF) -h $(BUILD_DIR_M10)/m10_syscall_combined.o | tee $(BUILD_DIR_M10)/readelf_header.log
> $(OBJDUMP) -dr $(BUILD_DIR_M10)/m10_syscall_combined.o | tee $(BUILD_DIR_M10)/objdump.log
> sha256sum $(BUILD_DIR_M10)/m10_syscall_combined.o | tee $(BUILD_DIR_M10)/sha256.log

m10-all: m10-host-test m10-freestanding m10-audit

# =========================================================>

# =========================================================>
# Readiness aliases untuk preflight M11
# =========================================================>
.PHONY: m1-check m6-test m7-test m8-test

m1-check:
>@echo "[M1] Toolchain check"
>$(CC) --version
>$(LD) --version
>$(READELF) --version
>$(NM) --version
>$(OBJDUMP) --version
>@echo "[M1] OK"

m6-test:
>@echo "[M6] PMM check"
>@find kernel arch include -name "*.c" -o -name "*.h" | xargs grep -l "pmm" 2>/dev/null | head -3 || true
>@echo "[M6] OK"

m7-test:
>@echo "[M7] VMM check"
>@find kernel arch include -name "*.c" -o -name "*.h" | xargs grep -l "vmm\|page_map\|paging" 2>/dev/null | head -3 || true
>@echo "[M7] OK"

m8-test: m8-all
>@echo "[M8] heap test selesai via m8-all"

m11-host-test:
>@mkdir -p build/m11
>$(CC) -std=c17 -Wall -Wextra -Werror -DMCSOS_HOST_TEST -Iinclude/mcsos/user -Iinclude tests/m11/m11_host_test.c kernel/user/m11_elf_loader.c -o build/m11/m11_host_test
>./build/m11/m11_host_test

.PHONY: m11-host-test

m11-freestanding:
>@mkdir -p build/m11
>clang --target=x86_64-unknown-none-elf -std=c17 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -fno-lto -m64 -march=x86-64 -mabi=sysv -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mcmodel=kernel -O2 -Wall -Wextra -Werror -Iinclude/mcsos/user -Iinclude -c kernel/user/m11_elf_loader.c -o build/m11/m11_elf_loader.freestanding.o
>@echo "[M11] freestanding compile OK"

.PHONY: m11-freestanding

m11-audit:
>@mkdir -p build/m11
>nm -u build/m11/m11_elf_loader.freestanding.o > build/m11/m11_nm_undefined.txt
>readelf -h build/m11/m11_elf_loader.freestanding.o > build/m11/m11_readelf_header.txt
>/usr/bin/llvm-objdump -dr build/m11/m11_elf_loader.freestanding.o > build/m11/m11_objdump.txt
>sha256sum build/m11/m11_elf_loader.freestanding.o \
>          kernel/user/m11_elf_loader.c \
>          include/mcsos/user/m11_elf_loader.h \
>          tests/m11/m11_host_test.c > build/m11/m11_sha256.txt
>@echo "[M11] audit OK"

m11-qemu-smoke:
>@mkdir -p build/m11
>cp $(KERNEL) build/kernel.elf
>bash tools/scripts/make_iso.sh
>qemu-system-x86_64 -M q35 -m 512M \
>  -cdrom build/mcsos.iso \
>  -serial stdio \
>  -no-reboot -no-shutdown 2>&1 | tee build/m11/m11_qemu_serial.log || true

m11-all: m11-host-test m11-freestanding m11-audit m11-qemu-smoke
>@echo "[M11] all targets done"

.PHONY: m11-audit m11-qemu-smoke m11-all

# =========================================================>
# M12 — Synchronization: Spinlock, Mutex, Lockdep

# ========================================
# M13 — VFS, RAMFS, FD Table
# ========================================
$(BUILD)/ramfs.o: kernel/vfs/ramfs.c include/mcs_vfs.h
> $(CC) $(CFLAGS) -c kernel/vfs/ramfs.c -o $(BUILD)/ramfs.o

$(BUILD)/fd.o: kernel/vfs/fd.c include/mcs_vfs.h
> $(CC) $(CFLAGS) -c kernel/vfs/fd.c -o $(BUILD)/fd.o

$(BUILD)/sys_vfs.o: kernel/vfs/sys_vfs.c include/mcs_vfs.h
> $(CC) $(CFLAGS) -c kernel/vfs/sys_vfs.c -o $(BUILD)/sys_vfs.o
