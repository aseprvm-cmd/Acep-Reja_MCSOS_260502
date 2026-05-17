#include "idt.h"
#include "io.h"
#include "panic.h"
#include "pic.h"
#include "pit.h"
#include "serial.h"
#include "pmm.h"

static struct pmm_state kernel_pmm;
static uint8_t kernel_pmm_bitmap[PMM_BITMAP_BYTES] __attribute__((aligned(4096)));

static void kernel_memory_init(void) {
    static struct boot_mem_region early_map[] = {
        { 0x00000000ULL, 0x0009f000ULL, BOOT_MEM_USABLE },
        { 0x0009f000ULL, 0x00001000ULL, BOOT_MEM_RESERVED },
        { 0x00100000ULL, 0x00300000ULL, BOOT_MEM_USABLE },
        { 0x00400000ULL, 0x00100000ULL, BOOT_MEM_KERNEL_AND_MODULES },
        { 0x00500000ULL, 0x07b00000ULL, BOOT_MEM_USABLE },
    };
    size_t count = sizeof(early_map) / sizeof(early_map[0]);

    bool ok = pmm_init_from_map(&kernel_pmm,
                                early_map, count,
                                kernel_pmm_bitmap,
                                sizeof(kernel_pmm_bitmap),
                                PMM_MAX_PHYS_BYTES);
    if (!ok) {
        serial_write_string("[m6] pmm_init_from_map failed\n");
        return;
    }

    serial_write_string("[m6] pmm initialized\n");
    serial_write_hex64(pmm_frame_count(&kernel_pmm));
    serial_write_string(" frames managed\n");
    serial_write_hex64(pmm_free_count(&kernel_pmm));
    serial_write_string(" frames free\n");

    uint64_t f = pmm_alloc_frame(&kernel_pmm);
    if (f == PMM_INVALID_FRAME) {
        serial_write_string("[m6] alloc failed\n");
        return;
    }
    serial_write_string("[m6] sample frame = ");
    serial_write_hex64(f);
    serial_write_string("\n");

    if (!pmm_free_frame(&kernel_pmm, f)) {
        serial_write_string("[m6] free failed\n");
        return;
    }
    serial_write_string("[m6] frame freed ok\n");
}

void kmain(void) {
    cpu_cli();
    serial_init();
    serial_write_string("[MCSOS:M5] boot: external interrupt bring-up start\n");
    idt_init();
    serial_write_string("[MCSOS:M5] idt: loaded\n");
    pic_remap(PIC_MASTER_OFFSET, PIC_SLAVE_OFFSET);
    pic_mask_all();
    pic_unmask_irq(0);
    serial_write_string("[MCSOS:M5] pic: remapped; mask master=");
    serial_write_hex64(pic_read_master_mask());
    serial_write_string(" slave=");
    serial_write_hex64(pic_read_slave_mask());
    serial_write_string("\n");
    pit_configure_hz(100u);
    serial_write_string("[MCSOS:M5] pit: configured 100Hz\n");
    serial_write_string("[MCSOS:M5] sti: enabling interrupts\n");
    cpu_sti();
#if defined(MCSOS_TEST_BREAKPOINT)
    __asm__ volatile ("int3");
#endif
    kernel_memory_init();
    for (;;) {
        cpu_hlt();
    }
}
