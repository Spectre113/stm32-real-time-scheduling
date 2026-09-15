#ifndef ISOLATED_PROFILE_H
#define ISOLATED_PROFILE_H

#include "scheduler_types.h"

typedef void (*IsolatedProfileCallback_t)(void);
typedef void (*IsolatedProfileSampleFn_t)(uint64_t exec_time_us);

void IsolatedProfile_Run(Task_t *task,
                         TaskRunFn run_task,
                         const char *title,
                         const char *description,
                         uint32_t window_ms,
                         uint8_t use_wfi,
                         IsolatedProfileCallback_t reset_stats,
                         IsolatedProfileSampleFn_t save_sample,
                         IsolatedProfileCallback_t print_summary);

#endif /* ISOLATED_PROFILE_H */
