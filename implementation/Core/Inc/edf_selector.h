#ifndef EDF_SELECTOR_H
#define EDF_SELECTOR_H

#include "scheduler_types.h"

SchedTaskRef_t *Scheduler_SelectChunkedEDF(SchedTaskRef_t *tasks,
                                           uint32_t count,
                                           uint64_t now_us);

#endif /* EDF_SELECTOR_H */
