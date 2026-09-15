#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

#include "scheduler_types.h"

int DHT11_Read(uint8_t *temp, uint8_t *hum);

void DHT11_Async_Reset(void);
uint8_t DHT11_Async_IsRunnable(uint64_t now_us);
DHT11_StepResult_t DHT11_Async_Step(uint64_t now_us,
                                    uint8_t *temp,
                                    uint8_t *hum);
int DHT11_Async_Result(void);

#endif /* DHT11_H */
