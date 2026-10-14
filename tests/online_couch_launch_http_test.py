"""Two independent native couches; only external Discord is a fixture."""
import asyncio
import json
import os
from pathlib import Path
import socket
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from aiohttp.test_utils import TestClient, TestServer
from aiohttp import web
from multiplayer.master_server.service import Settings, create_app
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.player_identity import VerifiedOwner
from multiplayer.master_server.tests.test_couch_online import catalog


def port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return str(s.getsockname()[1])


async def main():
    binary = str(Path(sys.argv[1]).resolve())
    mode = sys.argv[2] if len(sys.argv) > 2 else "core"
    assets = ROOT / "assets"
    sources = dict(line.split("\t", 1) for line in (assets / "asset-sources.tsv").read_text().splitlines()[1:])
    vista = sources["vista"]
    xbe = os.environ.get("HALO2_COUCH_TEST_XBE", str(assets / "multiplayer-map-pack/demos/update/Updates/default.xbe.03"))
    output = Path(os.environ.get("HALO2_COUCH_TEST_OUTPUT", str(ROOT / ".local/couch-captures" / mode)))
    output.mkdir(parents=True, exist_ok=True)
    owners = ("one", "two", "one-second", "two-second")
    with tempfile.TemporaryDirectory(prefix="halo2-couches-") as temp:
        path = Path(temp) / "playlists.json"
        fixture = catalog()
        for row in fixture["playlists"]:
            row["games"][0]["map"] = os.environ.get("HALO2_COUCH_TEST_MAP", "lockout")
            if os.environ.get("HALO2_COUCH_TEST_VARIANT"):
                row["team_size"]=0
                row["games"][0]["variant"]=os.environ["HALO2_COUCH_TEST_VARIANT"]
            row["rank_configuration"] = os.environ.get("HALO2_COUCH_TEST_RANK_CONFIGURATION", "retail-launch-2004")
            if os.environ.get("HALO2_MATCHMAKING_GATHERINGS") == "1":
                row["candidate_selection"] = "retail-candidates-native-relay-v1"
                row["gathering"] = dict(countdown_ms=[2147483647]*16, timeout_ms=150000,
                    provenance="Native eight-player validation dimensions; original full-count getter")
        path.write_text(json.dumps(fixture))
        verified = []
        async def verify(session, token, application):
            if token not in owners:
                raise Problem(401, "Fixture authorization denied")
            assert application == "1558522073272426519"
            verified.append(token)
            return VerifiedOwner("discord", str(123456789012345678 + owners.index(token)), token.replace("-", "_"))
        app = create_app(Settings(state=Path(temp), public_url="http://127.0.0.1:8080", allow_http=True,
            rate_per_minute=100000, matchmaking_catalog=str(path)), verify)
        if os.environ.get("HALO2_COUCH_TEST_TRACE"):
            @web.middleware
            async def trace(request,handler):
                response=await handler(request)
                value=json.loads(response.body) if response.body else {}
                print("HTTP",request.method,request.path,response.status,list(value),flush=True)
                return response
            app.middlewares.insert(0,trace)
        async with TestClient(TestServer(app)) as client:
            processes = []
            try:
                for owner in owners[:2]:
                    env = os.environ | dict(SDL_AUDIODRIVER="dummy", HTTP_PROXY="http://127.0.0.1:1", NO_PROXY="", no_proxy="",HALO2_COUCH_RENDER_BARRIER=temp)
                    processes.append(await asyncio.create_subprocess_exec(binary, str(ROOT), str(client.make_url("/")).rstrip("/"),
                        owner, port(), str(output), vista, xbe, mode,
                        stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE, env=env))
                results = await asyncio.wait_for(asyncio.gather(*(p.communicate() for p in processes)), 240)
                for owner, process, (out, err) in zip(owners, processes, results):
                    (output / (owner+".log")).write_bytes(out+err)
                    print(owner, process.returncode, out.decode(), err.decode(), flush=True)
                assert all(p.returncode == 0 for p in processes), "Native couch process failed"
                directory = next(iter(app.values()))
                db = directory.store.db
                expected = 4 if mode == "mixed" else 2
                for table in ("players", "player_profiles", "match_result_players", "match_result_memberships", "match_result_skill"):
                    count = 0 if mode == "render" and table.startswith("match_result_") else expected
                    if table=="match_result_memberships" and os.environ.get("HALO2_COUCH_TEST_VARIANT"):
                        count=0  # These bundled custom modes use individual FFA standings.
                    assert db.execute("SELECT count(*) FROM " + table).fetchone()[0] == count, table
                assert db.execute("SELECT count(DISTINCT player) FROM playlist_skill").fetchone()[0] == expected
                assert db.execute("SELECT count(*) FROM playlist_skill WHERE player LIKE 'guest-%'").fetchone()[0] == 0
                assert db.execute("SELECT count(*) FROM match_results WHERE validation='native-unanimous-v1'").fetchone()[0] == (0 if mode == "render" else 1)
                if mode != "render":
                    assert db.execute("SELECT count(*) FROM carnage_players").fetchone()[0] == expected
                    assert tuple(db.execute("SELECT admitted_players,published_players,format FROM carnage_reports").fetchone()) == (8,expected,"vista-pcr-consumed-v1")
                    public = db.execute("SELECT public_id FROM carnage_reports").fetchone()[0]
                    response = await client.get("/v1/history/matches/"+public)
                    assert response.status == 200
                    history = await response.json()
                    assert len(history["players"]) == expected
                    assert all(p["details_available"] for p in history["players"])
                    assert history["guest_details"] == "excluded"
                    for player in history["players"]:
                        response = await client.get("/v1/history/players",params={"discord_username":player["discord_username"]})
                        assert (await response.json())["players"][0]["id"] == player["id"]
                        response = await client.get("/v1/history/players/"+player["id"]+"/matches")
                        assert (await response.json())["matches"][0]["id"] == public
                    encoded = json.dumps(history)
                    assert all(p not in encoded for (p,) in db.execute("SELECT id FROM players"))
                    assert "guest-" not in encoded and "discord_token" not in encoded
                    print("PASS actual native PCR is readable by verified Discord username; guest details and private identities excluded")
                assert not directory.matchmaking.sessions and not directory.matchmaking.core.tickets
                assert all(room.closed for room in directory.match_transport.rooms.values())
                assert set(verified) == set(owners[:expected])
                print("PASS finite rendered match exits without publishing incomplete stats" if mode == "render" else
                      "PASS authenticated player results saved; guests absent from all persistent player/stat tables; all eight remain in local PCR")
            finally:
                for process in processes:
                    if process.returncode is None:
                        process.kill()
                        await process.wait()
    return 0


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
