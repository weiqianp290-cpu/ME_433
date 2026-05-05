"""
HW10 - IMU Letter Hunt (pygame zero)
====================================
Tilt the MPU6050 to move a crosshair cursor on screen.
A letter "A" appears at a random position. When the cursor is over the
letter, press the button on the Pico (GP15) to "click" it. The letter
then respawns at a new random position.

Pico firmware (hw10c.c) streams CSV at ~50 Hz over USB serial:
    ax,ay,az,button\\n

How to run from VSCode
----------------------
1. pip install pgzero pyserial
2. Plug in the Pico (it must already be running hw10c).
3. Make sure no other program (e.g. a serial monitor) is holding the port.
4. (Optional) override SERIAL_PORT below; otherwise we auto-detect.
5. Press Run in VSCode. The window opens; tilt the IMU to move; press
   the Pico button to fire.

Controls
--------
- Tilt IMU            : move the cursor
- Pico button (GP15)  : click on the letter under the cursor
- SPACE on keyboard   : recalibrate the IMU zero (hold the IMU level)
- ESC                 : quit
"""

import math
import random
import threading
import time

import pgzrun
import serial
import serial.tools.list_ports

# ===== Window =====
WIDTH = 800
HEIGHT = 600
TITLE = "HW10 - IMU Letter Hunt"

# ===== Serial =====
# Set to a string like "/dev/tty.usbmodem101" or "COM5" to override.
# Leave as None to auto-detect the Pico.
SERIAL_PORT = None
BAUD = 115200

# ===== IMU control =====
DEADZONE = 1500        # raw IMU units below this are ignored (anti-jitter)
SENSITIVITY = 0.06     # cursor px/sec per IMU unit
MAX_SPEED = 900        # cap on cursor speed (px/sec)
INVERT_X = False       # flip if cursor moves the wrong way left/right
INVERT_Y = False       # flip if cursor moves the wrong way up/down
CALIBRATE_SECONDS = 1.0  # how long to average IMU at startup as zero

# ===== Game =====
CURSOR_RADIUS = 10
HIT_RADIUS = 30        # how close cursor center must be to letter center
LETTER = "A"           # the letter to chase

# ===========================================================================
# Serial reader (background thread)
# ===========================================================================
_state_lock = threading.Lock()
_imu = {"ax": 0, "ay": 0, "az": 0, "button": 0, "ok": False}
_offset = {"ax": 0.0, "ay": 0.0}


def find_pico_port():
    """Best-effort auto-detect the Pico's USB serial port."""
    ports = list(serial.tools.list_ports.comports())
    # Match by Raspberry Pi Foundation VID
    for p in ports:
        if getattr(p, "vid", None) == 0x2E8A:
            return p.device
    # Fall back on description / device-name heuristics
    for p in ports:
        desc = (p.description or "").lower()
        dev = (p.device or "").lower()
        if "pico" in desc or "usbmodem" in dev or "ttyacm" in dev:
            return p.device
    return ports[0].device if ports else None


def serial_reader(port):
    try:
        ser = serial.Serial(port, BAUD, timeout=0.1)
        print(f"[serial] opened {port} @ {BAUD}")
    except Exception as e:
        print(f"[serial] could not open {port}: {e}")
        return
    ser.reset_input_buffer()
    while True:
        try:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if not line:
                continue
            parts = line.split(",")
            if len(parts) != 4:
                continue
            ax = int(parts[0]); ay = int(parts[1])
            az = int(parts[2]); btn = int(parts[3])
            with _state_lock:
                _imu["ax"] = ax
                _imu["ay"] = ay
                _imu["az"] = az
                _imu["button"] = btn
                _imu["ok"] = True
        except Exception:
            # bad line / decode error - just skip
            continue


_port = SERIAL_PORT or find_pico_port()
if _port is None:
    print("[serial] No serial port found. Plug in the Pico and re-run.")
else:
    threading.Thread(target=serial_reader, args=(_port,), daemon=True).start()


# ===========================================================================
# Game state
# ===========================================================================
cursor_x = WIDTH / 2
cursor_y = HEIGHT / 2
target_x = WIDTH / 2
target_y = HEIGHT / 2

score = 0
misses = 0
prev_button = 0
last_hit_time = -10.0
last_miss_time = -10.0

calib_samples = []
calibrating_until = time.time() + CALIBRATE_SECONDS


def new_target():
    global target_x, target_y
    margin = 70
    nx = random.randint(margin, WIDTH - margin)
    ny = random.randint(margin, HEIGHT - margin)
    # avoid spawning right under the cursor
    while math.hypot(nx - cursor_x, ny - cursor_y) < 120:
        nx = random.randint(margin, WIDTH - margin)
        ny = random.randint(margin, HEIGHT - margin)
    target_x, target_y = nx, ny


new_target()


def _deadzone(v):
    if abs(v) < DEADZONE:
        return 0.0
    return v - (DEADZONE if v > 0 else -DEADZONE)


# ===========================================================================
# pgzero callbacks
# ===========================================================================
def update(dt):
    global cursor_x, cursor_y, score, misses, prev_button
    global last_hit_time, last_miss_time, calibrating_until

    with _state_lock:
        ax = _imu["ax"]; ay = _imu["ay"]; button = _imu["button"]

    now = time.time()

    # Calibration: average ax/ay during a short window so wherever the IMU
    # rests becomes "zero". Prevents drift if the board isn't perfectly flat.
    if now < calibrating_until:
        calib_samples.append((ax, ay))
        return
    if calib_samples:
        _offset["ax"] = sum(s[0] for s in calib_samples) / len(calib_samples)
        _offset["ay"] = sum(s[1] for s in calib_samples) / len(calib_samples)
        calib_samples.clear()
        print(f"[calib] ax_offset={_offset['ax']:.0f} "
              f"ay_offset={_offset['ay']:.0f}")

    ax_c = ax - _offset["ax"]
    ay_c = ay - _offset["ay"]

    vx = _deadzone(ax_c) * SENSITIVITY
    vy = _deadzone(ay_c) * SENSITIVITY
    if INVERT_X:
        vx = -vx
    if INVERT_Y:
        vy = -vy
    vx = max(-MAX_SPEED, min(MAX_SPEED, vx))
    vy = max(-MAX_SPEED, min(MAX_SPEED, vy))

    cursor_x = max(0, min(WIDTH, cursor_x + vx * dt))
    cursor_y = max(0, min(HEIGHT, cursor_y + vy * dt))

    # rising-edge button click only
    if button == 1 and prev_button == 0:
        if math.hypot(cursor_x - target_x, cursor_y - target_y) <= HIT_RADIUS:
            score += 1
            last_hit_time = now
            new_target()
        else:
            misses += 1
            last_miss_time = now
    prev_button = button


def on_key_down(key):
    global calibrating_until
    if key == keys.SPACE:
        calib_samples.clear()
        calibrating_until = time.time() + CALIBRATE_SECONDS
    elif key == keys.ESCAPE:
        import sys
        sys.exit(0)


def draw():
    screen.fill((25, 28, 38))
    now = time.time()

    # No serial yet?
    with _state_lock:
        ok = _imu["ok"]
    if not ok:
        screen.draw.text(
            "Waiting for Pico...\n"
            f"port: {_port}\n"
            "Make sure hw10c is running and no serial monitor is open.",
            center=(WIDTH / 2, HEIGHT / 2),
            fontsize=28, color=(255, 200, 120), align="center")
        return

    if now < calibrating_until:
        screen.draw.text(
            "Hold the IMU level - calibrating...",
            center=(WIDTH / 2, HEIGHT / 2),
            fontsize=40, color=(255, 230, 120))
        return

    # Target hit-zone ring + letter (color flashes briefly on hit/miss)
    color = (255, 220, 60)
    if now - last_hit_time < 0.18:
        color = (120, 255, 130)
    elif now - last_miss_time < 0.18:
        color = (255, 120, 120)
    screen.draw.circle((int(target_x), int(target_y)),
                       HIT_RADIUS, (80, 85, 110))
    screen.draw.text(LETTER, center=(int(target_x), int(target_y)),
                     fontsize=84, color=color)

    # Cursor: filled dot + ring + crosshair
    cx, cy = int(cursor_x), int(cursor_y)
    screen.draw.filled_circle((cx, cy), CURSOR_RADIUS, (60, 200, 120))
    screen.draw.circle((cx, cy), CURSOR_RADIUS + 1, (255, 255, 255))
    screen.draw.line((cx - CURSOR_RADIUS - 8, cy),
                     (cx + CURSOR_RADIUS + 8, cy), (255, 255, 255))
    screen.draw.line((cx, cy - CURSOR_RADIUS - 8),
                     (cx, cy + CURSOR_RADIUS + 8), (255, 255, 255))

    # HUD
    screen.draw.text(f"Hits: {score}    Misses: {misses}",
                     topleft=(14, 10), fontsize=28, color=(230, 230, 230))
    screen.draw.text("SPACE: recalibrate    ESC: quit",
                     topright=(WIDTH - 14, 14), fontsize=20,
                     color=(160, 160, 180))

    with _state_lock:
        ax = _imu["ax"]; ay = _imu["ay"]; btn = _imu["button"]
    screen.draw.text(f"ax={ax:>6}  ay={ay:>6}  btn={btn}",
                     bottomleft=(12, HEIGHT - 10),
                     fontsize=18, color=(140, 180, 230))


pgzrun.go()
