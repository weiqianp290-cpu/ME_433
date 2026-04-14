import time
import pwmio
from board import GP16

servo = pwmio.PWMOut(GP16, frequency=50, duty_cycle=0)

MIN_DUTY_PERCENT = 2.5
MAX_DUTY_PERCENT = 11.0

def set_servo_angle(angle_deg):
    if angle_deg < 0:
        angle_deg = 0
    if angle_deg > 180:
        angle_deg = 180

    duty_percent = MIN_DUTY_PERCENT + (angle_deg / 180.0) * (MAX_DUTY_PERCENT - MIN_DUTY_PERCENT)
    duty_u16 = int((duty_percent / 100.0) * 65535)
    servo.duty_cycle = duty_u16

while True:
    for angle in range(0, 181, 4):
        set_servo_angle(angle)
        time.sleep(0.02)

    time.sleep(0.15)

    for angle in range(180, -1, -4):
        set_servo_angle(angle)
        time.sleep(0.02)

    time.sleep(0.15) 