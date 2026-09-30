# Reference drag tables

`OgivaCore` ships the G1 and G7 standard drag functions as Cd vs Mach tables, interpolated with the monotone PCHIP curve in `OgivaPchip.h`.

## Provenance

| Model | File | Points | Mach range | SHA-256 |
| --- | --- | --- | --- | --- |
| G1 | `tests/core/data/drag/mcg1.txt` | 79 | 0 to 5 | `0e40f44240abc62427065d5bf2db67fe4522a093e8a9a6c839f2acc930074635` |
| G7 | `tests/core/data/drag/mcg7.txt` | 84 | 0 to 5 | `7338f3bfa037b9c39c39894c269e1e7d46d81c27397caab43a9260ea3607fcd8` |

- Downloaded verbatim from JBM Ballistics, <https://jbmballistics.com/downloads.html> (`/downloads/mcg1.txt`, `/downloads/mcg7.txt`).
- JBM states the G functions come from the US Army Ballistic Research Laboratory (BRL) and that the original drag programs by Robert L. McCoy are posted with his permission. The site carries the notice "All content and logos © 2010-2026, JBM Ballistics".
- Reuse in Ogiva was checked by a lawyer before import.
- No primary BRL report tabulating these values was found on the DTIC mirror; the reports checked are listed in `ROADMAP.md` under M1.

## Regenerating the compiled tables

The arrays in `plugin/Ogiva/Source/OgivaCore/Private/OgivaReferenceDragData.h` are generated from the two files above. Python standard library and `clang-format` only:

```bash
python tools/drag-tables/generate_reference_drag_data.py \
  --g1 tests/core/data/drag/mcg1.txt \
  --g7 tests/core/data/drag/mcg7.txt \
  --out plugin/Ogiva/Source/OgivaCore/Private/OgivaReferenceDragData.h
```

`tests/core/DragTests.cpp` reads the same text files and checks that the compiled curves return every tabulated value exactly, so the generated header cannot drift from its source.
