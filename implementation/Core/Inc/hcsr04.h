#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>

int HCSR04_Read_cm_Blocking(void);

void HCSR04_Async_Reset(void);
void HCSR04_Async_Start(void);
uint8_t HCSR04_Async_IsRunnable(uint64_t now_us);
uint8_t HCSR04_Async_Finalize(int *distance_cm);
uint8_t HCSR04_Async_Timeout(int *distance_cm);
void HCSR04_Async_OnExti(uint16_t gpio_pin);

#endif /* HCSR04_H */
