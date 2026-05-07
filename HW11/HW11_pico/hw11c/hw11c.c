#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

// UART0 on Pico
#define UART_ID uart0
#define UART_BAUD 115200

// Pico UART0 pins
#define UART_TX_PIN 0   // GP0, connect to STM32 PA1 RX
#define UART_RX_PIN 1   // GP1, connect to STM32 PA0 TX

int main() {
    stdio_init_all();

    // Initialize UART0 for STM32 communication
    uart_init(UART_ID, UART_BAUD);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    sleep_ms(2000);
    printf("Pico HW11 UART bridge started\r\n");
    printf("USB -> Pico -> STM32, and STM32 -> Pico -> USB\r\n");

    while (true) {
        // 1. If computer sends a character to Pico USB serial,
        // send it out to STM32 through UART0 TX.
        int ch = getchar_timeout_us(0);
        if (ch != PICO_ERROR_TIMEOUT) {
            char c = (char)ch;
            uart_putc_raw(UART_ID, c);
            printf("Sent to STM32: %c\r\n", c);
        }

        // 2. If STM32 sends a character to Pico UART0 RX,
        // print it to the computer USB serial.
        if (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);
            printf("From STM32: %c\r\n", c);
        }

        sleep_ms(1);
    }
}