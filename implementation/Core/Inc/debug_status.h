#ifndef DEBUG_STATUS_H
#define DEBUG_STATUS_H

#include <stdint.h>

void DebugStatus_Print(uint32_t tau1_runs,
                       uint32_t tau2_runs,
                       int distance_cm,
                       uint8_t temp,
                       uint8_t hum,
                       int dht_result);

#endif /* DEBUG_STATUS_H */
