#include "cycle_metrics.h"

void CycleMetrics_Reset(CycleMetrics_t *metrics)
{
  metrics->total_cycles = 0ULL;
  metrics->min_cycles = 0U;
  metrics->max_cycles = 0U;
  metrics->count = 0U;
}

void CycleMetrics_Update(CycleMetrics_t *metrics, uint32_t cycles)
{
  metrics->count++;
  metrics->total_cycles += cycles;

  if (metrics->count == 1U)
  {
    metrics->min_cycles = cycles;
    metrics->max_cycles = cycles;
  }
  else
  {
    if (cycles < metrics->min_cycles)
    {
      metrics->min_cycles = cycles;
    }

    if (cycles > metrics->max_cycles)
    {
      metrics->max_cycles = cycles;
    }
  }
}
