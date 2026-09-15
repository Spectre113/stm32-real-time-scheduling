#include "main.h"
#include "dht11.h"
#include "platform_time.h"

#define DHT11_PORT GPIOA
#define DHT11_PIN GPIO_PIN_5

static DHT11_Context_t g_dht11_ctx;

static void DHT11_SetOutput(void)
{
  GPIO_InitTypeDef gpio_init = {0};
  gpio_init.Pin = DHT11_PIN;
  gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
  gpio_init.Pull = GPIO_PULLUP;
  gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_PORT, &gpio_init);
}

static void DHT11_SetInput(void)
{
  GPIO_InitTypeDef gpio_init = {0};
  gpio_init.Pin = DHT11_PIN;
  gpio_init.Mode = GPIO_MODE_INPUT;
  gpio_init.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DHT11_PORT, &gpio_init);
}

static int DHT11_ReadTransaction(uint8_t *temp, uint8_t *hum)
{
  uint8_t data[5] = {0};
  uint64_t t;

  DHT11_SetInput();
  delay_us(40);

  t = micros();
  while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
  {
    if ((micros() - t) > 200ULL) return -1;
  }

  t = micros();
  while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET)
  {
    if ((micros() - t) > 100ULL) return -2;
  }

  t = micros();
  while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
  {
    if ((micros() - t) > 100ULL) return -3;
  }

  for (int i = 0; i < 40; i++)
  {
    t = micros();
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET)
    {
      if ((micros() - t) > 100ULL) return -4;
    }

    uint64_t start = micros();
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
    {
      if ((micros() - start) > 100ULL) break;
    }

    if ((micros() - start) > 40ULL)
    {
      data[i / 8] |= (1U << (7 - (i % 8)));
    }
  }

  if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4])
  {
    return -5;
  }

  *hum = data[0];
  *temp = data[2];
  return 0;
}

int DHT11_Read(uint8_t *temp, uint8_t *hum)
{
  DHT11_SetOutput();
  HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
  HAL_Delay(30);

  return DHT11_ReadTransaction(temp, hum);
}

void DHT11_Async_Reset(void)
{
  g_dht11_ctx.state = DHT11_IDLE;
  g_dht11_ctx.wait_until_us = 0ULL;
  g_dht11_ctx.result = -99;
  DHT11_SetInput();
}

uint8_t DHT11_Async_IsRunnable(uint64_t now_us)
{
  if (g_dht11_ctx.state == DHT11_WAIT_START_LOW)
  {
    return ((int64_t)(now_us - g_dht11_ctx.wait_until_us) >= 0);
  }

  return (g_dht11_ctx.state == DHT11_IDLE) ||
         (g_dht11_ctx.state == DHT11_READ_TRANSACTION);
}

DHT11_StepResult_t DHT11_Async_Step(uint64_t now_us,
                                    uint8_t *temp,
                                    uint8_t *hum)
{
  switch (g_dht11_ctx.state)
  {
    case DHT11_IDLE:
      g_dht11_ctx.state = DHT11_START_LOW;
      DHT11_SetOutput();
      HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
      g_dht11_ctx.wait_until_us = now_us + 30000ULL;
      g_dht11_ctx.state = DHT11_WAIT_START_LOW;
      return DHT11_STEP_WAITING;

    case DHT11_WAIT_START_LOW:
      if ((int64_t)(now_us - g_dht11_ctx.wait_until_us) < 0)
      {
        return DHT11_STEP_WAITING;
      }
      g_dht11_ctx.state = DHT11_READ_TRANSACTION;
      /* Fall through: the remaining bitstream must run atomically. */

    case DHT11_READ_TRANSACTION:
      g_dht11_ctx.result = DHT11_ReadTransaction(temp, hum);
      g_dht11_ctx.state = (g_dht11_ctx.result == 0)
          ? DHT11_DONE : DHT11_ERROR;
      return (g_dht11_ctx.result == 0)
          ? DHT11_STEP_COMPLETE : DHT11_STEP_ERROR;

    default:
      g_dht11_ctx.result = -1;
      g_dht11_ctx.state = DHT11_ERROR;
      return DHT11_STEP_ERROR;
  }
}

int DHT11_Async_Result(void)
{
  return g_dht11_ctx.result;
}
