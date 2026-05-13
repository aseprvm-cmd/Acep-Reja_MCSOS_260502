#include "io.h"
#include "panic.h"
#include "serial.h"

_Noreturn void halt_forever(void) {
    cpu_cli();
    for (;;) {
        cpu_hlt();
    }
}

_Noreturn void kernel_panic(const char *reason, uint64_t code) {
    cpu_cli();
    serial_write_string("\n[MCSOS:PANIC] ");
    serial_write_string(reason);
    serial_write_string(" code=");
    serial_write_hex64(code);
    serial_write_string("\n");
    for (;;) {
        cpu_hlt();
    }
}
