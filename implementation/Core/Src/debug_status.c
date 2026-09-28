#include "debug_status.h"
#include "app_uart.h"

#include <stdio.h>

void DebugStatus_Print(uint32_t hcsr04_runs,
                       uint32_t dht11_runs,
                       int distance_cm,
                       uint8_t temp,
                       uint8_t hum,
                       int dht_result)
{
  char msg[180];

  if (dht_result == 0 && distance_cm >= 0)
  {
    snprintf(msg, sizeof(msg),
             "hcsr04_runs=%lu | dht11_runs=%lu | Distance=%d cm | Temp=%d C | Hum=%d %%\r\n",
             (unsigned long)hcsr04_runs, (unsigned long)dht11_runs, distance_cm,
             temp, hum);
  }
  else if (dht_result != 0 && distance_cm >= 0)
  {
    snprintf(msg, sizeof(msg),
             "hcsr04_runs=%lu | dht11_runs=%lu | Distance=%d cm | DHT11 error=%d\r\n",
             (unsigned long)hcsr04_runs, (unsigned long)dht11_runs, distance_cm,
             dht_result);
  }
  else if (dht_result == 0)
  {
    snprintf(msg, sizeof(msg),
             "hcsr04_runs=%lu | dht11_runs=%lu | HC-SR04 error=%d | Temp=%d C | Hum=%d %%\r\n",
             (unsigned long)hcsr04_runs, (unsigned long)dht11_runs, distance_cm,
             temp, hum);
  }
  else
  {
    snprintf(msg, sizeof(msg),
             "hcsr04_runs=%lu | dht11_runs=%lu | HC-SR04 error=%d | DHT11 error=%d\r\n",
             (unsigned long)hcsr04_runs, (unsigned long)dht11_runs, distance_cm,
             dht_result);
  }

  uart_print(msg);
}
