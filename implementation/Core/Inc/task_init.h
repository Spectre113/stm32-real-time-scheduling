#ifndef TASK_INIT_H
#define TASK_INIT_H

#include <stdint.h>

#include "scheduler_types.h"

void Task_InitPeriodic(Task_t *task, const char *name, uint64_t period_us,
                       uint32_t period_ms, uint64_t now_us);

#endif /* TASK_INIT_H */
