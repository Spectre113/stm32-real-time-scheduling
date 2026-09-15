#ifndef SCHEDULER_RELEASE_H
#define SCHEDULER_RELEASE_H

#include "scheduler_types.h"

void Task_AdvanceRelease(Task_t *task, uint64_t now_us);

#endif /* SCHEDULER_RELEASE_H */
