"""Egottol Copilot assistant package."""

from egottol.assistant.backends import (
    AnthropicBackend,
    ChatResponse,
    GeminiBackend,
    LLMBackend,
    OllamaBackend,
    OpenAIBackend,
    OpenRouterBackend,
    RuleBasedBackend,
    ToolCall,
    create_backend,
    get_backend,
)
from egottol.assistant.context import ContextBuilder
from egottol.assistant.copilot import Copilot, CopilotResponse
from egottol.assistant.providers import PROVIDER_CONFIGS, ProviderConfig, ProviderRegistry
from egottol.assistant.settings import CONFIG_PATH, CopilotSettings
from egottol.assistant.tools import COPILOT_TOOLS, ToolExecutor

__all__ = [
    "AnthropicBackend",
    "CONFIG_PATH",
    "ChatResponse",
    "COPILOT_TOOLS",
    "ContextBuilder",
    "Copilot",
    "CopilotResponse",
    "CopilotSettings",
    "GeminiBackend",
    "LLMBackend",
    "OllamaBackend",
    "OpenAIBackend",
    "OpenRouterBackend",
    "PROVIDER_CONFIGS",
    "ProviderConfig",
    "ProviderRegistry",
    "RuleBasedBackend",
    "ToolCall",
    "ToolExecutor",
    "create_backend",
    "get_backend",
]
