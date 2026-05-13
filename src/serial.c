#include "io.h"
#include "serial.h"

#define COM1 0x3F8u

void serial_init(void) {
    outb(COM1 + 1u, 0x00u);
    outb(COM1 + 3u, 0x80u);
    outb(COM1 + 0u, 0x03u);
    outb(COM1 + 1u, 0x00u);
    outb(COM1 + 3u, 0x03u);
    outb(COM1 + 2u, 0xC7u);
    outb(COM1 + 4u, 0x0Bu);
}

static int serial_transmit_empty(void) {
    return (inb(COM1 + 5u) & 0x20u) != 0;
}

void serial_write_char(char c) {
    while (!serial_transmit_empty()) {
        __asm__ volatile ("pause");
    }
    outb(COM1, (uint8_t)c);
}

void serial_write_string(const char *s) {
    while (*s != '\0') {
        if (*s == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(*s);
        ++s;
    }
}

void serial_write_hex64(uint64_t value) {
    static const char digits[] = "0123456789abcdef";
    serial_write_string("0x");
    for (int i = 60; i >= 0; i -= 4) {
        serial_write_char(digits[(value >> (unsigned)i) & 0xFu]);
    }
}

void serial_write_dec64(uint64_t value) {
    char buf[21];
    size_t i = 0;
    if (value == 0) {
        serial_write_char('0');
        return;
    }
    while (value != 0 && i < sizeof(buf)) {
        buf[i++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (i != 0) {
        serial_write_char(buf[--i]);
    }
}
