"""PyInstaller entrypoint for the Egottol PyQt6 desktop app."""

from __future__ import annotations

import runpy

if __name__ == "__main__":
    runpy.run_module("egottol.ui.main", run_name="__main__")
