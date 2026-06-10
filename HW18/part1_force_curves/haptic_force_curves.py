"""Generate normalized force-displacement curves for the HW18 haptic paddle."""

from pathlib import Path

import matplotlib
import numpy as np

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def normalize(force):
    """Scale a force array so its largest absolute value is 1."""
    peak = np.max(np.abs(force))
    return force / peak if peak > 0 else force


def virtual_spring(x):
    """Linear spring centered at x = 0."""
    return -x


def bump(x, center=0.0, width=0.22):
    """Repulsive localized bump based on the derivative of a Gaussian."""
    z = (x - center) / width
    return normalize(z * np.exp(-0.5 * z**2))


def dip(x, center=0.0, width=0.22):
    """Attractive detent; the negative of the bump curve."""
    return -bump(x, center, width)


def toggle_switch(x, stable_position=0.55):
    """Double-well force with stable positions at +/- stable_position."""
    force = x * (stable_position**2 - x**2)
    return normalize(force)


def main():
    x = np.linspace(-1.0, 1.0, 1201)
    curves = [
        (
            "Virtual spring",
            virtual_spring(x),
            r"$F_n=-x_n$",
            "Restoring force toward the center",
        ),
        (
            "Bump",
            bump(x),
            r"$z=(x_n-x_c)/w,\quad F_n=\mathrm{norm}(ze^{-z^2/2})$",
            "Repels the paddle from the bump center",
        ),
        (
            "Dip / detent",
            dip(x),
            r"$F_n=-\mathrm{norm}(ze^{-z^2/2})$",
            "Pulls the paddle into a center detent",
        ),
        (
            "Toggle switch",
            toggle_switch(x),
            r"$F_n=\mathrm{norm}[x_n(a^2-x_n^2)]$",
            "Stable states at x = -a and x = +a",
        ),
    ]

    fig, axes = plt.subplots(2, 2, figsize=(12, 8), sharex=True, sharey=True)

    for ax, (title, force, equation, description) in zip(axes.flat, curves):
        ax.plot(x, force, color="#1769aa", linewidth=2.5)
        ax.axhline(0, color="black", linewidth=0.8)
        ax.axvline(0, color="black", linewidth=0.8)
        ax.fill_between(x, 0, force, alpha=0.18, color="#1769aa")
        ax.set_title(title, fontsize=14, fontweight="bold")
        ax.text(
            0.5,
            0.94,
            equation,
            transform=ax.transAxes,
            ha="center",
            va="top",
            fontsize=10,
        )
        ax.text(
            0.5,
            0.05,
            description,
            transform=ax.transAxes,
            ha="center",
            va="bottom",
            fontsize=9,
            color="#333333",
        )
        ax.set_xlim(-1, 1)
        ax.set_ylim(-1.08, 1.08)
        ax.set_xticks(np.linspace(-1, 1, 5))
        ax.set_yticks(np.linspace(-1, 1, 5))
        ax.grid(True, alpha=0.25)
        ax.set_xlabel("Normalized displacement, $x_n$")
        ax.set_ylabel("Normalized force, $F_n$")

    fig.suptitle(
        "HW18 Haptic Paddle: Desired Force vs. Displacement",
        fontsize=17,
        fontweight="bold",
    )
    fig.text(
        0.5,
        0.012,
        "Positive force acts in the positive displacement direction. "
        "All commanded forces are limited to [-1, 1].",
        ha="center",
        fontsize=10,
    )
    fig.tight_layout(rect=(0, 0.04, 1, 0.94))

    output = Path(__file__).with_name("haptic_force_curves.png")
    fig.savefig(output, dpi=200, bbox_inches="tight")
    print(f"Saved {output}")


if __name__ == "__main__":
    main()
