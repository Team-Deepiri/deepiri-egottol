"""LLM backend implementations using aiohttp."""

from __future__ import annotations

import json
import re
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import Any, Dict, List

import aiohttp

from egottol.assistant.providers import ProviderConfig, ProviderRegistry


@dataclass
class ToolCall:
    id: str
    name: str
    arguments: Dict[str, Any]


@dataclass
class ChatResponse:
    content: str = ""
    tool_calls: List[ToolCall] = field(default_factory=list)
    raw: Dict[str, Any] = field(default_factory=dict)
    finish_reason: str = "stop"


class LLMBackend(ABC):
    """Abstract base for chat-completion backends."""

    def __init__(self, config: ProviderConfig, api_key: str, model: str):
        self.config = config
        self.api_key = api_key
        self.model = model

    @abstractmethod
    async def chat(
        self,
        messages: List[Dict[str, Any]],
        tools: List[Dict[str, Any]] | None = None,
    ) -> ChatResponse:
        ...

    async def complete(self, messages: List[Dict[str, str]]) -> str:
        """Simple text completion without tool routing."""
        response = await self.chat(messages)
        return response.content

    async def test_connection(self) -> tuple[bool, str]:
        try:
            reply = await self.complete([{"role": "user", "content": "Reply with OK"}])
            return True, reply.strip()[:120] or "Connected"
        except Exception as exc:
            return False, str(exc)

    async def _post_json(
        self,
        url: str,
        payload: Dict[str, Any],
        headers: Dict[str, str],
        timeout: int = 120,
    ) -> Dict[str, Any]:
        async with aiohttp.ClientSession() as session:
            async with session.post(
                url,
                json=payload,
                headers=headers,
                timeout=aiohttp.ClientTimeout(total=timeout),
            ) as resp:
                body = await resp.text()
                if resp.status >= 400:
                    raise RuntimeError(f"LLM request failed ({resp.status}): {body[:500]}")
                return json.loads(body)


class OpenAIBackend(LLMBackend):
    """OpenAI Chat Completions API."""

    async def chat(
        self,
        messages: List[Dict[str, Any]],
        tools: List[Dict[str, Any]] | None = None,
    ) -> ChatResponse:
        payload: Dict[str, Any] = {"model": self.model, "messages": messages}
        if tools:
            payload["tools"] = tools
            payload["tool_choice"] = "auto"

        data = await self._post_json(
            f"{self.config.base_url.rstrip('/')}/chat/completions",
            payload,
            {
                "Authorization": f"Bearer {self.api_key}",
                "Content-Type": "application/json",
            },
        )
        choice = data["choices"][0]
        message = choice.get("message", {})
        tool_calls = [
            ToolCall(
                id=tc["id"],
                name=tc["function"]["name"],
                arguments=json.loads(tc["function"].get("arguments") or "{}"),
            )
            for tc in message.get("tool_calls") or []
        ]
        return ChatResponse(
            content=message.get("content") or "",
            tool_calls=tool_calls,
            raw=data,
            finish_reason=choice.get("finish_reason", "stop"),
        )


class AnthropicBackend(LLMBackend):
    """Anthropic Messages API."""

    async def chat(
        self,
        messages: List[Dict[str, Any]],
        tools: List[Dict[str, Any]] | None = None,
    ) -> ChatResponse:
        system_parts: List[str] = []
        api_messages: List[Dict[str, Any]] = []
        for msg in messages:
            if msg["role"] == "system":
                system_parts.append(msg["content"])
            elif msg["role"] == "tool":
                api_messages.append(
                    {
                        "role": "user",
                        "content": [
                            {
                                "type": "tool_result",
                                "tool_use_id": msg.get("tool_call_id", ""),
                                "content": msg.get("content", ""),
                            }
                        ],
                    }
                )
            else:
                api_messages.append({"role": msg["role"], "content": msg["content"]})

        payload: Dict[str, Any] = {
            "model": self.model,
            "max_tokens": 4096,
            "messages": api_messages or [{"role": "user", "content": "Hello"}],
        }
        if system_parts:
            payload["system"] = "\n\n".join(system_parts)
        if tools:
            payload["tools"] = [
                {
                    "name": t["function"]["name"],
                    "description": t["function"].get("description", ""),
                    "input_schema": t["function"].get(
                        "parameters", {"type": "object", "properties": {}}
                    ),
                }
                for t in tools
            ]

        data = await self._post_json(
            f"{self.config.base_url.rstrip('/')}/messages",
            payload,
            {
                "x-api-key": self.api_key,
                "anthropic-version": "2023-06-01",
                "Content-Type": "application/json",
            },
        )

        text_parts: List[str] = []
        tool_calls: List[ToolCall] = []
        for block in data.get("content", []):
            if block.get("type") == "text":
                text_parts.append(block.get("text", ""))
            elif block.get("type") == "tool_use":
                tool_calls.append(
                    ToolCall(
                        id=block["id"],
                        name=block["name"],
                        arguments=block.get("input") or {},
                    )
                )

        return ChatResponse(
            content="".join(text_parts),
            tool_calls=tool_calls,
            raw=data,
            finish_reason=data.get("stop_reason", "end_turn"),
        )


class GeminiBackend(LLMBackend):
    """Google Gemini generateContent API."""

    async def chat(
        self,
        messages: List[Dict[str, Any]],
        tools: List[Dict[str, Any]] | None = None,
    ) -> ChatResponse:
        contents: List[Dict[str, Any]] = []
        system_instruction: str | None = None
        for msg in messages:
            if msg["role"] == "system":
                system_instruction = msg["content"]
                continue
            if msg["role"] == "tool":
                continue
            role = "model" if msg["role"] == "assistant" else "user"
            contents.append({"role": role, "parts": [{"text": msg["content"]}]})

        payload: Dict[str, Any] = {
            "contents": contents or [{"role": "user", "parts": [{"text": "Hello"}]}]
        }
        if system_instruction:
            payload["systemInstruction"] = {"parts": [{"text": system_instruction}]}
        if tools:
            payload["tools"] = [
                {
                    "functionDeclarations": [
                        {
                            "name": t["function"]["name"],
                            "description": t["function"].get("description", ""),
                            "parameters": t["function"].get(
                                "parameters", {"type": "object", "properties": {}}
                            ),
                        }
                        for t in tools
                    ]
                }
            ]

        url = (
            f"{self.config.base_url.rstrip('/')}/models/{self.model}:generateContent"
            f"?key={self.api_key}"
        )
        data = await self._post_json(url, payload, {"Content-Type": "application/json"})

        candidate = (data.get("candidates") or [{}])[0]
        content = candidate.get("content", {})
        text_parts: List[str] = []
        tool_calls: List[ToolCall] = []
        for part in content.get("parts", []):
            if "text" in part:
                text_parts.append(part["text"])
            elif "functionCall" in part:
                fc = part["functionCall"]
                tool_calls.append(
                    ToolCall(
                        id=fc.get("name", "gemini_call"),
                        name=fc["name"],
                        arguments=fc.get("args") or {},
                    )
                )

        return ChatResponse(
            content="".join(text_parts),
            tool_calls=tool_calls,
            raw=data,
            finish_reason=candidate.get("finishReason", "STOP"),
        )


class OpenRouterBackend(OpenAIBackend):
    """OpenRouter — OpenAI-compatible chat completions."""

    async def chat(
        self,
        messages: List[Dict[str, Any]],
        tools: List[Dict[str, Any]] | None = None,
    ) -> ChatResponse:
        payload: Dict[str, Any] = {"model": self.model, "messages": messages}
        if tools:
            payload["tools"] = tools
            payload["tool_choice"] = "auto"

        data = await self._post_json(
            f"{self.config.base_url.rstrip('/')}/chat/completions",
            payload,
            {
                "Authorization": f"Bearer {self.api_key}",
                "Content-Type": "application/json",
                "HTTP-Referer": "https://github.com/deepiri/deepiri-egottol",
                "X-Title": "Egottol Copilot",
            },
        )
        choice = data["choices"][0]
        message = choice.get("message", {})
        tool_calls = [
            ToolCall(
                id=tc.get("id") or tc["function"]["name"],
                name=tc["function"]["name"],
                arguments=json.loads(tc["function"].get("arguments") or "{}"),
            )
            for tc in message.get("tool_calls") or []
        ]
        return ChatResponse(
            content=message.get("content") or "",
            tool_calls=tool_calls,
            raw=data,
            finish_reason=choice.get("finish_reason", "stop"),
        )


class OllamaBackend(LLMBackend):
    """Local Ollama /api/chat endpoint."""

    async def chat(
        self,
        messages: List[Dict[str, Any]],
        tools: List[Dict[str, Any]] | None = None,
    ) -> ChatResponse:
        payload: Dict[str, Any] = {
            "model": self.model,
            "messages": messages,
            "stream": False,
        }
        if tools:
            payload["tools"] = [
                {
                    "type": "function",
                    "function": {
                        "name": t["function"]["name"],
                        "description": t["function"].get("description", ""),
                        "parameters": t["function"].get(
                            "parameters", {"type": "object", "properties": {}}
                        ),
                    },
                }
                for t in tools
            ]

        headers = {"Content-Type": "application/json"}
        if self.api_key:
            headers["Authorization"] = f"Bearer {self.api_key}"

        data = await self._post_json(
            f"{self.config.base_url.rstrip('/')}/api/chat",
            payload,
            headers,
        )
        message = data.get("message", {})
        tool_calls = [
            ToolCall(
                id=tc.get("function", {}).get("name", "ollama_call"),
                name=tc["function"]["name"],
                arguments=tc["function"].get("arguments") or {},
            )
            for tc in message.get("tool_calls") or []
        ]
        return ChatResponse(
            content=message.get("content") or "",
            tool_calls=tool_calls,
            raw=data,
            finish_reason="stop",
        )

    async def test_connection(self) -> tuple[bool, str]:
        url = f"{self.config.base_url.rstrip('/')}/api/tags"
        async with aiohttp.ClientSession() as session:
            async with session.get(url, timeout=aiohttp.ClientTimeout(total=10)) as resp:
                if resp.status >= 400:
                    body = await resp.text()
                    return False, f"HTTP {resp.status}: {body[:200]}"
                data = await resp.json()
                models = [m.get("name", "") for m in data.get("models", [])]
                if models:
                    return True, f"Ollama online ({len(models)} models)"
                return True, "Ollama online"


class RuleBasedBackend(LLMBackend):
    """Keyword-driven fallback when no LLM API key is available."""

    _PATTERNS: List[tuple[re.Pattern[str], str]] = [
        (re.compile(r"\b(summarize|summary|describe|overview)\b", re.I), "get_circuit_summary"),
        (re.compile(r"\b(run|start|execute)\b.*\b(sim|simulation)\b", re.I), "run_sim"),
        (re.compile(r"\bplace\b.*\b(resistor|cap|capacitor|inductor|gate)\b", re.I), "place_component"),
        (re.compile(r"\b(waveform|oscilloscope|trace|signal)\b", re.I), "explain_waveform"),
        (re.compile(r"\b(spice|\.tran|\.ac|\.dc|\.step)\b", re.I), "exec_spice_cmd"),
        (re.compile(r"\b(eii|pipeline|avionics)\b", re.I), "insert_eii_pipeline"),
    ]

    async def chat(
        self,
        messages: List[Dict[str, Any]],
        tools: List[Dict[str, Any]] | None = None,
    ) -> ChatResponse:
        user_text = ""
        for msg in reversed(messages):
            if msg.get("role") == "user":
                user_text = msg.get("content", "")
                break

        for pattern, tool_name in self._PATTERNS:
            if pattern.search(user_text):
                return ChatResponse(
                    content=f"I'll use `{tool_name}` to help with that.",
                    tool_calls=[
                        ToolCall(
                            id=f"rule_{tool_name}",
                            name=tool_name,
                            arguments=_default_args(tool_name, user_text),
                        )
                    ],
                    finish_reason="tool_calls",
                )

        return ChatResponse(
            content=(
                "Egottol Copilot (offline mode). I can help you place components, "
                "run simulations, explain waveforms, run SPICE commands, or summarize "
                "your circuit. Try: 'summarize the circuit' or 'run simulation'."
            ),
            finish_reason="stop",
        )

    async def test_connection(self) -> tuple[bool, str]:
        return True, "Rule-based fallback available"


def _default_args(tool_name: str, user_text: str) -> Dict[str, Any]:
    if tool_name == "place_component":
        key = "RES"
        if re.search(r"capacitor|\bcap\b", user_text, re.I):
            key = "CAP"
        elif re.search(r"inductor|\bind\b", user_text, re.I):
            key = "IND"
        elif re.search(r"gate|logic", user_text, re.I):
            key = "AND"
        return {"component_key": key, "x": 0.0, "y": 0.0}
    if tool_name == "explain_waveform":
        return {"signal": "output", "node": None}
    if tool_name == "exec_spice_cmd":
        return {"command": user_text.strip()}
    if tool_name == "insert_eii_pipeline":
        return {"pipeline_type": "adsb_decode", "target_component": None}
    return {}


BACKEND_MAP = {
    "openai": OpenAIBackend,
    "anthropic": AnthropicBackend,
    "gemini": GeminiBackend,
    "openrouter": OpenRouterBackend,
    "ollama": OllamaBackend,
}


def get_backend(provider: str, api_key: str, model: str) -> LLMBackend:
    config = ProviderRegistry.get(provider)
    if not api_key and provider != "ollama":
        return RuleBasedBackend(config, api_key, model)
    cls = BACKEND_MAP.get(provider, OpenAIBackend)
    return cls(config, api_key, model)


create_backend = get_backend
