"""Steam OpenID identity, with a private proof-bound native handoff.

Steam passwords and the Web API key never reach the game. Account identifiers
stay in private authentication storage; public rosters use chosen Halo names.
"""
import base64
import hashlib
import hmac
import html
import re
import secrets
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from urllib.parse import urlencode, parse_qs

from aiohttp import web
from .protocol import fields, require, text
from .player_identity import VerifiedOwner

OP = "https://steamcommunity.com/openid/login"
NS = "http://specs.openid.net/auth/2.0"
SELECT = NS + "/identifier_select"
MAX_SUBJECT_SESSIONS = 4
REFRESH_LIFETIME = 30 * 86400
MAX_REMEMBERED_SESSIONS = 100_000
MAX_PENDING_PER_SOURCE = 4
MAX_PENDING_PER_PREFIX = 8
MAX_ANONYMOUS_PENDING = 128
MAX_PENDING = 1000


@dataclass
class Pending:
    challenge: str
    expires: float
    status: str = "pending"
    identity: object = None
    source: str = ""
    prefix: str = ""
    confirmation: str = ""
    browser_token: str = ""
    attempts: int = 0


class SteamLogin:
    def __init__(self, directory, clock=time.time):
        self.directory, self.clock = directory, clock
        self.pending = {}
        self.db = directory.store.db
        self.db.executescript("""
        CREATE TABLE IF NOT EXISTS steam_login_sessions(
          token_hash TEXT PRIMARY KEY, subject TEXT NOT NULL, name TEXT NOT NULL,
          expires INTEGER NOT NULL);
        CREATE TABLE IF NOT EXISTS steam_login_nonces(
          nonce TEXT PRIMARY KEY, expires INTEGER NOT NULL);
        CREATE TABLE IF NOT EXISTS steam_login_refresh(
          token_hash TEXT PRIMARY KEY, subject TEXT NOT NULL, name TEXT NOT NULL,
          expires INTEGER NOT NULL);
        CREATE TABLE IF NOT EXISTS steam_login_refresh_sessions(
          session_hash TEXT PRIMARY KEY, refresh_hash TEXT NOT NULL);
        CREATE INDEX IF NOT EXISTS steam_refresh_expiry ON steam_login_refresh(expires);
        CREATE INDEX IF NOT EXISTS steam_refresh_subject ON steam_login_refresh(subject,expires);
        CREATE INDEX IF NOT EXISTS steam_refresh_issued ON steam_login_refresh_sessions(refresh_hash);
        """)

    def wall(self):return self.clock()

    def cleanup(self):
        now = self.clock()
        self.pending = {k: v for k, v in self.pending.items() if v.expires > now}
        with self.db:
            self.db.execute("DELETE FROM steam_login_sessions WHERE expires<=?", (now,))
            self.db.execute("DELETE FROM steam_login_nonces WHERE expires<=?", (now,))
            self.db.execute("DELETE FROM steam_login_refresh WHERE expires<=?", (now,))
            self.db.execute("DELETE FROM steam_login_refresh_sessions WHERE session_hash NOT IN (SELECT token_hash FROM steam_login_sessions)")

    def enabled(self):
        require(self.directory.settings.steam_enabled, "Steam sign-in is not configured", 503)

    def callback_url(self, state):
        return self.directory.settings.public_url + "/v1/auth/steam/callback/" + state

    async def start(self, request):
        self.enabled()
        source = self.directory.source(request)
        prefix = self.directory.source_prefix(source)
        self.directory.rate("steam-start:" + source, 20)
        body = await self.directory.body(request)
        fields(body, ("challenge", "confirmation"))
        challenge = text(body["challenge"], maximum=43)
        require(bool(re.fullmatch(r"[A-Za-z0-9_-]{43}", challenge)), "Invalid Steam login proof")
        confirmation = text(body["confirmation"], maximum=43)
        require(bool(re.fullmatch(r"[A-Za-z0-9_-]{43}", confirmation)), "Invalid confirmation proof")
        self.cleanup()
        anonymous = [p for p in self.pending.values() if p.identity is None]
        occupied = sum(p.source == source for p in anonymous)
        require(occupied < MAX_PENDING_PER_SOURCE and sum(p.prefix == prefix for p in anonymous) < MAX_PENDING_PER_PREFIX,
                "Too many pending Steam sign-ins from this source", 429)
        require(len(anonymous) < MAX_ANONYMOUS_PENDING and len(self.pending) < MAX_PENDING,
                "Steam sign-in capacity reached", 503)
        # Rejected prefix/capacity requests never consume the shared start
        # allowance. Existing provider-verified flows never use this bucket.
        self.directory.rate("global:steam-start", 60, burst=16)
        state = secrets.token_urlsafe(32)
        self.pending[state] = Pending(challenge, self.clock() + 300, source=source, prefix=prefix, confirmation=confirmation)
        url = OP + "?" + urlencode({
            "openid.ns": NS, "openid.mode": "checkid_setup",
            "openid.realm": self.directory.settings.public_url + "/",
            "openid.return_to": self.callback_url(state),
            "openid.identity": SELECT, "openid.claimed_id": SELECT})
        return web.json_response({"device": state, "url": url, "expires_in": 300, "interval": 1},
                                 status=201, headers={"Cache-Control": "no-store"})

    async def profile(self, subject):
        key = self.directory.settings.steam_key
        if not key:
            return "Steam Player"
        from .service import bounded_json
        async with self.directory.session.get(
                "https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v2/",
                params={"key": key, "steamids": subject}, allow_redirects=False) as response:
            require(response.status == 200, "Steam profile service unavailable", 502)
            body = await bounded_json(response)
        rows = body.get("response", {}).get("players", [])
        require(type(rows) is list and len(rows) == 1 and rows[0].get("steamid") == subject,
                "Steam returned a different profile", 502)
        return text(rows[0].get("personaname"), maximum=128)

    async def callback(self, request):
        self.enabled()
        self.directory.rate("steam-callback:" + self.directory.source(request), 40)
        state = request.match_info["state"]
        pending = self.pending.get(state)
        require(pending and pending.expires > self.clock() and pending.status == "pending",
                "Steam sign-in expired or already completed", 400)
        require(len(request.query) <= 16 and all(len(request.query.getall(k)) == 1 for k in request.query),
                "Invalid Steam response")
        values = dict(request.query)
        if values.get("openid.mode") == "cancel":
            pending.status = "declined"
            return self.finish_redirect()
        needed = {"openid.ns", "openid.mode", "openid.op_endpoint", "openid.claimed_id",
                  "openid.identity", "openid.return_to", "openid.response_nonce",
                  "openid.assoc_handle", "openid.signed", "openid.sig"}
        require(needed <= values.keys() and all(k.startswith("openid.") for k in values) and
                all(len(k) <= 64 and len(v) <= 1024 for k, v in values.items()), "Invalid Steam response")
        require(values["openid.ns"] == NS and values["openid.mode"] == "id_res" and
                values["openid.op_endpoint"] == OP and values["openid.return_to"] == self.callback_url(state) and
                values["openid.identity"] == values["openid.claimed_id"], "Steam response has a different destination")
        signed = values["openid.signed"].split(",")
        require(len(signed) == len(set(signed)) and
                {"op_endpoint", "claimed_id", "identity", "return_to", "response_nonce", "assoc_handle"} <= set(signed),
                "Steam response omitted a signed identity field")
        identity = re.fullmatch(r"https?://steamcommunity\.com/openid/id/([0-9]{17})", values["openid.claimed_id"])
        require(identity is not None, "Invalid Steam identity")
        nonce = values["openid.response_nonce"]
        try:
            issued = datetime.strptime(nonce[:20], "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc).timestamp()
        except ValueError:
            require(False, "Invalid Steam response nonce")
        require(20 < len(nonce) <= 256 and abs(self.clock() - issued) <= 300 and
                not self.db.execute("SELECT 1 FROM steam_login_nonces WHERE nonce=?", (nonce,)).fetchone(),
                "Steam response expired or replayed")
        pending.status = "verifying"
        try:
            check = values | {"openid.mode": "check_authentication"}
            async with self.directory.session.post(OP, data=check, allow_redirects=False) as response:
                require(response.status == 200 and not response.headers.get("Content-Encoding"),
                        "Steam verification unavailable", 502)
                raw = await response.content.read(4097)
                require(len(raw) <= 4096, "Steam verification response too large", 502)
            lines = raw.decode("utf-8").splitlines()
            pairs = [line.split(":", 1) for line in lines]
            require(all(len(pair) == 2 for pair in pairs) and len({pair[0] for pair in pairs}) == len(pairs),
                    "Invalid Steam verification response", 502)
            verified = dict(pairs)
            require(verified.get("ns") == NS and verified.get("is_valid") == "true",
                    "Steam signature rejected", 401)
            subject = identity.group(1)
            name = await self.profile(subject)
            require(pending.expires > self.clock(), "Steam sign-in expired")
            with self.db:
                require(not self.db.execute("SELECT 1 FROM steam_login_nonces WHERE nonce=?", (nonce,)).fetchone(),
                        "Steam response replayed")
                self.db.execute("INSERT INTO steam_login_nonces VALUES(?,?)", (nonce, int(self.clock()) + 600))
            pending.identity = VerifiedOwner("steam", subject, name)
            pending.status = "awaiting_confirmation"
            pending.browser_token = secrets.token_urlsafe(32)
        except BaseException:
            pending.status = "failed"
            raise
        path = "/v1/auth/steam/confirm/" + state
        response = web.Response(status=303, headers={"Location": path, "Cache-Control": "no-store",
            "Referrer-Policy": "no-referrer"})
        response.set_cookie("steam-confirm", pending.browser_token, path=path, max_age=300,
                            secure=not self.directory.settings.public_url.startswith("http://"),
                            httponly=True, samesite="Strict")
        return response

    async def confirm(self, request):
        state = request.match_info["state"]
        pending = self.pending.get(state)
        require(pending and pending.expires > self.clock() and pending.status == "awaiting_confirmation" and
                hmac.compare_digest(request.cookies.get("steam-confirm", ""), pending.browser_token),
                "Steam confirmation expired", 401)
        headers = {"Cache-Control": "no-store", "Referrer-Policy": "no-referrer",
                   "Content-Security-Policy": "default-src 'none'; form-action 'self'; frame-ancestors 'none'",
                   "X-Content-Type-Options": "nosniff"}
        if request.method == "GET":
            name = html.escape(pending.identity.username)
            return web.Response(text=f'<!doctype html><meta charset="utf-8"><title>Confirm Halo sign-in</title>'
                f'<h1>Sign in as {name}</h1><p>Enter the confirmation code shown in your Halo 2 game. '
                'Only continue if you started sign-in in that game. Never use a code sent by someone else.</p>'
                '<form method="post"><label>Game confirmation code <input name="code" maxlength="12" '
                'required autocomplete="off"></label><button>Authorize this game</button></form>',
                content_type="text/html", headers=headers)
        require(request.headers.get("Origin") == self.directory.settings.public_url,
                "Different confirmation origin", 403)
        raw = await request.content.read(1025)
        require(len(raw) <= 1024, "Confirmation too large")
        try:
            values = parse_qs(raw.decode("ascii"), strict_parsing=True)
        except (ValueError, UnicodeError):
            require(False, "Invalid confirmation")
        require(set(values) == {"code"} and len(values["code"]) == 1, "Invalid confirmation")
        pending.attempts += 1
        code = values["code"][0].strip().upper()
        digest = base64.urlsafe_b64encode(hashlib.sha256(code.encode("ascii")).digest()).rstrip(b"=").decode("ascii")
        accepted = bool(re.fullmatch(r"[0-9A-F]{12}", code)) and hmac.compare_digest(digest, pending.confirmation)
        if not accepted:
            if pending.attempts >= 5:
                pending.status = "failed"
                pending.browser_token = ""
            require(False, "Game confirmation code rejected", 401)
        # Both the immutable verified identity and client-generated code belong
        # to this single pending object; polling alone can never authorize it.
        require(self.pending.get(state) is pending and pending.status == "awaiting_confirmation" and
                pending.expires > self.clock(), "Steam confirmation expired", 401)
        pending.status = "complete"
        pending.browser_token = ""
        return self.finish_redirect()

    @staticmethod
    def finish_redirect():
        # The signed assertion and private account ID need not remain in the
        # visible confirmation URL. This page never carries an auth credential.
        return web.Response(status=303, headers={"Location": "/v1/auth/steam/complete",
            "Cache-Control": "no-store", "Referrer-Policy": "no-referrer"})

    async def complete_page(self, request):
        return self.finish_page("Steam sign-in response received. Return to Halo 2 to check your sign-in status.")

    @staticmethod
    def finish_page(message):
        return web.Response(text=message, content_type="text/plain", headers={
            "Cache-Control": "no-store", "X-Content-Type-Options": "nosniff",
            "Content-Security-Policy": "default-src 'none'", "Referrer-Policy": "no-referrer"})

    async def poll(self, request):
        self.enabled()
        body = await self.directory.body(request)
        fields(body, ("device", "verifier"))
        state = text(body["device"], maximum=43)
        verifier = text(body["verifier"], maximum=128)
        require(bool(re.fullmatch(r"[A-Za-z0-9_-]{43}", state)) and
                bool(re.fullmatch(r"[A-Za-z0-9_-]{43,128}", verifier)), "Invalid Steam login proof")
        self.directory.rate("steam-poll:" + self.directory.source(request), 600)
        self.directory.rate("steam-device:" + state, 120)
        pending = self.pending.get(state)
        challenge = base64.urlsafe_b64encode(hashlib.sha256(verifier.encode("ascii")).digest()).rstrip(b"=").decode("ascii")
        require(pending and pending.expires > self.clock() and hmac.compare_digest(pending.challenge, challenge),
                "Steam sign-in expired or proof rejected", 401)
        headers = {"Cache-Control": "no-store"}
        if pending.status != "complete":
            return web.json_response({"state": pending.status}, headers=headers)
        owner = pending.identity
        self.cleanup()
        token = "h2s_" + secrets.token_urlsafe(32)
        refresh = "h2r_" + secrets.token_urlsafe(32)
        with self.db:
            # The first DELETE acquires the write transaction before counting
            # capacity. Retain the newest sessions and rotate this subject's
            # oldest tokens without evicting any other account's sessions.
            self.db.execute("""DELETE FROM steam_login_sessions WHERE subject=? AND token_hash NOT IN
                (SELECT token_hash FROM steam_login_sessions WHERE subject=?
                 ORDER BY expires DESC,rowid DESC LIMIT ?)""", (owner.subject, owner.subject, MAX_SUBJECT_SESSIONS - 1))
            require(self.db.execute("SELECT count(*) FROM steam_login_sessions").fetchone()[0] < 1000,
                    "Steam identity capacity reached", 503)
            self.db.execute("INSERT INTO steam_login_sessions VALUES(?,?,?,?)",
                            (hashlib.sha256(token.encode()).hexdigest(), owner.subject, owner.username, int(self.clock()) + 28800))
            self.db.execute("DELETE FROM steam_login_refresh WHERE subject=? AND token_hash NOT IN "
                            "(SELECT token_hash FROM steam_login_refresh WHERE subject=? ORDER BY expires DESC,rowid DESC LIMIT ?)",
                            (owner.subject,owner.subject,MAX_SUBJECT_SESSIONS-1))
            # A remembered-session quota must never reject a verified login.
            # Its thirty-day retention budget is independent of the much
            # shorter active-session budget and never evicts another player.
            if self.db.execute("SELECT count(*) FROM steam_login_refresh").fetchone()[0] < MAX_REMEMBERED_SESSIONS:
                self.db.execute("INSERT INTO steam_login_refresh VALUES(?,?,?,?)",
                                (hashlib.sha256(refresh.encode()).hexdigest(),owner.subject,owner.username,int(self.clock())+REFRESH_LIFETIME))
                self.db.execute("INSERT INTO steam_login_refresh_sessions VALUES(?,?)",
                                (hashlib.sha256(token.encode()).hexdigest(),hashlib.sha256(refresh.encode()).hexdigest()))
            else:
                refresh = None
        del self.pending[state]
        result={"state": "signed_in", "token": token, "expires_in": 28800,
                "identity": {"id": owner.subject, "username": owner.username, "display_name": owner.username}}
        if refresh:
            result["refresh_token"]=refresh
        return web.json_response(result, headers=headers)

    def refresh_credential(self, request):
        token = request.headers.get("Authorization", "")
        require(re.fullmatch(r"Bearer h2r_[A-Za-z0-9_-]{43}", token), "Saved Steam sign-in rejected", 401)
        return hashlib.sha256(token[7:].encode()).hexdigest()

    async def resume(self, request):
        self.enabled()
        self.directory.rate("steam-resume:"+self.directory.source(request), 30)
        fields(await self.directory.body(request), ())
        digest = self.refresh_credential(request)
        self.cleanup()
        row = self.db.execute("SELECT subject,name,expires FROM steam_login_refresh WHERE token_hash=?", (digest,)).fetchone()
        require(row and row[2]>self.clock(), "Saved Steam sign-in expired", 401)
        token = "h2s_"+secrets.token_urlsafe(32)
        with self.db:
            self.db.execute("DELETE FROM steam_login_sessions WHERE subject=? AND token_hash NOT IN "
                            "(SELECT token_hash FROM steam_login_sessions WHERE subject=? ORDER BY expires DESC,rowid DESC LIMIT ?)",
                            (row[0],row[0],MAX_SUBJECT_SESSIONS-1))
            require(self.db.execute("SELECT count(*) FROM steam_login_sessions").fetchone()[0]<1000,
                    "Steam identity capacity reached",503)
            self.db.execute("INSERT INTO steam_login_sessions VALUES(?,?,?,?)",
                            (hashlib.sha256(token.encode()).hexdigest(),row[0],row[1],int(self.clock())+28800))
            self.db.execute("INSERT INTO steam_login_refresh_sessions VALUES(?,?)",(hashlib.sha256(token.encode()).hexdigest(),digest))
        return web.json_response({"state":"signed_in","token":token,"expires_in":28800,
            "identity":{"id":row[0],"username":row[1],"display_name":row[1]}},headers={"Cache-Control":"no-store"})

    async def forget(self, request):
        self.directory.rate("steam-forget:"+self.directory.source(request),30)
        fields(await self.directory.body(request), ())
        digest = self.refresh_credential(request)
        with self.db:
            self.db.execute("DELETE FROM steam_login_refresh WHERE token_hash=?",(digest,))
            self.db.execute("DELETE FROM steam_login_sessions WHERE token_hash IN (SELECT session_hash FROM steam_login_refresh_sessions WHERE refresh_hash=?)",(digest,))
            self.db.execute("DELETE FROM steam_login_refresh_sessions WHERE refresh_hash=?",(digest,))
        return web.json_response({"state":"signed_out"},headers={"Cache-Control":"no-store"})

    async def owner(self, token):
        require(bool(re.fullmatch(r"h2s_[A-Za-z0-9_-]{43}", token)), "Steam identity rejected", 401)
        row = self.db.execute("SELECT subject,name,expires FROM steam_login_sessions WHERE token_hash=?",
                              (hashlib.sha256(token.encode()).hexdigest(),)).fetchone()
        require(row and row[2] > self.clock(), "Steam sign-in expired", 401)
        return VerifiedOwner("steam", row[0], row[1])

    async def link_discord(self, request):
        """Typed provider IO; Rust owns canonical authorization and SQL adoption."""
        from .native_party_directory import run
        body=await self.directory.body(request)
        def features():
            return dict(header=request.headers.get('Authorization',''),steam_enabled=self.directory.settings.steam_enabled,
                        registration_now=int(self.directory.store.clock()))
        run(self,'link_preflight',family='identity',body=body,features=features())
        # Keep the shared provider/source admission budget while its generic
        # verification middleware is still a temporary migration boundary.
        await self.directory.owner(request)
        social=await self.directory.verify_owner(self.directory.session,body['discord_token'],self.directory.settings.discord_client_id)
        verified=dict(provider=social.provider,subject=social.subject,username=social.username) if isinstance(social,VerifiedOwner) else None
        result=run(self,'link',family='identity',body=body,features=features(),verified_owner=verified)
        return web.json_response(result,headers={'Cache-Control':'no-store'})

    def routes(self):
        return [web.post("/v1/auth/steam/start", self.start),
                web.get("/v1/auth/steam/complete", self.complete_page),
                web.get("/v1/auth/steam/callback/{state}", self.callback),
                web.get("/v1/auth/steam/confirm/{state}", self.confirm),
                web.post("/v1/auth/steam/confirm/{state}", self.confirm),
                web.post("/v1/auth/steam/poll", self.poll),
                web.post("/v1/auth/steam/resume", self.resume),
                web.post("/v1/auth/steam/forget", self.forget),
                web.post("/v1/account/link-discord", self.link_discord)]
