#ifndef MCSOS_PANIC_H
#define MCSOS_PANIC_H
#include "types.h"

_Noreturn void halt_forever(void);
_Noreturn void kernel_panic(const char *reason, uint64_t code);

#endif
