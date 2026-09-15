#ifndef APP_UART_H
#define APP_UART_H

#include <stdint.h>

void uart_print(const char *text);
void uart_print_u64(uint64_t value);
void uart_print_percent_x10000(uint64_t value_us, uint64_t total_us);

#endif /* APP_UART_H */
