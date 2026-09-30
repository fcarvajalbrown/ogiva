# Reference drop table

The M1 gate compares Ogiva against an independent point-mass solver on two projectiles. The table lives in `tests/core/data/trajectory/reference.csv` and is checked by `tests/core/ReferenceTrajectoryTests.cpp`.

## Provenance

| File | Rows | SHA-256 |
| --- | --- | --- |
| `tests/core/data/trajectory/reference.csv` | 20 (10 per case, 0 to 900 yd every 100 yd) | `78f6cd6648d1e52ac961bc5932c797d3007d91c61de0d06940bcc9c6bc2187f7` |

- Generated with py-ballisticcalc 2.3.1 (LGPL-3.0, <https://github.com/o-murphy/py-ballisticcalc>), its `rk4_engine`, run locally. No py-ballisticcalc code is copied into Ogiva; only its numeric output is committed. Whether committing that output is fine under the LGPL is for Felipe to confirm with his lawyer.
- JBM's online calculators, the first choice, are retired.
- The generator refuses to run unless the library's G1 and G7 tables equal `tests/core/data/drag/mcg1.txt` and `mcg7.txt` knot for knot, and the library uses the same PCHIP slopes as `OgivaPchip`.

## Cases

| Case | Drag model | BC (lb/in2) | Weight | Muzzle speed | Sight height | Zero |
| --- | --- | --- | --- | --- | --- | --- |
| `g7_308_175gr` | G7 | 0.243 | 175 gr, 0.308 in | 2600 ft/s | 1.5 in | 100 yd |
| `g1_308_150gr` | G1 | 0.400 | 150 gr, 0.308 in | 2800 ft/s | 1.5 in | 100 yd |

ICAO sea level (py-ballisticcalc's CIPM-2007 density, 1.22552 kg/m3, and its speed of sound are written into every row), no wind, no Coriolis, no spin drift. Rows stop at the last range where the path stays within 30 ft of the sight line: inside that band py-ballisticcalc holds air density constant, as Ogiva does, so differences are solver differences, not atmosphere modelling.

## Reference accuracy

py-ballisticcalc's RK4 evaluates the drag factor once per step, so it converges at first order in the Cd variation. The table uses a step multiplier of 0.001 (2.5 us steps); each row carries the difference from a 0.002 run (`height_convergence_m`, `time_convergence_s`), at most 2.1e-6 m and 2.3e-7 s. Its drag constant 2.08551e-4 is rounded to six digits and is 8.1e-7 low relative to the exact SI form.

## Gate tolerance

Chosen by Felipe, per row: height within 1e-7 x range^2 (m), time within 2e-6 s, speed within 1e-3 m/s. Measured worst case, Dormand-Prince at rtol = atol = 1e-12: height 7.3e-6 m at 823 m against a 6.8e-5 m tolerance, time 8.3e-7 s, speed 3.8e-4 m/s. The gate checks both a flight at the reference zero elevation and Ogiva's own `SolveZero` plus `SolveHold`.

## Regenerating

Create and activate a venv first, then:

```bash
python -m venv tools/reference-trajectory/.venv
tools/reference-trajectory/.venv/Scripts/python -m pip install -r tools/reference-trajectory/requirements.txt
tools/reference-trajectory/.venv/Scripts/python tools/reference-trajectory/generate_reference_trajectory.py \
  --g1 tests/core/data/drag/mcg1.txt \
  --g7 tests/core/data/drag/mcg7.txt \
  --out tests/core/data/trajectory/reference.csv
```

On Linux or macOS the venv interpreter is `.venv/bin/python`. A run takes about a minute.
