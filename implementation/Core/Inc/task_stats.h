#ifndef TASK_STATS_H
#define TASK_STATS_H

#include "scheduler_types.h"

void Task_ResetStats(Task_t *task);
void Task_UpdateExecHistogram(Task_t *task, uint64_t exec_us);
void Task_UpdateExecStats(Task_t *task, uint64_t exec_us);
void Task_CheckDeadline(Task_t *task, uint64_t response_time_us);
void Task_UpdateResponseStats(Task_t *task, uint64_t response_us);

#endif /* TASK_STATS_H */
