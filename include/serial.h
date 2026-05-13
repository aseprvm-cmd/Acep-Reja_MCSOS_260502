#ifndef MCSOS_SERIAL_H
#define MCSOS_SERIAL_H
#include "types.h"

void serial_init(void);
void serial_write_char(char c);
void serial_write_string(const char *s);
void serial_write_hex64(uint64_t value);
void serial_write_dec64(uint64_t value);

#endif
