#!/bin/sh
curl -sS http://127.0.0.1:18789/assets/index-yUL4-MTm.js \
  | grep -Eo '/api/[A-Za-z0-9_./:-]+|/ws[A-Za-z0-9_./:-]*|/socket[A-Za-z0-9_./:-]*|/rpc[A-Za-z0-9_./:-]*|/chat[A-Za-z0-9_./:-]*|/v1/[A-Za-z0-9_./:-]*' \
  | sort -u \
  | head -n 200
