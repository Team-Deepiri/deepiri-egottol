"""Load the optional C++ core extension (egottol._native)."""

from __future__ import annotations

from typing import Any, Callable

_native: Any | None = None
_import_error: str | None = None

try:
    import egottol._native as _native
except ImportError as exc:
    _native = None
    _import_error = str(exc)


def native_available() -> bool:
    return _native is not None and bool(_native.available())


def core_version() -> str | None:
    if _native is None:
        return None
    return str(_native.core_version())


def solve_linear(a, b):
    """Solve Ax=b with the C++ Matrix solver when the extension is built."""
    if _native is None:
        raise RuntimeError(f"egottol._native is not available: {_import_error}")
    return _native.solve_linear(a, b)


def prefer_native() -> bool:
    """True when the desktop/CI build bundled the native extension."""
    return native_available()
