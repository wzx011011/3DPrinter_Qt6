#!/usr/bin/env python3
"""Interactive driver for tools/ai_sidecar/agent.py headless acceptance runs.

Spawns the production sidecar, sends one user turn after `ready`,
auto-answers every permission_request with `allow` (mirrors the user
pressing the Allow button on the app's permission card), and prints the
full NDJSON event stream until turn_done.
"""
import json
import os
import subprocess
import sys
import threading
import time

PROMPT = sys.argv[1] if len(sys.argv) > 1 else (
    "请调用 get_app_state 查一下当前应用状态，然后用一句话告诉我：现在在哪个页面、有几个模型对象。"
)


def main() -> int:
    proc = subprocess.Popen(
        [sys.executable, os.path.join(os.path.dirname(__file__), "..", "..",
                                      "tools", "ai_sidecar", "agent.py")],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, encoding="utf-8", env=os.environ.copy(), bufsize=1,
    )
    assert proc.stdin is not None and proc.stdout is not None
    state = {"ready": False, "done": False, "tools": 0}

    def send(cmd: dict) -> None:
        proc.stdin.write(json.dumps(cmd, ensure_ascii=False) + "\n")
        proc.stdin.flush()

    for raw in proc.stdout:
        raw = raw.strip()
        if not raw:
            continue
        try:
            ev = json.loads(raw)
        except json.JSONDecodeError:
            print("[stderr]", raw)
            continue
        et = ev.get("type")
        if et == "ready" and not state["ready"]:
            state["ready"] = True
            print("[driver] ready -> sending prompt")
            send({"type": "send", "text": PROMPT})
        elif et == "permission_request":
            print(f"[driver] permission_request callId={ev.get('callId')} tool={ev.get('tool')} -> ALLOW")
            send({"type": "answer_permission", "callId": ev.get("callId"), "allow": True})
        elif et == "turn_done":
            state["done"] = True
            print(f"[driver] turn_done isError={ev.get('isError')} result={ev.get('result','')[:300]}")
            break
        else:
            if et == "tool_use":
                state["tools"] += 1
            print("[event]", json.dumps(ev, ensure_ascii=False)[:600])

    deadline = time.time() + 20
    while not state["done"] and time.time() < deadline:
        time.sleep(0.5)

    try:
        send({"type": "shutdown"})
    except Exception:
        pass
    try:
        proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        proc.kill()
    print(f"[driver] finished: tool_calls={state['tools']} turn_done={state['done']}")
    return 0 if state["done"] and state["tools"] >= 1 else 1


if __name__ == "__main__":
    sys.exit(main())
