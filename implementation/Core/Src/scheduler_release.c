#include "scheduler_release.h"

void Task_AdvanceRelease(Task_t *task, uint64_t now_us)
{
  task->next_release_us += task->period_us;

  while ((int64_t)(now_us - task->next_release_us) > 0)
  {
    task->skipped_release_count++;
    task->total_timing_failures++;
    task->next_release_us += task->period_us;
  }

  task->next_release_ms = (uint32_t)(task->next_release_us / 1000ULL);
}
