#!/usr/bin/env python3
"""M1's bounded, in-memory offer/answer service for a trusted HTTPS origin."""

import argparse
from base64 import b32encode
from dataclasses import dataclass
from http.server import BaseHTTPRequestHandler, HTTPServer
import re
import secrets
import ssl
import time


SDP_LIMIT = 16_384
ROOM_LIMIT = 64
JOIN_LIMIT = 1024
PENDING_PER_ROOM = 16
ROOM_LEASE = 90
JOIN_LEASE = 55
TOKEN = re.compile(r"[0-9a-f]{64}")
BINDING = re.compile(rb"[0-9a-f]{32}")
INVITE = re.compile(rb"[A-Z2-7]{24}")


def valid_sdp(sdp: bytes) -> bool:
    return (
        0 < len(sdp) < SDP_LIMIT and sdp.isascii() and b"\0" not in sdp
        and sdp.startswith(b"v=0\r\n") and b"\na=fingerprint:" in sdp
        and b"\na=candidate:" in sdp and b"\na=end-of-candidates" in sdp
    )


@dataclass
class Room:
    invite: bytes
    until: float


@dataclass
class Join:
    host: str
    binding: bytes
    offer: bytes
    until: float
    answer: bytes | None = None


class Rendezvous:
    def __init__(self) -> None:
        self.rooms: dict[str, Room] = {}
        self.invites: dict[bytes, str] = {}
        self.joins: dict[str, Join] = {}

    def expire(self, now: float) -> None:
        for token, room in list(self.rooms.items()):
            if now >= room.until:
                del self.rooms[token]
                del self.invites[room.invite]
        for token, join in list(self.joins.items()):
            if now >= join.until or join.host not in self.rooms:
                del self.joins[token]

    def handle(self, method: str, path: str, token: str, body: bytes, now: float) -> tuple[int, bytes]:
        self.expire(now)
        room = self.rooms.get(token)
        join = self.joins.get(token)

        if (method, path) == ("POST", "/v1/room"):
            if body or join:
                return 400, b""
            if not room:
                if len(self.rooms) >= ROOM_LIMIT:
                    return 503, b""
                invite = b32encode(secrets.token_bytes(15))
                while invite in self.invites:
                    invite = b32encode(secrets.token_bytes(15))
                room = self.rooms[token] = Room(invite, now + ROOM_LEASE)
                self.invites[invite] = token
            room.until = now + ROOM_LEASE
            return 200, room.invite

        if (method, path) == ("POST", "/v1/join"):
            fields = body.split(b"\n", 2)
            if room or len(fields) != 3:
                return 400, b""
            invite, binding, offer = fields
            if not INVITE.fullmatch(invite) or not BINDING.fullmatch(binding) or not valid_sdp(offer):
                return 400, b""
            if binding == b"0" * 32:
                return 400, b""
            host = self.invites.get(invite)
            if host is None:
                return 404, b""
            if join:
                return (204 if (host, binding, offer) == (join.host, join.binding, join.offer) else 409), b""
            peers = [other for other in self.joins.values() if other.host == host]
            if any(other.binding == binding for other in peers):
                return 409, b""
            if len(self.joins) >= JOIN_LIMIT or sum(other.answer is None for other in peers) >= PENDING_PER_ROOM:
                return 503, b""
            self.joins[token] = Join(host, binding, offer, now + JOIN_LEASE)
            return 204, b""

        if (method, path) == ("GET", "/v1/offer"):
            if not room:
                return 404, b""
            room.until = now + ROOM_LEASE
            for other in self.joins.values():
                if other.host == token and other.answer is None:
                    return 200, other.binding + b"\n" + other.offer
            return 204, b""

        if (method, path) == ("POST", "/v1/answer"):
            if not room:
                return 404, b""
            binding, sep, answer = body.partition(b"\n")
            if not sep or not BINDING.fullmatch(binding) or (answer and not valid_sdp(answer)):
                return 400, b""
            for other in self.joins.values():
                if (other.host, other.binding) == (token, binding):
                    if other.answer is not None and other.answer != answer:
                        return 409, b""
                    room.until = now + ROOM_LEASE
                    other.answer = answer
                    return 204, b""
            return 404, b""

        if (method, path) == ("GET", "/v1/answer"):
            if not join:
                return 404, b""
            if join.answer is None:
                return 204, b""
            if not join.answer:
                return 409, b""
            return 200, join.binding + b"\n" + join.answer

        return 404, b""


class Handler(BaseHTTPRequestHandler):
    def log_message(self, _format: str, *args: object) -> None:
        # SDP contains connection details. Keep request metadata out of application logs.
        pass

    def reply(self, status: int, data: bytes = b"") -> None:
        self.send_response(status)
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        if data:
            self.wfile.write(data)

    def handle_rendezvous(self) -> None:
        auth = self.headers.get_all("Authorization", [])
        if len(auth) != 1 or not auth[0].startswith("Bearer ") or not TOKEN.fullmatch(auth[0][7:]):
            self.reply(401)
            return
        lengths = self.headers.get_all("Content-Length", [])
        if self.headers.get("Transfer-Encoding") or len(lengths) > 1:
            self.reply(400)
            return
        length = lengths[0] if lengths else "0"
        if not re.fullmatch(r"[0-9]{1,5}", length):
            self.reply(400)
            return
        size = int(length)
        if size > SDP_LIMIT + 64:
            self.reply(413)
            return
        body = self.rfile.read(size)
        if len(body) != size or (self.command == "GET" and body):
            self.reply(400)
            return
        self.reply(*self.server.rendezvous.handle(self.command, self.path, auth[0][7:], body, time.monotonic()))

    def do_GET(self) -> None:
        self.handle_rendezvous()

    def do_POST(self) -> None:
        self.handle_rendezvous()


class Server(HTTPServer):
    request_queue_size = 32

    def __init__(self, port: int) -> None:
        super().__init__(("127.0.0.1", port), Handler)
        self.rendezvous = Rendezvous()

    def service_actions(self) -> None:
        self.rendezvous.expire(time.monotonic())

    def get_request(self):
        sock, addr = super().get_request()
        sock.settimeout(3)
        return sock, addr


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8471)
    parser.add_argument("--certificate")
    parser.add_argument("--key")
    args = parser.parse_args()
    if bool(args.certificate) != bool(args.key):
        parser.error("--certificate and --key must be used together")
    with Server(args.port) as server:
        if args.certificate:
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            context.minimum_version = ssl.TLSVersion.TLSv1_2
            context.load_cert_chain(args.certificate, args.key)
            # Defer TLS handshakes so the accepted socket has a deadline.
            server.socket = context.wrap_socket(server.socket, server_side=True, do_handshake_on_connect=False)
        print(f"Rendezvous listening on 127.0.0.1:{server.server_port}", flush=True)
        server.serve_forever()


if __name__ == "__main__":
    main()
