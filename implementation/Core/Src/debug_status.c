#include "debug_status.h"
#include "app_uart.h"

#include <stdio.h>

void DebugStatus_Print(uint32_t tau1_runs,
                       uint32_t tau2_runs,
                       int distance_cm,
                       uint8_t temp,
                       uint8_t hum,
                       int dht_result)
{
  char msg[180];

  if (dht_result == 0 && distance_cm >= 0)
  {
    snprintf(msg, sizeof(msg),
             "tau1_runs=%lu | tau2_runs=%lu | Distance=%d cm | Temp=%d C | Hum=%d %%\r\n",
             (unsigned long)tau1_runs, (unsigned long)tau2_runs, distance_cm,
             temp, hum);
  }
  else if (dht_result != 0 && distance_cm >= 0)
  {
    snprintf(msg, sizeof(msg),
             "tau1_runs=%lu | tau2_runs=%lu | Distance=%d cm | DHT11 error=%d\r\n",
             (unsigned long)tau1_runs, (unsigned long)tau2_runs, distance_cm,
             dht_result);
  }
  else if (dht_result == 0)
  {
    snprintf(msg, sizeof(msg),
             "tau1_runs=%lu | tau2_runs=%lu | HC-SR04 error=%d | Temp=%d C | Hum=%d %%\r\n",
             (unsigned long)tau1_runs, (unsigned long)tau2_runs, distance_cm,
             temp, hum);
  }
  else
  {
    snprintf(msg, sizeof(msg),
             "tau1_runs=%lu | tau2_runs=%lu | HC-SR04 error=%d | DHT11 error=%d\r\n",
             (unsigned long)tau1_runs, (unsigned long)tau2_runs, distance_cm,
             dht_result);
  }

  uart_print(msg);
}
