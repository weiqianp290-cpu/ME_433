# HW18 Haptic Force-Displacement Design

The paddle displacement and commanded force are normalized as

```text
x_n = x / x_max
F_n = F_desired / F_max
```

so both values stay between -1 and 1. Positive force acts in the positive
displacement direction.

## 1. Virtual spring

```text
F_n = -x_n
```

This effect always pushes the paddle back toward the center. In the physical
controller, damping can be added to reduce oscillation:

```text
F_command = clamp(F_n - b*v_n, -1, 1)
```

where `v_n` is normalized paddle velocity and `b` is the damping coefficient.

## 2. Bump

Let

```text
z = (x_n - x_c) / w
F_n = norm(z * exp(-z^2 / 2))
```

where `x_c` is the bump location and `w` controls its width. The force points
away from `x_c`, so the user feels a localized barrier or raised bump.

## 3. Dip / detent

```text
F_n = -norm(z * exp(-z^2 / 2))
```

This reverses the bump force. It attracts the paddle toward `x_c`, producing a
notch or detent that the paddle naturally settles into.

## 4. Toggle switch

```text
F_n = norm(x_n * (a^2 - x_n^2))
```

The two stable positions are `x_n = -a` and `x_n = +a`; the center is unstable.
For the plotted design, `a = 0.55`. Moving through the center makes the paddle
snap into the opposite stable state like a toggle switch.

The accompanying Python script generates
`haptic_force_curves.png` with all four normalized curves.
