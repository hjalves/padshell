#!/usr/bin/env python3
"""Serve a folder over Gemini for previewing the capsule (not for production).

usage: serve-gemini.py [root] [port]   (defaults: public/gemini 1965)

Makes a self-signed certificate for this machine on first run (needs openssl)
and keeps it in ~/.cache/padshell-gemini/, so TOFU clients see the same one.
"""
import mimetypes, os, socket, socketserver, ssl, subprocess, sys
from pathlib import Path
from urllib.parse import unquote, urlsplit

root = Path(sys.argv[1] if len(sys.argv) > 1 else "public/gemini").resolve()
port = int(sys.argv[2]) if len(sys.argv) > 2 else 1965
mimetypes.add_type("text/gemini", ".gmi")

certdir = Path.home() / ".cache" / "padshell-gemini"
cert, key = certdir / "cert.pem", certdir / "key.pem"
if not cert.exists():
    certdir.mkdir(parents=True, exist_ok=True)
    subprocess.run(["openssl", "req", "-x509", "-newkey", "ec", "-pkeyopt", "ec_paramgen_curve:prime256v1",
                    "-nodes", "-days", "3650", "-subj", "/CN=" + socket.gethostname(),
                    "-keyout", str(key), "-out", str(cert)], check=True, capture_output=True)

ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
ctx.minimum_version = ssl.TLSVersion.TLSv1_2
ctx.load_cert_chain(cert, key)


class Handler(socketserver.StreamRequestHandler):
    def respond(self, header, body=b""):
        self.wfile.write(header.encode() + b"\r\n" + body)
        print(f"{self.client_address[0]} {self.path!r} -> {header}", flush=True)

    def handle(self):
        self.path = ""
        line = self.rfile.readline(1026).decode("utf-8", "replace").strip()
        self.path = line
        url = urlsplit(line)
        if url.scheme != "gemini":
            return self.respond("59 Bad request")
        target = (root / unquote(url.path).lstrip("/")).resolve()
        if not target.is_relative_to(root):
            return self.respond("51 Not found")
        if target.is_dir():
            if not url.path.endswith("/"):
                return self.respond(f"31 {url.path or ''}/")
            target = target / "index.gmi"
        if not target.is_file():
            return self.respond("51 Not found")
        mime = mimetypes.guess_type(target.name)[0] or "application/octet-stream"
        if mime.startswith("text/"):
            mime += "; charset=utf-8"
        self.respond(f"20 {mime}", target.read_bytes())


class Server(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def get_request(self):
        sock, addr = super().get_request()
        sock.settimeout(10)
        return ctx.wrap_socket(sock, server_side=True), addr


print(f"serving {root} on gemini://{socket.gethostname()}:{port}/" if port != 1965
      else f"serving {root} on gemini://{socket.gethostname()}/", flush=True)
Server(("", port), Handler).serve_forever()
