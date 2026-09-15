#include "main.h"

#include <stdint.h>

#include "app_config.h"
#include "app_uart.h"
#include "minimal_superloop_config.h"
#include "minimal_superloop_profile.h"
#include "platform_time.h"

#if EXPERIMENT_MODE == EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE
/* Measures task bodies and the remaining clean two-task busy-polling Superloop. */
void MinimalSuperloopProfile_Run(void)
{
  uint64_t profile_start_us;
  uint64_t profile_end_us;
  uint64_t profile_finish_us;
  uint64_t profile_elapsed_us;
  uint64_t tau1_next_release_us;
  uint64_t tau2_next_release_us;
  uint64_t tau1_task_cycles = 0;
  uint64_t tau2_task_cycles = 0;
  uint64_t task_cycles;
  uint64_t tau1_task_us;
  uint64_t tau2_task_us;
  uint64_t task_us;
  uint64_t superloop_us;
  uint32_t dwt_measurement_overhead_cycles = UINT32_MAX;
  uint32_t tau1_runs = 0;
  uint32_t tau2_runs = 0;

  for (uint32_t i = 0; i < 1000U; i++)
  {
    uint32_t start_cycles = DWT->CYCCNT;
    uint32_t end_cycles = DWT->CYCCNT;
    uint32_t delta_cycles = end_cycles - start_cycles;

    if (delta_cycles < dwt_measurement_overhead_cycles)
    {
      dwt_measurement_overhead_cycles = delta_cycles;
    }
  }

  profile_start_us = scheduler_now_us();
  profile_end_us = profile_start_us + MINIMAL_PROFILE_WINDOW_US;
  tau1_next_release_us = profile_start_us;
  tau2_next_release_us = profile_start_us;

  while (1)
  {
    uint64_t now_us = scheduler_now_us();

    if ((int64_t)(now_us - profile_end_us) >= 0)
    {
      break;
    }

    if (now_us >= tau1_next_release_us)
    {
      uint32_t task_start_cycles = DWT->CYCCNT;
      Synthetic_Workload_us(MINIMAL_TAU1_WORKLOAD_US);
      uint32_t task_end_cycles = DWT->CYCCNT;
      tau1_task_cycles += Correct_DWT_Delta(task_end_cycles - task_start_cycles,
                                            dwt_measurement_overhead_cycles);

      uint64_t finish_us = scheduler_now_us();
      tau1_next_release_us += MINIMAL_TAU1_PERIOD_US;

      while (finish_us > tau1_next_release_us)
      {
        tau1_next_release_us += MINIMAL_TAU1_PERIOD_US;
      }

      tau1_runs++;
    }

    now_us = scheduler_now_us();

    if ((int64_t)(now_us - profile_end_us) >= 0)
    {
      break;
    }

    if (now_us >= tau2_next_release_us)
    {
      uint32_t task_start_cycles = DWT->CYCCNT;
      Synthetic_Workload_us(MINIMAL_TAU2_WORKLOAD_US);
      uint32_t task_end_cycles = DWT->CYCCNT;
      tau2_task_cycles += Correct_DWT_Delta(task_end_cycles - task_start_cycles,
                                            dwt_measurement_overhead_cycles);

      uint64_t finish_us = scheduler_now_us();
      tau2_next_release_us += MINIMAL_TAU2_PERIOD_US;

      while (finish_us > tau2_next_release_us)
      {
        tau2_next_release_us += MINIMAL_TAU2_PERIOD_US;
      }

      tau2_runs++;
    }
  }

  profile_finish_us = scheduler_now_us();
  profile_elapsed_us = profile_finish_us - profile_start_us;
  tau1_task_us = tau1_task_cycles / PlatformTime_CyclesPerUs();
  tau2_task_us = tau2_task_cycles / PlatformTime_CyclesPerUs();
  task_cycles = tau1_task_cycles + tau2_task_cycles;
  task_us = task_cycles / PlatformTime_CyclesPerUs();
  /* Includes end-window checks, task DWT measurements, and runs++ counters. */
  superloop_us = profile_elapsed_us > task_us
                 ? profile_elapsed_us - task_us : 0ULL;

  uart_print("\r\n=== CLEAN SUPERLOOP PROFILE ===\r\n");
  uart_print("scenario: ");
  uart_print(MINIMAL_WORKLOAD_SCENARIO_NAME);
  uart_print("\r\nwindow_us: ");
  uart_print_u64(profile_elapsed_us);
  uart_print("\r\n\r\ntau1:\r\n  runs: ");
  uart_print_u64(tau1_runs);
  uart_print("\r\n  task_us: ");
  uart_print_u64(tau1_task_us);
  uart_print("\r\n\r\ntau2:\r\n  runs: ");
  uart_print_u64(tau2_runs);
  uart_print("\r\n  task_us: ");
  uart_print_u64(tau2_task_us);
  uart_print("\r\n\r\ntask_execution:\r\n  total_us: ");
  uart_print_u64(task_us);
  uart_print("\r\n  percent: ");
  uart_print_percent_x10000(task_us, profile_elapsed_us);
  uart_print(" %\r\n\r\nsuperloop:\r\n  total_us: ");
  uart_print_u64(superloop_us);
  uart_print("\r\n  percent: ");
  uart_print_percent_x10000(superloop_us, profile_elapsed_us);
  uart_print(" %\r\n");

  uart_print("CSV_CLEAN_SUPERLOOP,SCENARIO=");
  uart_print(MINIMAL_WORKLOAD_SCENARIO_NAME);
  uart_print(",U=");
  uart_print_u64(MINIMAL_WORKLOAD_UTILIZATION_PERCENT);
  uart_print(",WINDOW_US=");
  uart_print_u64(profile_elapsed_us);
  uart_print(",TAU1_RUNS=");
  uart_print_u64(tau1_runs);
  uart_print(",TAU2_RUNS=");
  uart_print_u64(tau2_runs);
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
