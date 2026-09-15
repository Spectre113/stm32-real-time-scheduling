#include "profile_samples.h"
#include "app_uart.h"

#include <stdio.h>

#define TAU1_MAX_SAMPLES 700U
#define TAU2_MAX_SAMPLES 50U

static uint32_t g_tau1_exec_samples[TAU1_MAX_SAMPLES];
static uint32_t g_tau2_exec_samples[TAU2_MAX_SAMPLES];
static uint32_t g_tau1_sample_count;
static uint32_t g_tau2_sample_count;

void ProfileSamples_Reset(void)
{
  g_tau1_sample_count = 0U;
  g_tau2_sample_count = 0U;
}

void ProfileSamples_SaveTau1(uint64_t exec_time_us)
{
  if (g_tau1_sample_count < TAU1_MAX_SAMPLES)
  {
    g_tau1_exec_samples[g_tau1_sample_count++] = (uint32_t)exec_time_us;
  }
}

void ProfileSamples_SaveTau2(uint64_t exec_time_us)
{
  if (g_tau2_sample_count < TAU2_MAX_SAMPLES)
  {
    g_tau2_exec_samples[g_tau2_sample_count++] = (uint32_t)exec_time_us;
  }
}

static void ProfileSamples_PrintList(const uint32_t *samples, uint32_t count)
{
  char msg[64];

  for (uint32_t i = 0U; i < count; i++)
  {
    snprintf(msg, sizeof(msg), "%lu", (unsigned long)samples[i]);
    uart_print(msg);

    if (i + 1U < count) uart_print(",");
    if ((i + 1U) % 20U == 0U) uart_print("\r\n");
  }
}

void ProfileSamples_Print(void)
{
  uart_print("\r\nTAU1_EXEC_SAMPLES_US:\r\n");
  ProfileSamples_PrintList(g_tau1_exec_samples, g_tau1_sample_count);
  uart_print("\r\n\r\nTAU2_EXEC_SAMPLES_US:\r\n");
  ProfileSamples_PrintList(g_tau2_exec_samples, g_tau2_sample_count);
  uart_print("\r\n");
}
