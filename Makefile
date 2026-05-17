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
	$(BUILD)/kernel.o

# --------------------------------------------------------->
# Phony
# --------------------------------------------------------->
.PHONY: all build audit grade breakpoint check-m6 clean distclean

# =========================================================>
# Buat direktori build di awal
# =========================================================>
$(shell mkdir -p $(BUILD))

# =========================================================>
# Default
# =========================================================>
all: build audit

# =========================================================>
# Build M5
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
# Audit M5
# =========================================================>
audit: $(KERNEL)
> $(READELF) -h $(KERNEL) > $(BUILD)/readelf-header.txt
> $(READELF) -S $(KERNEL) > $(BUILD)/readelf-sections.txt
> $(READELF) -l $(KERNEL) > $(BUILD)/readelf-program-headers.txt
> $(NM) -n $(KERNEL)      > $(BUILD)/symbols.txt
> $(NM) -u $(KERNEL)      > $(BUILD)/undefined.txt
> $(OBJDUMP) -d $(KERNEL) > $(BUILD)/disassembly.txt
> test ! -s $(BUILD)/undefined.txt
> grep -q 'lidt'  $(BUILD)/disassembly.txt
> grep -q 'iretq' $(BUILD)/disassembly.txt
> grep -q 'outb'  $(BUILD)/disassembly.txt
> grep -q 'sti'   $(BUILD)/disassembly.txt
> grep -q 'hlt'   $(BUILD)/disassembly.txt

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
# Cleanup
# =========================================================>
clean:
> rm -rf $(BUILD)

distclean: clean
> rm -rf iso_root limine evidence
