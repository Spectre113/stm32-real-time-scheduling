#ifndef PLATFORM_TIME_H
#define PLATFORM_TIME_H

#include <stdint.h>

void PlatformTime_Init(uint32_t cycles_per_us);
uint32_t PlatformTime_CyclesPerUs(void);

void DWT_Init(void);
uint64_t micros(void);
uint64_t scheduler_now_us(void);
void delay_us(uint32_t us);
void Synthetic_Workload_us(uint64_t duration_us);

static inline uint32_t Correct_DWT_Delta(uint32_t delta,
                                         uint32_t measurement_overhead)
{
  return delta > measurement_overhead ? delta - measurement_overhead : 0U;
}

#endif /* PLATFORM_TIME_H */
