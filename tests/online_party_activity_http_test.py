"""Real local account/party HTTP handoff with external Discord replaced.

The subprocess uses the production OnlineAccount transport. The Python owner
prepares only verified fixture accounts, a target invitation and an SDK join
secret. No real Discord activity invitation or game launch is claimed.
"""
import asyncio
import json
from pathlib import Path
import socket
import sys
import tempfile
import uuid

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

try:
    from aiohttp import ClientSession
    from aiohttp.test_utils import TestServer
    from multiplayer.master_server.service import Settings, create_app
    from multiplayer.master_server.player_identity import VerifiedOwner
    from multiplayer.master_server.protocol import Problem
    from multiplayer.master_server.tests.test_matchmaking_http import catalog
except ImportError:
    print("SKIP native activity HTTP fixture: backend dependencies unavailable")
    raise SystemExit(77)


async def run(executable):
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        (root/"catalog.json").write_text(json.dumps(catalog()))

        async def verify(session, token, application):
            if token not in ("one", "two", "three"):
                raise Problem(401, "Invalid fixture authorization")
            subjects = {"one":"111111111111111111", "two":"222222222222222222", "three":"333333333333333333"}
            return VerifiedOwner("discord", subjects[token], "pilot_"+token)

        app = create_app(Settings(state=root,public_url="http://127.0.0.1:8080",allow_http=True,
            matchmaking_catalog=str(root/"catalog.json"),rate_per_minute=100000), verify)
        async with TestServer(app) as server, ClientSession() as client:
            endpoint = str(server.make_url(""))

            async def post(path, bearer, body, expected=200):
                async with client.post(endpoint+path,headers={"Authorization": "Bearer "+bearer},json=body) as response:
                    if response.status != expected:
                        raise RuntimeError("Activity fixture HTTP status mismatch")
                    return await response.json()

            host = await post("/v1/account/session", "one", {"screen_name": "Host Pilot"}, 201)
            await post("/v1/account/session", "two", {"screen_name": "Before Launch"}, 201)
            await post("/v1/account/session", "three", {"screen_name": "Before Third Launch"}, 201)
            created = await post("/v1/party/commands", host["token"], {"action": "create", "request_id": str(uuid.uuid4())})
            version = created["party"]["version"]
            await post("/v1/party/commands", host["token"], {"action": "invite_discord", "discord_id": "222222222222222222", "version": version, "request_id": str(uuid.uuid4())})
            await post("/v1/party/commands", host["token"], {"action": "invite_discord", "discord_id": "333333333333333333", "version": version, "request_id": str(uuid.uuid4())})
            link = await post("/v1/party/activity", host["token"], {"version": version})
            processes = []

            async def launch(third=False):
                with socket.socket() as reserve:
                    reserve.bind(("127.0.0.1", 0));port = reserve.getsockname()[1]
                process = await asyncio.create_subprocess_exec(str(executable),endpoint,str(port),
                    *( ["third"] if third else [] ),
                    stdin=asyncio.subprocess.PIPE,stdout=asyncio.subprocess.PIPE,stderr=asyncio.subprocess.PIPE)
                processes.append(process)
                process.stdin.write((link["join_secret"]+"\n").encode())
                await process.stdin.drain()
                ready = await asyncio.wait_for(process.stdout.readline(),25)
                if ready != b"READY native party admission\n":
                    raise RuntimeError("Native activity admission failed")
                return process

            try:
                first = await launch()
                refreshed = await post("/v1/party/activity",host["token"],
                    {"version":version+1,"join_secret":link["join_secret"]})
                assert refreshed["party_id"]==link["party_id"] and refreshed["join_secret"]==link["join_secret"]
                assert refreshed["current_size"]==2 and refreshed["expires_in"]<=link["expires_in"]
                second = await launch(True)
                async with client.get(endpoint+"/v1/party",headers={"Authorization": "Bearer "+host["token"]}) as response:
                    value = await response.json()
                assert [m["name"] for m in value["party"]["members"]] == ["Host Pilot", "Activity Pilot", "Third Pilot"]
                assert not value["party"]["outgoing"]
                directory = next(iter(app.values()))
                assert directory.store.db.execute("SELECT count(*) FROM party_activity_links").fetchone()[0] == 1
                for process in (first,second):
                    stdout,stderr = await asyncio.wait_for(process.communicate(b"finish\n"),25)
                    for private in (link["join_secret"],host["token"],"111111111111111111","222222222222222222","333333333333333333"):
                        if private.encode() in stdout+stderr:
                            raise RuntimeError("Private activity fixture output leak")
                    if process.returncode or not stdout.startswith(b"PASS native HTTP activity redemption"):
                        raise RuntimeError("Native activity HTTP fixture failed")
                    print(stdout.decode().strip())
                # Signing out a recipient clears that client's presentation;
                # accepted membership and the leader's invitations persist.
                assert directory.store.db.execute("SELECT count(*) FROM party_activity_links").fetchone()[0] == 1
                await post("/v1/party/commands",host["token"],
                    {"action":"unready","version":value["party"]["version"],"request_id":str(uuid.uuid4())})
                assert directory.store.db.execute("SELECT count(*) FROM party_activity_links").fetchone()[0] == 0
                print("PASS two independent native inbox acceptances, retained party link, chosen names and consent-change revocation")
            finally:
                for process in processes:
                    if process.returncode is None:
                        process.kill();await process.wait()


if __name__ == "__main__":
    asyncio.run(run(Path(sys.argv[1]).resolve()))
