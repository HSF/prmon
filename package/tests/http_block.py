#! /usr/bin/env python3
#
# Copyright (C) 2018-2026 CERN
# License Apache2 - see LICENCE file

"""Simple HTTP server that delivers a known block
of data to the caller

One GET parameter is recognised, which is "blocks"
that specifies how many 1KB blocks are returned to the
client (defaults to 1000, thus ~1MB delivered)"""

import argparse
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlparse

MY_STRING = b"somehow the world seems more curious than when i was a child xx\n"
MY_BLOCK = MY_STRING * 16  # 1KB


class BlockHandler(BaseHTTPRequestHandler):
    """Return the requested number of 1KB blocks for any GET request"""

    def do_GET(self):  # noqa: N802 (name required by BaseHTTPRequestHandler)
        """Handle a GET request"""
        query = parse_qs(urlparse(self.path).query)
        try:
            blocks = int(query.get("blocks", ["1000"])[0])
        except ValueError:
            self.send_error(400, "Invalid block value")
            return

        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(MY_BLOCK) * blocks))
        self.end_headers()
        for _ in range(blocks):
            self.wfile.write(MY_BLOCK)


def main():
    """parse arguments and serve until killed"""
    parser = argparse.ArgumentParser(description="HTTP data block server")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--host", default="localhost")
    args = parser.parse_args()

    with HTTPServer((args.host, args.port), BlockHandler) as server:
        server.serve_forever()


if __name__ == "__main__":
    main()
