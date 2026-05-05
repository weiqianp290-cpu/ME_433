# HW9 Part 5: Moving Average Filter (MAF)
# new[i] = (1/X) * sum(signal[i-X+1 .. i])
# For each CSV pick a window size X by trying a few and choosing the best by eye.
# Plot: black = unfiltered, red = filtered.
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


def maf(signal, X):
    """Moving average over the last X samples. Pad-with-zero behavior:
    the first X samples assume the prior buffer was 0."""
    n = len(signal)
    out = np.zeros(n)
    running = 0.0
    buf = np.zeros(X)
    idx = 0
    for i in range(n):
        running -= buf[idx]
        buf[idx] = signal[i]
        running += signal[i]
        idx = (idx + 1) % X
        out[i] = running / X
    return out


def fft_mag(y, Fs):
    n = len(y)
    frq = np.arange(n) / (n / Fs)
    frq = frq[: n // 2]
    Y = np.fft.fft(y) / n
    return frq, np.abs(Y[: n // 2])


def plot_maf(csv_path, X, out_png):
    t, y = load_csv(csv_path)
    Fs = len(t) / t[-1]
    yf = maf(y, X)

    frq, Y = fft_mag(y, Fs)
    _, Yf = fft_mag(yf, Fs)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9, 6))
    name = os.path.basename(csv_path)

    ax1.plot(t, y, "k-", linewidth=0.6, label="unfiltered")
    ax1.plot(t, yf, "r-", linewidth=1.2, label="MAF X=%d" % X)
    ax1.set_xlabel("Time (s)")
    ax1.set_ylabel("Amplitude")
    ax1.set_title(f"{name} - Moving Average Filter, X = {X} samples (Fs = {Fs:.1f} Hz)")
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
    # X chosen by eye after trying several values per CSV.
    configs = {
        "sigA": 500,   # Fs ~10000 Hz, smooths the high-freq components
        "sigB": 200,   # Fs ~3300 Hz, removes the dense high-freq noise
        "sigC": 25,    # Fs ~2500 Hz, mild smoothing on the square-wave edges
        "sigD": 25,    # Fs ~400 Hz, removes ripple while preserving shape
    }
    for name, X in configs.items():
        plot_maf(
            os.path.join(here, f"{name}.csv"),
            X,
            os.path.join(here, f"part5_{name}_maf.png"),
        )
