#ifndef SCHEDULER_TYPES_H
#define SCHEDULER_TYPES_H

#include <stdint.h>

/* This is fixed profiling-storage layout, not a manual experiment switch. */
#ifndef EXEC_HIST_BINS
#define EXEC_HIST_BINS 10
#endif

typedef struct
{
  const char *name;

  uint64_t period_us;
  uint64_t deadline_us;
  uint64_t next_release_us;

  uint32_t period_ms;
  uint32_t deadline_ms;
  uint32_t next_release_ms;

  uint32_t run_count;

  uint64_t total_exec_us;
  uint64_t min_exec_us;
  uint64_t max_exec_us;

  uint64_t total_response_us;
  uint64_t min_response_us;
  uint64_t max_response_us;

  uint32_t deadline_miss_count;
  uint64_t max_lateness_us;

  uint64_t skipped_release_count;
  uint64_t total_timing_failures;

  uint8_t job_active;
  uint64_t active_release_us;
  uint64_t remaining_exec_us;
  uint64_t accumulated_exec_us;

  uint32_t exec_hist[EXEC_HIST_BINS];
} Task_t;

typedef void (*TaskRunFn)(void);

typedef enum
{
  SCHED_TASK_SYNTHETIC = 0,
  SCHED_TASK_STAGED_HCSR04,
  SCHED_TASK_STAGED_DHT11
} SchedTaskKind_t;

typedef struct
{
  Task_t *task;
  TaskRunFn run;
  uint64_t workload_us;
  uint8_t enabled;
  SchedTaskKind_t kind;
} SchedTaskRef_t;

typedef enum
{
  HCSR04_IDLE = 0,
  HCSR04_TRIGGER,
  HCSR04_WAIT_ECHO_RISE,
  HCSR04_WAIT_ECHO_FALL,
  HCSR04_COMPLETE,
  HCSR04_ERROR
} HCSR04_State_t;

typedef enum
{
  DHT11_IDLE = 0,
  DHT11_START_LOW,
  DHT11_WAIT_START_LOW,
  DHT11_READ_TRANSACTION,
  DHT11_DONE,
  DHT11_ERROR
} DHT11_State_t;

typedef enum
{
  DHT11_STEP_WAITING = 0,
  DHT11_STEP_COMPLETE,
  DHT11_STEP_ERROR
} DHT11_StepResult_t;

typedef struct
{
  DHT11_State_t state;
  uint64_t wait_until_us;
  int result;
} DHT11_Context_t;

#endif /* SCHEDULER_TYPES_H */
