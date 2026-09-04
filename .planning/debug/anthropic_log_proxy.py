#!/usr/bin/env python3
"""Logging reverse proxy: local claude.exe -> Zhipu Anthropic-compat endpoint.

Binds 127.0.0.1:27418, forwards POST bodies to
https://open.bigmodel.cn/api/anthropic/* verbatim, and logs each
request/response pair to .planning/debug/anthropic_proxy_log/ so we can see
exactly which parameter Zhipu rejects with error 1210.
"""
import datetime
import os
import urllib.request
from http.server import BaseHTTPRequestHandler, HTTPServer

UPSTREAM = "https://open.bigmodel.cn/api/anthropic"
LOG_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "anthropic_proxy_log")
os.makedirs(LOG_DIR, exist_ok=True)
COUNTER = {"n": 0}


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length)
        COUNTER["n"] += 1
        n = COUNTER["n"]
        stamp = datetime.datetime.now().strftime("%H%M%S")

        req = urllib.request.Request(
            UPSTREAM + self.path,
            data=body,
            method="POST",
        )
        for h in ("Authorization", "x-api-key", "anthropic-version",
                  "anthropic-beta", "content-type"):
            v = self.headers.get(h)
            if v:
                req.add_header(h, v)

        status, resp_body = 502, b'{"error":"proxy upstream failure"}'
        try:
            with urllib.request.urlopen(req, timeout=120) as resp:
                status = resp.status
                resp_body = resp.read()
        except urllib.error.HTTPError as e:
            status = e.code
            resp_body = e.read()
        except Exception as e:  # noqa: BLE001
            resp_body = ('{"error":"%s"}' % e).encode()

        with open(os.path.join(LOG_DIR, f"{stamp}_{n:02d}_{status}.md"), "wb") as f:
            f.write(f"# POST {self.path} -> {status}\n\n".encode())
            f.write(b"## request headers\n\n```\n")
            f.write("\n".join(
                f"{k}: {v}" for k, v in self.headers.items()
                if k.lower() not in ("authorization", "x-api-key")
            ).encode())
            f.write(b"\n```\n\n## request body\n\n```json\n")
            f.write(body)
            f.write(b"\n```\n\n## response body\n\n```json\n")
            f.write(resp_body)
            f.write(b"\n```\n")

        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(resp_body)))
        self.end_headers()
        self.wfile.write(resp_body)

    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-Length", "2")
        self.end_headers()
        self.wfile.write(b"{}")

    def log_message(self, fmt, *args):  # quiet
        pass


if __name__ == "__main__":
    HTTPServer(("127.0.0.1", 27418), Handler).serve_forever()
