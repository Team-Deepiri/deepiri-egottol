"""Tests for Egottol Copilot settings persistence."""

import json
from pathlib import Path

import pytest

from egottol.assistant.settings import CopilotSettings


@pytest.fixture
def settings_file(tmp_path: Path) -> Path:
    return tmp_path / "copilot.json"


def test_default_settings_round_trip(settings_file):
    original = CopilotSettings(
        api_keys={"openai": "sk-test"},
        selected_provider="anthropic",
        selected_model="claude-3-5-haiku-latest",
    )
    original.save(settings_file)
    loaded = CopilotSettings.load(settings_file)
    assert loaded.api_keys == original.api_keys
    assert loaded.selected_provider == "anthropic"
    assert loaded.selected_model == "claude-3-5-haiku-latest"


def test_load_missing_file_returns_defaults(tmp_path, monkeypatch):
    monkeypatch.delenv("OPENAI_API_KEY", raising=False)
    monkeypatch.delenv("ANTHROPIC_API_KEY", raising=False)
    missing = tmp_path / "missing.json"
    settings = CopilotSettings.load(missing)
    assert settings.api_keys == {}
    assert settings.selected_provider == "openai"
    assert settings.selected_model == ""


def test_load_corrupt_file_returns_defaults(settings_file):
    settings_file.write_text("{not valid json", encoding="utf-8")
    settings = CopilotSettings.load(settings_file)
    assert settings.api_keys == {}
    assert settings.selected_provider == "openai"


def test_get_api_key_prefers_stored_over_env(settings_file, monkeypatch):
    monkeypatch.setenv("OPENAI_API_KEY", "env-key")
    settings = CopilotSettings(api_keys={"openai": "file-key"})
    assert settings.get_api_key("openai") == "file-key"


def test_get_api_key_falls_back_to_env(monkeypatch):
    monkeypatch.setenv("ANTHROPIC_API_KEY", "env-anthropic")
    settings = CopilotSettings()
    assert settings.get_api_key("anthropic") == "env-anthropic"


def test_get_model_uses_selected_model(settings_file):
    settings = CopilotSettings(
        selected_provider="openai",
        selected_model="gpt-4o",
    )
    assert settings.get_model() == "gpt-4o"


def test_save_creates_parent_directory(tmp_path):
    nested = tmp_path / "cfg" / "egottol" / "copilot.json"
    CopilotSettings(selected_provider="ollama").save(nested)
    assert nested.exists()
    data = json.loads(nested.read_text(encoding="utf-8"))
    assert data["selected_provider"] == "ollama"
