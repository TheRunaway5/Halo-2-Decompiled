"""HTTP directory, owner enrollment and pinned-peer anti-entropy."""
import asyncio
import contextlib
import hmac
import ipaddress
import json
import logging
import os
import re
import ssl
import time
import uuid
from dataclasses import dataclass, field
from pathlib import Path
from urllib.parse import urlsplit

from aiohttp import ClientError, ClientSession, ClientTimeout, TCPConnector, web

from .protocol import (VERSION, MAX_BODY, MAX_SYNC, MAX_LEASE, SKEW, Problem, canonical, parse,
                       fields, text, integer, identifier, public_key, public_hex, master_id,
                       load_key, require, signed, verified)
from .store import Store
from .player_identity import VerifiedOwner, VERIFIED_DISCORD_USERNAME

LOG = logging.getLogger("halo2.directory")


def endpoint(value, allow_http=False):
    parts = urlsplit(value)
    require(parts.scheme == "https" or (allow_http and parts.scheme == "http"), "Endpoint requires HTTPS")
    require(parts.hostname and parts.port != 0 and not parts.username and not parts.password and
            not parts.query and not parts.fragment and parts.path in ("", "/"), "Invalid master endpoint")
    return value.rstrip("/")


@dataclass
class Settings:
    state: Path = Path(".local/master")
    bind: str = "127.0.0.1"
    port: int = 8080
    public_url: str = "https://localhost:8080"
    allow_http: bool = False
    behind_tls_proxy: bool = False
    cert: str = ""
    tls_key: str = ""
    admin_token: str = ""
    lease: int = 90
    sync_interval: float = 15
    rate_per_minute: int = 600
    discord_client_id: str = "1558522073272426519"
    peers: list = field(default_factory=list)
    matchmaking_catalog: str = ""

    @classmethod
    def environment(cls):
        state = Path(os.environ.get("H2_MASTER_STATE", ".local/master"))
        token_path = os.environ.get("H2_MASTER_ADMIN_TOKEN_FILE", "")
        peers_path = Path(os.environ.get("H2_MASTER_PEERS_FILE", str(state / "peers.json")))
        return cls(state=state, bind=os.environ.get("H2_MASTER_BIND", "127.0.0.1"),
                   port=int(os.environ.get("H2_MASTER_PORT", "8080")),
                   public_url=os.environ.get("H2_MASTER_PUBLIC_URL", "https://localhost:8080"),
                   allow_http=os.environ.get("H2_MASTER_ALLOW_HTTP") == "1",
                   behind_tls_proxy=os.environ.get("H2_MASTER_BEHIND_TLS_PROXY") == "1",
                   cert=os.environ.get("H2_MASTER_TLS_CERT", ""), tls_key=os.environ.get("H2_MASTER_TLS_KEY", ""),
                   admin_token=Path(token_path).read_text().strip() if token_path else "",
                   lease=int(os.environ.get("H2_MASTER_LEASE_SECONDS", "90")),
                   sync_interval=float(os.environ.get("H2_MASTER_SYNC_SECONDS", "15")),
                   rate_per_minute=int(os.environ.get("H2_MASTER_RATE_PER_MINUTE", "600")),
                   discord_client_id=os.environ.get("H2_DISCORD_CLIENT_ID", "1558522073272426519"),
                   peers=parse(peers_path.read_bytes()) if peers_path.exists() else [],
                   matchmaking_catalog=os.environ.get("H2_MATCHMAKING_CATALOG", ""))

    def validate(self):
        endpoint(self.public_url, self.allow_http)
        integer(self.port, 1, 65535); integer(self.lease, 1, MAX_LEASE)
        integer(self.rate_per_minute, 1, 100000)
        require(0.1 <= self.sync_interval <= 3600, "Invalid sync interval")
        require(bool(re.fullmatch(r"[0-9]{17,20}", self.discord_client_id)), "Invalid Discord application ID")
        require(self.allow_http or self.behind_tls_proxy or (self.cert and self.tls_key),
                "Configure direct TLS or an isolated TLS reverse proxy; HTTP is development-only")
        require(not self.admin_token or len(self.admin_token) >= 32, "Admin token must contain at least 32 characters")
        require(type(self.peers) is list and len(self.peers) <= 8, "Maximum eight configured peers")
        seen = set()
        for peer in self.peers:
            fields(peer, ("id", "public_key", "url"))
            require(master_id(public_key(peer["public_key"])) == identifier(peer["id"]), "Pinned peer key/ID mismatch")
            endpoint(peer["url"], self.allow_http)
            require(peer["id"] not in seen, "Duplicate peer")
            seen.add(peer["id"])


async def bounded_json(response, limit=MAX_BODY):
    require(not response.headers.get("Content-Encoding"), "Compressed upstream responses are not accepted", 502)
    require(response.content_length is None or response.content_length <= limit, "Response too large", 502)
    body = bytearray()
    async for chunk in response.content.iter_chunked(8192):
        body.extend(chunk)
        require(len(body) <= limit, "Response too large", 502)
    return parse(body)


async def discord_owner(session, token, client_id):
    # Reuse the existing OAuth access token; no login, token exchange or account
    # password system here. Verify app/scope as well as identity server-side.
    require(bool(token) and len(token) <= 2048 and "\n" not in token and "\r" not in token, "Invalid bearer token", 401)
    headers = {"Authorization": "Bearer " + token}
    async with session.get("https://discord.com/api/v10/oauth2/@me", headers=headers, allow_redirects=False) as response:
        require(response.status == 200, "Discord authorization rejected", 401)
        authorization = await bounded_json(response)
    require(type(authorization) is dict and authorization.get("application", {}).get("id") == client_id and
            type(authorization.get("scopes")) is list and "identify" in authorization["scopes"],
            "Wrong Discord application or scope", 403)
    async with session.get("https://discord.com/api/v10/users/@me", headers=headers, allow_redirects=False) as response:
        require(response.status == 200, "Discord identity rejected", 401)
        user = await bounded_json(response)
    require(type(user) is dict and type(user.get("id")) is str and bool(re.fullmatch(r"[0-9]{17,20}", user["id"])) and
            user.get("bot") is not True, "Invalid Discord owner", 403)
    require(type(user.get("username")) is str and bool(re.fullmatch(r"[a-z0-9_.]{2,32}", user["username"])), "Invalid Discord username", 403)
    return VerifiedOwner("discord", user["id"], user["username"])


class Directory:
    def __init__(self, settings, owner_verifier=None, clock=time.time):
        settings.validate()
        self.settings = settings
        settings.state.mkdir(parents=True, exist_ok=True, mode=0o700)
        settings.state.chmod(0o700)
        key_path, db_path = settings.state / "master-key.json", settings.state / "registry.sqlite3"
        require(key_path.exists() or not db_path.exists(), "Existing database is missing its master key")
        self.key = load_key(key_path)
        self.store = Store(db_path, self.key, settings.lease, clock)
        self.id = self.store.id
        self.trusted = {p["id"]: p["public_key"] for p in settings.peers}
        require(self.id not in self.trusted, "Master cannot peer with itself")
        self.peer_status = {p["id"]: {"last_success": None, "failures": 0} for p in settings.peers}
        self.buckets = {}
        self.started = time.monotonic()
        self.verify_owner = owner_verifier or discord_owner
        self.session = None
        self.maintenance_task = None
        from .matchmaking_http import MatchmakingApi, load_catalog
        self.matchmaking = MatchmakingApi(self, load_catalog(settings.matchmaking_catalog))
        from .clans_http import ClanApi
        self.clans = ClanApi(self)
        from .voice_blocks import VoiceBlocks
        self.voice_blocks = VoiceBlocks(self)
        from .clan_voice import ClanVoice
        self.clan_voice = ClanVoice(self.clans)
        from .parties_http import PartyApi
        self.parties = PartyApi(self)
        from .clan_voice import PartyVoice
        self.party_voice = PartyVoice(self.clans, self.parties)

    def rate(self, key, limit=None):
        now = time.monotonic()
        # Bound attacker-controlled source-address bookkeeping too.
        if len(self.buckets) >= 4096:
            self.buckets = {k: v for k, v in self.buckets.items() if now - v[1] < 60}
            require(key in self.buckets or len(self.buckets) < 4096, "Rate limiter capacity reached", 429)
        maximum = limit or self.settings.rate_per_minute
        tokens, previous = self.buckets.get(key, (float(maximum), now))
        tokens = min(maximum, tokens + (now - previous) * maximum / 60)
        require(tokens >= 1, "Rate limit exceeded", 429)
        self.buckets[key] = (tokens - 1, now)

    async def body(self, request, limit=MAX_BODY):
        require(request.content_type == "application/json", "Use application/json", 415)
        require(not request.headers.get("Content-Encoding"), "Compressed request bodies are not accepted", 415)
        require(request.content_length is None or request.content_length <= limit, "Request too large", 413)
        raw = bytearray()
        async with asyncio.timeout(5):
            async for chunk in request.content.iter_chunked(8192):
                raw.extend(chunk)
                require(len(raw) <= limit, "Request too large", 413)
        return parse(raw)

    async def owner(self, request, admin=False):
        token = request.headers.get("Authorization", "")
        require(token.startswith("Bearer "), "Bearer credential required", 401)
        token = token[7:]
        if admin:
            require(self.settings.admin_token and hmac.compare_digest(token, self.settings.admin_token), "Operator authorization rejected", 403)
            return None
        self.rate("owner:" + self.source(request), 20)
        owner = await self.verify_owner(self.session, token, self.settings.discord_client_id)
        if isinstance(owner, VerifiedOwner):
            request[VERIFIED_DISCORD_USERNAME] = owner.username
            owner = (owner.provider, owner.subject)
        self.rate("owner-id:" + ":".join(owner), 20)
        return owner

    def source(self, request):
        # Explicit proxy mode requires an isolated backend and a proxy which
        # overwrites/appends X-Forwarded-For (the supplied Caddy configuration).
        value = request.remote or "unknown"
        if self.settings.behind_tls_proxy:
            forwarded = request.headers.get("X-Forwarded-For", "").split(",")[-1].strip()
            if forwarded:
                value = str(ipaddress.ip_address(forwarded))
        return value

    async def health(self, request):
        self.store.db.execute("SELECT 1").fetchone()
        require(self.maintenance_task is not None and not self.maintenance_task.done(), "Maintenance worker unavailable", 503)
        return web.json_response({"status": "ok", "ready": True, "master_id": self.id,
                                  "uptime_seconds": int(time.monotonic() - self.started),
                                  "peers": self.peer_status})

    async def masters(self, request):
        return web.json_response({"master": {"id": self.id, "public_key": public_hex(self.key),
                                             "url": self.settings.public_url}, "peers": self.settings.peers})

    async def discover(self, request):
        require(len(request.query) == len(set(request.query)), "Duplicate discovery filter")
        values = self.store.discover(dict(request.query))
        if "server_id" in request.match_info:
            sid = identifier(request.match_info["server_id"])
            values = [v for v in values if v["server_id"] == sid]
            require(bool(values), "Server not found", 404)
            return web.json_response(values[0])
        return web.json_response({"servers": values})

    async def enroll(self, request):
        admin = request.path.startswith("/v1/admin/")
        owner = await self.owner(request, admin)
        body = await self.body(request)
        fields(body, ("server_public_key",), ("ttl_seconds", "owner_label") if admin else ("ttl_seconds",))
        ttl = integer(body.get("ttl_seconds", 86400 * 30), 1, 86400 * 90)
        key = public_key(body["server_public_key"])
        if admin:
            owner = ("operator", text(body.get("owner_label", "operator-bootstrap")))
        result = self.store.enroll(*owner, key, ttl)
        LOG.info(json.dumps({"event": "credential_enrolled", "credential_id": result["credential_id"], "provider": owner[0]}))
        return web.json_response(result, status=201)

    async def credentials(self, request):
        owner = await self.owner(request)
        return web.json_response({"credentials": self.store.credentials(*owner)})

    async def revoke(self, request):
        admin = request.path.startswith("/v1/admin/")
        owner = await self.owner(request, admin)
        self.store.revoke(request.match_info["credential_id"], *(owner or ()))
        return web.json_response({"revoked": True})

    async def announce(self, request):
        if "server_id" in request.match_info:
            sid = identifier(request.match_info["server_id"])
        else:
            sid = None
        envelope = await self.body(request)
        fields(envelope, ("payload", "signature"))
        if sid:
            require(type(envelope["payload"]) is dict and envelope["payload"].get("server_id") == sid, "Server path mismatch", 403)
        value = self.store.announce(envelope, request.method, request.path)
        return web.json_response(self.store.public(value), status=201 if request.path == "/v1/servers" else 200)

    def sync_payload(self, value, kind, sender, nonce=None):
        fields(value, ("v", "kind", "sender_id", "recipient_id", "issued_at", "nonce"), ("records",) if kind == "snapshot" else ())
        integer(value["v"], VERSION, VERSION)
        require(value["kind"] == kind and value["sender_id"] == sender and
                value["recipient_id"] == self.id, "Federation audience or kind mismatch", 401)
        integer(value["issued_at"], 1, 2**53 - 1)
        require(abs(int(self.store.clock()) - value["issued_at"]) <= SKEW, "Federation request expired", 401)
        identifier(value["nonce"])
        if nonce:
            require(value["nonce"] == nonce, "Federation response nonce mismatch", 401)

    async def sync(self, request):
        envelope = await self.body(request)
        fields(envelope, ("payload", "signature"))
        require(type(envelope["payload"]) is dict, "Invalid federation envelope")
        sender = identifier(envelope["payload"].get("sender_id"))
        require(sender in self.trusted, "Untrusted federation peer", 403)
        value = verified(envelope, self.trusted[sender])
        self.sync_payload(value, "sync", sender)
        self.rate("peer:" + sender, 600)
        self.store.nonce(sender, value["nonce"])
        response = signed(self.key, {"v": VERSION, "kind": "snapshot", "sender_id": self.id,
                    "recipient_id": sender, "issued_at": int(self.store.clock()), "nonce": value["nonce"],
                    "records": self.store.snapshot()})
        raw = canonical(response)
        require(len(raw) <= MAX_SYNC, "Snapshot exceeds federation size limit", 503)
        return web.Response(body=raw, content_type="application/json")

    async def pull(self, peer):
        nonce = str(uuid.uuid4())
        envelope = signed(self.key, {"v": VERSION, "kind": "sync", "sender_id": self.id,
                            "recipient_id": peer["id"], "issued_at": int(self.store.clock()), "nonce": nonce})
        try:
            async with self.session.post(peer["url"].rstrip("/") + "/v1/federation/sync", json=envelope, allow_redirects=False) as response:
                require(response.status == 200, "Peer synchronization rejected", 502)
                result = await bounded_json(response, MAX_SYNC)
            value = verified(result, peer["public_key"])
            self.sync_payload(value, "snapshot", peer["id"], nonce)
            self.store.merge(value.get("records"), self.trusted)
            self.peer_status[peer["id"]] = {"last_success": int(time.time()), "failures": 0}
        except (Problem, ClientError, OSError, TimeoutError, ValueError) as error:
            status = self.peer_status[peer["id"]]
            status["failures"] += 1
            # Log transitions, not every unchanged partition poll. No credentials.
            if status["failures"] == 1:
                LOG.warning(json.dumps({"event": "peer_unavailable", "peer_id": peer["id"], "error": type(error).__name__}))

    async def maintenance(self):
        while True:
            self.store.cleanup()
            self.matchmaking.cleanup()
            if hasattr(self, "match_transport"):
                await self.match_transport.cleanup()
            self.clans.cleanup()
            self.parties.cleanup()
            await asyncio.gather(*(self.pull(peer) for peer in self.settings.peers))
            await asyncio.sleep(self.settings.sync_interval)

    async def lifetime(self, app):
        self.session = ClientSession(timeout=ClientTimeout(total=5, connect=2), auto_decompress=False,
                                     headers={"Accept-Encoding": "identity"},
                                     connector=TCPConnector(limit=16, limit_per_host=4), trust_env=False)
        task = self.maintenance_task = asyncio.create_task(self.maintenance())
        LOG.info(json.dumps({"event": "master_started", "master_id": self.id, "http_development": self.settings.allow_http}))
        try:
            yield
        finally:
            task.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await task
            await self.session.close()
            self.store.db.close()
            LOG.info(json.dumps({"event": "master_stopped", "master_id": self.id}))


def create_app(settings, owner_verifier=None, clock=time.time):
    directory = Directory(settings, owner_verifier, clock)

    @web.middleware
    async def boundary(request, handler):
        started = time.monotonic()
        status = 500
        try:
            require(len(request.raw_path) <= 1024, "Request path too long", 414)
            require(not request.headers.get("Content-Encoding"), "Compressed bodies are not accepted", 415)
            require(settings.allow_http or settings.behind_tls_proxy or request.secure, "HTTPS required", 403)
            directory.rate("ip:" + directory.source(request))
            result = await handler(request)
            status = result.status
            result.headers["Cache-Control"] = "no-store"
            return result
        except Problem as error:
            status = error.status
            return web.json_response({"error": error.message}, status=status, headers={"Cache-Control": "no-store"})
        except web.HTTPException as error:
            status = error.status
            return web.json_response({"error": error.reason}, status=status)
        except (ValueError, TypeError, KeyError, UnicodeError):
            status = 400
            return web.json_response({"error": "Malformed request"}, status=status)
        except TimeoutError:
            status = 504
            return web.json_response({"error": "Operation timed out"}, status=status)
        except ClientError:
            status = 502
            return web.json_response({"error": "Identity provider unavailable"}, status=status)
        finally:
            # Route templates only: no tokens, IPs, raw query text or owner IDs.
            route = request.match_info.route.resource
            LOG.info(json.dumps({"event": "request", "route": route.canonical if route else "unknown",
                                 "method": request.method, "status": status,
                                 "duration_ms": round((time.monotonic() - started) * 1000, 2)}))

    app = web.Application(middlewares=[boundary], client_max_size=MAX_BODY,
                          handler_args={"auto_decompress": False, "max_field_size": 4096, "max_line_size": 2048})
    app[web.AppKey("directory", Directory)] = directory
    app.cleanup_ctx.append(directory.lifetime)
    app.add_routes([
        web.get("/v1/health", directory.health), web.get("/v1/masters", directory.masters),
        web.get("/v1/servers", directory.discover), web.post("/v1/servers", directory.announce),
        web.get("/v1/servers/{server_id}", directory.discover),
        web.patch("/v1/servers/{server_id}", directory.announce),
        web.delete("/v1/servers/{server_id}", directory.announce),
        web.post("/v1/servers/{server_id}/heartbeat", directory.announce),
        web.post("/v1/credentials", directory.enroll), web.get("/v1/credentials", directory.credentials),
        web.delete("/v1/credentials/{credential_id}", directory.revoke),
        web.post("/v1/admin/credentials", directory.enroll),
        web.delete("/v1/admin/credentials/{credential_id}", directory.revoke),
        web.post("/v1/federation/sync", directory.sync),
    ])
    app.add_routes(directory.matchmaking.routes())
    from .search_qos import SearchQos
    directory.search_qos = SearchQos(directory.matchmaking)
    app.add_routes(directory.search_qos.routes())
    app.on_shutdown.append(directory.search_qos.close)
    from .gathering_sessions import GatheringSessions
    directory.matchmaking.gatherings = GatheringSessions(directory.matchmaking)
    from .match_transport import MatchTransport
    directory.match_transport = MatchTransport(directory.matchmaking)
    app.add_routes(directory.match_transport.routes())
    app.on_shutdown.append(directory.match_transport.close)
    app.add_routes(directory.clans.routes())
    app.router.add_put("/v1/account/voice-blocks", directory.voice_blocks.update)
    app.add_routes(directory.parties.routes())
    app.router.add_get("/v1/clans/voice", directory.clan_voice.connect)
    app.on_shutdown.append(directory.clan_voice.close)
    app.router.add_get("/v1/party/voice", directory.party_voice.connect)
    app.on_shutdown.append(directory.party_voice.close)
    return app


def main():
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    logging.getLogger("aiohttp.access").disabled = True
    settings = Settings.environment()
    context = None
    if settings.cert and settings.tls_key:
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.minimum_version = ssl.TLSVersion.TLSv1_2
        context.load_cert_chain(settings.cert, settings.tls_key)
    web.run_app(create_app(settings), host=settings.bind, port=settings.port,
                ssl_context=context, access_log=None, print=None, shutdown_timeout=5)
