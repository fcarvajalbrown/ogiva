import argparse
import csv
import math
import sys
from dataclasses import dataclass
from pathlib import Path

import py_ballisticcalc
from py_ballisticcalc import (
    Ammo,
    Atmo,
    Calculator,
    DragModel,
    Shot,
    TableG1,
    TableG7,
    Unit,
    Weapon,
)
from py_ballisticcalc.engines.base_engine import BaseEngineConfigDict

EXPECTED_VERSION = "2.3.1"
HEIGHT_LIMIT_FT = 30.0
STEP_MULTIPLIERS = (0.002, 0.001)


@dataclass(frozen=True)
class Case:
    name: str
    table_name: str
    ballistic_coefficient: float
    weight_grains: float
    diameter_inches: float
    muzzle_speed_fps: float
    sight_height_inches: float
    zero_range_yards: float
    max_range_yards: float
    row_step_yards: float


CASES = (
    Case("g7_308_175gr", "G7", 0.243, 175.0, 0.308, 2600.0, 1.5, 100.0, 1000.0, 100.0),
    Case("g1_308_150gr", "G1", 0.400, 150.0, 0.308, 2800.0, 1.5, 100.0, 1000.0, 100.0),
)

TABLES = {"G1": TableG1, "G7": TableG7}

COLUMNS = (
    "case",
    "drag_model",
    "ballistic_coefficient_lb_in2",
    "muzzle_speed_mps",
    "sight_height_m",
    "zero_range_m",
    "air_density_kg_m3",
    "speed_of_sound_mps",
    "zero_elevation_rad",
    "range_m",
    "time_s",
    "height_m",
    "windage_m",
    "speed_mps",
    "height_convergence_m",
    "time_convergence_s",
)


def read_knots(path):
    knots = []
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        fields = line.split()
        if len(fields) == 2:
            knots.append((float(fields[0]), float(fields[1])))
    return knots


def check_table(name, table, path):
    ours = read_knots(path)
    theirs = [(point["Mach"], point["CD"]) for point in table]
    if ours != theirs:
        sys.exit(f"{name} table differs from {path}")


def make_shot(case):
    drag = DragModel(case.ballistic_coefficient, TABLES[case.table_name], case.weight_grains, case.diameter_inches)
    ammo = Ammo(drag, Unit.FPS(case.muzzle_speed_fps))
    weapon = Weapon(sight_height=Unit.Inch(case.sight_height_inches), twist=Unit.Inch(0))
    return Shot(weapon=weapon, ammo=ammo, atmo=Atmo.icao(altitude=Unit.Meter(0)))


def run(case, step_multiplier):
    config = BaseEngineConfigDict(cStepMultiplier=step_multiplier)
    calculator = Calculator(config=config, engine="rk4_engine")
    shot = make_shot(case)
    zero = calculator.set_weapon_zero(shot, Unit.Yard(case.zero_range_yards))
    result = calculator.fire(shot, Unit.Yard(case.max_range_yards), Unit.Yard(case.row_step_yards))
    return shot, zero, result.trajectory


def build_rows(case):
    runs = [run(case, multiplier) for multiplier in STEP_MULTIPLIERS]
    shot, zero, finest = runs[-1]
    _, _, coarser = runs[-2]
    atmo = shot.atmo
    rows = []
    for fine, coarse in zip(finest, coarser):
        height_ft = fine.height >> Unit.Foot
        if abs(height_ft) >= HEIGHT_LIMIT_FT:
            break
        rows.append(
            {
                "case": case.name,
                "drag_model": case.table_name,
                "ballistic_coefficient_lb_in2": case.ballistic_coefficient,
                "muzzle_speed_mps": Unit.FPS(case.muzzle_speed_fps) >> Unit.MPS,
                "sight_height_m": Unit.Inch(case.sight_height_inches) >> Unit.Meter,
                "zero_range_m": Unit.Yard(case.zero_range_yards) >> Unit.Meter,
                "air_density_kg_m3": atmo.density_metric,
                "speed_of_sound_mps": atmo.mach >> Unit.MPS,
                "zero_elevation_rad": zero >> Unit.Radian,
                "range_m": fine.distance >> Unit.Meter,
                "time_s": fine.time,
                "height_m": fine.height >> Unit.Meter,
                "windage_m": fine.windage >> Unit.Meter,
                "speed_mps": fine.velocity >> Unit.MPS,
                "height_convergence_m": abs((fine.height >> Unit.Meter) - (coarse.height >> Unit.Meter)),
                "time_convergence_s": abs(fine.time - coarse.time),
            }
        )
    return rows


def format_value(value):
    if isinstance(value, float):
        return repr(value)
    return str(value)


def main():
    parser = argparse.ArgumentParser(description="Generate Ogiva reference drop tables with py-ballisticcalc.")
    parser.add_argument("--g1", required=True, help="Ogiva mcg1.txt, checked against the library's G1 table")
    parser.add_argument("--g7", required=True, help="Ogiva mcg7.txt, checked against the library's G7 table")
    parser.add_argument("--out", required=True, help="Output CSV path")
    args = parser.parse_args()

    if py_ballisticcalc.__version__ != EXPECTED_VERSION:
        sys.exit(f"expected py-ballisticcalc {EXPECTED_VERSION}, found {py_ballisticcalc.__version__}")
    check_table("G1", TableG1, args.g1)
    check_table("G7", TableG7, args.g7)

    rows = [row for case in CASES for row in build_rows(case)]
    with open(args.out, "w", encoding="utf-8", newline="\n") as output:
        writer = csv.writer(output, lineterminator="\n")
        writer.writerow(COLUMNS)
        for row in rows:
            writer.writerow(format_value(row[column]) for column in COLUMNS)

    for case in CASES:
        case_rows = [row for row in rows if row["case"] == case.name]
        worst_height = max(row["height_convergence_m"] for row in case_rows)
        worst_time = max(row["time_convergence_s"] for row in case_rows)
        print(
            f"{case.name}: {len(case_rows)} rows to {case_rows[-1]['range_m']:.1f} m, "
            f"height convergence {worst_height:.3e} m, time convergence {worst_time:.3e} s"
        )
    print(f"drag constant ratio vs SI: {2.08551e-04 / 0.3048 / (math.pi / 8.0 * 1.225 / 703.06957964):.9f}")


if __name__ == "__main__":
    main()
