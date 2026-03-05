import json
import subprocess
import time
import uuid

session_id = f"selftest_{uuid.uuid4().hex[:8]}"
cmd = [
    "/usr/local/bin/docker", "exec", "moltbot-gateway-max",
    "sh", "-lc",
    f"node /app/dist/index.js agent --local --session-id {session_id} -m 你好，请只回复：OpenClaw已连通 --json"
]

print("SESSION", session_id)
start = time.time()
proc = subprocess.run(cmd, capture_output=True, text=True, timeout=180)
print("EXIT", proc.returncode)
print("DURATION_S", round(time.time() - start, 2))

stdout = proc.stdout or ""
stderr = proc.stderr or ""

obj = None
for line in reversed(stdout.splitlines()):
    line = line.strip()
    if not line:
        continue
    try:
        obj = json.loads(line)
        break
    except Exception:
        continue

def find_reply(x):
    if isinstance(x, dict):
        for key in ["reply", "text", "content", "message", "output", "final"]:
            val = x.get(key)
            if isinstance(val, str) and val.strip():
                if len(val) > 5:
                    return val.strip()
        for v in x.values():
            got = find_reply(v)
            if got:
                return got
    elif isinstance(x, list):
        for v in x:
            got = find_reply(v)
            if got:
                return got
    return None

reply = find_reply(obj) if obj is not None else None

if reply:
    print("REPLY_BEGIN")
    print(reply)
    print("REPLY_END")
else:
    print("REPLY_NOT_FOUND")
    print("STDOUT_TAIL_BEGIN")
    print(stdout[-2500:])
    print("STDOUT_TAIL_END")

if stderr.strip():
    print("STDERR_TAIL_BEGIN")
    print(stderr[-1200:])
    print("STDERR_TAIL_END")
