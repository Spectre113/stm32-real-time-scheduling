#include "app_config.h"
#include "edf_selector.h"
#include "dht11.h"
#include "hcsr04.h"

#include <stddef.h>

static uint64_t Scheduler_TaskAbsoluteDeadline(const Task_t *task)
{
  uint64_t release_us = task->job_active ? task->active_release_us
                                         : task->next_release_us;
  return release_us + task->deadline_us;
}

static uint8_t Scheduler_TaskReady(const SchedTaskRef_t *task_ref,
                                   uint64_t now_us)
{
  Task_t *task = task_ref->task;

  if (task->job_active)
  {
    if (task_ref->kind == SCHED_TASK_STAGED_HCSR04)
    {
      #if ENABLE_REAL_TAU1
      return HCSR04_Async_IsRunnable(now_us);
      #else
      return 0U;
      #endif
    }

    if (task_ref->kind == SCHED_TASK_STAGED_DHT11)
    {
      #if ENABLE_REAL_TAU2
      return DHT11_Async_IsRunnable(now_us);
      #else
      return 0U;
      #endif
    }

    return 1U;
  }

  return ((int64_t)(now_us - task->next_release_us) >= 0);
}

SchedTaskRef_t *Scheduler_SelectChunkedEDF(SchedTaskRef_t *tasks,
                                           uint32_t count,
                                           uint64_t now_us)
{
  SchedTaskRef_t *selected = NULL;
  uint64_t selected_deadline_us = 0ULL;

  for (uint32_t i = 0U; i < count; i++)
  {
    Task_t *task = tasks[i].task;

    if (!tasks[i].enabled || task == NULL)
    {
      continue;
    }

    if ((tasks[i].kind == SCHED_TASK_SYNTHETIC) &&
        (tasks[i].workload_us == 0ULL))
    {
      continue;
    }

    if (!Scheduler_TaskReady(&tasks[i], now_us))
    {
      continue;
    }

    uint64_t absolute_deadline_us = Scheduler_TaskAbsoluteDeadline(task);
    if (selected == NULL || absolute_deadline_us < selected_deadline_us)
    {
      selected = &tasks[i];
      selected_deadline_us = absolute_deadline_us;
    }
  }

  return selected;
}
