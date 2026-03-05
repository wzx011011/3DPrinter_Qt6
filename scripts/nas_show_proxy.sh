#!/bin/sh
/usr/local/bin/docker exec moltbot-gateway-max sh -lc "sed -n '1,60p' /root/.openclaw/proxy-setup.js"
/usr/local/bin/docker exec moltbot-gateway-max sh -lc "env | grep -Ei 'http_proxy|https_proxy|all_proxy|no_proxy' || true"
