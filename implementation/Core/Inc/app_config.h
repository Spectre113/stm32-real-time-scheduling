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

/* Synthetic workload scenarios. */
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
#define EXPERIMENT_ISOLATED_TAU1 1
#define EXPERIMENT_ISOLATED_TAU2 2
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

/* 2 = IMU + LiDAR; 3 = IMU + LiDAR + Camera (integrated mode only). */
#ifndef INTEGRATED_SYNTH_TASK_COUNT
  #define INTEGRATED_SYNTH_TASK_COUNT 2
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

#ifndef ENABLE_REAL_TAU1
  #define ENABLE_REAL_TAU1 1
#endif

#ifndef ENABLE_REAL_TAU2
  #define ENABLE_REAL_TAU2 1
#endif

#ifndef ENABLE_SYNTH_IMU
  #define ENABLE_SYNTH_IMU 0
#endif

#ifndef ENABLE_SYNTH_LIDAR
  #define ENABLE_SYNTH_LIDAR 0
#endif

#ifndef ENABLE_SYNTH_CONTROL
  #define ENABLE_SYNTH_CONTROL 0
#endif

#ifndef ENABLE_DEBUG_PRINT
  #define ENABLE_DEBUG_PRINT 0
#endif

#ifndef ENABLE_POLLING_PROFILE
  #define ENABLE_POLLING_PROFILE 1
#endif

#ifndef SCHEDULER_MODE
  #define SCHEDULER_MODE SCHED_BUSY_POLLING
#endif

#endif /* APP_CONFIG_H */
