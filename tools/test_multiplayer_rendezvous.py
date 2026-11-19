#!/usr/bin/env python3
"""Run the C rendezvous test against a short-lived, certificate-verified local server."""
from pathlib import Path
import ssl
import subprocess
import sys
from tempfile import TemporaryDirectory
from threading import Thread

from multiplayer_rendezvous import Server


def main() -> None:
    executable = Path(sys.argv[1]).resolve(strict=True)
    with TemporaryDirectory() as scratch:
        cert = Path(scratch) / "cert.pem"
        key = Path(scratch) / "key.pem"
        subprocess.run([
            "openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes",
            "-subj", "/CN=Savage Wind Test CA", "-addext", "subjectAltName=IP:127.0.0.1",
            "-days", "1", "-keyout", key, "-out", cert,
        ], check=True, capture_output=True)
        with Server(0) as server:
            tls = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            tls.minimum_version = ssl.TLSVersion.TLSv1_2
            tls.load_cert_chain(cert, key)
            server.socket = tls.wrap_socket(server.socket, server_side=True, do_handshake_on_connect=False)
            thread = Thread(target=server.serve_forever)
            thread.start()
            try:
                subprocess.run([executable, f"https://127.0.0.1:{server.server_port}", cert], check=True, timeout=50)
            finally:
                server.shutdown()
                thread.join()


if __name__ == "__main__":
    main()
