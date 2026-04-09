#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT        i2c0
#define I2C_SDA_PIN     4
#define I2C_SCL_PIN     5
#define I2C_BAUDRATE    100000

#define HEARTBEAT_PIN   16    // LED directly connected to Pico GP15

#define MCP23008_ADDR   0x20  // A0, A1, A2 all tied to GND

#define IODIR_REG       0x00
#define GPIO_REG        0x09
#define OLAT_REG        0x0A

#define BUTTON_MASK     0x01  // GP0
#define LED_MASK        0x80  // GP7

void mcp_write_reg(uint8_t reg, uint8_t value) {
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = value;
    i2c_write_blocking(I2C_PORT, MCP23008_ADDR, buf, 2, false);
}

uint8_t mcp_read_reg(uint8_t reg) {
    uint8_t value;
    i2c_write_blocking(I2C_PORT, MCP23008_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MCP23008_ADDR, &value, 1, false);
    return value;
}

int main() {
    // heartbeat LED on Pico
    gpio_init(HEARTBEAT_PIN);
    gpio_set_dir(HEARTBEAT_PIN, GPIO_OUT);
    gpio_put(HEARTBEAT_PIN, 0);

    // I2C setup
    i2c_init(I2C_PORT, I2C_BAUDRATE);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    sleep_ms(50);

    // MCP23008:
    // GP0 input, GP7 output, others input
    // 0x7F = 0111 1111
    mcp_write_reg(IODIR_REG, 0x7F);

    // start with GP7 low
    mcp_write_reg(OLAT_REG, 0x00);

    bool hb_state = false;
    uint32_t count = 0;

    while (true) {
        uint8_t gpio_value = mcp_read_reg(GPIO_REG);

        // pull-up button:
        // not pressed = 1
        // pressed = 0
        if ((gpio_value & BUTTON_MASK) == 0) {
            mcp_write_reg(OLAT_REG, LED_MASK);   // turn on GP7 LED
        } else {
            mcp_write_reg(OLAT_REG, 0x00);       // turn off GP7 LED
        }

        // heartbeat LED blink
        count++;
        if (count >= 10) {
            hb_state = !hb_state;
            gpio_put(HEARTBEAT_PIN, hb_state);
            count = 0;
        }

        sleep_ms(25);
    }

    return 0;
}