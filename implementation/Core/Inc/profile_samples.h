#ifndef PROFILE_SAMPLES_H
#define PROFILE_SAMPLES_H

#include <stdint.h>

void ProfileSamples_Reset(void);
void ProfileSamples_SaveTau1(uint64_t exec_time_us);
void ProfileSamples_SaveTau2(uint64_t exec_time_us);
void ProfileSamples_Print(void);

#endif /* PROFILE_SAMPLES_H */
