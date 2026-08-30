#!/usr/bin/env python3
"""OWzx AI sidecar — Claude Agent SDK harness driving OWzx Slicer over MCP.

Decision record: docs/ai-control.md (OWzx-only feature). The Qt app runs an
MCP server (src/core/ai/McpHttpServer.cpp) exposing the whole slicer as tools;
this sidecar is the agent harness the app embeds: Claude Agent SDK (bundled
claude.exe CLI) pointed at Zhipu's Anthropic-compatible endpoint running GLM.

Protocol (NDJSON over stdio, UTF-8):
  host -> sidecar commands:
    {"type":"send","text":"..."}                 run one user turn (awaits turn_done)
    {"type":"cancel"}                            interrupt the running turn
    {"type":"reset"}                             drop the conversation (new session)
    {"type":"answer_permission","callId":N,"allow":true}
  sidecar -> host events:
    {"type":"ready","sdk":"<version>"}           harness connected (after MCP connect)
    {"type":"assistant_text","text":"..."}       assistant text segment
    {"type":"tool_use","id":"...","name":"...","input":{...}}
    {"type":"tool_result","id":"...","ok":true,"summary":"..."}
    {"type":"permission_request","callId":N,"tool":"...","input":{...}}
    {"type":"turn_done","isError":false,"durationMs":123,"sessionId":"..."}
    {"type":"error","message":"..."}             fatal/turn error

Environment injected by the host:
  OWZX_MCP_URL       http://127.0.0.1:<port>/mcp
  OWZX_MCP_TOKEN     bearer token for that server
  OWZX_MODEL         model id (default: glm-5.3-flash)
  ANTHROPIC_BASE_URL https://open.bigmodel.cn/api/anthropic (Zhipu compat)
  ANTHROPIC_AUTH_TOKEN  <GLM api key>  (never logged by this process)
"""

from __future__ import annotations

import asyncio
import json
import os
import sys
import threading

MAX_TURNS = 24  # bound one user turn's tool loop (harness safety valve)

# ── stdio plumbing ───────────────────────────────────────────────────────────


def emit(event: dict) -> None:
    sys.stdout.write(json.dumps(event, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def _tool_summary(content) -> str:
    """Compact one-line summary of a tool result for the host's UI cards."""
    try:
        if isinstance(content, str):
            return content[:300]
        if isinstance(content, list):
            for block in content:
                if isinstance(block, dict) and block.get("type") == "text":
                    return str(block.get("text", ""))[:300]
        if content is None:
            return ""
        return json.dumps(content, ensure_ascii=False)[:300]
    except Exception:
        return ""


# ── harness ──────────────────────────────────────────────────────────────────


def _emit_message_events(msg, emit_fn) -> None:
    """Translate one SDK receive_response() message into host NDJSON events.

    claude-agent-sdk 0.2.147 yields dataclass instances (AssistantMessage with
    .content blocks being TextBlock/ToolUseBlock objects, UserMessage carrying
    ToolResultBlock objects, ResultMessage with cost/usage fields) -- NOT dicts.
    Parsing lives at module level so the offline selftest can exercise it with
    constructed instances (a dict-style .get() crash here once escaped the
    selftest because no live turn had ever run).
    """
    from claude_agent_sdk import (
        AssistantMessage,
        ResultMessage,
        TextBlock,
        ToolResultBlock,
        ToolUseBlock,
        UserMessage,
    )

    if isinstance(msg, AssistantMessage):
        for block in (msg.content or []):
            if isinstance(block, TextBlock):
                if block.text:
                    emit_fn({"type": "assistant_text", "text": block.text})
            elif isinstance(block, ToolUseBlock):
                emit_fn({
                    "type": "tool_use",
                    "id": block.id or "",
                    "name": block.name or "",
                    "input": block.input or {},
                })
    elif isinstance(msg, UserMessage):
        content = msg.content
        blocks = content if isinstance(content, list) else []
        for block in blocks:
            if isinstance(block, ToolResultBlock):
                emit_fn({
                    "type": "tool_result",
                    "id": block.tool_use_id or "",
                    "ok": not block.is_error,
                    "summary": _tool_summary(block.content),
                })
    elif isinstance(msg, ResultMessage):
        emit_fn({
            "type": "turn_done",
            "isError": bool(msg.is_error),
            "durationMs": int(msg.duration_ms or 0),
            "sessionId": msg.session_id or "",
            "result": msg.result or "",
        })


class Harness:
    def __init__(self) -> None:
        self.mcp_url = os.environ.get("OWZX_MCP_URL", "")
        self.mcp_token = os.environ.get("OWZX_MCP_TOKEN", "")
        self.model = os.environ.get("OWZX_MODEL", "glm-5.3-flash")
        self.client = None
        self.busy = False
        self.turn_task: asyncio.Task | None = None
        self._perm_futures: dict[int, asyncio.Future] = {}
        self._perm_counter = 0
        self._loop: asyncio.AbstractEventLoop | None = None
        # tool name -> readOnlyHint from the app's MCP tools/list annotations.
        # Read-only tools are auto-allowed; only destructive tools (per the
        # approved plan) interrupt the user with a confirmation card.
        self._readonly_tools: set[str] = set()

    def _refresh_readonly_tools(self, retries: int = 3) -> None:
        """Fetch MCP annotations so can_use_tool can auto-allow read-only tools.

        Retried: right after launch the app's main thread may still be busy
        loading QML, and the loopback MCP server lives in that event loop —
        the first tools/list can time out even though nothing is wrong.
        """
        import time
        import urllib.request

        # Proxy-free opener: this request is loopback. Machines that export
        # HTTP(S)_PROXY without NO_PROXY would otherwise route it through the
        # proxy and time out.
        opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
        request = urllib.request.Request(
            self.mcp_url,
            data=json.dumps({"jsonrpc": "2.0", "id": 900, "method": "tools/list"}).encode(),
            headers={
                "Content-Type": "application/json",
                "Authorization": f"Bearer {self.mcp_token}",
            },
            method="POST",
        )
        last_exc: Exception | None = None
        for attempt in range(retries):
            try:
                with opener.open(request, timeout=5) as resp:
                    payload = json.loads(resp.read().decode("utf-8"))
                for tool in payload.get("result", {}).get("tools", []):
                    ann = tool.get("annotations") or {}
                    if ann.get("readOnlyHint"):
                        self._readonly_tools.add(tool.get("name", ""))
                return
            except Exception as exc:  # noqa: BLE001 -- retry, then fall back
                last_exc = exc
                if attempt + 1 < retries:
                    time.sleep(2)
        # Non-fatal fallback: without annotations every tool asks for
        # confirmation. Log to stderr (sidecar log) instead of emitting an
        # error event so the chat transcript stays clean.
        print(f"[sidecar] tools/list annotations unavailable after "
              f"{retries} attempts: {last_exc}", file=sys.stderr, flush=True)

    # -- permission bridge: SDK callback -> host card -> stdin answer --------
    async def can_use_tool(self, tool_name, tool_input, context):  # noqa: ANN001
        from claude_agent_sdk import PermissionResultAllow, PermissionResultDeny

        if tool_name in self._readonly_tools:
            return PermissionResultAllow(behavior="allow")
        # The SDK presents MCP tools as mcp__<server>__<name> while the
        # server's tools/list returns bare names -- match both forms.
        if tool_name.startswith("mcp__") and tool_name.split("__", 2)[-1] in self._readonly_tools:
            return PermissionResultAllow(behavior="allow")

        self._perm_counter += 1
        call_id = self._perm_counter
        fut: asyncio.Future = self._loop.create_future()
        self._perm_futures[call_id] = fut
        emit({
            "type": "permission_request",
            "callId": call_id,
            "tool": tool_name,
            "input": tool_input,
        })
        allowed = await fut
        self._perm_futures.pop(call_id, None)
        if allowed:
            return PermissionResultAllow(behavior="allow")
        return PermissionResultDeny(
            behavior="deny", message="The user denied this action in OWzx Slicer."
        )

    def answer_permission(self, call_id: int, allow: bool) -> None:
        fut = self._perm_futures.get(int(call_id))
        if fut is not None and not fut.done():
            fut.set_result(bool(allow))

    # -- lifecycle ------------------------------------------------------------
    def _build_options(self):  # noqa: ANN202
        from claude_agent_sdk import ClaudeAgentOptions

        mcp = {
            "owzx": {
                "type": "http",
                "url": self.mcp_url,
                "headers": {"Authorization": f"Bearer {self.mcp_token}"},
            }
        }
        return ClaudeAgentOptions(
            # This is an in-app controller, not a general coding agent. Keep
            # Claude's filesystem/shell/browser tool surface out of the model
            # context; all application actions must use OWzx MCP tools.
            tools=[],
            # NOTE: allowed_tools must stay EMPTY. SDK patterns listed there are
            # pre-approved WITHOUT prompting, which bypasses can_use_tool and
            # silently auto-allowed destructive tools (delete_object executed
            # with no confirmation card). Gating lives entirely in can_use_tool:
            # annotations auto-allow read-only tools; destructive tools wait for
            # the in-app confirmation card.
            allowed_tools=[],
            system_prompt=(
                "You are the OWzx Slicer in-app assistant. For every request "
                "about the slicer, project, models, plates, settings, UI, "
                "slicing, or export, use only the available mcp__owzx__ tools. "
                "Never use filesystem, shell, browser, or other Claude tools. "
                "Inspect state before mutating it and report the tool result."
            ),
            mcp_servers=mcp,
            strict_mcp_config=True,
            setting_sources=[],  # do not read user/project Claude settings
            model=self.model,
            can_use_tool=self.can_use_tool,
            max_turns=MAX_TURNS,
        )

    async def connect(self) -> None:
        from claude_agent_sdk import ClaudeSDKClient

        self._refresh_readonly_tools()
        self.client = ClaudeSDKClient(self._build_options())
        await self.client.connect()

    async def reset(self) -> None:
        if self.turn_task and not self.turn_task.done():
            self.turn_task.cancel()
        if self.client:
            await self.client.disconnect()
        for fut in self._perm_futures.values():
            if not fut.done():
                fut.set_result(False)
        self._perm_futures.clear()
        await self.connect()
        emit({"type": "ready", "sdk": _sdk_version(), "note": "conversation reset"})

    # -- one user turn --------------------------------------------------------
    async def run_turn(self, text: str) -> None:
        try:
            await self.client.query(text)
            async for msg in self.client.receive_response():
                _emit_message_events(msg, emit)
        except asyncio.CancelledError:
            emit({"type": "turn_done", "isError": False, "cancelled": True})
        except Exception as exc:  # turn-level failure is recoverable
            emit({"type": "error", "message": f"{type(exc).__name__}: {exc}"})
            emit({"type": "turn_done", "isError": True})

    # -- command dispatch (called on the asyncio loop) ------------------------
    def handle_command(self, cmd: dict) -> None:
        ctype = cmd.get("type")
        if ctype == "send":
            text = str(cmd.get("text", ""))
            if not text:
                emit({"type": "error", "message": "empty send"})
                return
            if self.busy:
                emit({"type": "error", "message": "busy: wait for turn_done"})
                return

            async def _turn() -> None:
                self.busy = True
                try:
                    await self.run_turn(text)
                finally:
                    self.busy = False

            self.turn_task = self._loop.create_task(_turn())
        elif ctype == "cancel":
            if self.client:
                try:
                    self.client.interrupt()
                except Exception as exc:
                    emit({"type": "error", "message": f"interrupt failed: {exc}"})
        elif ctype == "reset":
            self._loop.create_task(self._safe_reset())
        elif ctype == "answer_permission":
            self.answer_permission(cmd.get("callId", 0), bool(cmd.get("allow")))
        elif ctype == "shutdown":
            self._loop.create_task(self._shutdown())
        else:
            emit({"type": "error", "message": f"unknown command: {ctype}"})

    async def _safe_reset(self) -> None:
        try:
            await self.reset()
        except Exception as exc:
            emit({"type": "error", "message": f"reset failed: {exc}"})

    async def _shutdown(self) -> None:
        try:
            if self.client:
                await self.client.disconnect()
        finally:
            os._exit(0)


def _sdk_version() -> str:
    try:
        import claude_agent_sdk

        return getattr(claude_agent_sdk, "__version__", "unknown")
    except Exception:
        return "unknown"


# ── selftest (no network, no API key) ────────────────────────────────────────


def selftest() -> int:
    """CI-safe checks: SDK import, MCP http config shape, permission plumbing."""
    import claude_agent_sdk
    from claude_agent_sdk import ClaudeAgentOptions

    options = ClaudeAgentOptions(
        mcp_servers={
            "owzx": {
                "type": "http",
                "url": "http://127.0.0.1:65535/mcp",
                "headers": {"Authorization": "Bearer selftest"},
            }
        },
        strict_mcp_config=True,
        setting_sources=[],
        model="glm-5.3-flash",
        max_turns=MAX_TURNS,
    )
    assert options.model == "glm-5.3-flash"

    # Permission plumbing without a client: resolve a future the way
    # answer_permission does during a real turn.
    harness = Harness.__new__(Harness)
    loop = asyncio.new_event_loop()
    harness._loop = loop
    fut = loop.create_future()
    harness._perm_futures = {7: fut}
    harness.answer_permission(7, True)
    assert loop.run_until_complete(fut) is True

    # Permission gating: read-only tools auto-allow without a card; only
    # destructive tools wait for the host's answer_permission. The SDK
    # presents MCP tools with an mcp__<server>__ prefix -- both forms must
    # auto-allow.
    harness._readonly_tools = {"get_app_state"}
    harness._perm_counter = 0
    allow = loop.run_until_complete(
        harness.can_use_tool("mcp__owzx__get_app_state", {}, None))
    assert type(allow).__name__ == "PermissionResultAllow", allow
    allow = loop.run_until_complete(
        harness.can_use_tool("get_app_state", {}, None))
    assert type(allow).__name__ == "PermissionResultAllow", allow
    assert harness._perm_counter == 0, "read-only tool must not request a card"
    pending = loop.create_task(
        harness.can_use_tool("mcp__owzx__clear_project", {}, None))
    loop.run_until_complete(asyncio.sleep(0))
    assert harness._perm_counter == 1, "destructive tool must request a card"
    harness.answer_permission(1, False)
    deny = loop.run_until_complete(pending)
    assert type(deny).__name__ == "PermissionResultDeny", deny

    # NDJSON emit smoke (captured via replace_stdout is overkill; just format)
    line = json.dumps({"type": "ready", "sdk": _sdk_version()}, ensure_ascii=False)
    assert json.loads(line)["type"] == "ready"

    # Message parsing: 0.2.147 yields dataclass instances, not dicts. This is
    # the regression lock for the 'AssistantMessage' object has no attribute
    # 'get' crash that only surfaced on the first live GLM turn.
    from claude_agent_sdk import (
        AssistantMessage,
        ResultMessage,
        TextBlock,
        ToolResultBlock,
        ToolUseBlock,
        UserMessage,
    )

    events: list[dict] = []
    _emit_message_events(
        AssistantMessage(
            model="glm-5.3-flash",
            content=[TextBlock(text="hello"), ToolUseBlock(id="t1", name="get_app_state", input={})],
        ),
        events.append,
    )
    _emit_message_events(
        UserMessage(content=[ToolResultBlock(tool_use_id="t1", content="ok", is_error=False)]),
        events.append,
    )
    _emit_message_events(
        ResultMessage(
            subtype="success", duration_ms=5, duration_api_ms=4,
            is_error=False, num_turns=1, session_id="s1", result="done",
        ),
        events.append,
    )
    assert events == [
        {"type": "assistant_text", "text": "hello"},
        {"type": "tool_use", "id": "t1", "name": "get_app_state", "input": {}},
        {"type": "tool_result", "id": "t1", "ok": True, "summary": "ok"},
        {"type": "turn_done", "isError": False, "durationMs": 5, "sessionId": "s1", "result": "done"},
    ], events

    print(f"selftest ok: sdk={_sdk_version()} python={sys.version.split()[0]}")
    return 0


# ── entry ────────────────────────────────────────────────────────────────────


async def _main() -> None:
    harness = Harness()
    harness._loop = asyncio.get_running_loop()

    ready = asyncio.Event()

    async def _connect() -> None:
        try:
            await harness.connect()
            emit({"type": "ready", "sdk": _sdk_version()})
        except Exception as exc:
            emit({"type": "error", "message": f"connect failed: {exc}"})
            os._exit(2)
        finally:
            ready.set()

    loop = asyncio.get_running_loop()
    connect_task = loop.create_task(_connect())

    # stdin reader thread -> loop dispatch (NDJSON commands)
    def _reader() -> None:
        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue
            try:
                cmd = json.loads(line)
            except json.JSONDecodeError:
                continue
            loop.call_soon_threadsafe(harness.handle_command, cmd)
        # Host pipe closed (app killed without a graceful shutdown): exit now
        # instead of idling forever and leaking a python+claude.exe pair.
        loop.call_soon_threadsafe(lambda: os._exit(0))

    threading.Thread(target=_reader, daemon=True).start()

    await connect_task
    await asyncio.Event().wait()  # run until process is killed / shutdown cmd


def main() -> int:
    if sys.platform == "win32":
        try:
            sys.stdout.reconfigure(encoding="utf-8")
            sys.stdin.reconfigure(encoding="utf-8")
        except Exception:
            pass
    if "--selftest" in sys.argv:
        return selftest()
    if not os.environ.get("OWZX_MCP_URL") or not os.environ.get("OWZX_MCP_TOKEN"):
        print("OWZX_MCP_URL / OWZX_MCP_TOKEN are required", file=sys.stderr)
        return 2
    asyncio.run(_main())
    return 0


if __name__ == "__main__":
    sys.exit(main())
