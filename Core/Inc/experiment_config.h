#ifndef EXPERIMENT_CONFIG_H
#define EXPERIMENT_CONFIG_H

/*
 * Default experiment configuration.
 *
 * The automation runner rewrites this file temporarily for each run and
 * restores it afterwards. Manual runs can be configured here as well.
 */

#ifndef WORKLOAD_SCENARIO
#define WORKLOAD_SCENARIO WORKLOAD_SCENARIO_U65
#endif

#ifndef SCHED_ALGO
#define SCHED_ALGO SCHED_ALGO_SUPERLOOP
#endif

#ifndef EXPERIMENT_MODE
#define EXPERIMENT_MODE EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE
#endif

#ifndef PROFILE_WINDOW_US
#define PROFILE_WINDOW_US 10000000ULL
#endif

#ifndef MINIMAL_PROFILE_WINDOW_US
#define MINIMAL_PROFILE_WINDOW_US 60000000ULL
#endif

#ifndef EDF_CHUNK_US
#define EDF_CHUNK_US 1000ULL
#endif

#endif /* EXPERIMENT_CONFIG_H */
