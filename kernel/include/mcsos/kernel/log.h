#ifndef MCSOS_KERNEL_LOG_H
#define MCSOS_KERNEL_LOG_H

#include <stdint.h>

void log_init(void);
void log_putc(char c);
void log_write(const char *s);
void log_writeln(const char *s);
void log_hex64(uint64_t value);
void log_key_value_hex64(const char *key, uint64_t value);

#endif

/* M11 integration smoke test — cetak process image plan ke serial log */
void m11_integration_smoke_test(void);
