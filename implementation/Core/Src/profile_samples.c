#include "profile_samples.h"
#include "app_config.h"
#include "app_uart.h"

#include <stdio.h>

#if ENABLE_EXTENDED_STATS
#define HCSR04_MAX_SAMPLES 700U
#define DHT11_MAX_SAMPLES 50U

static uint32_t g_hcsr04_exec_samples[HCSR04_MAX_SAMPLES];
static uint32_t g_dht11_exec_samples[DHT11_MAX_SAMPLES];
static uint32_t g_hcsr04_sample_count;
static uint32_t g_dht11_sample_count;

void ProfileSamples_Reset(void)
{
  g_hcsr04_sample_count = 0U;
  g_dht11_sample_count = 0U;
}

void ProfileSamples_SaveHCSR04(uint64_t exec_time_us)
{
  if (g_hcsr04_sample_count < HCSR04_MAX_SAMPLES)
  {
    g_hcsr04_exec_samples[g_hcsr04_sample_count++] = (uint32_t)exec_time_us;
  }
}

void ProfileSamples_SaveDHT11(uint64_t exec_time_us)
{
  if (g_dht11_sample_count < DHT11_MAX_SAMPLES)
  {
    g_dht11_exec_samples[g_dht11_sample_count++] = (uint32_t)exec_time_us;
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
  uart_print("\r\nHCSR04_EXEC_SAMPLES_US:\r\n");
  ProfileSamples_PrintList(g_hcsr04_exec_samples, g_hcsr04_sample_count);
  uart_print("\r\n\r\nDHT11_EXEC_SAMPLES_US:\r\n");
  ProfileSamples_PrintList(g_dht11_exec_samples, g_dht11_sample_count);
  uart_print("\r\n");
}

#else
void ProfileSamples_Reset(void) {}
void ProfileSamples_SaveHCSR04(uint64_t exec_time_us) { (void)exec_time_us; }
void ProfileSamples_SaveDHT11(uint64_t exec_time_us) { (void)exec_time_us; }
void ProfileSamples_Print(void) {}
#endif
