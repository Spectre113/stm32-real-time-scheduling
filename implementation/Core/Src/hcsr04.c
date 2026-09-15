#include "main.h"
#include "hcsr04.h"
#include "platform_time.h"
#include "scheduler_types.h"

#define HCSR04_TRIG_PORT GPIOB
#define HCSR04_TRIG_PIN GPIO_PIN_2
#define HCSR04_ECHO_PORT GPIOC
#define HCSR04_ECHO_PIN GPIO_PIN_0
#define HCSR04_TIMEOUT_US 30000U

static volatile HCSR04_State_t g_hcsr04_state = HCSR04_IDLE;
static volatile uint8_t g_hcsr04_rise_event = 0U;
static volatile uint8_t g_hcsr04_fall_event = 0U;
static volatile uint32_t g_hcsr04_echo_rise_cycles = 0U;
static volatile uint32_t g_hcsr04_echo_fall_cycles = 0U;
static volatile uint32_t g_hcsr04_timeout_cycles = 0U;
static volatile uint32_t g_hcsr04_rise_timeout_cycles = 0U;
static volatile uint8_t g_hcsr04_rise_late = 0U;

int HCSR04_Read_cm_Blocking(void)
{
  uint64_t start_time;
  uint64_t echo_start;
  uint64_t echo_end;
  uint64_t duration_us;

  HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);
  delay_us(2);

  HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_SET);
  delay_us(10);
  HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);

  start_time = micros();

  while (HAL_GPIO_ReadPin(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN) == GPIO_PIN_RESET)
  {
    if ((micros() - start_time) > HCSR04_TIMEOUT_US)
    {
      return -1;
    }
  }

  echo_start = micros();

  while (HAL_GPIO_ReadPin(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN) == GPIO_PIN_SET)
  {
    if ((micros() - echo_start) > HCSR04_TIMEOUT_US)
    {
      return -2;
    }
  }

  echo_end = micros();
  duration_us = echo_end - echo_start;

  return (int)(duration_us / 58ULL);
}

void HCSR04_Async_Reset(void)
{
  uint32_t primask = __get_PRIMASK();

  __disable_irq();
  g_hcsr04_state = HCSR04_IDLE;
  g_hcsr04_rise_event = 0U;
  g_hcsr04_fall_event = 0U;
  __set_PRIMASK(primask);

  g_hcsr04_timeout_cycles = 0U;
  g_hcsr04_rise_timeout_cycles = 0U;
  g_hcsr04_rise_late = 0U;
}

void HCSR04_Async_Start(void)
{
  uint32_t primask;

  g_hcsr04_state = HCSR04_TRIGGER;
  HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);
  delay_us(2);

  primask = __get_PRIMASK();
  __disable_irq();
  g_hcsr04_rise_event = 0U;
  g_hcsr04_fall_event = 0U;
  g_hcsr04_rise_late = 0U;
  g_hcsr04_rise_timeout_cycles = DWT->CYCCNT +
      ((uint32_t)HCSR04_TIMEOUT_US * PlatformTime_CyclesPerUs());
  g_hcsr04_timeout_cycles = g_hcsr04_rise_timeout_cycles;
  g_hcsr04_state = HCSR04_WAIT_ECHO_RISE;
  __set_PRIMASK(primask);

  HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_SET);
  delay_us(10);
  HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);
}

uint8_t HCSR04_Async_IsRunnable(uint64_t now_us)
{
  HCSR04_State_t state = g_hcsr04_state;
  uint32_t current_cycles;

  (void)now_us;

  if ((state == HCSR04_COMPLETE) && (g_hcsr04_fall_event != 0U))
  {
    return 1U;
  }

  if ((state == HCSR04_WAIT_ECHO_RISE) ||
      (state == HCSR04_WAIT_ECHO_FALL))
  {
    current_cycles = DWT->CYCCNT;
    return ((int32_t)(current_cycles - g_hcsr04_timeout_cycles) >= 0);
  }

  return 0U;
}

uint8_t HCSR04_Async_Finalize(int *distance_cm)
{
  uint32_t primask = __get_PRIMASK();
  uint32_t rise_cycles;
  uint32_t fall_cycles;
  uint32_t duration_cycles;
  uint8_t rise_late;

  __disable_irq();
  if ((g_hcsr04_state != HCSR04_COMPLETE) ||
      (g_hcsr04_fall_event == 0U))
  {
    __set_PRIMASK(primask);
    return 0U;
  }

  rise_cycles = g_hcsr04_echo_rise_cycles;
  fall_cycles = g_hcsr04_echo_fall_cycles;
  rise_late = g_hcsr04_rise_late;
  g_hcsr04_state = HCSR04_IDLE;
  g_hcsr04_rise_event = 0U;
  g_hcsr04_fall_event = 0U;
  g_hcsr04_rise_late = 0U;
  __set_PRIMASK(primask);

  duration_cycles = fall_cycles - rise_cycles;
  *distance_cm = (rise_late != 0U)
      ? -1 : (int)((duration_cycles / PlatformTime_CyclesPerUs()) / 58U);
  g_hcsr04_timeout_cycles = 0U;
  g_hcsr04_rise_timeout_cycles = 0U;

  return 1U;
}

uint8_t HCSR04_Async_Timeout(int *distance_cm)
{
  uint32_t primask = __get_PRIMASK();
  uint32_t current_cycles;
  HCSR04_State_t state;
  int sensor_error;

  __disable_irq();
  state = g_hcsr04_state;

  if ((state != HCSR04_WAIT_ECHO_RISE) &&
      (state != HCSR04_WAIT_ECHO_FALL))
  {
    __set_PRIMASK(primask);
    return 0U;
  }

  current_cycles = DWT->CYCCNT;
  if ((int32_t)(current_cycles - g_hcsr04_timeout_cycles) < 0)
  {
    __set_PRIMASK(primask);
    return 0U;
  }

  sensor_error = (state == HCSR04_WAIT_ECHO_RISE) ? -1 : -2;
  if (g_hcsr04_rise_late != 0U)
  {
    sensor_error = -1;
  }

  *distance_cm = sensor_error;
  g_hcsr04_state = HCSR04_ERROR;
  g_hcsr04_rise_event = 0U;
  g_hcsr04_fall_event = 0U;
  g_hcsr04_rise_late = 0U;
  g_hcsr04_state = HCSR04_IDLE;
  __set_PRIMASK(primask);

  g_hcsr04_timeout_cycles = 0U;
  g_hcsr04_rise_timeout_cycles = 0U;
  return 1U;
}

void HCSR04_Async_OnExti(uint16_t gpio_pin)
{
  HCSR04_State_t state;
  uint32_t cycles;
  GPIO_PinState level;

  if (gpio_pin != HCSR04_ECHO_PIN)
  {
    return;
  }

  cycles = DWT->CYCCNT;
  level = HAL_GPIO_ReadPin(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN);
  state = g_hcsr04_state;

  if ((state == HCSR04_WAIT_ECHO_RISE) && (level == GPIO_PIN_SET))
  {
    g_hcsr04_echo_rise_cycles = cycles;
    g_hcsr04_rise_event = 1U;
    g_hcsr04_rise_late =
        ((int32_t)(cycles - g_hcsr04_rise_timeout_cycles) > 0) ? 1U : 0U;
    g_hcsr04_timeout_cycles = cycles +
        ((uint32_t)HCSR04_TIMEOUT_US * PlatformTime_CyclesPerUs());
    g_hcsr04_state = HCSR04_WAIT_ECHO_FALL;
  }
  else if ((state == HCSR04_WAIT_ECHO_FALL) && (level == GPIO_PIN_RESET))
  {
    g_hcsr04_echo_fall_cycles = cycles;
    g_hcsr04_fall_event = 1U;
    g_hcsr04_state = HCSR04_COMPLETE;
  }
}
