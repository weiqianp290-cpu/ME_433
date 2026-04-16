#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/time.h"

// =========================
// I2C pins and addresses
// =========================
#define I2C_PORT        i2c0
#define I2C_SDA_PIN     4
#define I2C_SCL_PIN     5
#define I2C_BAUDRATE    400000

#define OLED_ADDR       0x3C
#define MPU6050_ADDR    0x68

// =========================
// OLED definitions
// SSD1306 128x64
// =========================
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_BUF_SIZE   (OLED_WIDTH * OLED_HEIGHT / 8)

static uint8_t oled_buf[OLED_BUF_SIZE];

// =========================
// MPU6050 registers
// =========================
#define CONFIG          0x1A
#define GYRO_CONFIG     0x1B
#define ACCEL_CONFIG    0x1C
#define PWR_MGMT_1      0x6B
#define PWR_MGMT_2      0x6C

#define ACCEL_XOUT_H    0x3B
#define WHO_AM_I        0x75

typedef struct {
    int16_t ax_raw;
    int16_t ay_raw;
    int16_t az_raw;
    int16_t temp_raw;
    int16_t gx_raw;
    int16_t gy_raw;
    int16_t gz_raw;

    float ax_g;
    float ay_g;
    float az_g;
    float temp_c;
    float gx_dps;
    float gy_dps;
    float gz_dps;
} mpu6050_data_t;

// ======================================================
// Helpers
// ======================================================
static inline int clamp_int(int v, int min_v, int max_v) {
    if (v < min_v) return min_v;
    if (v > max_v) return max_v;
    return v;
}

// ======================================================
// OLED low-level
// ======================================================
void oled_write_cmd(uint8_t cmd) {
    uint8_t buf[2];
    buf[0] = 0x00;
    buf[1] = cmd;
    i2c_write_blocking(I2C_PORT, OLED_ADDR, buf, 2, false);
}

void oled_write_data(const uint8_t *data, size_t len) {
    uint8_t temp[17];
    temp[0] = 0x40;

    while (len > 0) {
        size_t chunk = (len > 16) ? 16 : len;
        memcpy(&temp[1], data, chunk);
        i2c_write_blocking(I2C_PORT, OLED_ADDR, temp, chunk + 1, false);
        data += chunk;
        len -= chunk;
    }
}

void oled_init(void) {
    sleep_ms(100);

    oled_write_cmd(0xAE); // display off
    oled_write_cmd(0x20); // memory mode
    oled_write_cmd(0x00); // horizontal

    oled_write_cmd(0xB0);
    oled_write_cmd(0xC8);
    oled_write_cmd(0x00);
    oled_write_cmd(0x10);

    oled_write_cmd(0x40);
    oled_write_cmd(0x81);
    oled_write_cmd(0x7F);

    oled_write_cmd(0xA1);
    oled_write_cmd(0xA6);
    oled_write_cmd(0xA8);
    oled_write_cmd(0x3F);

    oled_write_cmd(0xA4);
    oled_write_cmd(0xD3);
    oled_write_cmd(0x00);

    oled_write_cmd(0xD5);
    oled_write_cmd(0x80);

    oled_write_cmd(0xD9);
    oled_write_cmd(0xF1);

    oled_write_cmd(0xDA);
    oled_write_cmd(0x12);

    oled_write_cmd(0xDB);
    oled_write_cmd(0x40);

    oled_write_cmd(0x8D);
    oled_write_cmd(0x14);

    oled_write_cmd(0xAF); // display on

    memset(oled_buf, 0, sizeof(oled_buf));
}

void oled_show(void) {
    oled_write_cmd(0x21);
    oled_write_cmd(0x00);
    oled_write_cmd(0x7F);

    oled_write_cmd(0x22);
    oled_write_cmd(0x00);
    oled_write_cmd(0x07);

    oled_write_data(oled_buf, sizeof(oled_buf));
}

void oled_clear(void) {
    memset(oled_buf, 0, sizeof(oled_buf));
}

void oled_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;

    int index = x + (y / 8) * OLED_WIDTH;
    uint8_t mask = 1 << (y % 8);

    if (on) {
        oled_buf[index] |= mask;
    } else {
        oled_buf[index] &= ~mask;
    }
}

void oled_draw_line(int x0, int y0, int x1, int y1, bool on) {
    int dx = abs(x1 - x0);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (true) {
        oled_set_pixel(x0, y0, on);
        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Draw a thicker line so it looks more solid on the OLED
void oled_draw_thick_line(int x0, int y0, int x1, int y1) {
    oled_draw_line(x0, y0, x1, y1, true);
    oled_draw_line(x0 + 1, y0, x1 + 1, y1, true);
    oled_draw_line(x0 - 1, y0, x1 - 1, y1, true);
    oled_draw_line(x0, y0 + 1, x1, y1 + 1, true);
    oled_draw_line(x0, y0 - 1, x1, y1 - 1, true);
}

// Draw a real visible center cross + small square
void oled_draw_center_marker(int cx, int cy) {
    // horizontal arm
    for (int x = cx - 5; x <= cx + 5; x++) {
        oled_set_pixel(x, cy, true);
    }

    // vertical arm
    for (int y = cy - 5; y <= cy + 5; y++) {
        oled_set_pixel(cx, y, true);
    }

    // tiny 3x3 square in center so it never looks like just one thin line
    for (int x = cx - 1; x <= cx + 1; x++) {
        for (int y = cy - 1; y <= cy + 1; y++) {
            oled_set_pixel(x, y, true);
        }
    }
}

// ======================================================
// MPU6050
// ======================================================
bool mpu_write_reg(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    int ret = i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf, 2, false);
    return ret == 2;
}

bool mpu_read_reg(uint8_t reg, uint8_t *value) {
    int ret1 = i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true);
    if (ret1 != 1) return false;

    int ret2 = i2c_read_blocking(I2C_PORT, MPU6050_ADDR, value, 1, false);
    return ret2 == 1;
}

bool mpu_read_bytes(uint8_t start_reg, uint8_t *buf, size_t len) {
    int ret1 = i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &start_reg, 1, true);
    if (ret1 != 1) return false;

    int ret2 = i2c_read_blocking(I2C_PORT, MPU6050_ADDR, buf, len, false);
    return ret2 == (int)len;
}

bool mpu_check_who_am_i(uint8_t *who) {
    return mpu_read_reg(WHO_AM_I, who);
}

bool mpu_init(void) {
    if (!mpu_write_reg(PWR_MGMT_1, 0x00)) return false;
    sleep_ms(10);

    if (!mpu_write_reg(PWR_MGMT_2, 0x00)) return false;
    sleep_ms(10);

    // accel ±2g
    if (!mpu_write_reg(ACCEL_CONFIG, 0x00)) return false;
    sleep_ms(10);

    // gyro ±2000 dps
    if (!mpu_write_reg(GYRO_CONFIG, 0x18)) return false;
    sleep_ms(10);

    // low-pass filter
    if (!mpu_write_reg(CONFIG, 0x03)) return false;
    sleep_ms(10);

    return true;
}

bool mpu_read_all(mpu6050_data_t *d) {
    uint8_t buf[14];

    if (!mpu_read_bytes(ACCEL_XOUT_H, buf, 14)) return false;

    d->ax_raw   = (int16_t)((buf[0]  << 8) | buf[1]);
    d->ay_raw   = (int16_t)((buf[2]  << 8) | buf[3]);
    d->az_raw   = (int16_t)((buf[4]  << 8) | buf[5]);
    d->temp_raw = (int16_t)((buf[6]  << 8) | buf[7]);
    d->gx_raw   = (int16_t)((buf[8]  << 8) | buf[9]);
    d->gy_raw   = (int16_t)((buf[10] << 8) | buf[11]);
    d->gz_raw   = (int16_t)((buf[12] << 8) | buf[13]);

    d->ax_g = d->ax_raw * 0.000061f;
    d->ay_g = d->ay_raw * 0.000061f;
    d->az_g = d->az_raw * 0.000061f;

    d->gx_dps = d->gx_raw * 0.007630f;
    d->gy_dps = d->gy_raw * 0.007630f;
    d->gz_dps = d->gz_raw * 0.007630f;

    d->temp_c = d->temp_raw / 340.0f + 36.53f;

    return true;
}

// ======================================================
// Drawing
// ======================================================
void draw_gravity_line(const mpu6050_data_t *d) {
    int cx = OLED_WIDTH / 2;
    int cy = OLED_HEIGHT / 2;

    oled_clear();

    // Longer line
    int dx = d->ax_raw / 700;
    int dy = d->ay_raw / 700;

    dx = clamp_int(dx, -50, 50);
    dy = clamp_int(dy, -26, 26);

    // Current "natural" direction version
    int x1 = cx - dx;
    int y1 = cy + dy;

    x1 = clamp_int(x1, 0, OLED_WIDTH - 1);
    y1 = clamp_int(y1, 0, OLED_HEIGHT - 1);

    oled_draw_center_marker(cx, cy);
    oled_draw_thick_line(cx, cy, x1, y1);

    oled_show();
}

// ======================================================
// Main
// ======================================================
int main() {
    stdio_init_all();
    sleep_ms(2000);

    i2c_init(I2C_PORT, I2C_BAUDRATE);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    printf("HW5 start\n");

    oled_init();
    oled_clear();
    oled_show();

    uint8_t who = 0;
    if (!mpu_check_who_am_i(&who)) {
        printf("ERROR: could not read WHO_AM_I\n");
        while (1) sleep_ms(500);
    }

    printf("WHO_AM_I = 0x%02X\n", who);

    if (!(who == 0x68 || who == 0x98)) {
        printf("ERROR: unexpected WHO_AM_I value\n");
        while (1) sleep_ms(500);
    }

    if (!mpu_init()) {
        printf("ERROR: MPU6050 init failed\n");
        while (1) sleep_ms(500);
    }

    printf("MPU6050 initialized successfully\n");

    mpu6050_data_t data;

    absolute_time_t last_imu_time = get_absolute_time();
    absolute_time_t last_oled_time = get_absolute_time();
    absolute_time_t last_print_time = get_absolute_time();

    while (1) {
        // IMU at ~100 Hz
        if (absolute_time_diff_us(last_imu_time, get_absolute_time()) >= 10000) {
            last_imu_time = get_absolute_time();
            mpu_read_all(&data);
        }

        // OLED at ~30 Hz for smoother/stabler display
        if (absolute_time_diff_us(last_oled_time, get_absolute_time()) >= 33000) {
            last_oled_time = get_absolute_time();
            draw_gravity_line(&data);
        }

        // Serial print at ~10 Hz so terminal is readable
        if (absolute_time_diff_us(last_print_time, get_absolute_time()) >= 100000) {
            last_print_time = get_absolute_time();

            printf("AX:%6d AY:%6d AZ:%6d | "
                   "GX:%6d GY:%6d GZ:%6d | "
                   "T: %.2f C\n",
                   data.ax_raw, data.ay_raw, data.az_raw,
                   data.gx_raw, data.gy_raw, data.gz_raw,
                   data.temp_c);
        }

        sleep_ms(1);
    }

    return 0;
}