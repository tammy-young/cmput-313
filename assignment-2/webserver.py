#!/usr/bin/env python3
import http.server
import socketserver

if __name__ == '__main__':
    handler = http.server.SimpleHTTPRequestHandler
    with socketserver.TCPServer(("", 8001), handler) as httpd:
        httpd.serve_forever()
