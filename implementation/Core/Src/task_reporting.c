#include "task_reporting.h"
#include "app_uart.h"

#include <stdio.h>

void TaskReporting_PrintExecHistogram(const char *title, const Task_t *task)
{
  char msg[128];
  static const char *labels[EXEC_HIST_BINS] =
  {
    "0-5ms", "5-10ms", "10-15ms", "15-20ms", "20-25ms",
    "25-30ms", "30-32ms", "32-34ms", "34-36ms", "36ms+"
  };

  snprintf(msg, sizeof(msg), "%s\r\n", title);
  uart_print(msg);

  for (int i = 0; i < EXEC_HIST_BINS; i++)
  {
    snprintf(msg, sizeof(msg), "  %-8s : %lu\r\n", labels[i],
             (unsigned long)task->exec_hist[i]);
    uart_print(msg);
  }
}

void TaskReporting_PrintCsvTask(const char *scheduler_name,
                                const char *scenario_name,
                                uint32_t utilization_percent,
                                const char *task_label,
                                const Task_t *task,
                                uint64_t workload_us)
{
  char msg[384];
  uint64_t exec_avg_us = 0U;
  uint64_t response_avg_us = 0U;

  if (task->run_count > 0U)
  {
    exec_avg_us = task->total_exec_us / task->run_count;
    response_avg_us = task->total_response_us / task->run_count;
  }

  snprintf(msg, sizeof(msg),
           "CSV_TASK,SCHED=%s,SCENARIO=%s,U=%lu,TASK=%s,RUNS=%lu,C_US=%lu,T_MS=%lu,D_MS=%lu,EXEC_AVG_US=%lu,RESP_AVG_US=%lu,RESP_MAX_US=%lu,MISSES=%lu,SKIPPED=%lu,FAILURES=%lu,MAX_LATENESS_US=%lu\r\n",
           scheduler_name, scenario_name, (unsigned long)utilization_percent,
           task_label, (unsigned long)task->run_count,
           (unsigned long)workload_us, (unsigned long)task->period_ms,
           (unsigned long)task->deadline_ms, (unsigned long)exec_avg_us,
           (unsigned long)response_avg_us,
           (unsigned long)task->max_response_us,
           (unsigned long)task->deadline_miss_count,
           (unsigned long)task->skipped_release_count,
           (unsigned long)task->total_timing_failures,
           (unsigned long)task->max_lateness_us);
  uart_print(msg);
}
