#!/usr/bin/env python3
"""Generate C++ EE lookup table from egottol/knowledge/ee_symptoms.json."""

from __future__ import annotations

import json
import sys
from pathlib import Path


def c_escape(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <ee_symptoms.json> <output.inc>", file=sys.stderr)
        return 1

    src = Path(sys.argv[1])
    out = Path(sys.argv[2])
    data = json.loads(src.read_text(encoding="utf-8"))
    entries = data.get("entries", [])

    lines = [
        "// Generated from egottol/knowledge/ee_symptoms.json — do not edit.",
        f"// Source: {src.name}",
        "static const Entry kEntries[] = {",
    ]
    for e in entries:
        symptoms = " ".join(e.get("symptoms", []))
        lines.append(
            '    {'
            f'"{c_escape(e["id"])}", '
            f'"{c_escape(symptoms)}", '
            f'"{c_escape(e.get("combination", ""))}", '
            f'"{c_escape(e.get("behavior", ""))}", '
            f'"{c_escape(e.get("use", ""))}"'
            '},'
        )
    lines.append("};")
    lines.append(f"static const size_t kEntryCount = {len(entries)};")
    lines.append("")

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
