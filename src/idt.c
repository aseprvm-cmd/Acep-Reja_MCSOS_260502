#include "idt.h"
#include "io.h"
#include "panic.h"
#include "pic.h"
#include "pit.h"
#include "serial.h"

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

extern void (*const isr_stub_table[48])(void);

static struct idt_entry g_idt[256];

static void idt_set_gate(uint8_t vector, void (*handler)(void), uint16_t selector, uint8_t type_attr) {
    uint64_t addr = (uint64_t)handler;
    g_idt[vector].offset_low = (uint16_t)(addr & 0xFFFFu);
    g_idt[vector].selector = selector;
    g_idt[vector].ist = 0;
    g_idt[vector].type_attr = type_attr;
    g_idt[vector].offset_mid = (uint16_t)((addr >> 16u) & 0xFFFFu);
    g_idt[vector].offset_high = (uint32_t)((addr >> 32u) & 0xFFFFFFFFu);
    g_idt[vector].zero = 0;
}

static void lidt(const struct idt_pointer *ptr) {
    __asm__ volatile ("lidt (%0)" :: "r"(ptr) : "memory");
}

void idt_init(void) {
    const uint16_t cs = x86_64_read_cs();
    for (uint8_t i = 0; i < 48u; ++i) {
        idt_set_gate(i, isr_stub_table[i], cs, 0x8Eu);
    }
    struct idt_pointer ptr = {
        .limit = (uint16_t)(sizeof(g_idt) - 1u),
        .base = (uint64_t)&g_idt[0],
    };
    lidt(&ptr);
}

void x86_64_trap_dispatch(struct trap_frame *frame) {
    if (frame == (struct trap_frame *)0) {
        kernel_panic("null trap frame", 0);
    }

    if (frame->vector >= PIC_MASTER_OFFSET && frame->vector < (PIC_SLAVE_OFFSET + 8u)) {
        uint8_t irq = (uint8_t)(frame->vector - PIC_MASTER_OFFSET);
        if (irq == 0u) {
            timer_on_irq0();
        } else {
            serial_write_string("[MCSOS:IRQ] unexpected irq=");
            serial_write_dec64(irq);
            serial_write_string("\n");
        }
        pic_send_eoi(irq);
        return;
    }

    if (frame->vector == 3u) {
        serial_write_string("[MCSOS:TRAP] breakpoint rip=");
        serial_write_hex64(frame->rip);
        serial_write_string("\n");
        return;
    }

    serial_write_string("[MCSOS:EXCEPTION] vector=");
    serial_write_dec64(frame->vector);
    serial_write_string(" error=");
    serial_write_hex64(frame->error_code);
    serial_write_string(" rip=");
    serial_write_hex64(frame->rip);
    serial_write_string("\n");
    kernel_panic("unhandled CPU exception", frame->vector);
}
