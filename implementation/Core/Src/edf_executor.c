#include "edf_executor.h"
#include "app_config.h"
#include "dht11.h"
#include "hcsr04.h"
#include "platform_time.h"
#include "scheduler_release.h"
#include "task_stats.h"

void Scheduler_CompleteChunkedTask(Task_t *task)
{
  uint64_t finish_us = scheduler_now_us();
  uint64_t response_time = finish_us - task->active_release_us;

  task->run_count++;
  Task_UpdateExecStats(task, task->accumulated_exec_us);
  Task_UpdateResponseStats(task, response_time);
  Task_CheckDeadline(task, response_time);
  Task_AdvanceRelease(task, finish_us);

  task->job_active = 0U;
  task->active_release_us = 0ULL;
  task->remaining_exec_us = 0ULL;
  task->accumulated_exec_us = 0ULL;
}

#if ENABLE_REAL_TAU1
static void Scheduler_RunChunkedHCSR04(Task_t *task,
                                       EdfExecutorContext_t *context)
{
  uint64_t exec_start = micros();
  uint64_t exec_finish;
  uint8_t job_complete = 0U;
  int distance_cm;

  if (!task->job_active)
  {
    task->active_release_us = task->next_release_us;
    task->remaining_exec_us = 0ULL;
    task->accumulated_exec_us = 0ULL;
    task->job_active = 1U;
    HCSR04_Async_Start();
  }
  else if (HCSR04_Async_Finalize(&distance_cm) != 0U)
  {
    *context->distance_cm = distance_cm;
    job_complete = 1U;
  }
  else if (HCSR04_Async_Timeout(&distance_cm) != 0U)
  {
    *context->distance_cm = distance_cm;
    job_complete = 1U;
  }

  exec_finish = micros();
  task->accumulated_exec_us += exec_finish - exec_start;

  if (job_complete != 0U) Scheduler_CompleteChunkedTask(task);
}
#endif

#if ENABLE_REAL_TAU2
static void Scheduler_RunChunkedDHT11(Task_t *task,
                                      EdfExecutorContext_t *context)
{
  uint64_t exec_start = micros();
  uint64_t exec_finish;
  DHT11_StepResult_t step_result;

  if (!task->job_active)
  {
    task->active_release_us = task->next_release_us;
    task->remaining_exec_us = 0ULL;
    task->accumulated_exec_us = 0ULL;
    task->job_active = 1U;
  }

  step_result = DHT11_Async_Step(scheduler_now_us(), context->temp,
                                 context->hum);
  exec_finish = micros();
  task->accumulated_exec_us += exec_finish - exec_start;

  if ((step_result == DHT11_STEP_COMPLETE) ||
      (step_result == DHT11_STEP_ERROR))
  {
    *context->dht_result = DHT11_Async_Result();
    context->save_sample(task, task->accumulated_exec_us);
    Scheduler_CompleteChunkedTask(task);
    DHT11_Async_Reset();
  }
}
#endif

void Scheduler_RunChunkedTask(SchedTaskRef_t *selected,
                              EdfExecutorContext_t *context)
{
  Task_t *task = selected->task;

  if (selected->kind == SCHED_TASK_STAGED_HCSR04)
  {
    #if ENABLE_REAL_TAU1
    Scheduler_RunChunkedHCSR04(task, context);
    #endif
    return;
  }

  if (selected->kind == SCHED_TASK_STAGED_DHT11)
  {
    #if ENABLE_REAL_TAU2
    Scheduler_RunChunkedDHT11(task, context);
    #endif
    return;
  }

  if (!task->job_active)
  {
    task->active_release_us = task->next_release_us;
    task->remaining_exec_us = selected->workload_us;
    task->accumulated_exec_us = 0ULL;
    task->job_active = 1U;
  }

  uint64_t chunk_us = EDF_CHUNK_US;
  if (task->remaining_exec_us < chunk_us) chunk_us = task->remaining_exec_us;

  uint64_t exec_start = micros();
  Synthetic_Workload_us(chunk_us);
  uint64_t exec_finish = micros();

  task->accumulated_exec_us += exec_finish - exec_start;
  task->remaining_exec_us -= chunk_us;

  if (task->remaining_exec_us == 0ULL) Scheduler_CompleteChunkedTask(task);
}
