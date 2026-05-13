#include "idt.h"
#include "io.h"
#include "panic.h"
#include "pic.h"
#include "pit.h"
#include "serial.h"

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

    for (;;) {
        cpu_hlt();
    }
}
