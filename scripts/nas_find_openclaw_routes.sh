#!/bin/sh
/usr/local/bin/docker exec moltbot-gateway-max sh -lc "grep -Rni 'app\.post\|router\.post\|/chat\|/api' /app/src | head -n 260"
