#ifndef AEGIS_TIMER_H
#define AEGIS_TIMER_H

#include <stdint.h>
#include "types.h"

#define PIT_FREQUENCY 1193182
#define TIMER_IRQ 0

void timer_init(uint32_t frequency);
void timer_handler(void);
uint64_t timer_ticks(void);

#endif