#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*
 * Manual experiment configuration.
 *
 * Change values in this file for a normal CubeIDE build. The automation
 * runner temporarily writes experiment_config.h first; its values override
 * the defaults below without modifying this file.
 */
#include "experiment_config.h"

/* Scheduler algorithms. */
#define SCHED_ALGO_SUPERLOOP 0
#define SCHED_ALGO_CHUNKED_EDF 1

/* workload workload scenarios. */
#define WORKLOAD_SCENARIO_U50 0
#define WORKLOAD_SCENARIO_U65 1
#define WORKLOAD_SCENARIO_U80 2
#define WORKLOAD_SCENARIO_U90 3
#define WORKLOAD_SCENARIO_U95 4
#define WORKLOAD_SCENARIO_U100 5
#define WORKLOAD_SCENARIO_U110 6
#define WORKLOAD_SCENARIO_U75 7

/* Experiment modes. */
#define EXPERIMENT_INTEGRATED 0
#define EXPERIMENT_ISOLATED_HCSR04 1
#define EXPERIMENT_ISOLATED_DHT11 2
#define EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE 3
#define EXPERIMENT_SUPERLOOP_CHECKS_PROFILE 4
#define EXPERIMENT_SUPERLOOP_SCALABILITY_CLEAN 5
#define EXPERIMENT_SUPERLOOP_SCALABILITY_CHECKS 6

/* Integrated scheduler idle policy. */
#define SCHED_BUSY_POLLING 0
#define SCHED_WFI_OPTIMIZED 1

#ifndef WORKLOAD_SCENARIO
  #define WORKLOAD_SCENARIO WORKLOAD_SCENARIO_U50
#endif

#ifndef SCHED_ALGO
  #define SCHED_ALGO SCHED_ALGO_SUPERLOOP
#endif

#ifndef EXPERIMENT_MODE
  #define EXPERIMENT_MODE EXPERIMENT_INTEGRATED
#endif

/* 2 = tau1 + tau2; 3 = tau1 + tau2 + tau3 (integrated mode only). */
#ifndef INTEGRATED_TASK_COUNT
  #define INTEGRATED_TASK_COUNT 2
#endif

/* Measurement windows, in microseconds. */
#ifndef PROFILE_WINDOW_US
  #define PROFILE_WINDOW_US 60000000ULL
#endif

#ifndef MINIMAL_PROFILE_WINDOW_US
  #define MINIMAL_PROFILE_WINDOW_US 60000000ULL
#endif

#ifndef SCALABILITY_PROFILE_WINDOW_US
  #define SCALABILITY_PROFILE_WINDOW_US 60000000ULL
#endif

#ifndef SCALABILITY_TASK_COUNT
  #define SCALABILITY_TASK_COUNT 2
#endif

#ifndef EDF_CHUNK_US
  #define EDF_CHUNK_US 1000ULL
#endif

/* Physical HC-SR04 ultrasonic distance sensor; requires connected hardware. */
#ifndef ENABLE_HCSR04
  #define ENABLE_HCSR04 1
#endif

/* Physical DHT11 temperature/humidity sensor; requires connected hardware. */
#ifndef ENABLE_DHT11
  #define ENABLE_DHT11 1
#endif

/* Periodic test task tau1: T = D = 10 ms, timed CPU workload, no sensor. */
#ifndef ENABLE_TAU1
  #define ENABLE_TAU1 0
#endif

/* Periodic test task tau2: T = D = 50 ms, timed CPU workload, no sensor. */
#ifndef ENABLE_TAU2
  #define ENABLE_TAU2 0
#endif

/* Periodic test task tau3: T = D = 200 ms, timed CPU workload, no sensor.
 * Enabled by default for the three-task experiment configuration. */
#ifndef ENABLE_TAU3
  #define ENABLE_TAU3 (INTEGRATED_TASK_COUNT == 3)
#endif

#ifndef ENABLE_DEBUG_PRINT
  #define ENABLE_DEBUG_PRINT 0
#endif

#ifndef ENABLE_POLLING_PROFILE
  #define ENABLE_POLLING_PROFILE 1
#endif

/* Compact paper metrics by default in integrated experiments. */
#ifndef ENABLE_EXTENDED_STATS
  #define ENABLE_EXTENDED_STATS (EXPERIMENT_MODE != EXPERIMENT_INTEGRATED)
#endif
#if (ENABLE_EXTENDED_STATS != 0) && (ENABLE_EXTENDED_STATS != 1)
  #error "ENABLE_EXTENDED_STATS must be 0 or 1"
#endif

#ifndef SCHEDULER_MODE
  #define SCHEDULER_MODE SCHED_BUSY_POLLING
#endif

#endif /* APP_CONFIG_H */
