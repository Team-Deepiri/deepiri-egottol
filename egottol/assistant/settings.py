"""Copilot settings persisted to ~/.config/egottol/copilot.json."""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Dict, Optional

from pydantic import BaseModel, Field

from egottol.assistant.providers import ProviderRegistry

CONFIG_DIR = Path.home() / ".config" / "egottol"
CONFIG_PATH = CONFIG_DIR / "copilot.json"
DEFAULT_CONFIG_DIR = CONFIG_DIR
DEFAULT_CONFIG_PATH = CONFIG_PATH


class CopilotSettings(BaseModel):
    """User preferences and API keys for the Egottol Copilot."""

    api_keys: Dict[str, str] = Field(default_factory=dict)
    default_models: Dict[str, str] = Field(default_factory=dict)
    selected_provider: str = "openai"
    selected_model: str = ""
    encrypt_keys: bool = False

    @classmethod
    def load(cls, path: Optional[Path] = None) -> CopilotSettings:
        cfg_path = path or CONFIG_PATH
        if cfg_path.exists():
            try:
                data = json.loads(cfg_path.read_text(encoding="utf-8"))
                return cls.model_validate(data)
            except (json.JSONDecodeError, ValueError):
                return cls()
        settings = cls()
        for name in ProviderRegistry.all_providers():
            env_key = ProviderRegistry.get(name).env_key_name
            env_val = os.environ.get(env_key, "")
            if env_val and name != "ollama":
                settings.api_keys[name] = env_val
        return settings

    def save(self, path: Optional[Path] = None) -> Path:
        cfg_path = path or CONFIG_PATH
        cfg_path.parent.mkdir(parents=True, exist_ok=True)
        cfg_path.write_text(
            self.model_dump_json(indent=2),
            encoding="utf-8",
        )
        return cfg_path

    def get_api_key(self, provider: str) -> str:
        key = self.api_keys.get(provider, "")
        if key:
            return key
        env_key = ProviderRegistry.get(provider).env_key_name
        return os.environ.get(env_key, "")

    def get_model(self, provider: Optional[str] = None) -> str:
        name = provider or self.selected_provider
        if name == self.selected_provider and self.selected_model:
            return self.selected_model
        return self.default_models.get(
            name,
            ProviderRegistry.get_default_model(name),
        )
