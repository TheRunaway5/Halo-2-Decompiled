"""Local administration, a real HTTP simulator and a failover discovery client."""
import argparse
import contextlib
import fcntl
import json
import os
import secrets
import signal
import sys
import threading
import time
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode
from urllib.request import Request, build_opener, HTTPRedirectHandler, ProxyHandler

from .protocol import (VERSION, MAX_SYNC, Problem, canonical, parse, public_hex, master_id,
                       server_id, load_key, save_private, signed, require, metadata)
from .service import endpoint


class NoRedirect(HTTPRedirectHandler):
    def redirect_request(self, *args):
        return None


class ApiError(Exception):
    def __init__(self, status, value):
        self.status, self.value = status, value
        super().__init__(f"HTTP {status}: {value}")


def request(base, method, path, value=None, token=None):
    headers = {"Content-Type": "application/json"}
    if token:
        headers["Authorization"] = "Bearer " + token
    req = Request(base.rstrip("/") + path, data=canonical(value) if value is not None else None,
                  headers=headers, method=method)
    try:
        # No ambient proxy or redirects can receive enrollment credentials.
        with build_opener(ProxyHandler({}), NoRedirect()).open(req, timeout=3) as response:
            data = response.read(MAX_SYNC + 1)
            require(len(data) <= MAX_SYNC, "Response too large")
            return parse(data)
    except HTTPError as error:
        raise ApiError(error.code, parse(error.read(16384))) from None


def describe_master(directory):
    directory = Path(directory)
    key = load_key(directory / "master-key.json")
    token_path = directory / "admin-token"
    if not token_path.exists():
        fd = os.open(token_path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, "w") as output:
            output.write(secrets.token_urlsafe(48) + "\n")
    if not (directory / "peers.json").exists():
        save_private(directory / "peers.json", [], True)
    return {"id": master_id(public_hex(key)), "public_key": public_hex(key)}


def initialize_demo(a, b, a_url="http://127.0.0.1:18081", b_url="http://127.0.0.1:18082"):
    first, second = describe_master(a), describe_master(b)
    save_private(Path(a) / "peers.json", [{**second, "url": b_url}])
    save_private(Path(b) / "peers.json", [{**first, "url": a_url}])
    return {"a": first, "b": second}


class Instance:
    def __init__(self, path):
        self.path = Path(path)
        self.key = load_key(path)
        self.value = parse(self.path.read_bytes())
        self.value["server_id"] = server_id(public_hex(self.key))

    def save(self):
        save_private(self.path, self.value)

    def envelope(self, method, path, data=None):
        credential = self.value["credential"]
        self.value["sequence"] = self.value.get("sequence", 0) + 1
        self.save()  # Reserve sequence durably before sending, including lost replies.
        return signed(self.key, {"v": VERSION, "kind": "announcement", "master_id": credential["origin_master_id"],
                 "server_id": self.value["server_id"], "credential_id": credential["credential_id"],
                 "sequence": self.value["sequence"], "issued_at": int(time.time()), "method": method,
                 "path": path, "metadata": data or {}})

    def send(self, method, suffix="", data=None):
        path = "/v1/servers" + ("/" + self.value["server_id"] if suffix or method != "POST" else "") + suffix
        return request(self.value["master"], method, path, self.envelope(method, path, data))


def enroll(instance, base, token, operator=False, owner="simulator", ttl=86400 * 30):
    identity = request(base, "GET", "/v1/masters")["master"]
    require(master_id(identity["public_key"]) == identity["id"], "Invalid master identity")
    data = {"server_public_key": public_hex(instance.key), "ttl_seconds": ttl}
    if operator:
        data["owner_label"] = owner
    path = "/v1/admin/credentials" if operator else "/v1/credentials"
    challenge = request(base, "POST", path + "/challenge", data, token)
    require(challenge.get("kind") == "enrollment" and challenge.get("master_id") == identity["id"]
            and challenge.get("server_public_key") == data["server_public_key"]
            and challenge.get("ttl_seconds") == ttl, "Invalid enrollment challenge")
    data["proof"] = signed(instance.key, challenge)
    result = request(base, "POST", path, data, token)
    require(result["origin_master_id"] == identity["id"] and result["server_id"] == instance.value["server_id"], "Enrollment identity mismatch")
    instance.value.update(master=base, credential=result, sequence=0)
    instance.save()
    return result


def discover(masters, filters=None):
    errors = []
    query = "?" + urlencode(filters) if filters else ""
    for base in masters:
        try:
            result = request(base, "GET", "/v1/servers" + query)
            return {"master_used": base, "servers": result["servers"]}
        except (OSError, URLError, ApiError, Problem) as error:
            errors.append(type(error).__name__)
    raise RuntimeError("No configured master answered: " + ", ".join(errors))


def simulate(instance, data, heartbeat=20, duration=0, crash_after=0, update_every=0, maps=(), gametypes=()):
    metadata(data)
    stopped = threading.Event()
    for sig in (signal.SIGINT, signal.SIGTERM):
        signal.signal(sig, lambda *_: stopped.set())
    started, next_beat, next_update, iteration = time.monotonic(), 0.0, 0.0, 0
    connected, unavailable = False, False
    try:
        while not stopped.wait(0.05):
            elapsed = time.monotonic() - started
            if crash_after and elapsed >= crash_after:
                os._exit(23)  # Intentional crash test: no unregister/finally.
            if duration and elapsed >= duration:
                break
            changed = bool(update_every and elapsed >= next_update)
            if changed:
                iteration += 1
                data["players"] = iteration % (data["max_players"] + 1)
                if maps:
                    data["map"] = maps[iteration % len(maps)]
                if gametypes:
                    data["gametype"] = gametypes[iteration % len(gametypes)]
                next_update = elapsed + update_every
            beat_due = elapsed >= next_beat
            if not beat_due and not (connected and changed):
                continue
            if beat_due:
                next_beat = elapsed + heartbeat
            try:
                if not connected:
                    try:
                        instance.send("POST", data=data)
                    except ApiError as error:
                        if error.status != 409:
                            raise
                        # A crash/lost reply can leave this instance registered.
                        instance.send("PATCH", data=data)
                    print(json.dumps({"event": "simulator_registered", "server_id": instance.value["server_id"]}), flush=True)
                else:
                    if changed:
                        instance.send("PATCH", data=data)
                    if beat_due:
                        instance.send("POST", "/heartbeat")
                connected, unavailable = True, False
            except (OSError, ApiError) as error:
                if isinstance(error, ApiError) and error.status not in (404, 429, 500, 502, 503, 504):
                    raise
                connected = False
                if not unavailable:
                    print(json.dumps({"event": "simulator_master_unavailable", "retrying": True}), flush=True)
                    unavailable = True
    finally:
        try:
            instance.send("DELETE")
            print(json.dumps({"event": "simulator_unregistered"}), flush=True)
        except (OSError, ApiError):
            print(json.dumps({"event": "simulator_unregister_failed", "expires_automatically": True}), flush=True)


@contextlib.contextmanager
def identity_lock(path):
    lock = Path(str(path) + ".lock")
    lock.parent.mkdir(parents=True, exist_ok=True)
    with lock.open("w") as file:
        fcntl.flock(file, fcntl.LOCK_EX | fcntl.LOCK_NB)
        yield


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    init_master = commands.add_parser("init-master")
    init_master.add_argument("--directory", required=True, type=Path)
    demo = commands.add_parser("init-demo")
    demo.add_argument("--a", required=True, type=Path); demo.add_argument("--b", required=True, type=Path)
    demo.add_argument("--a-url", default="http://127.0.0.1:18081"); demo.add_argument("--b-url", default="http://127.0.0.1:18082")
    for name in ("init-server", "enroll", "run", "revoke"):
        command = commands.add_parser(name)
        command.add_argument("--identity", required=True, type=Path)
        if name in ("enroll", "revoke"):
            command.add_argument("--allow-http", action="store_true")
            group = command.add_mutually_exclusive_group(required=True)
            group.add_argument("--admin-token-file", type=Path)
            group.add_argument("--discord-token-stdin", action="store_true")
        if name == "enroll":
            command.add_argument("--master", required=True)
            command.add_argument("--owner", default="simulator")
            command.add_argument("--ttl", type=int, default=86400 * 30)
        if name == "run":
            command.add_argument("--allow-http", action="store_true")
            command.add_argument("--metadata", type=Path, required=True)
            command.add_argument("--heartbeat", type=float, default=20)
            command.add_argument("--duration", type=float, default=0)
            command.add_argument("--crash-after", type=float, default=0)
            command.add_argument("--update-every", type=float, default=0)
            command.add_argument("--maps", nargs="*", default=[])
            command.add_argument("--gametypes", nargs="*", default=[])
    listing = commands.add_parser("list")
    listing.add_argument("--masters", nargs="+", required=True)
    listing.add_argument("--allow-http", action="store_true")
    for name in ("game-version", "map", "gametype", "region", "server-type", "available-slots"):
        listing.add_argument("--" + name)
    args = parser.parse_args()
    try:
        if args.command == "init-master":
            result = describe_master(args.directory)
        elif args.command == "init-demo":
            result = initialize_demo(args.a, args.b, args.a_url, args.b_url)
        elif args.command == "list":
            for base in args.masters:
                endpoint(base, args.allow_http)
            filters = {k: getattr(args, k) for k in ("game_version", "map", "gametype", "region", "server_type", "available_slots") if getattr(args, k) is not None}
            result = discover(args.masters, filters)
        else:
            with identity_lock(args.identity):
                instance = Instance(args.identity)
                if args.command == "init-server":
                    instance.save()
                    result = {"server_id": instance.value["server_id"], "public_key": public_hex(instance.key)}
                elif args.command == "run":
                    endpoint(instance.value["master"], args.allow_http)
                    require(0.1 <= args.heartbeat <= 60 and args.duration >= 0 and args.crash_after >= 0 and args.update_every >= 0, "Invalid simulator timing")
                    simulate(instance, parse(args.metadata.read_bytes()), args.heartbeat, args.duration, args.crash_after,
                             args.update_every, args.maps, args.gametypes)
                    return
                else:
                    token = args.admin_token_file.read_text().strip() if args.admin_token_file else sys.stdin.readline().strip()
                    base = args.master if args.command == "enroll" else instance.value["master"]
                    endpoint(base, args.allow_http)
                    if args.command == "enroll":
                        result = enroll(instance, base, token, bool(args.admin_token_file), args.owner, args.ttl)
                    else:
                        path = "/v1/admin/credentials/" if args.admin_token_file else "/v1/credentials/"
                        result = request(base, "DELETE", path + instance.value["credential"]["credential_id"], token=token)
        print(json.dumps(result, indent=2))
    except (Problem, ApiError, OSError, ValueError, KeyError, RuntimeError) as error:
        print(json.dumps({"error": str(error)}), file=sys.stderr)
        raise SystemExit(1) from None


if __name__ == "__main__":
    main()
