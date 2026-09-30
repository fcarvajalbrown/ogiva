import argparse
import pathlib
import subprocess
import sys


def read_table(path):
    machs = []
    coefficients = []
    for line_number, line in enumerate(path.read_text(encoding="ascii").splitlines(), start=1):
        if not line.strip():
            continue
        fields = line.split()
        if len(fields) != 2:
            sys.exit(f"{path}:{line_number}: expected 2 columns, got {len(fields)}")
        for field in fields:
            float(field)
        machs.append(fields[0])
        coefficients.append(fields[1])
    return machs, coefficients


def emit_array(name, values):
    lines = [f"\tinline constexpr std::array<double, {len(values)}> {name}{{"]
    lines += [f"\t\t{value}," for value in values]
    lines.append("\t};")
    return lines


def main():
    parser = argparse.ArgumentParser(description="Generate OgivaCore reference drag arrays from Mach/Cd text tables.")
    parser.add_argument("--g1", type=pathlib.Path, required=True)
    parser.add_argument("--g7", type=pathlib.Path, required=True)
    parser.add_argument("--out", type=pathlib.Path, required=True)
    parser.add_argument("--clang-format", default="clang-format")
    args = parser.parse_args()

    lines = ["#pragma once", "", "#include <array>", "", "namespace Ogiva::ReferenceDragData", "{"]
    for label, path in (("G1", args.g1), ("G7", args.g7)):
        machs, coefficients = read_table(path)
        lines += emit_array(f"{label}Mach", machs)
        lines.append("")
        lines += emit_array(f"{label}DragCoefficient", coefficients)
        lines.append("")
    lines[-1] = "}"
    args.out.write_text("\n".join(lines) + "\n", encoding="ascii", newline="\n")
    subprocess.run([args.clang_format, "-i", str(args.out)], check=True)


if __name__ == "__main__":
    main()
