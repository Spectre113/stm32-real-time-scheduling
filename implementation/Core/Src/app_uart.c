#include "main.h"
#include "app_uart.h"

#include <string.h>

/* USART3 is generated and owned by CubeMX in main.c. */
extern UART_HandleTypeDef huart3;

void uart_print(const char *text)
{
  HAL_UART_Transmit(&huart3, (uint8_t *)text, strlen(text), HAL_MAX_DELAY);
}

void uart_print_u64(uint64_t value)
{
  char digits[21];
  uint32_t length = 0;

  do
  {
    digits[length++] = (char)('0' + (value % 10ULL));
    value /= 10ULL;
  } while (value > 0ULL);

  for (uint32_t i = 0; i < length / 2U; i++)
  {
    char digit = digits[i];
    digits[i] = digits[length - 1U - i];
    digits[length - 1U - i] = digit;
  }

  digits[length] = '\0';
  uart_print(digits);
}

void uart_print_percent_x10000(uint64_t value_us, uint64_t total_us)
{
  uint64_t percent_x10000 = (value_us * 1000000ULL) / total_us;
  uint32_t fraction = (uint32_t)(percent_x10000 % 10000ULL);
  char fraction_text[6];

  uart_print_u64(percent_x10000 / 10000ULL);
  fraction_text[0] = '.';
  fraction_text[1] = (char)('0' + (fraction / 1000U));
  fraction_text[2] = (char)('0' + ((fraction / 100U) % 10U));
  fraction_text[3] = (char)('0' + ((fraction / 10U) % 10U));
  fraction_text[4] = (char)('0' + (fraction % 10U));
  fraction_text[5] = '\0';
  uart_print(fraction_text);
}
