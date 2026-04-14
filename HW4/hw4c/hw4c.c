#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "font.h"

#define I2C_PORT i2c0
#define I2C_SDA  4
#define I2C_SCL  5
#define LED_PIN  16

int main() {
    stdio_init_all();

    // External LED on GP16
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // I2C init for OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    sleep_ms(250);

    // OLED init
    ssd1306_setup();

    while (true) {
        // LED on + pixel on
        gpio_put(LED_PIN, 1);
        ssd1306_clear();
        ssd1306_drawPixel(10, 10, 1);
        ssd1306_update();
        sleep_ms(500);

        // LED off + pixel off
        gpio_put(LED_PIN, 0);
        ssd1306_clear();
        ssd1306_update();
        sleep_ms(500);
    }

    return 0;
}