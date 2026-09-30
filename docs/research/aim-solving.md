# Aim solving and zeroing: literature and reference solvers

Research note for the M1 zeroing, holdover and windage item. Felipe asked for current literature before the `SolveAim` code lands.

## Findings

- **No recent academic paper on small-arms zeroing algorithms turned up.** Searches of 2025 and 2026 literature returned missile trajectory prediction, projectile-range optimisation and fire-control patents, none of which gives a zero or aim solver. The current practice is visible in open-source solvers and vendor technical documents instead.
- **bclibc / py-ballisticcalc** (LGPL-3.0, algorithm only, no code copied) solves the zero with a damped Newton loop on the height error at the zero distance, converging to a configurable `cZeroFindingAccuracy`. When that loop exhausts its iteration budget it falls back to a guaranteed method: a golden-section search brackets the maximum-range angle, then Ridder's method finds the zero angle below it. The fallback is 10 to 50 times more expensive, and it is what makes the solver safe near maximum range, where the low-angle and high-angle roots merge [1][2].
- **Hornady 4DOF** separates the zero angle (bore relative to line of sight, fixed once found) from the zero range (which moves with the atmosphere). The user can enter either. The shooting angle is the line-of-sight angle to level ground, and Hornady reports that simple calculators degrade beyond about 15 degrees of incline, which 4DOF handles by using the velocity vector's alignment with gravity [3][4].
- **Rifleman's rule** (horizontal range = slant range x cos(incline)) is the classical approximation that a full 3D aim solve replaces [5].
- **Broyden's method**, a generalised secant method, is the standard multi-variable shooting solver in trajectory design software. It estimates the Jacobian instead of re-evaluating it, and one comparison measured it about 40% faster than Newton-Raphson with numerical partials, at equal solution quality [6].
- **The NATO Armaments Ballistic Kernel** (STANAG 4355 modified point mass) is the NATO reference engine for fire control, validated for small arms. Its aim iteration is not described in public sources [7].

## What this suggests for Ogiva

- Solve elevation and azimuth together with Broyden's generalised secant, starting from the line-of-sight direction plus a vacuum drop estimate. The miss is measured in the plane through the target perpendicular to the line of sight, found with `FlyToPlane`.
- Keep a guaranteed fallback like bclibc's: if the secant iteration fails, bracket the elevation below the maximum-range angle and use a bracketing root finder. Report a target beyond maximum range as an error instead of returning the high-angle root.
- Treat the zero as a fixed angle once solved, as Hornady does, so holds at other ranges and atmospheres are measured relative to it.

## Sources

1. bclibc, C++ solver engine: <https://github.com/ballistics-lab/bclibc>; micropython-bclibc: <https://github.com/ballistics-lab/micropython-bclibc>
2. py-ballisticcalc: <https://github.com/o-murphy/py-ballisticcalc>; releases: <https://github.com/o-murphy/py-ballisticcalc/releases/tag/v3.0.0-beta.1>
3. Hornady 4DOF instructions: <https://www.hornady.com/team-hornady/ballistic-information/ballistic-resources/4dof-instructions>
4. Hornady 4DOF technical paper: <https://static.hornady.media/site/hornady/files/ballistic/hornady-4dof-technical-paper-v2.pdf>
5. Rifleman's rule: <https://en.wikipedia.org/wiki/Rifleman's_rule>
6. Comparisons between Newton-Raphson and Broyden's methods for trajectory design problems (AGI): <https://www.agi.com/getmedia/77659f3e-3123-4714-a218-002459c5da71/Comparisons-Between-Newton-Raphson-and-Broyden-s-Methods-for-Trajectory-Design-Problems.pdf>
7. Validation of the NATO Armaments Ballistic Kernel for use in small-arms fire control systems: <https://www.sciencedirect.com/science/article/pii/S2214914717300569>
