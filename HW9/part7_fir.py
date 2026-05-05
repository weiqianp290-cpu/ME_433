# HW9 Part 7: FIR low-pass filter from a windowed sinc
#
# We design the filter taps h[k] from a sinc kernel and a window, then
# apply them with a manual loop:  out[i] = sum_k  h[k] * signal[i-k]
#
# Tap count N is chosen from the desired transition bandwidth BW and the
# window's main-lobe width:
#     Hamming:    N ≈ 3.3  / (BW/Fs)
#     Blackman:   N ≈ 5.5  / (BW/Fs)
#     Hanning:    N ≈ 3.1  / (BW/Fs)
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


def fft_mag(y, Fs):
    n = len(y)
    frq = np.arange(n) / (n / Fs)
    frq = frq[: n // 2]
    Y = np.fft.fft(y) / n
    return frq, np.abs(Y[: n // 2])


# --- window functions -------------------------------------------------------
def window_function(name, N):
    n = np.arange(N)
    if name == "rectangular":
        return np.ones(N)
    if name == "hanning":
        return 0.5 - 0.5 * np.cos(2 * np.pi * n / (N - 1))
    if name == "hamming":
        return 0.54 - 0.46 * np.cos(2 * np.pi * n / (N - 1))
    if name == "blackman":
        return (
            0.42
            - 0.5 * np.cos(2 * np.pi * n / (N - 1))
            + 0.08 * np.cos(4 * np.pi * n / (N - 1))
        )
    raise ValueError(f"unknown window {name}")


def sinc_lowpass_taps(N, fc, Fs, window_name):
    """Build N FIR coefficients for a low-pass with cutoff fc (Hz)."""
    if N % 2 == 0:
        N += 1                          # force odd length so we have a center tap
    M = (N - 1) // 2
    fc_n = fc / Fs                      # normalized cutoff (0..0.5)
    h = np.zeros(N)
    for i in range(N):
        k = i - M
        if k == 0:
            h[i] = 2 * fc_n
        else:
            h[i] = np.sin(2 * np.pi * fc_n * k) / (np.pi * k)
    h *= window_function(window_name, N)
    h /= np.sum(h)                      # unity DC gain
    return h


def apply_fir(signal, h):
    """Manual convolution loop:  out[i] = sum_k h[k] * signal[i-k].
    Pad the start of the signal with signal[0] so we don't introduce a
    fake step into the filter (which would show up as a low-freq lump
    in the FFT)."""
    N = len(h)
    n = len(signal)
    pad = np.full(N - 1, signal[0])
    extended = np.concatenate([pad, signal])
    h_rev = h[::-1]
    out = np.zeros(n)
    for i in range(n):
        out[i] = np.dot(h_rev, extended[i : i + N])
    return out


def num_taps_for(window_name, bw_hz, Fs):
    main_lobe = {"hamming": 3.3, "blackman": 5.5, "hanning": 3.1, "rectangular": 0.9}
    return int(np.ceil(main_lobe[window_name] / (bw_hz / Fs)))


def plot_fir(csv_path, fc, bw, window_name, out_png):
    t, y = load_csv(csv_path)
    Fs = len(t) / t[-1]

    N = num_taps_for(window_name, bw, Fs)
    h = sinc_lowpass_taps(N, fc, Fs, window_name)
    yf = apply_fir(y, h)

    # Skip the startup region (first N-1 samples) when comparing FFTs so the
    # filter's transient response doesn't show up as fake low-frequency energy.
    skip = len(h) - 1
    frq, Y = fft_mag(y[skip:], Fs)
    _, Yf = fft_mag(yf[skip:], Fs)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9, 6))
    name = os.path.basename(csv_path)

    ax1.plot(t, y, "k-", linewidth=0.6, label="unfiltered")
    ax1.plot(t, yf, "r-", linewidth=1.2, label="FIR sinc")
    ax1.set_xlabel("Time (s)")
    ax1.set_ylabel("Amplitude")
    ax1.set_title(
        f"{name} - FIR sinc LP, N={len(h)} taps, {window_name} window, "
        f"fc={fc} Hz, BW={bw} Hz  (Fs = {Fs:.1f} Hz)"
    )
    ax1.legend(loc="upper right")

    ax2.loglog(frq, Y, "k-", linewidth=0.6, label="unfiltered FFT")
    ax2.loglog(frq, Yf, "r-", linewidth=1.0, label="filtered FFT")
    ax2.set_xlabel("Frequency (Hz)")
    ax2.set_ylabel("|Y(freq)|")
    ax2.legend(loc="lower left")

    plt.tight_layout()
    plt.savefig(out_png, dpi=120)
    plt.close(fig)
    print(f"saved {out_png}  (N={len(h)}, window={window_name})")


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    # (cutoff_Hz, bandwidth_Hz, window_name) chosen from the part-4 FFTs.
    configs = {
        "sigA": (15, 50, "hamming"),     # Fs ~10000 Hz, keep ~3 Hz signal + 25 Hz peak
        "sigB": (10, 20, "blackman"),    # Fs ~3300 Hz, low-frequency content only
        "sigC": (50, 100, "hamming"),    # Fs ~2500 Hz, mild smoothing on square wave
        "sigD": (8, 12, "blackman"),     # Fs ~400 Hz, slow signal
    }
    for name, (fc, bw, w) in configs.items():
        plot_fir(
            os.path.join(here, f"{name}.csv"),
            fc, bw, w,
            os.path.join(here, f"part7_{name}_fir.png"),
        )
