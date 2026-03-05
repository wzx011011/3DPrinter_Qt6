import json
import os
import subprocess

container = "moltbot-gateway-max"
win_ip = os.environ.get("WIN_IP", "192.168.100.68")

raw = subprocess.check_output([
    "/usr/local/bin/docker", "exec", container, "cat", "/root/.openclaw/openclaw.json"
], text=True)
cfg = json.loads(raw)
base_url = cfg.get("models", {}).get("providers", {}).get("ollama", {}).get("baseUrl")
primary = cfg.get("agents", {}).get("defaults", {}).get("model", {}).get("primary")
print("baseUrl=", base_url)
print("primary=", primary)

code = subprocess.check_output([
    "/usr/local/bin/docker", "exec", container, "sh", "-lc",
    f"curl -s -o /dev/null -w '%{{http_code}}' http://{win_ip}:11434/api/tags"
], text=True).strip()
print("connectivity_http=", code)
