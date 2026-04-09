from machine import Pin, PWM
from utime import sleep

# Servo signal wire on GP16
SERVO_PIN = 16

servo = PWM(Pin(SERVO_PIN))
servo.freq(50)

MIN_DUTY_PERCENT = 2.5
MAX_DUTY_PERCENT = 11.0

def set_servo_angle(angle):
    if angle < 0:
        angle = 0
    if angle > 180:
        angle = 180

    duty_percent = MIN_DUTY_PERCENT + (angle / 180.0) * (MAX_DUTY_PERCENT - MIN_DUTY_PERCENT)
    duty_u16 = int((duty_percent / 100.0) * 65535)
    servo.duty_u16(duty_u16)

while True:
    for angle in range(0, 181, 4):
        set_servo_angle(angle)
        sleep(0.02)

    sleep(0.15)

    for angle in range(180, -1, -4):
        set_servo_angle(angle)
        sleep(0.02)

    sleep(0.15)