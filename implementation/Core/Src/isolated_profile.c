#include "main.h"
#include "isolated_profile.h"
#include "app_uart.h"
#include "platform_time.h"
#include "task_stats.h"

void IsolatedProfile_Run(Task_t *task,
                         TaskRunFn run_task,
                         const char *title,
                         const char *description,
                         uint32_t window_ms,
                         uint8_t use_wfi,
                         IsolatedProfileCallback_t reset_stats,
                         IsolatedProfileSampleFn_t save_sample,
                         IsolatedProfileCallback_t print_summary)
{
  uint32_t start_ms = HAL_GetTick();

  uart_print(title);
  uart_print(description);
  reset_stats();

  while ((uint32_t)(HAL_GetTick() - start_ms) < window_ms)
  {
    uint32_t release_ms = HAL_GetTick();
    uint64_t exec_start = micros();
    run_task();
    uint64_t exec_finish = micros();
    uint32_t finish_ms = HAL_GetTick();
    uint64_t exec_time = exec_finish - exec_start;
    uint64_t response_time = ((uint32_t)(finish_ms - release_ms)) * 1000ULL;
    uint32_t next_release_ms;

    Task_UpdateExecStats(task, exec_time);
    save_sample(exec_time);
    Task_UpdateResponseStats(task, response_time);
    Task_CheckDeadline(task, response_time);

    next_release_ms = release_ms + task->period_ms;
    while ((int32_t)(HAL_GetTick() - next_release_ms) < 0)
    {
      if (use_wfi != 0U) __WFI();
    }
  }

  print_summary();

  while (1)
  {
    if (use_wfi != 0U) __WFI();
  }
}
