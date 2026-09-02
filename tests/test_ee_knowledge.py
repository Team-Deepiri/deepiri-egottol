"""EE knowledge lookup tests."""

from egottol.knowledge import format_lookup, lookup_ee_design


def test_flyback_symptom_hits():
    hits = lookup_ee_design("relay kills MOSFET")
    assert hits
    assert any("Diode" in h["combination"] or "flyback" in h["id"] for h in hits)


def test_led_symptom_hits():
    hits = lookup_ee_design("LED burned out")
    assert hits
    assert hits[0]["id"] == "led-burn"


def test_format_lookup_mentions_doc():
    text = format_lookup("buck converter")
    assert "buck" in text.lower() or "MOSFET" in text
    assert "docs/ee" in text


def test_empty_query():
    assert lookup_ee_design("") == []
