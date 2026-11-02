"""Private populated legacy-state backup, Rust adoption and rollback controls.

The original state is generated through the real legacy HTTP/room adapter and
production Rust ABI. Provider credentials, map facts, PCR and aliases are
declared fixtures. No played match, real production snapshot or image rollback
is inferred from this host process comparison.
"""
from contextlib import closing
import asyncio
import hashlib
import http.client
import importlib.util
import json
import os
from pathlib import Path
import shutil
import sqlite3
import subprocess
import sys
import time
import uuid


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def sha(data):
    return hashlib.sha256(data).hexdigest()


def sql(path, query, values=()):
    with closing(sqlite3.connect(path)) as db, db:
        return db.execute(query, values).fetchall()


def snapshot(path):
    """Order-independent row fingerprints; reports disclose no private rows."""
    with closing(sqlite3.connect(path)) as db:
        db.execute("PRAGMA query_only=ON")
        tables = [row[0] for row in db.execute("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name")]
        result = {}
        quote = lambda name: '"' + name.replace('"', '""') + '"'
        for table in tables:
            columns = [row[1] for row in db.execute("PRAGMA table_info(" + quote(table) + ")")]
            rows = db.execute("SELECT * FROM " + quote(table) + " ORDER BY " + ",".join(map(quote, columns)))
            digest, count = hashlib.sha256(), 0
            for row in rows:
                values = [{"blob": value.hex()} if isinstance(value, bytes) else value for value in row]
                encoded = json.dumps(values, separators=(",", ":"), ensure_ascii=True).encode()
                digest.update(len(encoded).to_bytes(8, "big")); digest.update(encoded); count += 1
            result[table] = dict(rows=count, sha256=digest.hexdigest(), columns=columns)
        return result


def clone_state(source, target):
    """Use SQLite backup, including committed WAL pages, and preserve keys."""
    target.mkdir(mode=0o700)
    deadline = time.monotonic() + 30
    def progress(status, remaining, total):
        if time.monotonic() > deadline:
            raise TimeoutError("Private SQLite backup exceeded30s")
    with closing(sqlite3.connect((source / "registry.sqlite3").as_uri() + "?mode=ro", uri=True)) as origin:
        origin.execute("PRAGMA query_only=ON")
        with closing(sqlite3.connect(target / "registry.sqlite3")) as destination:
            origin.backup(destination, pages=128, progress=progress, sleep=0.01)
    (target / "registry.sqlite3").chmod(0o600)
    for name in ("master-key.json", "eos-openid-key.pem"):
        shutil.copyfile(source / name, target / name)
        (target / name).chmod(0o600)


def populate_legacy(scratch, catalog, Legacy):
    """Normal legacy room flow, without Rust-specific HTTP fault controls."""
    import aiohttp
    scratch.mkdir()
    entries = json.loads(catalog.read_text())["playlists"]
    entry = dict(id="state-room-fixture", revision="state-room-v1", name="State room fixture",
                 description="Declared legacy state producer.", ranked=False, quickmatch=False, team_size=0,
                 minimum_players=3, maximum_players=16, maximum_party=2, skill=dict(kind="custom", radius=0),
                 stages=[dict(after_seconds=0, rank_numerator=1, rank_denominator=1, minimum_connection=50)],
                 required_compatibility="halo2-native-queue-v10", games=[dict(id="slayer", map="lockout", variant="Slayer", weight=1)],
                 rank_configuration=next(row["rank_configuration"] for row in entries if row["id"] == "team-slayer"),
                 provenance="Private legacy state fixture, synthetic PCR; no original service or played-game claim.")
    fixture = scratch / "catalog.json"
    fixture.write_text(json.dumps(dict(version=1, playlists=[entry])))
    server = Legacy(None, scratch / "state", scratch, fixture)
    database = scratch / "state/registry.sqlite3"
    tokens = ["h2s_" + letter * 43 for letter in "TU"]
    with closing(sqlite3.connect(database)) as db, db:
        db.executemany("INSERT INTO steam_login_sessions VALUES(?,?,?,?)", [
            (sha(token.encode()), str(76561197960287972 + index), "Private state fixture", int(time.time()) + 7200)
            for index, token in enumerate(tokens)])

    async def produce():
        base = f"http://{server.host}:{server.port}"
        streams, tasks = [], []
        async with aiohttp.ClientSession() as client:
            async def request(method, path, status, token, body=None):
                async with client.request(method, base + path, headers={"Authorization": "Bearer " + token}, json=body) as response:
                    value = await response.json()
                    assert response.status == status, (path, response.status, status, value.get("error"))
                    return value
            async def read(wire, queue=None):
                async for message in wire:
                    if message.type == aiohttp.WSMsgType.TEXT:
                        value = json.loads(message.data)
                        if "ping" in value:
                            await wire.send_json({"pong": value["ping"]})
                        elif "probe" in value:
                            await wire.send_json({"echo": value["probe"]})
                        elif queue is not None:
                            await queue.put(value)
            async def until(queue, predicate, timeout=15):
                deadline = time.monotonic() + timeout
                while True:
                    value = await asyncio.wait_for(queue.get(), max(0.001, deadline - time.monotonic()))
                    if predicate(value):
                        return value
            try:
                grants = []
                for index, token in enumerate(tokens):
                    body = dict(screen_name="Legacy Room " + str(index), maps=["lockout"],
                                compatibility="halo2-native-queue-v10", language=0,
                                local_players=[dict(screen_name="Transient Guest")] if index == 0 else [])
                    grant = (await request("POST", "/v1/matchmaking/session", 201, token, body))["token"]
                    grants.append(grant)
                    wire = await client.ws_connect(base + "/v1/matchmaking/qos", headers={"Authorization": "Bearer " + grant}, compress=0)
                    streams.append(wire); tasks.append(asyncio.create_task(read(wire)))
                    await request("POST", "/v1/matchmaking/ticket", 201, grant,
                                  dict(playlist_id=entry["id"], revision=entry["revision"], request_id=str(uuid.uuid4())))
                deadline = time.monotonic() + 20
                while True:
                    ticket = await request("GET", "/v1/matchmaking/ticket?progress=1", 200, grants[0])
                    if ticket["match"] is not None:
                        break
                    assert time.monotonic() < deadline, "Legacy fixture did not form a match"
                    await asyncio.sleep(0.05)
                descriptors = [await request("GET", "/v1/matchmaking/transport", 200, grant) for grant in grants]
                assert descriptors[0]["id"] == descriptors[1]["id"] and len(descriptors[0]["players"]) == 3
                rooms, queues = [], []
                for descriptor in descriptors:
                    wire = await client.ws_connect(base + "/v1/matches/" + descriptor["id"] + "/relay?completion=1",
                                                   headers={"Authorization": "Bearer " + descriptor["token"]}, compress=0)
                    streams.append(wire); rooms.append(wire); queue = asyncio.Queue(); queues.append(queue)
                    tasks.append(asyncio.create_task(read(wire, queue)))
                loading = [await until(queue, lambda row: row.get("phase") == "loading") for queue in queues]
                host = loading[0]["host"]
                assert loading[1]["host"] == host
                began = time.monotonic()
                for wire in rooms:
                    await wire.send_json({"loaded": "0123456789abcdef:000001a"})
                for queue in queues:
                    await until(queue, lambda row: row.get("phase") == "running")
                elapsed = time.monotonic() - began
                assert elapsed >= 9.5
                raw = bytearray(0xdc78); raw[:2] = raw[0x10:0x12] = b"\x01\x01"; raw[0x64] = 2
                for slot, row in enumerate(descriptors[0]["players"]):
                    at = 0x370 + slot * 0x94; raw[at] = 1; raw[at + 2:at + 10] = bytes.fromhex(row["identity"])
                    name = row["name"].encode("utf-16-le"); raw[at + 12:at + 12 + len(name)] = name
                    raw[at + 0x88] = 255; raw[at + 0x90] = 0 if slot == descriptors[0]["slot"] else 1
                host_index = next(i for i, descriptor in enumerate(descriptors) if descriptor["slot"] == host)
                digest = sha(raw)
                await rooms[host_index].send_json({"postgame": raw.hex()})
                for queue in queues:
                    assert (await until(queue, lambda row: "postgame" in row))["postgame"] == raw.hex()
                for wire in rooms:
                    await wire.send_json({"confirm_result": digest})
                for queue in queues:
                    assert (await until(queue, lambda row: "completed" in row))["completed"] == digest
                skills = sql(database, "SELECT player,raw_rank,xp FROM playlist_skill")
                assert len(skills) == 2 and sum(row[2] for row in skills) > 0
                return dict(persistent_xp_total=sum(row[2] for row in skills), real_start_elapsed_seconds=elapsed,
                            physical_connections=2, logical_seats=3)
            finally:
                for wire in streams:
                    await wire.close()
                await asyncio.gather(*tasks)
    try:
        observed = asyncio.run(produce())
    finally:
        server.close()
    return dict(observed=observed, fixture=dict(compatibility="halo2-native-queue-v10", declared=True,
                                               original_profile=False, real_gameplay=False, catalog_sha256=sha(fixture.read_bytes())))


def qualify(source, scratch, binary, library, caddy, unused_shim, output):
    scratch.mkdir()
    service = load(source / "tools/test_rust_matchmaking_service.py", "state_service")
    delivery = load(source / "tools/rust_service_delivery_controls.py", "state_gateway")
    endpoint = delivery.port()
    issuer = f"https://localhost:{endpoint}"
    audience = "private-state-adoption-fixture"
    clean = {k: v for k, v in os.environ.items() if not k.startswith("H2_") and k not in ("STEAM_KEY", "LD_PRELOAD")}
    clean.update(H2_SECURITY_LIBRARY=str(library), HALO2_MATCHMAKING_TEST_INPUTS="0")
    checks, observations = [], {}
    owners, gateway = [], None
    fixture_root = scratch / "legacy-fixture"

    class Legacy(service.Server):
        def __init__(self, unused_binary, state, case, catalog=None, **unused):
            self.host, self.port = "127.0.0.1", delivery.port()
            self.log = case / "legacy.log"
            self.stream = self.log.open("ab")
            env = dict(clean, H2_MASTER_STATE=str(state), H2_MASTER_BIND=self.host, H2_MASTER_PORT=str(self.port),
                       H2_MASTER_PUBLIC_URL=issuer, H2_MASTER_BEHIND_TLS_PROXY="1", H2_STEAM_ENABLED="1",
                       H2_MATCHMAKING_CATALOG=str(catalog or ""), H2_EOS_OPENID_AUDIENCE=audience,
                       H2_EOS_FRIEND_STORAGE="0")
            self.process = subprocess.Popen([sys.executable, "-B", "-m", "multiplayer.master_server"], cwd=source, env=env,
                                            stdout=subprocess.PIPE, stderr=self.stream)
            owners.append(self)
            try:
                deadline = time.monotonic() + 20
                while True:
                    try:
                        if self.request("GET", "/v1/health")[0] == 200:
                            break
                    except (OSError, http.client.HTTPException):
                        pass
                    assert self.process.poll() is None, self.log.read_text()[-2000:]
                    assert time.monotonic() < deadline, "Legacy fixture did not start"
                    time.sleep(0.05)
            except BaseException:
                self.close()
                raise

        def close(self):
            if not self.stream.closed:
                super().close()

    def expect(method, path, status=200, **kw):
        actual, value = gateway.request(method, path, **kw)
        assert actual == status, (method, path, actual, status, value.get("error"))
        return value

    tokens = ["h2s_" + letter * 43 for letter in "TUVW"]
    subjects = [str(76561197960287972 + n) for n in range(4)]
    names = ["Migration Pilot " + str(n) for n in range(4)]
    grants = []
    current = None

    def login():
        nonlocal grants
        grants = [expect("POST", "/v1/account/session", 201, token=token, body={"screen_name": name})["token"]
                  for token, name in zip(tokens, names)]

    def command(index, family, action, status=200, **values):
        if family == "party" and action not in ("create", "accept") and "version" not in values:
            values["version"] = expect("GET", "/v1/party?privacy=1", token=grants[index])["party"]["version"]
        body = dict(action=action, request_id=str(uuid.uuid4())) | values
        return expect("POST", "/v1/" + family + "/commands", status, token=grants[index], body=body), body

    def stable_views():
        result = {}
        for index, grant in enumerate(grants):
            for route in ("account/profile", "clans", "party?privacy=1", "friends"):
                result[str(index) + "/" + route] = expect("GET", "/v1/" + route, token=grant)
        public = expect("GET", "/v1/history/players?discord_username=state_alias_0")["players"]
        assert len(public) == 1
        public_id = public[0]["id"]
        history = expect("GET", "/v1/history/players/" + public_id + "/matches")
        assert len(history["matches"]) == 1
        result["history"] = history
        result["report"] = expect("GET", "/v1/history/matches/" + history["matches"][0]["id"])
        result["jwks"] = expect("GET", "/v1/auth/eos/jwks")
        return result

    def switch(backend, state, catalog, label):
        nonlocal gateway, current
        if gateway:
            gateway.close()
        if current:
            current.close()
        case = scratch / label
        case.mkdir()
        if backend == "legacy":
            current = Legacy(binary, state, case, catalog)
        else:
            current = service.Server(binary, state, case, catalog, tls=True, environment={
                "H2_PUBLIC_URL": issuer, "H2_EOS_OPENID_AUDIENCE": audience})
            owners.append(current)
        gateway = delivery.Gateway(caddy, source, case, current, current.port, endpoint, cert, private)

    try:
        generated = populate_legacy(fixture_root, source / "multiplayer/master_server/examples/matchmaking-online.json", Legacy)
        observations["legacy_room_fixture"] = dict(observed=generated["observed"], fixture=generated["fixture"],
                                                    adapter="Legacy Python HTTP and production Rust ABI", real_gameplay=False)
        original = fixture_root / "state"
        catalog = fixture_root / "catalog.json"
        database = original / "registry.sqlite3"
        fixture_tables = snapshot(database)
        assert fixture_tables["match_results"]["rows"] == 1
        assert fixture_tables["match_result_players"]["rows"] == 2
        assert fixture_tables["playlist_skill"]["rows"] == 2
        assert generated["observed"]["persistent_xp_total"] > 0
        checks.append("legacy HTTP/room completion creates nonempty hidden XP, unanimous result/admission, public PCR and two persistent players while excluding the guest")
        # Alias/provider rows and extra private grants model an existing linked
        # account database, without claiming genuine provider authorization.
        with closing(sqlite3.connect(database)) as db, db:
            db.executemany("INSERT INTO steam_login_sessions VALUES(?,?,?,?)", [
                (sha(token.encode()), subject, "Private state fixture", int(time.time()) + 7200)
                for token, subject in zip(tokens[2:], subjects[2:])])
        cert, private = delivery.tls_files(scratch)
        switch("legacy", original, catalog, "legacy-baseline")
        login()
        for index, subject in enumerate(subjects):
            who = sql(database, "SELECT player FROM player_identities WHERE provider='steam' AND subject=?", (subject,))[0][0]
            sql(database, "INSERT INTO player_identities VALUES('discord',?,?)", (str(811111111111111111 + index), who))
            sql(database, "UPDATE player_profiles SET discord_username=? WHERE player=?", ("state_alias_" + str(index), who))
        expect("PUT", "/v1/account/profile", token=grants[0], body={"appearance": "ff020304013f1f81a5", "service_tag": "A117"})
        command(0, "clans", "create", name="Migration Clan")
        command(0, "clans", "invite", username="state_alias_1")
        invite = expect("GET", "/v1/clans", token=grants[1])["invitations"][0]
        command(1, "clans", "accept", target=invite["id"])
        command(2, "clans", "create", name="Other Clan")
        _, replay = command(0, "party", "create")
        command(0, "party", "invite", username="state_alias_1")
        invite = expect("GET", "/v1/party", token=grants[1])["invitations"][0]
        command(1, "party", "accept", target=invite["id"], version=invite["version"])
        for index in (0, 1):
            tag = expect("GET", "/v1/friends", token=grants[1 - index])["friend_tag"]
            peer = command(index, "friends", "search", tag=tag)[0]["search_result"]
            if index == 0:
                command(0, "friends", "request", target=peer["id"])
            else:
                command(1, "friends", "accept", target=peer["id"])
        expect("PUT", "/v1/account/voice-blocks", token=tokens[0], body={"blocked": [str(811111111111111114)]})
        baseline = stable_views()
        jwks = baseline["jwks"]
        gateway.close(); gateway = None
        current.close(); current = None
        before = snapshot(database)
        key_hashes = {name: sha((original / name).read_bytes()) for name in ("master-key.json", "eos-openid-key.pem")}
        clone = scratch / "cutover-state"
        clone_state(original, clone)
        assert snapshot(clone / "registry.sqlite3") == before
        checks.append("SQLite backup clones every populated table and both original signing keys into private owned storage")
        (output / "legacy-before.json").write_text(json.dumps(before, indent=2) + "\n")
        switch("rust", clone, catalog, "rust-cutover")
        after_open = snapshot(clone / "registry.sqlite3")
        differences = {table: {"before": value, "after": after_open.get(table)} for table, value in before.items()
                       if table != "clock_floor" and after_open.get(table) != value}
        observations["startup_table_differences"] = differences
        assert not differences, differences
        expect("GET", "/v1/account/profile", 401, token=grants[0])
        login()
        adopted = stable_views()
        assert adopted == baseline, {key: {"before": baseline[key], "after": adopted[key]} for key in baseline if baseline[key] != adopted[key]}
        assert expect("GET", "/v1/health")["ready"] is False
        assert {name: sha((clone / name).read_bytes()) for name in key_hashes} == key_hashes
        checks.append("Rust opens the populated legacy clone without changing existing table contents, preserves canonical/public identities, profiles, social memberships, history and JWKS, and rejects old runtime grants")
        expect("POST", "/v1/party/commands", token=grants[0], body=replay)
        assert sql(clone / "registry.sqlite3", "SELECT count(*) FROM parties")[0][0] == before["parties"]["rows"]
        expect("PUT", "/v1/account/profile", token=grants[0], body={"appearance": "0a00000a0000000000", "service_tag": "B118"})
        changed = stable_views()
        assert changed["0/account/profile"]["service_tag"] == "B118"
        checks.append("Rust replays the legacy party command without duplicating the party and commits a new canonical profile revision")
        gateway.close(); gateway = None
        current.close(); current = None
        after_rust = snapshot(clone / "registry.sqlite3")
        switch("legacy", clone, catalog, "legacy-rollback")
        before_rollback_login = snapshot(clone / "registry.sqlite3")
        differences = {table: {"before": value, "after": before_rollback_login.get(table)} for table, value in after_rust.items()
                       if table != "clock_floor" and before_rollback_login.get(table) != value}
        observations["rollback_table_differences"] = differences
        assert not differences, differences
        login()
        rolled = stable_views()
        assert rolled == changed, [key for key in changed if rolled[key] != changed[key]]
        assert rolled["jwks"] == jwks
        checks.append("the real legacy host process reopens Rust-written state and reads the same canonical/profile/social/history views and signing identity")
        assert snapshot(database) == before, "Cutover or rollback modified the protected legacy source database"
        assert {name: sha((original / name).read_bytes()) for name in key_hashes} == key_hashes
        observations["nonempty_tables"] = {table: value["rows"] for table, value in before.items() if value["rows"]}
        observations["baseline_table_hashes_sha256"] = sha(json.dumps(before, sort_keys=True).encode())
        observations["source_state_unchanged"] = True
        return dict(passed=True, checks=checks, observations=observations, state_fixture=True,
                    real_production_snapshot=False, real_provider=False, original_live=False,
                    played_world=False, docker_image_executed=False, service_image_rollback=False,
                    host_process_state_rollback=True, installed_client_input=False, full_matchmaking_ready=False)
    finally:
        if gateway:
            gateway.close()
        for owner in reversed(owners):
            owner.close()
        for path in scratch.rglob("*.log"):
            (output / (str(path.relative_to(scratch)).replace("/", "__"))).write_bytes(path.read_bytes()[-65536:])
        (output / "state-observations.json").write_text(json.dumps(dict(checks=checks, observations=observations), indent=2) + "\n")
