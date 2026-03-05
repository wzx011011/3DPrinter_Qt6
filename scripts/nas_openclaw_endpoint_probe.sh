#!/bin/sh
for p in /v1/chat/completions /chat/completions /api/chat/completions /v1/responses /api/v1/chat/completions; do
  echo "=== $p ==="
  /usr/bin/curl -sS -i -X OPTIONS "http://127.0.0.1:18789$p" | head -n 15
  echo
  /usr/bin/curl -sS -i "http://127.0.0.1:18789$p" | head -n 15
  echo
  /usr/bin/curl -sS -i -X POST -H "Content-Type: application/json" \
    -d '{"model":"ollama/qwen3.5:9b","messages":[{"role":"user","content":"hi"}],"stream":false}' \
    "http://127.0.0.1:18789$p" | head -n 20
  echo
 done
