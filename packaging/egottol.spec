# -*- mode: python ; coding: utf-8 -*-
"""PyInstaller spec for deepiri-egottol (PyQt6 schematic lab)."""

from __future__ import annotations

import sys
from pathlib import Path

from PyInstaller.utils.hooks import collect_data_files, collect_submodules

ROOT = Path(SPECPATH).resolve().parent.parent

datas = [
    (str(ROOT / "io" / "eii_weights.schema.json"), "io"),
    (str(ROOT / "vhdl"), "vhdl"),
    (str(ROOT / "egottol" / "vhdl"), "egottol/vhdl"),
]

hiddenimports = [
    "PyQt6.QtCore",
    "PyQt6.QtGui",
    "PyQt6.QtWidgets",
    "pyqtgraph",
    "numpy",
    "scipy",
]
hiddenimports += collect_submodules("pyqtgraph")

a = Analysis(
    [str(ROOT / "packaging" / "egottol_entry.py")],
    pathex=[str(ROOT)],
    binaries=[],
    datas=datas,
    hiddenimports=hiddenimports,
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=["cupy", "onnxruntime", "onnx", "pytest"],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name="Egottol",
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=False,
    console=False,
)

coll = COLLECT(
    exe,
    a.binaries,
    a.zipfiles,
    a.datas,
    strip=False,
    upx=False,
    upx_exclude=[],
    name="Egottol",
)

if sys.platform == "darwin":
    app = BUNDLE(
        coll,
        name="Egottol.app",
        icon=None,
        bundle_identifier="com.deepiri.egottol",
    )
