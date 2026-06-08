"""
HW14 HX711 force sensor capture and plots.

Run after flashing hw14c.uf2 to the Pico:
    python3 plot_hx711.py --samples 800

The script sends the desired sample count to the Pico, receives CSV rows:
    time_ms,raw,filtered
and saves:
    hx711_time.png
    hx711_fft.png
"""

import argparse
import csv
import math
import os
import time
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/matplotlib-hw14")

import matplotlib.pyplot as plt
import numpy as np
import serial
import serial.tools.list_ports


BAUD = 115200


def find_pico_port():
    ports = list(serial.tools.list_ports.comports())

    for port in ports:
        if getattr(port, "vid", None) == 0x2E8A:
            return port.device

    for port in ports:
        desc = (port.description or "").lower()
        dev = (port.device or "").lower()
        if "pico" in desc or "usbmodem" in dev or "ttyacm" in dev:
            return port.device

    return ports[0].device if ports else None


def read_capture(port, samples):
    rows = []

    with serial.Serial(port, BAUD, timeout=2) as ser:
        time.sleep(0.5)
        ser.reset_input_buffer()
        print(f"Requesting {samples} samples from Pico...", flush=True)
        ser.write(f"{samples}\n".encode("ascii"))

        # The firmware buffers all samples and prints them only after the whole
        # capture finishes, so the serial line is silent during collection.
        # Allow enough time for a slow/low-rate capture before declaring a fault.
        NO_DATA_TIMEOUT = 60.0
        last_data_time = time.monotonic()
        while True:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if not line:
                if time.monotonic() - last_data_time > NO_DATA_TIMEOUT:
                    raise RuntimeError(
                        f"No data received for {NO_DATA_TIMEOUT:.0f} seconds. Check that the "
                        "new UF2 is flashed, the Pico serial monitor is closed, and "
                        "DT/SCK/GND/VCC are connected correctly (DT must toggle low)."
                    )
                continue
            last_data_time = time.monotonic()
            if line == "done":
                break
            if line.startswith("time_ms"):
                continue
            if "," not in line:
                print(f"[pico] {line}", flush=True)
                continue

            parts = line.split(",")
            if len(parts) != 3:
                continue

            try:
                rows.append((float(parts[0]), float(parts[1]), float(parts[2])))
            except ValueError:
                continue

            if len(rows) % 100 == 0:
                print(f"received {len(rows)}/{samples}", flush=True)

    if len(rows) == 0:
        raise RuntimeError("No data received from the Pico.")

    return np.array(rows)


def save_time_plot(data, output_dir):
    t = data[:, 0] / 1000.0
    raw = data[:, 1]
    filtered = data[:, 2]

    plt.figure(figsize=(10, 5))
    plt.plot(t, raw, label="Raw", linewidth=1)
    plt.plot(t, filtered, label="IIR filtered", linewidth=2)
    plt.xlabel("Time (s)")
    plt.ylabel("HX711 reading")
    plt.title("HX711 Force Sensor Data")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()

    path = output_dir / "hx711_time.png"
    plt.savefig(path, dpi=200)
    plt.close()
    return path


def save_fft_plot(data, output_dir):
    t = data[:, 0] / 1000.0
    raw = data[:, 1] - np.mean(data[:, 1])
    filtered = data[:, 2] - np.mean(data[:, 2])

    dt = np.mean(np.diff(t))
    fs = 1.0 / dt
    freqs = np.fft.rfftfreq(len(t), dt)

    raw_fft = np.abs(np.fft.rfft(raw)) / len(raw)
    filtered_fft = np.abs(np.fft.rfft(filtered)) / len(filtered)

    nyquist = fs / 2.0
    max_freq = min(45.0, nyquist)

    plt.figure(figsize=(10, 5))
    plt.plot(freqs, raw_fft, label="Raw", linewidth=1)
    plt.plot(freqs, filtered_fft, label="IIR filtered", linewidth=2)
    plt.xlim(0, max_freq)
    plt.xlabel("Frequency (Hz)")
    plt.ylabel("Magnitude")
    plt.title(f"HX711 FFT, fs = {fs:.1f} Hz, Nyquist = {nyquist:.1f} Hz")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()

    path = output_dir / "hx711_fft.png"
    plt.savefig(path, dpi=200)
    plt.close()
    return path


def save_csv(data, output_dir):
    path = output_dir / "hx711_capture.csv"

    with path.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["time_ms", "raw", "filtered"])
        writer.writerows(data)

    return path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--samples", type=int, default=800)
    parser.add_argument("--port", default=None)
    args = parser.parse_args()

    port = args.port or find_pico_port()
    if port is None:
        raise RuntimeError("No serial port found. Plug in the Pico and try again.")

    output_dir = Path(__file__).resolve().parent
    print(f"Opening {port} at {BAUD} baud", flush=True)
    data = read_capture(port, args.samples)

    time_path = save_time_plot(data, output_dir)
    fft_path = save_fft_plot(data, output_dir)
    csv_path = save_csv(data, output_dir)

    duration = (data[-1, 0] - data[0, 0]) / 1000.0
    fs = (len(data) - 1) / duration if duration > 0 else math.nan

    print(f"Saved {csv_path}", flush=True)
    print(f"Saved {time_path}", flush=True)
    print(f"Saved {fft_path}", flush=True)
    print(f"Collected {len(data)} samples at about {fs:.1f} Hz", flush=True)


if __name__ == "__main__":
    main()
