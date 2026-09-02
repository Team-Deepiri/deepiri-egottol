"""Electrical design knowledge index — symptom → combination lookup."""

from __future__ import annotations

import json
import re
from functools import lru_cache
from pathlib import Path
from typing import Any, Dict, List

_DATA = Path(__file__).resolve().parent / "ee_symptoms.json"


@lru_cache(maxsize=1)
def _load() -> Dict[str, Any]:
    with _DATA.open(encoding="utf-8") as f:
        return json.load(f)


def lookup_ee_design(query: str, limit: int = 5) -> List[Dict[str, Any]]:
    """Return ranked EE design entries matching *query* (symptom keywords)."""
    q = (query or "").strip().lower()
    if not q:
        return []
    tokens = set(re.findall(r"[a-z0-9.+-]+", q))
    scored: List[tuple[int, Dict[str, Any]]] = []
    for entry in _load().get("entries", []):
        bag = " ".join(entry.get("symptoms", [])).lower()
        bag += " " + entry.get("combination", "").lower()
        bag += " " + entry.get("use", "").lower()
        score = 0
        for t in tokens:
            if t in bag:
                score += 2 if len(t) > 2 else 1
        for phrase in entry.get("symptoms", []):
            if phrase.lower() in q:
                score += 5
        if score:
            scored.append((score, entry))
    scored.sort(key=lambda x: (-x[0], x[1].get("id", "")))
    return [e for _, e in scored[: max(1, limit)]]


def format_lookup(query: str, limit: int = 5) -> str:
    hits = lookup_ee_design(query, limit=limit)
    if not hits:
        return (
            f"No EE design hits for {query!r}. "
            "Try: flyback, LED current, buck, crystal, floorplan, H-bridge."
        )
    lines = [f"EE design matches for {query!r}:"]
    for h in hits:
        lines.append(
            f"- [{h.get('id')}] {h.get('combination')}\n"
            f"  Behavior: {h.get('behavior')}\n"
            f"  Use: {h.get('use')}\n"
            f"  Doc: {h.get('doc')}"
        )
    return "\n".join(lines)
