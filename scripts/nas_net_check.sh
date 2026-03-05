#!/bin/sh
IP="$1"
if [ -z "$IP" ]; then
  IP="192.168.100.68"
fi

echo "CHECK_IP=$IP"
/usr/bin/curl -s -o /dev/null -w "HOST_HTTP=%{http_code}\n" "http://$IP:11434/api/tags"
/usr/local/bin/docker exec moltbot-gateway-max sh -lc "curl -s -o /dev/null -w 'CONTAINER_HTTP=%{http_code}\n' http://$IP:11434/api/tags"
/usr/local/bin/docker exec moltbot-gateway-max sh -lc "curl -s --noproxy '*' -o /dev/null -w 'CONTAINER_NOPROXY_HTTP=%{http_code}\n' http://$IP:11434/api/tags"
