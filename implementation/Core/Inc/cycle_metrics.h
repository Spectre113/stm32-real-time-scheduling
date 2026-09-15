#ifndef CYCLE_METRICS_H
#define CYCLE_METRICS_H

#include <stdint.h>

typedef struct
{
  uint64_t total_cycles;
  uint32_t min_cycles;
  uint32_t max_cycles;
  uint32_t count;
} CycleMetrics_t;

void CycleMetrics_Reset(CycleMetrics_t *metrics);
void CycleMetrics_Update(CycleMetrics_t *metrics, uint32_t cycles);

#endif /* CYCLE_METRICS_H */
