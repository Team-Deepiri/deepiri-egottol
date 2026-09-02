"""Shared helpers for shelling out to the native egottol-cli binary."""

from __future__ import annotations

import shutil
from pathlib import Path


def find_egottol_cli() -> str | None:
    """Return path to egottol-cli on PATH or in a local build/ tree."""
    env = shutil.which("egottol-cli")
    if env:
        return env
    here = Path(__file__).resolve().parents[1]
    candidate = here / "build" / "egottol-cli"
    if candidate.is_file():
        return str(candidate)
    return None
