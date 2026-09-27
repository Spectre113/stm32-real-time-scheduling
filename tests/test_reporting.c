#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "task_reporting.h"

static char output[1024];
void uart_print(const char *text) { snprintf(output, sizeof(output), "%s", text); }
static void report(Task_t *task, uint64_t now)
{
  TaskReporting_PrintCsvTask("CHUNKED_EDF", "U50", 50, "TEST", task, 10000, now);
  assert(strstr(output, "\r\n") != NULL);
}
int main(void)
{
  Task_t task = {0};
  task.deadline_us = 10000;
  task.next_release_us = 1000;
  report(&task, 999);
  assert(strstr(output, "PENDING=0,PENDING_OVERDUE=0,PENDING_AGE_US=0,PENDING_EXEC_US=0"));
  report(&task, 1000);
  assert(strstr(output, "PENDING=1,PENDING_OVERDUE=0,PENDING_AGE_US=0"));
  task.job_active = 1;
  task.active_release_us = 1000;
  task.remaining_exec_us = 2000;
  task.accumulated_exec_us = 8000;
  report(&task, 11000);
  assert(strstr(output, "PENDING=1,PENDING_OVERDUE=0,PENDING_AGE_US=10000,PENDING_EXEC_US=8000"));
  report(&task, 11001);
  assert(strstr(output, "PENDING=1,PENDING_OVERDUE=1,PENDING_AGE_US=10001,PENDING_EXEC_US=8000"));
  assert(task.job_active && task.remaining_exec_us == 2000 && task.run_count == 0);
  assert(task.deadline_miss_count == 0);
  task.job_active = 0;
  task.accumulated_exec_us = 0;
  task.next_release_us = 20000;
  task.run_count = 2;
  task.total_response_us = 12000;
  report(&task, 15000);
  assert(strstr(output, "RESP_AVG_US=6000"));
  assert(strstr(output, "PENDING=0,PENDING_OVERDUE=0"));
  task.job_active = 1;
  task.active_release_us = 0;
  task.accumulated_exec_us = UINT64_MAX;
  report(&task, UINT64_MAX);
  assert(strstr(output, "PENDING_AGE_US=18446744073709551615,PENDING_EXEC_US=18446744073709551615"));
  puts("reporting boundary tests passed");
}
