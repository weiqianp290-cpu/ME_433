#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "pico/time.h"
#include "ssd1306.h"
#include "font.h"

#define I2C_PORT i2c0
#define I2C_SDA  4
#define I2C_SCL  5
#define LED_PIN  16

#define ADC_INPUT 0       // ADC0
#define ADC_PIN   26      // GP26 is ADC0

// Draw one ASCII character at (x, y)
// Each character is 5x8 pixels
void drawChar(int x, int y, char c) {
    if (c < 0x20 || c > 0x7F) {
        return;
    }

    int index = c - 0x20;

    for (int col = 0; col < 5; col++) {
        unsigned char line = ASCII[index][col];

        for (int row = 0; row < 8; row++) {
            if ((line >> row) & 0x01) {
                ssd1306_drawPixel(x + col, y + row, 1);
            } else {
                ssd1306_drawPixel(x + col, y + row, 0);
            }
        }
    }
}

// Draw a null-terminated string starting at (x, y)
void drawMessage(int x, int y, char *m) {
    int cursor_x = x;

    while (*m != '\0') {
        drawChar(cursor_x, y, *m);
        cursor_x += 6;   // 5 pixels wide + 1 pixel spacing
        m++;
    }
}

int main() {
    stdio_init_all();

    // -------------------------
    // External LED on GP16
    // -------------------------
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // -------------------------
    // I2C init for OLED
    // -------------------------
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    sleep_ms(250);

    // -------------------------
    // OLED init
    // -------------------------
    ssd1306_setup();

    // -------------------------
    // ADC0 init
    // -------------------------
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_INPUT);

    char msg_voltage[32];
    char msg_fps[32];

    while (true) {
        uint32_t t_start = to_us_since_boot(get_absolute_time());

        // Read ADC0 and convert to volts
        uint16_t raw = adc_read();
        float voltage = raw * 3.3f / 4095.0f;

        // Heartbeat state: toggle every 500 ms
        uint32_t t_now = to_us_since_boot(get_absolute_time());
        bool blink_on = ((t_now / 500000) % 2) == 0;

        gpio_put(LED_PIN, blink_on ? 1 : 0);

        ssd1306_clear();

        // Keep requirement 1 visible: one blinking pixel
        if (blink_on) {
            ssd1306_drawPixel(120, 0, 1);
        }

        // Requirement 2 + 3: draw text using your own functions
        sprintf(msg_voltage, "ADC0 = %.2f V", voltage);
        drawMessage(0, 0, msg_voltage);

        // Update once first so display time is included in FPS calculation
        ssd1306_update();

        uint32_t t_end = to_us_since_boot(get_absolute_time());
        uint32_t dt = t_end - t_start;

        float fps = 0.0f;
        if (dt > 0) {
            fps = 1000000.0f / (float)dt;
        }

        // Draw again with fps included
        ssd1306_clear();

        if (blink_on) {
            ssd1306_drawPixel(120, 0, 1);
        }

        sprintf(msg_voltage, "ADC0 = %.2f V", voltage);
        sprintf(msg_fps, "fps = %.1f", fps);

        drawMessage(0, 0, msg_voltage);
        drawMessage(0, 24, msg_fps);

        ssd1306_update();

        sleep_ms(50);
    }

    return 0;
}