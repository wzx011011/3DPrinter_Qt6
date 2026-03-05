import re
import subprocess

container = "moltbot-gateway-max"
win_ip = "192.168.100.68"
path = "/root/.openclaw/proxy-setup.js"

content = subprocess.check_output([
    "/usr/local/bin/docker", "exec", container, "cat", path
], text=True)

pattern = r"const mergedNoProxy = '([^']*)';"
m = re.search(pattern, content)
if not m:
    raise SystemExit("mergedNoProxy not found")

parts = [p.strip() for p in m.group(1).split(",") if p.strip()]
if win_ip not in parts:
    parts.append(win_ip)
new_value = ",".join(parts)
new_line = f"const mergedNoProxy = '{new_value}';"
content2 = re.sub(pattern, new_line, content)

local_tmp = "/tmp/proxy-setup.patched.js"
with open(local_tmp, "w", encoding="utf-8") as f:
    f.write(content2)

subprocess.run([
    "/usr/local/bin/docker", "cp", local_tmp, f"{container}:{path}"
], check=True)

print("PATCHED_NO_PROXY", win_ip)
