#ifndef TASK_REPORTING_H
#define TASK_REPORTING_H

#include "scheduler_types.h"

void TaskReporting_PrintExecHistogram(const char *title, const Task_t *task);
void TaskReporting_PrintCsvTask(const char *scheduler_name,
                                const char *scenario_name,
                                uint32_t utilization_percent,
                                const char *task_label,
                                const Task_t *task,
                                uint64_t workload_us);

#endif /* TASK_REPORTING_H */
