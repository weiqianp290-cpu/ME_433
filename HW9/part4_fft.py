# HW9 Part 4: Plot signal vs time and FFT for each CSV
# Generates one PNG per CSV with two subplots: signal vs time, and FFT magnitude.
import csv
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


def compute_fft(y, Fs):
    """Single-sided FFT magnitude. Returns (freqs, |Y|)."""
    n = len(y)
    k = np.arange(n)
    T = n / Fs
    frq = k / T                  # two-sided frequency range
    frq = frq[: n // 2]          # one-sided range
    Y = np.fft.fft(y) / n        # FFT and normalization
    Y = Y[: n // 2]
    return frq, np.abs(Y)


def plot_signal_and_fft(csv_path, out_png):
    t, y = load_csv(csv_path)
    # Sample rate from data: number of samples / total time
    Fs = len(t) / t[-1]
    frq, Ymag = compute_fft(y, Fs)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9, 6))

    ax1.plot(t, y, "b-")
    ax1.set_xlabel("Time (s)")
    ax1.set_ylabel("Amplitude")
    ax1.set_title(
        f"{csv_path.split('/')[-1]}  -  {len(y)} samples,  Fs ≈ {Fs:.2f} Hz"
    )

    ax2.loglog(frq, Ymag, "b-")
    ax2.set_xlabel("Frequency (Hz)")
    ax2.set_ylabel("|Y(freq)|")
    ax2.set_title("FFT")

    plt.tight_layout()
    plt.savefig(out_png, dpi=120)
    plt.close(fig)
    print(f"saved {out_png}  (Fs = {Fs:.2f} Hz)")


if __name__ == "__main__":
    import os

    here = os.path.dirname(os.path.abspath(__file__))
    for name in ["sigA", "sigB", "sigC", "sigD"]:
        plot_signal_and_fft(
            os.path.join(here, f"{name}.csv"),
            os.path.join(here, f"part4_{name}_fft.png"),
        )
