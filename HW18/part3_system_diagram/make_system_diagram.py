"""Create the HW18 horizontal haptic-paddle system block diagram."""

from pathlib import Path

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch


OUT_DIR = Path(__file__).parent


def box(ax, x, y, w, h, title, subtitle, face, edge):
    patch = FancyBboxPatch(
        (x, y),
        w,
        h,
        boxstyle="round,pad=0.04,rounding_size=0.14",
        linewidth=1.7,
        edgecolor=edge,
        facecolor=face,
        zorder=3,
    )
    ax.add_patch(patch)
    ax.text(
        x + w / 2,
        y + h * 0.62,
        title,
        ha="center",
        va="center",
        fontsize=12.5,
        fontweight="bold",
        color=edge,
        zorder=4,
    )
    ax.text(
        x + w / 2,
        y + h * 0.30,
        subtitle,
        ha="center",
        va="center",
        fontsize=9.5,
        color=edge,
        zorder=4,
    )


def arrow(ax, start, end, label="", color="#475569", rad=0.0, dashed=False):
    patch = FancyArrowPatch(
        start,
        end,
        arrowstyle="-|>",
        mutation_scale=13,
        linewidth=1.7,
        color=color,
        linestyle="--" if dashed else "-",
        connectionstyle=f"arc3,rad={rad}",
        shrinkA=3,
        shrinkB=3,
        zorder=2,
    )
    ax.add_patch(patch)
    if label:
        mx = (start[0] + end[0]) / 2
        my = (start[1] + end[1]) / 2 + (0.27 if rad == 0 else 0.18)
        ax.text(
            mx,
            my,
            label,
            ha="center",
            va="center",
            fontsize=8.8,
            color="#334155",
            bbox=dict(facecolor="white", edgecolor="none", pad=1.5, alpha=0.92),
            zorder=5,
        )


def main():
    fig, ax = plt.subplots(figsize=(18, 9))
    fig.patch.set_facecolor("#f8fafc")
    ax.set_facecolor("#f8fafc")
    ax.set_xlim(0, 18)
    ax.set_ylim(0, 9)
    ax.axis("off")

    ax.text(
        9,
        8.48,
        "HW18 Haptic Paddle Control System",
        ha="center",
        va="center",
        fontsize=23,
        fontweight="bold",
        color="#0f172a",
    )
    ax.text(
        9,
        8.05,
        "Command path (top) and sensor feedback paths (bottom)",
        ha="center",
        va="center",
        fontsize=11,
        color="#64748b",
    )

    # Main command path
    box(
        ax,
        0.45,
        5.35,
        2.35,
        1.18,
        "Computer GUI",
        "Graphics + USB serial",
        "#ede9fe",
        "#5b21b6",
    )
    box(
        ax,
        3.35,
        5.35,
        2.75,
        1.18,
        "Raspberry Pi Pico",
        "Haptic model / PD loop, 80 Hz",
        "#dcfce7",
        "#166534",
    )
    box(
        ax,
        6.65,
        5.35,
        2.55,
        1.18,
        "STM32 Controller",
        "Motor-current PI loop, 1 kHz",
        "#dbeafe",
        "#1d4ed8",
    )
    box(
        ax,
        9.75,
        5.35,
        2.15,
        1.18,
        "H-Bridge",
        "PWM + direction",
        "#ffedd5",
        "#c2410c",
    )
    box(
        ax,
        12.45,
        5.35,
        1.85,
        1.18,
        "DC Motor",
        "Torque output",
        "#fee2e2",
        "#b91c1c",
    )
    box(
        ax,
        14.85,
        5.35,
        2.55,
        1.18,
        "Haptic Paddle",
        "Human force / motion",
        "#ecfccb",
        "#3f6212",
    )

    arrow(ax, (2.8, 5.94), (3.35, 5.94), "USB serial")
    arrow(ax, (6.1, 5.94), (6.65, 5.94), "desired current")
    arrow(ax, (9.2, 5.94), (9.75, 5.94), "PWM, DIR")
    arrow(ax, (11.9, 5.94), (12.45, 5.94), "motor voltage")
    arrow(ax, (14.3, 5.94), (14.85, 5.94), "shaft torque")

    # Sensor and feedback path
    box(
        ax,
        3.35,
        1.62,
        2.75,
        1.18,
        "Load Cell + HX711",
        "Measured paddle force",
        "#fef3c7",
        "#92400e",
    )
    box(
        ax,
        7.15,
        1.62,
        2.55,
        1.18,
        "AS5600 Encoder",
        "Magnetic shaft position",
        "#cffafe",
        "#0e7490",
    )
    box(
        ax,
        10.75,
        1.62,
        2.35,
        1.18,
        "Current Sensor",
        "Motor-current feedback",
        "#e0e7ff",
        "#4338ca",
    )

    # Mechanical relationships from the paddle/motor to sensors.
    arrow(
        ax,
        (16.13, 5.35),
        (4.73, 2.80),
        "",
        color="#94a3b8",
        rad=-0.16,
        dashed=True,
    )
    arrow(
        ax,
        (13.38, 5.35),
        (8.43, 2.80),
        "",
        color="#94a3b8",
        rad=-0.12,
        dashed=True,
    )
    arrow(
        ax,
        (10.83, 5.35),
        (11.93, 2.80),
        "",
        color="#94a3b8",
        rad=0.08,
        dashed=True,
    )

    # Electrical feedback returns to the two controllers.
    arrow(
        ax,
        (4.73, 2.80),
        (4.25, 5.35),
        "DOUT / SCK, 3.3 V",
        color="#0f766e",
        rad=0.16,
    )
    arrow(
        ax,
        (8.43, 2.80),
        (5.25, 5.35),
        "I2C position, 3.3 V",
        color="#0f766e",
        rad=-0.13,
    )
    arrow(
        ax,
        (11.93, 2.80),
        (8.30, 5.35),
        "ADC current",
        color="#4338ca",
        rad=-0.12,
    )

    ax.text(
        0.55,
        0.52,
        "Solid arrows: electrical/control signals     "
        "Dashed arrows: mechanical or physical coupling",
        ha="left",
        va="center",
        fontsize=9.5,
        color="#64748b",
    )
    ax.text(
        17.45,
        0.52,
        "All logic signals: 3.3 V",
        ha="right",
        va="center",
        fontsize=9.5,
        color="#64748b",
    )

    fig.tight_layout(pad=0.5)
    png_path = OUT_DIR / "haptic_system_block_diagram.png"
    svg_path = OUT_DIR / "haptic_system_block_diagram.svg"
    fig.savefig(png_path, dpi=200, bbox_inches="tight", facecolor=fig.get_facecolor())
    fig.savefig(svg_path, bbox_inches="tight", facecolor=fig.get_facecolor())
    print(f"Saved {png_path}")
    print(f"Saved {svg_path}")


if __name__ == "__main__":
    main()
