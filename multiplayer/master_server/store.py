"""Bounded SQLite registry. Only the origin master can advance a listing."""
import sqlite3
import time
import uuid
from pathlib import Path

from .protocol import (MAX_RECORDS, MAX_LEASE, SKEW, VERSION, canonical, parse, fields,
                       integer, identifier, master_id, server_id, metadata, require, signed, verified)

MAX_OWNER_ACTIVE_CREDENTIALS = 8
MAX_OWNER_CREDENTIAL_KEYS = 64


class Store:
    def __init__(self, path, key, lease=90, clock=time.time):
        Path(path).parent.mkdir(parents=True, exist_ok=True)
        self.db = sqlite3.connect(path)
        self.db.row_factory = sqlite3.Row
        self.db.executescript("""
        PRAGMA journal_mode=WAL;
        PRAGMA foreign_keys=ON;
        CREATE TABLE IF NOT EXISTS owners(id TEXT PRIMARY KEY, provider TEXT NOT NULL,
          subject TEXT NOT NULL, UNIQUE(provider,subject));
        CREATE TABLE IF NOT EXISTS credentials(id TEXT PRIMARY KEY, server TEXT UNIQUE NOT NULL,
          owner TEXT REFERENCES owners(id), public_key TEXT NOT NULL, expires INTEGER NOT NULL,
          revoked INTEGER NOT NULL DEFAULT 0, sequence INTEGER NOT NULL DEFAULT 0);
        CREATE TABLE IF NOT EXISTS records(origin TEXT NOT NULL, server TEXT NOT NULL,
          revision INTEGER NOT NULL, expires INTEGER NOT NULL, envelope BLOB NOT NULL,
          PRIMARY KEY(origin,server));
        CREATE TABLE IF NOT EXISTS nonces(peer TEXT NOT NULL, nonce TEXT NOT NULL,
          expires INTEGER NOT NULL, PRIMARY KEY(peer,nonce));
        CREATE TABLE IF NOT EXISTS enrollment_challenges(provider TEXT NOT NULL, subject TEXT NOT NULL,
          payload BLOB PRIMARY KEY, expires INTEGER NOT NULL);
        CREATE TABLE IF NOT EXISTS clock_floor(id INTEGER PRIMARY KEY CHECK(id=1), value INTEGER NOT NULL);
        INSERT OR IGNORE INTO clock_floor VALUES(1,0);
        CREATE TABLE IF NOT EXISTS master_identity(id INTEGER PRIMARY KEY CHECK(id=1), value TEXT NOT NULL);
        CREATE INDEX IF NOT EXISTS active_expiry ON records(expires);
        """)
        from .protocol import public_hex
        self.key, self.id, self.lease, self.clock = key, master_id(public_hex(key)), lease, clock
        with self.db:
            self.db.execute("INSERT OR IGNORE INTO master_identity VALUES(1,?)", (self.id,))
            require(self.db.execute("SELECT value FROM master_identity WHERE id=1").fetchone()[0] == self.id,
                    "Database belongs to another master key")

    def now(self):
        # A wall-clock correction cannot restore an expiration already observed.
        now = max(int(self.clock()), self.db.execute("SELECT value FROM clock_floor WHERE id=1").fetchone()[0])
        self.db.execute("UPDATE clock_floor SET value=? WHERE id=1", (now,))
        return now

    def owner(self, provider, subject):
        row = self.db.execute("SELECT id FROM owners WHERE provider=? AND subject=?", (provider, subject)).fetchone()
        if row:
            return row[0]
        require(self.db.execute("SELECT count(*) FROM owners").fetchone()[0] < MAX_RECORDS, "Owner capacity reached", 503)
        value = str(uuid.uuid4())
        self.db.execute("INSERT INTO owners VALUES(?,?,?)", (value, provider, subject))
        return value

    def enrollment_challenge(self, provider, subject, key, ttl):
        with self.db:
            now = self.now()
            self.db.execute("DELETE FROM enrollment_challenges WHERE expires<=?", (now,))
            require(self.db.execute("SELECT count(*) FROM enrollment_challenges WHERE provider=? AND subject=?",
                                   (provider, subject)).fetchone()[0] < 8,
                    "Owner enrollment challenge limit reached", 429)
            require(self.db.execute("SELECT count(*) FROM enrollment_challenges").fetchone()[0] < MAX_RECORDS,
                    "Enrollment challenge capacity reached", 503)
            payload = dict(v=VERSION, kind="enrollment", master_id=self.id, provider=provider, subject=subject,
                           server_public_key=key, ttl_seconds=ttl, nonce=str(uuid.uuid4()), expires_at=now + 120)
            self.db.execute("INSERT INTO enrollment_challenges VALUES(?,?,?,?)",
                            (provider, subject, canonical(payload), now + 120))
        return payload

    def enroll(self, provider, subject, key, ttl, proof):
        # Commit observed expiry even if the subsequent proof is rejected.
        with self.db:
            now = self.now()
            self.db.execute("DELETE FROM enrollment_challenges WHERE expires<=?", (now,))
        # Verify possession before creating any durable owner/key binding.
        payload = verified(proof, key)
        with self.db:
            now = self.now()
            challenge = self.db.execute("SELECT expires FROM enrollment_challenges WHERE provider=? AND subject=? AND payload=?",
                                        (provider, subject, canonical(payload))).fetchone()
            require(challenge and challenge["expires"] > now,
                    "Enrollment challenge expired or replayed", 401)
            require(payload["master_id"] == self.id and payload["server_public_key"] == key
                    and payload["ttl_seconds"] == ttl, "Enrollment proof binding mismatch", 401)
            owner = self.owner(provider, subject)
            sid = server_id(key)
            previous = self.db.execute("SELECT * FROM credentials WHERE server=?", (sid,)).fetchone()
            require(previous is None, "Server key already enrolled; use a fresh instance key", 409)
            # now() has already acquired SQLite's write transaction. The quota
            # checks and insert therefore serialize across registry processes.
            require(self.db.execute("SELECT count(*) FROM credentials WHERE owner=?", (owner,)).fetchone()[0]
                    < MAX_OWNER_CREDENTIAL_KEYS, "Owner credential key limit reached", 429)
            require(self.db.execute("SELECT count(*) FROM credentials WHERE owner=? AND revoked=0 AND expires>?",
                                    (owner, now)).fetchone()[0] < MAX_OWNER_ACTIVE_CREDENTIALS,
                    "Owner active credential limit reached", 429)
            # Retain inactive keys as replay/ownership history while releasing
            # their live quota. One owner can retain at most 64 historical keys.
            require(self.db.execute("SELECT count(*) FROM credentials WHERE revoked=0 AND expires>?", (now,)).fetchone()[0]
                    < MAX_RECORDS, "Credential capacity reached", 503)
            cid = str(uuid.uuid4())
            expires = now + ttl
            self.db.execute("INSERT INTO credentials(id,server,owner,public_key,expires) VALUES(?,?,?,?,?)",
                            (cid, sid, owner, key, expires))
            self.db.execute("DELETE FROM enrollment_challenges WHERE payload=?", (canonical(payload),))
        return {"credential_id": cid, "server_id": sid, "owner_id": owner,
                "origin_master_id": self.id, "expires_at": expires}

    def credentials(self, provider, subject):
        return [dict(row) for row in self.db.execute("""SELECT c.id AS credential_id,c.server AS server_id,
            c.expires AS expires_at,c.revoked FROM credentials c JOIN owners o ON c.owner=o.id
            WHERE o.provider=? AND o.subject=?""", (provider, subject))]

    def _record(self, credential, state, data, registered=None):
        now = self.now()
        row = self.db.execute("SELECT revision FROM records WHERE origin=? AND server=?", (self.id, credential["server"])).fetchone()
        require(row is not None or self.db.execute("SELECT count(*) FROM records").fetchone()[0] < MAX_RECORDS,
                "Directory capacity reached", 503)
        payload = {"v": VERSION, "kind": "listing", "origin_master_id": self.id,
                   "server_id": credential["server"], "server_public_key": credential["public_key"],
                   "owner_id": credential["owner"], "credential_id": credential["id"],
                   "revision": (row[0] if row else 0) + 1, "registered_at": registered or now,
                   "last_heartbeat": now, "expires_at": min(now + self.lease, credential["expires"]),
                   "state": state, "metadata": data}
        envelope = signed(self.key, payload)
        self._put(envelope)
        return payload

    def _put(self, envelope):
        value = envelope["payload"]
        self.db.execute("INSERT OR REPLACE INTO records VALUES(?,?,?,?,?)",
                        (value["origin_master_id"], value["server_id"], value["revision"],
                         value["expires_at"], canonical(envelope)))

    def revoke(self, cid, provider=None, subject=None):
        identifier(cid)
        with self.db:
            credential = self.db.execute("SELECT * FROM credentials WHERE id=?", (cid,)).fetchone()
            require(credential is not None, "Credential not found", 404)
            if provider is not None:
                owner = self.db.execute("SELECT provider,subject FROM owners WHERE id=?", (credential["owner"],)).fetchone()
                require(tuple(owner) == (provider, subject), "Not the credential owner", 403)
            if not credential["revoked"]:
                self.db.execute("UPDATE credentials SET revoked=1 WHERE id=?", (cid,))
                self._record(credential, "revoked", {})

    def announce(self, envelope, method, path):
        fields(envelope, ("payload", "signature"))
        payload = envelope["payload"]
        fields(payload, ("v", "kind", "master_id", "server_id", "credential_id", "sequence", "issued_at", "method", "path", "metadata"))
        integer(payload["v"], VERSION, VERSION)
        require(payload["kind"] == "announcement", "Invalid announcement kind")
        identifier(payload["server_id"]); identifier(payload["credential_id"])
        integer(payload["sequence"], 1, 2**53 - 1)
        integer(payload["issued_at"], 1, 2**53 - 1)
        require(payload["master_id"] == self.id and payload["method"] == method and payload["path"] == path,
                "Signature audience, method or path mismatch", 401)
        with self.db:
            now = self.now()
            require(abs(now - payload["issued_at"]) <= SKEW, "Announcement outside freshness window", 401)
            credential = self.db.execute("SELECT * FROM credentials WHERE id=?", (payload["credential_id"],)).fetchone()
            require(credential is not None, "Unknown credential", 401)
            verified(envelope, credential["public_key"])
            require(credential["server"] == payload["server_id"], "Credential belongs to another server", 403)
            require(not credential["revoked"] and credential["expires"] > now, "Revoked or expired credential", 403)
            require(payload["sequence"] > credential["sequence"], "Replayed announcement", 409)
            row = self.db.execute("SELECT envelope FROM records WHERE origin=? AND server=?", (self.id, credential["server"])).fetchone()
            previous = parse(row[0])["payload"] if row else None
            if method == "POST" and path == "/v1/servers":
                require(not previous or previous["state"] != "active" or previous["expires_at"] <= now, "Server already registered", 409)
                data, registered = metadata(payload["metadata"]), now
            else:
                require(previous is not None and previous["state"] == "active" and previous["expires_at"] > now, "Registration not active; register again", 404)
                registered = previous["registered_at"]
                if method == "PATCH":
                    fields(payload["metadata"], (), (*previous["metadata"], "region"))
                    data = metadata(previous["metadata"] | payload["metadata"])
                else:
                    require(payload["metadata"] == {}, "Heartbeat/delete metadata must be empty")
                    data = previous["metadata"]
            result = self._record(credential, "deleted" if method == "DELETE" else "active", data, registered)
            self.db.execute("UPDATE credentials SET sequence=? WHERE id=?", (payload["sequence"], credential["id"]))
        return result

    def public(self, value):
        return {"server_id": value["server_id"], "owner_id": value["owner_id"],
                "origin_master_id": value["origin_master_id"], "registered_at": value["registered_at"],
                "last_heartbeat": value["last_heartbeat"], "expires_at": value["expires_at"],
                "reachability": "unverified", **value["metadata"]}

    def discover(self, filters=None):
        filters = filters or {}
        allowed = {"game_version", "map", "gametype", "region", "server_type", "available_slots"}
        require(filters.keys() <= allowed, "Unknown discovery filter")
        slots = integer(int(filters.get("available_slots", "0")), 0, 16)
        with self.db:
            now = self.now()
            rows = self.db.execute("SELECT envelope FROM records WHERE expires>? ORDER BY origin,server", (now,)).fetchall()
        results = {}
        for row in rows:
            value = parse(row[0])["payload"]
            data = value["metadata"]
            if value["state"] != "active" or data["visibility"] != "public":
                continue
            if any(data.get(k) != v for k, v in filters.items() if k != "available_slots"):
                continue
            if data["max_players"] - data["players"] < slots:
                continue
            # Stable conflict resolution for the same key enrolled at two origins.
            results.setdefault(value["server_id"], self.public(value))
        return sorted(results.values(), key=lambda value: value["server_id"])

    def snapshot(self):
        return [parse(row[0]) for row in self.db.execute("SELECT envelope FROM records ORDER BY origin,server")]

    def nonce(self, peer, nonce):
        identifier(nonce)
        with self.db:
            now = self.now()
            self.db.execute("DELETE FROM nonces WHERE expires<=?", (now,))
            require(self.db.execute("SELECT count(*) FROM nonces").fetchone()[0] < 10000, "Federation request capacity reached", 429)
            try:
                self.db.execute("INSERT INTO nonces VALUES(?,?,?)", (peer, nonce, now + SKEW * 2 + 1))
            except sqlite3.IntegrityError:
                require(False, "Replayed federation request", 409)

    def merge(self, records, trusted):
        require(type(records) is list and len(records) <= MAX_RECORDS, "Federation snapshot too large", 413)
        with self.db:
            now = self.now()
            for envelope in records:
                fields(envelope, ("payload", "signature"))
                value = envelope["payload"]
                fields(value, ("v", "kind", "origin_master_id", "server_id", "server_public_key", "owner_id", "credential_id",
                               "revision", "registered_at", "last_heartbeat", "expires_at", "state", "metadata"))
                origin = identifier(value["origin_master_id"])
                if origin == self.id:
                    # Never accept a peer as a writer for our own authoritative bank.
                    continue
                require(origin in trusted, "Untrusted listing origin", 403)
                verified(envelope, trusted[origin])
                require(master_id(trusted[origin]) == origin, "Origin key mismatch", 401)
                integer(value["v"], VERSION, VERSION)
                require(value["kind"] == "listing", "Invalid listing kind")
                for field in ("owner_id", "credential_id", "server_id"):
                    identifier(value[field])
                require(server_id(value["server_public_key"]) == value["server_id"], "Server key/UUID mismatch")
                integer(value["revision"], 1, 2**53 - 1)
                for field in ("registered_at", "last_heartbeat", "expires_at"):
                    integer(value[field], 1, 2**53 - 1)
                require(value["registered_at"] <= value["last_heartbeat"] <= now + SKEW and
                        value["expires_at"] <= value["last_heartbeat"] + MAX_LEASE, "Invalid origin lease")
                require(value["state"] in ("active", "deleted", "revoked"), "Invalid listing state")
                if value["state"] == "active":
                    require(value["expires_at"] > value["last_heartbeat"], "Empty active lease")
                    metadata(value["metadata"])
                else:
                    require(type(value["metadata"]) is dict, "Invalid tombstone")
                    if value["metadata"]:
                        metadata(value["metadata"])
                row = self.db.execute("SELECT revision,envelope FROM records WHERE origin=? AND server=?", (origin, value["server_id"])).fetchone()
                if row and value["revision"] <= row[0]:
                    require(value["revision"] != row[0] or canonical(envelope) == row[1], "Origin equivocation", 409)
                    continue
                require(row is not None or self.db.execute("SELECT count(*) FROM records").fetchone()[0] < MAX_RECORDS, "Directory capacity reached", 503)
                self._put(envelope)

    def cleanup(self):
        # Active rows are selected by absolute expiry; retain one last signed
        # envelope per instance as a durable high-water mark against resurrection.
        with self.db:
            now = self.now()
            self.db.execute("DELETE FROM nonces WHERE expires<=?", (now,))
