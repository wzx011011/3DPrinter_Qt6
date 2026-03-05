#!/bin/sh
URL="http://127.0.0.1:18789/v1/chat/completions"
JSON='{"model":"ollama/qwen3.5:9b","messages":[{"role":"user","content":"请只回复：OpenClaw已连通Qwen3.5-9B"}],"temperature":0,"stream":false}'

printf "POST %s\n" "$URL"
/usr/bin/curl -sS -D /tmp/oc_headers.txt -o /tmp/oc_body.txt -H "Content-Type: application/json" -X POST "$URL" -d "$JSON"

head -n 20 /tmp/oc_headers.txt
printf "\nBODY:\n"
head -c 800 /tmp/oc_body.txt; echo
