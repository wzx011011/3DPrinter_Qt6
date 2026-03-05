import os
import json
import subprocess

container = os.environ.get("OC_CONTAINER", "moltbot-gateway-max")
win_ip = os.environ.get("WIN_IP", "192.168.106.1")

raw = subprocess.check_output([
    "/usr/local/bin/docker",
    "exec",
    container,
    "cat",
    "/root/.openclaw/openclaw.json",
], text=True)

cfg = json.loads(raw)
providers = cfg.setdefault("models", {}).setdefault("providers", {})
providers["ollama"] = {
    "baseUrl": f"http://{win_ip}:11434/v1",
    "api": "openai-completions",
    "apiKey": "ollama",
    "models": [
        {
            "id": "qwen3.5:9b",
            "name": "Qwen3.5 9B (Windows Local Ollama)",
            "reasoning": False,
            "input": ["text"],
            "cost": {"input": 0, "output": 0, "cacheRead": 0, "cacheWrite": 0},
            "contextWindow": 262144,
            "maxTokens": 8192,
        }
    ],
}

defaults = cfg.setdefault("agents", {}).setdefault("defaults", {})
defaults.setdefault("models", {})["ollama/qwen3.5:9b"] = {}
model_cfg = defaults.setdefault("model", {})
model_cfg["primary"] = "ollama/qwen3.5:9b"
model_cfg.setdefault("fallbacks", ["zai/glm-5"])

patched = "/tmp/openclaw.localollama.json"
with open(patched, "w", encoding="utf-8") as f:
    json.dump(cfg, f, ensure_ascii=False, indent=2)

subprocess.run([
    "/usr/local/bin/docker",
    "cp",
    patched,
    f"{container}:/root/.openclaw/openclaw.json",
], check=True)

print("PATCH_OK", win_ip)
