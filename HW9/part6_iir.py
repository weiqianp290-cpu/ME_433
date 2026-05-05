# HW9 Part 6: IIR low-pass filter
# new_average[i] = A * new_average[i-1] + B * signal[i],  with A + B = 1
# A larger -> smoother but laggier; B larger -> faster response.
import csv
import os
import numpy as np
import matplotlib.pyplot as plt


def load_csv(path):
    t, y = [], []
    with open(path) as f:
        for row in csv.reader(f):
            if not row:
                continue
            t.append(float(row[0]))
            y.append(float(row[1]))
    return np.array(t), np.array(y)


def iir(signal, A, B):
    """First-order IIR. Assumes A+B = 1. new[i] = A*new[i-1] + B*signal[i]."""
    out = np.zeros(len(signal))
    out[0] = signal[0]              # initialize with the first sample
    for i in range(1, len(signal)):
        out[i] = A * out[i - 1] + B * signal[i]
    return out


def fft_mag(y, Fs):
    n = len(y)
    frq = np.arange(n) / (n / Fs)
    frq = frq[: n // 2]
    Y = np.fft.fft(y) / n
    return frq, np.abs(Y[: n // 2])


def plot_iir(csv_path, A, B, out_png):
    t, y = load_csv(csv_path)
    Fs = len(t) / t[-1]
    yf = iir(y, A, B)

    frq, Y = fft_mag(y, Fs)
    _, Yf = fft_mag(yf, Fs)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9, 6))
    name = os.path.basename(csv_path)

    ax1.plot(t, y, "k-", linewidth=0.6, label="unfiltered")
    ax1.plot(t, yf, "r-", linewidth=1.2, label=f"IIR A={A}, B={B}")
    ax1.set_xlabel("Time (s)")
    ax1.set_ylabel("Amplitude")
    ax1.set_title(f"{name} - IIR Filter, A = {A}, B = {B}  (Fs = {Fs:.1f} Hz)")
    ax1.legend(loc="upper right")

    ax2.loglog(frq, Y, "k-", linewidth=0.6, label="unfiltered FFT")
    ax2.loglog(frq, Yf, "r-", linewidth=1.0, label="filtered FFT")
    ax2.set_xlabel("Frequency (Hz)")
    ax2.set_ylabel("|Y(freq)|")
    ax2.legend(loc="lower left")

    plt.tight_layout()
    plt.savefig(out_png, dpi=120)
    plt.close(fig)
    print(f"saved {out_png}")


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    # (A, B) chosen by eye. With A+B=1, larger A = heavier smoothing.
    configs = {
        "sigA": (0.99, 0.01),    # fast Fs needs strong smoothing for HF noise
        "sigB": (0.95, 0.05),    # moderate smoothing
        "sigC": (0.90, 0.10),    # mild smoothing on square wave
        "sigD": (0.80, 0.20),    # slow Fs; lighter smoothing keeps shape
    }
    for name, (A, B) in configs.items():
        plot_iir(
            os.path.join(here, f"{name}.csv"),
            A,
            B,
            os.path.join(here, f"part6_{name}_iir.png"),
        )
