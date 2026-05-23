#ifndef __CPU_STATUS_H__
#define __CPU_STATUS_H__

#include <stdint.h>

void read_ram_usage(uint32_t *mem_usage);
void read_cpu_usage(float *cpu_usage);

#endif /*__CPU_STATUS__*/