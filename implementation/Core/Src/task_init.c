#include "task_init.h"

#include "task_stats.h"

void Task_InitPeriodic(Task_t *task, const char *name, uint64_t period_us,
                       uint32_t period_ms, uint64_t now_us)
{
  task->name = name;
  task->period_us = period_us;
  task->deadline_us = period_us;
  task->next_release_us = now_us + period_us;
  task->period_ms = period_ms;
  task->deadline_ms = period_ms;
  task->next_release_ms = (uint32_t)(task->next_release_us / 1000ULL);

  Task_ResetStats(task);
}
