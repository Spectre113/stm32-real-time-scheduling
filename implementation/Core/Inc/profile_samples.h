#ifndef PROFILE_SAMPLES_H
#define PROFILE_SAMPLES_H

#include <stdint.h>

void ProfileSamples_Reset(void);
void ProfileSamples_SaveHCSR04(uint64_t exec_time_us);
void ProfileSamples_SaveDHT11(uint64_t exec_time_us);
void ProfileSamples_Print(void);

#endif /* PROFILE_SAMPLES_H */
