#!/usr/bin/env python3
"""Multi-turn driver: N sequential turns in ONE sidecar session.

Each prompt is sent only after the previous turn's turn_done. Prints
per-turn timing and the tool calls made -- reproduces the in-app
multi-turn busy-stuck seen during the real-UI sweep.
"""
import json
import os
import subprocess
import sys
import time

PROMPTS = [
    "当前应用在哪个页面？有几个模型？",
    "调用 get_scene 列出所有模型的坐标",
    "调用 get_app_state 再确认一次对象数量",
]
TIMEOUT_S = 90


def main() -> int:
    proc = subprocess.Popen(
        [sys.executable, os.path.join(os.path.dirname(__file__), "..", "..",
                                      "tools", "ai_sidecar", "agent.py")],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, encoding="utf-8", env=os.environ.copy(), bufsize=1,
    )
    assert proc.stdin is not None and proc.stdout is not None
    state = {"ready": False, "turn": 0, "done": False, "tools": 0, "start": 0.0,
             "fails": 0}

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
            print("[stderr]", raw[:200])
            continue
        et = ev.get("type")
        if et == "ready" and not state["ready"]:
            state["ready"] = True
            state["start"] = time.time()
            print(f"[turn 1] send: {PROMPTS[0]}")
            send({"type": "send", "text": PROMPTS[0]})
        elif et == "permission_request":
            print(f"  [perm] {ev.get('tool')} -> ALLOW")
            send({"type": "answer_permission", "callId": ev.get("callId"), "allow": True})
        elif et == "tool_use":
            state["tools"] += 1
            print(f"  [tool_use] {ev.get('name')}")
        elif et == "turn_done":
            n = state["turn"] + 1
            print(f"  [turn_done] isError={ev.get('isError')} tools={state['tools']} "
                  f"{time.time() - state['start']:.1f}s")
            state["turn"] = n
            state["tools"] = 0
            if n < len(PROMPTS):
                state["start"] = time.time()
                print(f"[turn {n + 1}] send: {PROMPTS[n]}")
                send({"type": "send", "text": PROMPTS[n]})
            else:
                state["done"] = True
                break
        elif et == "error":
            state["fails"] += 1
            print(f"  [error] {ev.get('message', '')[:200]}")
        else:
            print(f"  [{et}]", str(ev)[:160])

    if state["done"]:
        print(f"[driver] ALL {len(PROMPTS)} TURNS COMPLETED")
        rc = 0
    else:
        print(f"[driver] INCOMPLETE after timeout: turns finished={state['turn']}")
        rc = 1
    try:
        send({"type": "shutdown"})
        proc.wait(timeout=8)
    except Exception:
        proc.kill()
    return rc


if __name__ == "__main__":
    sys.exit(main())
