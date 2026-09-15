#include "main.h"
#include "platform_time.h"

static uint32_t g_cycles_per_us = 1U;

void PlatformTime_Init(uint32_t cycles_per_us)
{
  g_cycles_per_us = cycles_per_us == 0U ? 1U : cycles_per_us;
}

uint32_t PlatformTime_CyclesPerUs(void)
{
  return g_cycles_per_us;
}

void DWT_Init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

  #if (__CORTEX_M == 7)
  DWT->LAR = 0xC5ACCE55;
  #endif

  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint64_t micros(void)
{
  static uint32_t last_cycles = 0;
  static uint64_t high_cycles = 0;
  uint32_t current_cycles = DWT->CYCCNT;

  if (current_cycles < last_cycles)
  {
    high_cycles += (1ULL << 32);
  }

  last_cycles = current_cycles;

  return (high_cycles + current_cycles) / g_cycles_per_us;
}

uint64_t scheduler_now_us(void)
{
  return micros();
}

void delay_us(uint32_t us)
{
  uint64_t start = micros();
  while ((micros() - start) < us)
  {
  }
}

void Synthetic_Workload_us(uint64_t duration_us)
{
  uint64_t start = micros();

  while ((micros() - start) < duration_us)
  {
    __NOP();
  }
}
