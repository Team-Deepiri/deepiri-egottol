from dataclasses import dataclass
from typing import Dict, List


@dataclass(frozen=True)
class ProviderConfig:
    name: str
    label: str
    base_url: str
    default_model: str
    env_key_name: str


PROVIDER_CONFIGS: Dict[str, ProviderConfig] = {
    "openai": ProviderConfig(
        name="openai",
        label="OpenAI",
        base_url="https://api.openai.com/v1",
        default_model="gpt-4o-mini",
        env_key_name="OPENAI_API_KEY",
    ),
    "anthropic": ProviderConfig(
        name="anthropic",
        label="Anthropic",
        base_url="https://api.anthropic.com/v1",
        default_model="claude-3-5-haiku-20241022",
        env_key_name="ANTHROPIC_API_KEY",
    ),
    "gemini": ProviderConfig(
        name="gemini",
        label="Gemini",
        base_url="https://generativelanguage.googleapis.com/v1beta",
        default_model="gemini-2.0-flash",
        env_key_name="GEMINI_API_KEY",
    ),
    "openrouter": ProviderConfig(
        name="openrouter",
        label="OpenRouter",
        base_url="https://openrouter.ai/api/v1",
        default_model="openai/gpt-4o-mini",
        env_key_name="OPENROUTER_API_KEY",
    ),
    "ollama": ProviderConfig(
        name="ollama",
        label="Ollama",
        base_url="http://127.0.0.1:11434",
        default_model="llama3.2",
        env_key_name="OLLAMA_HOST",
    ),
}


class ProviderRegistry:
    @staticmethod
    def all_providers() -> List[str]:
        return list(PROVIDER_CONFIGS.keys())

    @staticmethod
    def get(name: str) -> ProviderConfig:
        if name not in PROVIDER_CONFIGS:
            raise KeyError(f"Unknown provider: {name}")
        return PROVIDER_CONFIGS[name]

    @staticmethod
    def get_default_model(name: str) -> str:
        return ProviderRegistry.get(name).default_model

    @staticmethod
    def labels() -> Dict[str, str]:
        return {k: v.label for k, v in PROVIDER_CONFIGS.items()}
