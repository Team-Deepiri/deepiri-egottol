"""Export helpers — waveforms and (later) UQE / Mermaid.

Waveform export shells out to results already produced by ``egottol-cli -o``,
or rewrites a CSV through a thin validation pass.
"""

from __future__ import annotations

import argparse
import csv
import shutil
import subprocess
import sys
from pathlib import Path


def _find_cli() -> str | None:
    env = shutil.which("egottol-cli")
    if env:
        return env
    here = Path(__file__).resolve().parents[1]
    candidate = here / "build" / "egottol-cli"
    if candidate.is_file():
        return str(candidate)
    return None


def export_waveform(circuit: Path, out: Path, mode: str = "tran") -> int:
    cli = _find_cli()
    if cli is None:
        print("egottol-cli not found — build the native CLI first", file=sys.stderr)
        return 1
    flag = {"tran": "--tran", "ac": "--ac", "op": "--op", "dc": "--op"}.get(mode, "--tran")
    proc = subprocess.run([cli, "sim", str(circuit), flag, "-o", str(out)], check=False)
    return int(proc.returncode)


def validate_csv(path: Path) -> bool:
    with path.open(newline="") as f:
        reader = csv.reader(f)
        rows = list(reader)
    return len(rows) >= 2 and rows[0] and rows[0][0].lower().startswith("time")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="python -m egottol.export")
    parser.add_argument("circuit", type=Path, help="Input netlist (.cir)")
    parser.add_argument(
        "--format",
        choices=("csv",),
        default="csv",
        help="Export format (csv only for now; uqe/mermaid are native)",
    )
    parser.add_argument(
        "--mode",
        choices=("tran", "ac", "op", "dc"),
        default="tran",
        help="Analysis whose waveform to export",
    )
    parser.add_argument("-o", "--output", type=Path, required=True, help="Output path")
    args = parser.parse_args(argv)

    if args.format != "csv":
        print(f"Unsupported format: {args.format}", file=sys.stderr)
        return 1

    code = export_waveform(args.circuit, args.output, mode=args.mode)
    if code == 0 and args.output.is_file() and not validate_csv(args.output):
        print(f"Wrote {args.output} but CSV header looks wrong", file=sys.stderr)
        return 2
    return code


if __name__ == "__main__":
    raise SystemExit(main())
