#include "main.h"

#include <stdint.h>

#include "app_config.h"
#include "app_uart.h"
#include "platform_time.h"
#include "superloop_scalability_profile.h"
#include "workload_config.h"

#if (EXPERIMENT_MODE == EXPERIMENT_SUPERLOOP_SCALABILITY_CLEAN) || \
    (EXPERIMENT_MODE == EXPERIMENT_SUPERLOOP_SCALABILITY_CHECKS)
typedef struct
{
  uint64_t period_us;
  uint64_t workload_us;
  uint64_t next_release_us;
  uint64_t task_cycles;
  uint32_t runs;
} ScalabilityTask_t;

static uint32_t Scalability_DWT_Measurement_Overhead(void)
{
  uint32_t overhead_cycles = UINT32_MAX;

  for (uint32_t i = 0; i < 1000U; i++)
  {
    uint32_t start_cycles = DWT->CYCCNT;
    uint32_t end_cycles = DWT->CYCCNT;
    uint32_t delta_cycles = end_cycles - start_cycles;

    if (delta_cycles < overhead_cycles)
    {
      overhead_cycles = delta_cycles;
    }
  }

  return overhead_cycles;
}

static void Scalability_InitTasks(ScalabilityTask_t *tasks,
                                  uint64_t profile_start_us)
{
  tasks[0] = (ScalabilityTask_t) {
      SCALABILITY_TAU1_PERIOD_US, SCALABILITY_TAU1_WORKLOAD_US,
      profile_start_us, 0ULL, 0U};
  tasks[1] = (ScalabilityTask_t) {
      SCALABILITY_TAU2_PERIOD_US, SCALABILITY_TAU2_WORKLOAD_US,
      profile_start_us, 0ULL, 0U};

  #if SCALABILITY_TASK_COUNT >= 3
  tasks[2] = (ScalabilityTask_t) {
      SCALABILITY_TAU3_PERIOD_US, SCALABILITY_TAU3_WORKLOAD_US,
      profile_start_us, 0ULL, 0U};
  #endif

  #if SCALABILITY_TASK_COUNT >= 4
  tasks[3] = (ScalabilityTask_t) {
      SCALABILITY_TAU4_PERIOD_US, SCALABILITY_TAU4_WORKLOAD_US,
      profile_start_us, 0ULL, 0U};
  #endif
}

static void Scalability_PrintCsvRuns(const ScalabilityTask_t *tasks)
{
  for (uint32_t task_index = 0; task_index < SCALABILITY_TASK_COUNT;
       task_index++)
  {
    uart_print(",TAU");
    uart_print_u64(task_index + 1U);
    uart_print("_RUNS=");
    uart_print_u64(tasks[task_index].runs);
  }
}
#endif

#if EXPERIMENT_MODE == EXPERIMENT_SUPERLOOP_SCALABILITY_CLEAN
static void Scalability_PrintTask(uint32_t task_index,
                                  const ScalabilityTask_t *task)
{
  uart_print("\r\n\r\ntau");
  uart_print_u64(task_index + 1U);
  uart_print(":\r\n  runs: ");
  uart_print_u64(task->runs);
  uart_print("\r\n  task_us: ");
  uart_print_u64(task->task_cycles / PlatformTime_CyclesPerUs());
}

/* Clean busy-polling scalability profile: task-body DWT only. */
void SuperloopScalabilityProfile_RunClean(void)
{
  ScalabilityTask_t tasks[SCALABILITY_TASK_COUNT];
  uint64_t profile_start_us;
  uint64_t profile_end_us;
  uint64_t profile_finish_us;
  uint64_t profile_elapsed_us;
  uint64_t task_cycles = 0ULL;
  uint64_t task_us;
  uint64_t superloop_us;
  uint32_t dwt_measurement_overhead_cycles;
  uint8_t profile_finished = 0U;

  dwt_measurement_overhead_cycles = Scalability_DWT_Measurement_Overhead();
  profile_start_us = scheduler_now_us();
  profile_end_us = profile_start_us + SCALABILITY_PROFILE_WINDOW_US;
  Scalability_InitTasks(tasks, profile_start_us);

  while (profile_finished == 0U)
  {
    for (uint32_t task_index = 0; task_index < SCALABILITY_TASK_COUNT;
         task_index++)
    {
      ScalabilityTask_t *task = &tasks[task_index];
      uint64_t now_us = scheduler_now_us();

      if ((int64_t)(now_us - profile_end_us) >= 0)
      {
        profile_finished = 1U;
        break;
      }

      if (now_us >= task->next_release_us)
      {
        uint32_t task_start_cycles = DWT->CYCCNT;
        Synthetic_Workload_us(task->workload_us);
        uint32_t task_end_cycles = DWT->CYCCNT;
        uint64_t finish_us;

        task->task_cycles += Correct_DWT_Delta(
            task_end_cycles - task_start_cycles,
            dwt_measurement_overhead_cycles);

        finish_us = scheduler_now_us();
        task->next_release_us += task->period_us;

        while ((int64_t)(finish_us - task->next_release_us) > 0)
        {
          task->next_release_us += task->period_us;
        }

        task->runs++;
      }
    }
  }

  profile_finish_us = scheduler_now_us();
  profile_elapsed_us = profile_finish_us - profile_start_us;

  for (uint32_t task_index = 0; task_index < SCALABILITY_TASK_COUNT;
       task_index++)
  {
    task_cycles += tasks[task_index].task_cycles;
  }

  task_us = task_cycles / PlatformTime_CyclesPerUs();
  /* Includes polling, end checks, task DWT measurement, and runs++ counters. */
  superloop_us = profile_elapsed_us > task_us
                 ? profile_elapsed_us - task_us : 0ULL;

  uart_print("\r\n=== SUPERLOOP SCALABILITY CLEAN ===\r\nscenario: ");
  uart_print(SCALABILITY_WORKLOAD_SCENARIO_NAME);
  uart_print("\r\ntasks: ");
  uart_print_u64(SCALABILITY_TASK_COUNT);
  uart_print("\r\nwindow_us: ");
  uart_print_u64(profile_elapsed_us);

  for (uint32_t task_index = 0; task_index < SCALABILITY_TASK_COUNT;
       task_index++)
  {
    Scalability_PrintTask(task_index, &tasks[task_index]);
  }

  uart_print("\r\n\r\ntask_execution:\r\n  total_us: ");
  uart_print_u64(task_us);
  uart_print("\r\n  percent: ");
  uart_print_percent_x10000(task_us, profile_elapsed_us);
  uart_print(" %\r\n\r\nsuperloop:\r\n  total_us: ");
  uart_print_u64(superloop_us);
  uart_print("\r\n  percent: ");
  uart_print_percent_x10000(superloop_us, profile_elapsed_us);
  uart_print(" %\r\n");

  uart_print("CSV_SUPERLOOP_SCALE_CLEAN,SCENARIO=");
  uart_print(SCALABILITY_WORKLOAD_SCENARIO_NAME);
  uart_print(",U=");
  uart_print_u64(SCALABILITY_WORKLOAD_UTILIZATION_PERCENT);
  uart_print(",TASKS=");
  uart_print_u64(SCALABILITY_TASK_COUNT);
  uart_print(",WINDOW_US=");
  uart_print_u64(profile_elapsed_us);
  Scalability_PrintCsvRuns(tasks);
  uart_print(",TASK_US=");
  uart_print_u64(task_us);
  uart_print(",TASK_PCT=");
  uart_print_percent_x10000(task_us, profile_elapsed_us);
  uart_print(",SUPERLOOP_US=");
  uart_print_u64(superloop_us);
  uart_print(",SUPERLOOP_PCT=");
  uart_print_percent_x10000(superloop_us, profile_elapsed_us);
  uart_print("\r\n");

  while (1)
  {
  }
}
#endif

#if EXPERIMENT_MODE == EXPERIMENT_SUPERLOOP_SCALABILITY_CHECKS
/* Diagnostic scalability profile: readiness and release DWT are intentional. */
void SuperloopScalabilityProfile_RunChecks(void)
{
  ScalabilityTask_t tasks[SCALABILITY_TASK_COUNT];
  uint64_t profile_start_us;
  uint64_t profile_end_us;
  uint64_t profile_finish_us;
  uint64_t profile_elapsed_us;
  uint64_t task_cycles = 0ULL;
  uint64_t readiness_cycles = 0ULL;
  uint64_t release_maintenance_cycles = 0ULL;
  uint64_t check_cycles;
  uint64_t task_us;
  uint64_t readiness_us;
  uint64_t release_us;
  uint64_t checks_us;
  uint32_t dwt_measurement_overhead_cycles;
  uint8_t profile_finished = 0U;

  dwt_measurement_overhead_cycles = Scalability_DWT_Measurement_Overhead();
  profile_start_us = scheduler_now_us();
  profile_end_us = profile_start_us + SCALABILITY_PROFILE_WINDOW_US;
  Scalability_InitTasks(tasks, profile_start_us);

  while (profile_finished == 0U)
  {
    for (uint32_t task_index = 0; task_index < SCALABILITY_TASK_COUNT;
         task_index++)
    {
      ScalabilityTask_t *task = &tasks[task_index];
      uint32_t readiness_start_cycles = DWT->CYCCNT;
      uint64_t now_us = scheduler_now_us();
      uint8_t task_ready = (now_us >= task->next_release_us);
      uint32_t readiness_end_cycles = DWT->CYCCNT;

      readiness_cycles += Correct_DWT_Delta(
          readiness_end_cycles - readiness_start_cycles,
          dwt_measurement_overhead_cycles);

      if ((int64_t)(now_us - profile_end_us) >= 0)
      {
        profile_finished = 1U;
        break;
      }

      if (task_ready != 0U)
      {
        uint32_t task_start_cycles = DWT->CYCCNT;
        Synthetic_Workload_us(task->workload_us);
        uint32_t task_end_cycles = DWT->CYCCNT;
        uint32_t release_start_cycles;
        uint32_t release_end_cycles;
        uint64_t finish_us;

        task->task_cycles += Correct_DWT_Delta(
            task_end_cycles - task_start_cycles,
            dwt_measurement_overhead_cycles);

        release_start_cycles = DWT->CYCCNT;
        finish_us = scheduler_now_us();
        task->next_release_us += task->period_us;

        while ((int64_t)(finish_us - task->next_release_us) > 0)
        {
          task->next_release_us += task->period_us;
        }

        release_end_cycles = DWT->CYCCNT;
        release_maintenance_cycles += Correct_DWT_Delta(
            release_end_cycles - release_start_cycles,
            dwt_measurement_overhead_cycles);
        task->runs++;
      }
    }
  }

  profile_finish_us = scheduler_now_us();
  profile_elapsed_us = profile_finish_us - profile_start_us;

  for (uint32_t task_index = 0; task_index < SCALABILITY_TASK_COUNT;
       task_index++)
  {
    task_cycles += tasks[task_index].task_cycles;
  }

  check_cycles = readiness_cycles + release_maintenance_cycles;
  task_us = task_cycles / PlatformTime_CyclesPerUs();
  readiness_us = readiness_cycles / PlatformTime_CyclesPerUs();
  release_us = release_maintenance_cycles / PlatformTime_CyclesPerUs();
  checks_us = check_cycles / PlatformTime_CyclesPerUs();

  uart_print("\r\n=== SUPERLOOP SCALABILITY CHECKS ===\r\nscenario: ");
  uart_print(SCALABILITY_WORKLOAD_SCENARIO_NAME);
  uart_print("\r\ntasks: ");
  uart_print_u64(SCALABILITY_TASK_COUNT);
  uart_print("\r\nwindow_us: ");
  uart_print_u64(profile_elapsed_us);
  uart_print("\r\n\r\ntask_execution:\r\n  total_us: ");
  uart_print_u64(task_us);
  uart_print("\r\n  percent: ");
  uart_print_percent_x10000(task_us, profile_elapsed_us);
  uart_print(" %\r\n\r\nreadiness:\r\n  total_us: ");
  uart_print_u64(readiness_us);
  uart_print("\r\n  percent: ");
  uart_print_percent_x10000(readiness_us, profile_elapsed_us);
  uart_print(" %\r\n\r\nrelease_maintenance:\r\n  total_us: ");
  uart_print_u64(release_us);
  uart_print("\r\n  percent: ");
  uart_print_percent_x10000(release_us, profile_elapsed_us);
  uart_print(" %\r\n\r\nmeasured_checks:\r\n  total_us: ");
  uart_print_u64(checks_us);
  uart_print("\r\n  percent: ");
  uart_print_percent_x10000(checks_us, profile_elapsed_us);
  uart_print(" %\r\n");

  uart_print("CSV_SUPERLOOP_SCALE_CHECKS,SCENARIO=");
  uart_print(SCALABILITY_WORKLOAD_SCENARIO_NAME);
  uart_print(",U=");
  uart_print_u64(SCALABILITY_WORKLOAD_UTILIZATION_PERCENT);
  uart_print(",TASKS=");
  uart_print_u64(SCALABILITY_TASK_COUNT);
  uart_print(",WINDOW_US=");
  uart_print_u64(profile_elapsed_us);
  Scalability_PrintCsvRuns(tasks);
  uart_print(",TASK_US=");
  uart_print_u64(task_us);
  uart_print(",TASK_PCT=");
  uart_print_percent_x10000(task_us, profile_elapsed_us);
  uart_print(",READINESS_US=");
  uart_print_u64(readiness_us);
  uart_print(",READINESS_PCT=");
  uart_print_percent_x10000(readiness_us, profile_elapsed_us);
  uart_print(",RELEASE_US=");
  uart_print_u64(release_us);
  uart_print(",RELEASE_PCT=");
  uart_print_percent_x10000(release_us, profile_elapsed_us);
  uart_print(",CHECKS_US=");
  uart_print_u64(checks_us);
  uart_print(",CHECKS_PCT=");
  uart_print_percent_x10000(checks_us, profile_elapsed_us);
  uart_print("\r\n");

  while (1)
  {
  }
}
#endif
