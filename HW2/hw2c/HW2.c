#include "pico/stdlib.h"
#include "hardware/pwm.h"

// HW2: RC Servo in C....
// Servo signal wire is connected to GP16.
#define SERVO_PIN 16

// 50 Hz servo PWM:
// 125 MHz system clock / 125 = 1 MHz PWM clock
// 1 MHz clock with wrap 19999 gives a 20 ms period (50 Hz)
#define SERVO_CLKDIV 125.0f
#define SERVO_WRAP 19999

// Keep a reasonably safe duty range first
#define SERVO_MIN_DUTY 2.5f
#define SERVO_MAX_DUTY 11.0f

static void servo_pwm_init(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_clkdiv(slice, SERVO_CLKDIV);
    pwm_set_wrap(slice, SERVO_WRAP);
    pwm_set_enabled(slice, true);
}

// Set the angle of the RC servo using PWM
static void servo_set_angle(uint pin, float angle_deg) {
    if (angle_deg < 0.0f) {
        angle_deg = 0.0f;
    }
    if (angle_deg > 180.0f) {
        angle_deg = 180.0f;
    }

    float duty_percent = SERVO_MIN_DUTY +
                         (angle_deg / 180.0f) * (SERVO_MAX_DUTY - SERVO_MIN_DUTY);

    uint16_t level = (uint16_t)((duty_percent / 100.0f) * (SERVO_WRAP + 1));
    pwm_set_gpio_level(pin, level);
}

int main(void) {
    servo_pwm_init(SERVO_PIN);

    while (true) {
        // 0 -> 180
        for (float angle = 0.0f; angle <= 180.0f; angle += 4.0f) {
            servo_set_angle(SERVO_PIN, angle);
            sleep_ms(20);
        }

        sleep_ms(150);

        // 180 -> 0
        for (float angle = 180.0f; angle >= 0.0f; angle -= 4.0f) {
            servo_set_angle(SERVO_PIN, angle);
            sleep_ms(20);
        }

        sleep_ms(150);
    }
}