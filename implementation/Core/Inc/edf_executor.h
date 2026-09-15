#ifndef EDF_EXECUTOR_H
#define EDF_EXECUTOR_H

#include "scheduler_types.h"

void Scheduler_CompleteChunkedTask(Task_t *task);

typedef void (*EdfExecutorSampleFn_t)(Task_t *task, uint64_t exec_time_us);

typedef struct
{
  int *distance_cm;
  uint8_t *temp;
  uint8_t *hum;
  int *dht_result;
  EdfExecutorSampleFn_t save_sample;
} EdfExecutorContext_t;

void Scheduler_RunChunkedTask(SchedTaskRef_t *selected,
                              EdfExecutorContext_t *context);

#endif /* EDF_EXECUTOR_H */
