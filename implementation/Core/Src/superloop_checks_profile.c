#include "main.h"

#include <stdint.h>

#include "app_config.h"
#include "app_uart.h"
#include "minimal_superloop_config.h"
#include "platform_time.h"
#include "superloop_checks_profile.h"

#if EXPERIMENT_MODE == EXPERIMENT_SUPERLOOP_CHECKS_PROFILE
/* Diagnostic profile: DWT instrumentation is intentionally part of checks. */
void SuperloopChecksProfile_Run(void)
{
  uint64_t profile_start_us;
  uint64_t profile_end_us;
  uint64_t profile_finish_us;
  uint64_t profile_elapsed_us;
  uint64_t tau1_next_release_us;
  uint64_t tau2_next_release_us;
  uint64_t readiness_cycles = 0ULL;
  uint64_t release_maintenance_cycles = 0ULL;
  uint64_t tau1_task_cycles = 0ULL;
  uint64_t tau2_task_cycles = 0ULL;
  uint64_t task_cycles;
  uint64_t check_cycles;
  uint64_t tau1_task_us;
  uint64_t tau2_task_us;
  uint64_t task_us;
  uint64_t readiness_us;
  uint64_t release_us;
  uint64_t checks_us;
  uint32_t dwt_measurement_overhead_cycles = UINT32_MAX;
  uint32_t tau1_runs = 0U;
  uint32_t tau2_runs = 0U;

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
    uint32_t check_start_cycles = DWT->CYCCNT;
    uint64_t now_us = scheduler_now_us();
    uint8_t tau1_ready = (now_us >= tau1_next_release_us);
    uint32_t check_end_cycles = DWT->CYCCNT;

    readiness_cycles += Correct_DWT_Delta(check_end_cycles - check_start_cycles,
                                          dwt_measurement_overhead_cycles);

    if ((int64_t)(now_us - profile_end_us) >= 0)
    {
      break;
    }

    if (tau1_ready != 0U)
    {
      uint32_t task_start_cycles = DWT->CYCCNT;
      Synthetic_Workload_us(MINIMAL_TAU1_WORKLOAD_US);
      uint32_t task_end_cycles = DWT->CYCCNT;
      uint32_t release_start_cycles;
      uint32_t release_end_cycles;
      uint64_t finish_us;

      tau1_task_cycles += Correct_DWT_Delta(task_end_cycles - task_start_cycles,
                                            dwt_measurement_overhead_cycles);
      release_start_cycles = DWT->CYCCNT;
      finish_us = scheduler_now_us();
      tau1_next_release_us += MINIMAL_TAU1_PERIOD_US;

      while ((int64_t)(finish_us - tau1_next_release_us) > 0)
      {
        tau1_next_release_us += MINIMAL_TAU1_PERIOD_US;
      }

      release_end_cycles = DWT->CYCCNT;
      release_maintenance_cycles += Correct_DWT_Delta(
          release_end_cycles - release_start_cycles,
          dwt_measurement_overhead_cycles);
      tau1_runs++;
    }

    check_start_cycles = DWT->CYCCNT;
    now_us = scheduler_now_us();
    uint8_t tau2_ready = (now_us >= tau2_next_release_us);
    check_end_cycles = DWT->CYCCNT;

    readiness_cycles += Correct_DWT_Delta(check_end_cycles - check_start_cycles,
                                          dwt_measurement_overhead_cycles);

    if ((int64_t)(now_us - profile_end_us) >= 0)
    {
      break;
    }

    if (tau2_ready != 0U)
    {
      uint32_t task_start_cycles = DWT->CYCCNT;
      Synthetic_Workload_us(MINIMAL_TAU2_WORKLOAD_US);
      uint32_t task_end_cycles = DWT->CYCCNT;
      uint32_t release_start_cycles;
      uint32_t release_end_cycles;
      uint64_t finish_us;

      tau2_task_cycles += Correct_DWT_Delta(task_end_cycles - task_start_cycles,
                                            dwt_measurement_overhead_cycles);
      release_start_cycles = DWT->CYCCNT;
      finish_us = scheduler_now_us();
      tau2_next_release_us += MINIMAL_TAU2_PERIOD_US;

      while ((int64_t)(finish_us - tau2_next_release_us) > 0)
      {
        tau2_next_release_us += MINIMAL_TAU2_PERIOD_US;
      }

      release_end_cycles = DWT->CYCCNT;
      release_maintenance_cycles += Correct_DWT_Delta(
          release_end_cycles - release_start_cycles,
          dwt_measurement_overhead_cycles);
      tau2_runs++;
    }
  }

  profile_finish_us = scheduler_now_us();
  profile_elapsed_us = profile_finish_us - profile_start_us;
  task_cycles = tau1_task_cycles + tau2_task_cycles;
  check_cycles = readiness_cycles + release_maintenance_cycles;
  tau1_task_us = tau1_task_cycles / PlatformTime_CyclesPerUs();
  tau2_task_us = tau2_task_cycles / PlatformTime_CyclesPerUs();
  task_us = task_cycles / PlatformTime_CyclesPerUs();
  readiness_us = readiness_cycles / PlatformTime_CyclesPerUs();
  release_us = release_maintenance_cycles / PlatformTime_CyclesPerUs();
  checks_us = check_cycles / PlatformTime_CyclesPerUs();

  uart_print("\r\n=== SUPERLOOP CHECKS PROFILE ===\r\nscenario: ");
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
  uart_print(" %\r\n\r\nreadiness_checks:\r\n  total_cycles: ");
  uart_print_u64(readiness_cycles);
  uart_print("\r\n  total_us: ");
  uart_print_u64(readiness_us);
  uart_print("\r\n  percent: ");
  uart_print_percent_x10000(readiness_us, profile_elapsed_us);
  uart_print(" %\r\n\r\nrelease_maintenance:\r\n  total_cycles: ");
  uart_print_u64(release_maintenance_cycles);
  uart_print("\r\n  total_us: ");
  uart_print_u64(release_us);
  uart_print("\r\n  percent: ");
  uart_print_percent_x10000(release_us, profile_elapsed_us);
  uart_print(" %\r\n\r\nmeasured_checks:\r\n  total_cycles: ");
  uart_print_u64(check_cycles);
  uart_print("\r\n  total_us: ");
  uart_print_u64(checks_us);
  uart_print("\r\n  percent: ");
  uart_print_percent_x10000(checks_us, profile_elapsed_us);
  uart_print(" %\r\n");

  uart_print("CSV_SUPERLOOP_CHECKS,SCENARIO=");
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
  uart_print(",READINESS_CYCLES=");
  uart_print_u64(readiness_cycles);
  uart_print(",READINESS_US=");
  uart_print_u64(readiness_us);
  uart_print(",READINESS_PCT=");
  uart_print_percent_x10000(readiness_us, profile_elapsed_us);
  uart_print(",RELEASE_CYCLES=");
  uart_print_u64(release_maintenance_cycles);
  uart_print(",RELEASE_US=");
  uart_print_u64(release_us);
  uart_print(",RELEASE_PCT=");
  uart_print_percent_x10000(release_us, profile_elapsed_us);
  uart_print(",CHECKS_CYCLES=");
  uart_print_u64(check_cycles);
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
