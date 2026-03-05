#!/bin/sh
IP="$1"
if [ -z "$IP" ]; then
  IP="192.168.100.68"
fi
/usr/local/bin/docker exec moltbot-gateway-max sh -lc "node -r /root/.openclaw/proxy-setup.js -e \"fetch('http://$IP:11434/api/tags').then(r=>r.text().then(t=>console.log('APP_HTTP='+r.status+' LEN='+t.length))).catch(e=>{console.error('APP_ERR='+e.message); process.exit(2);})\""
