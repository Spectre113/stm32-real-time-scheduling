#include "task_stats.h"

void Task_ResetStats(Task_t *task)
{
  task->run_count = 0;
  task->total_exec_us = 0;
  task->min_exec_us = 0;
  task->max_exec_us = 0;
  task->total_response_us = 0;
  task->min_response_us = 0;
  task->max_response_us = 0;
  task->deadline_miss_count = 0;
  task->max_lateness_us = 0;
  task->skipped_release_count = 0;
  task->total_timing_failures = 0;
  task->job_active = 0;
  task->active_release_us = 0;
  task->remaining_exec_us = 0;
  task->accumulated_exec_us = 0;

  for (int i = 0; i < EXEC_HIST_BINS; i++)
  {
    task->exec_hist[i] = 0;
  }
}

void Task_UpdateExecHistogram(Task_t *task, uint64_t exec_us)
{
  if (exec_us < 5000ULL) task->exec_hist[0]++;
  else if (exec_us < 10000ULL) task->exec_hist[1]++;
  else if (exec_us < 15000ULL) task->exec_hist[2]++;
  else if (exec_us < 20000ULL) task->exec_hist[3]++;
  else if (exec_us < 25000ULL) task->exec_hist[4]++;
  else if (exec_us < 30000ULL) task->exec_hist[5]++;
  else if (exec_us < 32000ULL) task->exec_hist[6]++;
  else if (exec_us < 34000ULL) task->exec_hist[7]++;
  else if (exec_us < 36000ULL) task->exec_hist[8]++;
  else task->exec_hist[9]++;
}

void Task_UpdateExecStats(Task_t *task, uint64_t exec_us)
{
  task->total_exec_us += exec_us;

  if (task->run_count == 1U)
  {
    task->min_exec_us = exec_us;
    task->max_exec_us = exec_us;
  }
  else
  {
    if (exec_us < task->min_exec_us) task->min_exec_us = exec_us;
    if (exec_us > task->max_exec_us) task->max_exec_us = exec_us;
  }

  Task_UpdateExecHistogram(task, exec_us);
}

void Task_CheckDeadline(Task_t *task, uint64_t response_time_us)
{
  if (response_time_us > task->deadline_us)
  {
    uint64_t lateness_us = response_time_us - task->deadline_us;
    task->deadline_miss_count++;
    task->total_timing_failures++;

    if (lateness_us > task->max_lateness_us)
    {
      task->max_lateness_us = lateness_us;
    }
  }
}

void Task_UpdateResponseStats(Task_t *task, uint64_t response_us)
{
  task->total_response_us += response_us;

  if (task->run_count == 1U)
  {
    task->min_response_us = response_us;
    task->max_response_us = response_us;
  }
  else
  {
    if (response_us < task->min_response_us) task->min_response_us = response_us;
    if (response_us > task->max_response_us) task->max_response_us = response_us;
  }
}
