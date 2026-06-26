"""Egottol Copilot — chat orchestration over LLM backends and tools."""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from typing import Any, Callable, Dict, List, Optional

from egottol.assistant.backends import LLMBackend, create_backend
from egottol.assistant.context import ContextBuilder
from egottol.assistant.providers import ProviderRegistry
from egottol.assistant.settings import CopilotSettings
from egottol.assistant.tools import COPILOT_TOOLS, ToolExecutor
from egottol.models.base import Circuit

_CIRCUIT_MUTATING_TOOLS = frozenset({
    "place_component",
    "insert_eii_pipeline",
    "suggest_analog_ai_stack",
    "tune_analog_ai",
    "optimize_crossbar",
    "auto_tune_circuit",
})

_SIM_UPDATING_TOOLS = frozenset({
    "run_sim",
    "run_eii_sim",
    "run_nsp",
    "analyze_spikes",
    "auto_tune_circuit",
})


@dataclass
class CopilotResponse:
    """User-facing copilot reply."""

    message: str
    tool_results: List[Dict[str, Any]] = field(default_factory=list)
    circuit_changed: bool = False
    raw: Dict[str, Any] = field(default_factory=dict)


class Copilot:
    """Circuit-aware assistant using a selectable LLM provider."""

    MAX_TOOL_ROUNDS = 5

    def __init__(
        self,
        circuit: Circuit,
        settings: CopilotSettings | None = None,
        sim_results: Dict[str, Any] | None = None,
        on_circuit_changed: Callable[[], None] | None = None,
    ):
        self.circuit = circuit
        self.settings = settings or CopilotSettings.load()
        self.sim_results = sim_results or {}
        self.on_circuit_changed = on_circuit_changed
        self._history: List[Dict[str, str]] = []
        self._backend: LLMBackend | None = None

    @property
    def executor(self) -> ToolExecutor:
        return ToolExecutor(
            self.circuit,
            sim_results=self.sim_results,
            on_circuit_changed=self._mark_circuit_changed,
        )

    def _mark_circuit_changed(self) -> None:
        if self.on_circuit_changed:
            self.on_circuit_changed()

    def _resolve_backend(self) -> LLMBackend:
        provider = self.settings.selected_provider
        model = self.settings.get_model(provider)
        api_key = self.settings.get_api_key(provider)
        return create_backend(provider, api_key, model)

    def _build_messages(self, user_message: str) -> List[Dict[str, Any]]:
        ctx = ContextBuilder(self.circuit, self.sim_results)
        messages: List[Dict[str, Any]] = [
            {"role": "system", "content": ctx.build_system_message()},
        ]
        messages.extend(self._history)
        messages.append({"role": "user", "content": user_message})
        return messages

    async def chat(self, message: str) -> CopilotResponse:
        """Send a user message and return the assistant response."""
        self._backend = self._resolve_backend()
        messages = self._build_messages(message)
        tool_results: List[Dict[str, Any]] = []
        circuit_changed = False
        final_text = ""

        for _ in range(self.MAX_TOOL_ROUNDS):
            response = await self._backend.chat(messages, tools=COPILOT_TOOLS)

            if not response.tool_calls:
                final_text = response.content
                break

            assistant_content = response.content or ""
            messages.append(
                {
                    "role": "assistant",
                    "content": assistant_content,
                    "tool_calls": [
                        {
                            "id": tc.id,
                            "type": "function",
                            "function": {
                                "name": tc.name,
                                "arguments": json.dumps(tc.arguments),
                            },
                        }
                        for tc in response.tool_calls
                    ],
                }
            )

            for tc in response.tool_calls:
                result = await self.executor.execute(tc.name, tc.arguments)
                tool_results.append({"tool": tc.name, "arguments": tc.arguments, "result": result})
                if tc.name in _CIRCUIT_MUTATING_TOOLS:
                    circuit_changed = True
                if tc.name in _SIM_UPDATING_TOOLS and result.get("ok"):
                    self.sim_results = self.executor.sim_results

                messages.append(
                    {
                        "role": "tool",
                        "tool_call_id": tc.id,
                        "content": json.dumps(result),
                    }
                )

            if response.content:
                final_text = response.content
        else:
            final_text = final_text or "Reached maximum tool rounds."

        self._history.append({"role": "user", "content": message})
        self._history.append({"role": "assistant", "content": final_text})

        return CopilotResponse(
            message=final_text,
            tool_results=tool_results,
            circuit_changed=circuit_changed,
            raw={"provider": self.settings.selected_provider, "model": self.settings.selected_model},
        )

    def update_context(
        self,
        circuit: Circuit | None = None,
        sim_results: Dict[str, Any] | None = None,
    ) -> None:
        if circuit is not None:
            self.circuit = circuit
        if sim_results is not None:
            self.sim_results = sim_results

    async def test_connection_async(
        self,
        provider: str,
        api_key: str | None = None,
        model: str | None = None,
    ) -> tuple[bool, str]:
        cfg = ProviderRegistry.get(provider)
        key = api_key if api_key is not None else self.settings.get_api_key(provider)
        mdl = model or self.settings.get_model(provider) or cfg.default_model
        backend = create_backend(provider, key, mdl)
        return await backend.test_connection()

    def test_connection(
        self,
        provider: str,
        api_key: str | None = None,
        model: str | None = None,
    ) -> tuple[bool, str]:
        import asyncio

        return asyncio.run(self.test_connection_async(provider, api_key, model))

    def chat_sync(self, message: str) -> CopilotResponse:
        import asyncio

        return asyncio.run(self.chat(message))

    def update_settings(self, settings: CopilotSettings) -> None:
        self.settings = settings
        settings.save()

    def clear_history(self) -> None:
        self._history.clear()
