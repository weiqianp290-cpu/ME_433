"""
HW16 - Send 'a' to the Nucleo, collect the current-control data, and plot it
to tune kp/ki.

After each 'a', the STM32 prints 400 lines:  index  desired(mA)  actual(mA)
then prints 'done'.

Usage:
    1. Set PORT below to your serial port (mac: /dev/tty.usbmodemXXXX)
    2. pip install pyserial matplotlib
    3. python plot_current.py
"""

import serial
import matplotlib.pyplot as plt

# ---- edit this ----
PORT = "/dev/tty.usbmodem1102"   # mac: run 'ls /dev/tty.usbmodem*' to find it
BAUD = 115200
NSAMP = 400
# -------------------

ser = serial.Serial(PORT, BAUD, timeout=2)

# send 'a'
ser.reset_input_buffer()
ser.write(b'a')

index, desired, actual = [], [], []

while True:
    line = ser.readline().decode(errors='ignore').strip()
    if not line:
        continue
    if line.startswith("done"):
        break
    parts = line.split()
    if len(parts) == 3:
        try:
            i, d, a = int(parts[0]), int(parts[1]), int(parts[2])
        except ValueError:
            continue
        index.append(i)
        desired.append(d)
        actual.append(a)
        if len(index) >= NSAMP:
            break

ser.close()

print(f"Received {len(index)} points")

plt.figure(figsize=(10, 5))
plt.plot(index, desired, label="desired (mA)", linewidth=2)
plt.plot(index, actual, label="actual (mA)", linewidth=1)
plt.xlabel("sample (1 kHz)")
plt.ylabel("current (mA)")
plt.title("HW16 Current Control (PI)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("current_control.png", dpi=120)
plt.show()
