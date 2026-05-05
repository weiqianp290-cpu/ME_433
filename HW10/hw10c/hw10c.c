#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// ===================== Pin definitions =====================
#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

#define BUTTON_PIN 15

// MPU6050 default I2C address
#define MPU6050_ADDR 0x68

// MPU6050 registers
#define MPU6050_WHO_AM_I     0x75
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B

// ===================== I2C helper functions =====================
static void mpu6050_write_reg(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, data, 2, false);
}

static uint8_t mpu6050_read_reg(uint8_t reg) {
    uint8_t value = 0;

    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU6050_ADDR, &value, 1, false);

    return value;
}

static int16_t mpu6050_read_16(uint8_t high_reg) {
    uint8_t high = mpu6050_read_reg(high_reg);
    uint8_t low = mpu6050_read_reg(high_reg + 1);

    return (int16_t)((high << 8) | low);
}

static void mpu6050_init() {
    // Wake up MPU6050.
    // By default, MPU6050 starts in sleep mode.
    mpu6050_write_reg(MPU6050_PWR_MGMT_1, 0x00);
    sleep_ms(100);
}

int main() {
    stdio_init_all();

    // Wait for USB serial connection
    sleep_ms(2000);

    // ===================== I2C setup =====================
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);

    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // ===================== Button setup =====================
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // ===================== MPU6050 setup =====================
    uint8_t who = mpu6050_read_reg(MPU6050_WHO_AM_I);
    printf("MPU6050 WHO_AM_I = 0x%02X\n", who);

    mpu6050_init();

    while (true) {
        int16_t ax = mpu6050_read_16(MPU6050_ACCEL_XOUT_H);
        int16_t ay = mpu6050_read_16(MPU6050_ACCEL_XOUT_H + 2);
        int16_t az = mpu6050_read_16(MPU6050_ACCEL_XOUT_H + 4);

        // Internal pull-up:
        // not pressed = 1
        // pressed = 0
        // Convert to:
        // not pressed = 0
        // pressed = 1
        int button_pressed = !gpio_get(BUTTON_PIN);

        // Send CSV data to Python:
        // ax,ay,az,button
        printf("%d,%d,%d,%d\n", ax, ay, az, button_pressed);

        sleep_ms(20); // about 50 Hz
    }

    return 0;
}