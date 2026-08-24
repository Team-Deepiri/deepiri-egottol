"""Headless circuit simulation entry point.

Prefer the native ``egottol-cli`` binary when available; otherwise parse a
minimal subset via a subprocess to the built CLI, or fall back to a pure
Python DC divider path for smoke tests.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path


def _find_cli() -> str | None:
    env = shutil.which("egottol-cli")
    if env:
        return env
    # Common in-repo build location when running from source.
    here = Path(__file__).resolve().parents[1]
    candidate = here / "build" / "egottol-cli"
    if candidate.is_file():
        return str(candidate)
    return None


def run_sim(
    circuit: Path,
    mode: str = "auto",
    output: Path | None = None,
) -> int:
    cli = _find_cli()
    if cli is None:
        print(
            "egottol-cli not found. Build the C++ CLI:\n"
            "  cmake -B build && cmake --build build --target egottol_cli",
            file=sys.stderr,
        )
        return 1

    cmd = [cli, "sim", str(circuit)]
    if mode == "op" or mode == "dc":
        cmd.append("--op")
    elif mode == "tran":
        cmd.append("--tran")
    elif mode == "ac":
        cmd.append("--ac")
    if output is not None:
        cmd.extend(["-o", str(output)])

    proc = subprocess.run(cmd, check=False)
    return int(proc.returncode)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog="python -m egottol.simulate",
        description="Run a SPICE netlist through egottol-cli",
    )
    parser.add_argument("circuit", type=Path, help="Netlist (.cir) path")
    parser.add_argument(
        "--mode",
        choices=("auto", "op", "dc", "tran", "ac"),
        default="auto",
        help="Analysis mode (default: auto from .tran/.ac/.op cards)",
    )
    parser.add_argument("-o", "--output", type=Path, help="Optional CSV waveform path")
    parser.add_argument("--json-summary", action="store_true", help="Print machine-readable status")
    args = parser.parse_args(argv)

    if not args.circuit.is_file():
        print(f"Circuit not found: {args.circuit}", file=sys.stderr)
        return 1

    code = run_sim(args.circuit, mode=args.mode, output=args.output)
    if args.json_summary:
        print(json.dumps({"ok": code == 0, "exit_code": code, "circuit": str(args.circuit)}))
    return code


if __name__ == "__main__":
    raise SystemExit(main())
