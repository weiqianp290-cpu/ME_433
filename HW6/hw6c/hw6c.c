#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "tusb.h"
#include "usb_descriptors.h"

// =========================
// Pin definitions
// =========================
#define I2C_PORT        i2c0
#define I2C_SDA_PIN     4
#define I2C_SCL_PIN     5

#define BUTTON_PIN      14
#define MODE_LED_PIN    15

// =========================
// MPU6050 definitions
// =========================
#define MPU6050_ADDR         0x68
#define MPU6050_REG_PWR1     0x6B
#define MPU6050_REG_ACCEL    0x3B
#define MPU6050_REG_CONFIG   0x1A
#define MPU6050_REG_ACCELCFG 0x1C

// =========================
// Mode definitions
// =========================
typedef enum {
    MODE_IMU = 0,
    MODE_CIRCLE = 1
} mouse_mode_t;

static volatile mouse_mode_t current_mode = MODE_IMU;

// =========================
// Direction tuning
// regular mode direction fixed
// =========================
#define X_SIGN  -1
#define Y_SIGN   1

// =========================
// Button debounce
// =========================
static bool last_button_level = true;
static absolute_time_t last_debounce_time;

// =========================
// Circle mode state
// =========================
static float circle_theta = 0.0f;

// =========================
// MPU6050 low-level helpers
// =========================
static void mpu6050_write_reg(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf, 2, false);
}

static void mpu6050_read_bytes(uint8_t reg, uint8_t *buf, size_t len) {
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU6050_ADDR, buf, len, false);
}

static void mpu6050_init(void) {
    sleep_ms(100);
    mpu6050_write_reg(MPU6050_REG_PWR1, 0x00);      // wake up
    sleep_ms(10);
    mpu6050_write_reg(MPU6050_REG_CONFIG, 0x03);    // low-pass filter
    mpu6050_write_reg(MPU6050_REG_ACCELCFG, 0x00);  // +/-2g
    sleep_ms(10);
}

static void mpu6050_read_accel(int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t data[6];
    mpu6050_read_bytes(MPU6050_REG_ACCEL, data, 6);

    *ax = (int16_t)((data[0] << 8) | data[1]);
    *ay = (int16_t)((data[2] << 8) | data[3]);
    *az = (int16_t)((data[4] << 8) | data[5]);
}

// =========================
// Convert accel to discrete mouse speed
// =========================
static int8_t accel_to_mouse_delta(int16_t a) {
    int16_t mag = (a >= 0) ? a : -a;
    int8_t out = 0;

    if (mag < 2500) {
        out = 0;
    } else if (mag < 5000) {
        out = 1;
    } else if (mag < 9000) {
        out = 3;
    } else if (mag < 13000) {
        out = 5;
    } else {
        out = 8;
    }

    return (a >= 0) ? out : -out;
}

// =========================
// Button handling
// GP14 ---- button ---- GND
// internal pull-up enabled
//
// not pressed = 1
// pressed     = 0
// =========================
static void update_button_and_mode(void) {
    bool level = gpio_get(BUTTON_PIN);

    if (level != last_button_level) {
        last_debounce_time = get_absolute_time();
    }

    if (absolute_time_diff_us(last_debounce_time, get_absolute_time()) > 30000) {
        static bool stable_level = true;

        if (level != stable_level) {
            stable_level = level;

            // falling edge = button pressed
            if (stable_level == false) {
                current_mode = (current_mode == MODE_IMU) ? MODE_CIRCLE : MODE_IMU;
            }
        }
    }

    last_button_level = level;
}

// =========================
// LED indication
// regular mode  -> solid ON
// remote mode   -> fast blink
// =========================
static void update_mode_led(void) {
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    if (current_mode == MODE_IMU) {
        // regular mode: solid on
        gpio_put(MODE_LED_PIN, 1);
    } else {
        // remote mode: fast blinking
        // toggle every 80 ms
        bool blink_state = ((now_ms / 80) % 2) == 0;
        gpio_put(MODE_LED_PIN, blink_state);
    }
}

// =========================
// Get mouse movement from IMU
// X acceleration -> mouse X
// Y acceleration -> mouse Y
// =========================
static void get_imu_mouse_delta(int8_t *dx, int8_t *dy) {
    int16_t ax, ay, az;
    (void)az;

    mpu6050_read_accel(&ax, &ay, &az);

    int8_t mx = accel_to_mouse_delta(ax);
    int8_t my = accel_to_mouse_delta(ay);

    *dx = (int8_t)(X_SIGN * mx);
    *dy = (int8_t)(Y_SIGN * my);
}

// =========================
// Get circle motion delta
// larger and slower circle
// =========================
static void get_circle_mouse_delta(int8_t *dx, int8_t *dy) {
    float radius = 5.0f;
    *dx = (int8_t)lroundf(radius * cosf(circle_theta));
    *dy = (int8_t)lroundf(radius * sinf(circle_theta));

    circle_theta += 0.08f;
    if (circle_theta > 2.0f * (float)M_PI) {
        circle_theta -= 2.0f * (float)M_PI;
    }
}

// =========================
// USB HID mouse task
// send report about every 10 ms (~100 Hz)
// =========================
static void hid_task(void) {
    static uint32_t start_ms = 0;
    const uint32_t interval_ms = 10;

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if (now_ms - start_ms < interval_ms) {
        return;
    }
    start_ms = now_ms;

    update_button_and_mode();
    update_mode_led();

    if (!tud_mounted()) {
        return;
    }
    if (!tud_hid_ready()) {
        return;
    }

    int8_t dx = 0;
    int8_t dy = 0;

    if (current_mode == MODE_IMU) {
        get_imu_mouse_delta(&dx, &dy);
    } else {
        get_circle_mouse_delta(&dx, &dy);
    }

    // buttons = 0, wheel = 0, pan = 0
    tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, dx, dy, 0, 0);
}

// =========================
// Main
// =========================
int main(void) {
    stdio_init_all();
    tusb_init();

    // LED output
    gpio_init(MODE_LED_PIN);
    gpio_set_dir(MODE_LED_PIN, GPIO_OUT);
    gpio_put(MODE_LED_PIN, 1);   // start in regular mode -> solid on

    // Button input with internal pull-up
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    last_button_level = gpio_get(BUTTON_PIN);
    last_debounce_time = get_absolute_time();

    // I2C init for MPU6050
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    sleep_ms(200);
    mpu6050_init();

    while (1) {
        tud_task();
        hid_task();
    }

    return 0;
}