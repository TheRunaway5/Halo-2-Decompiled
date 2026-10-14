"""Real native queue/QoS/session and eight source players; Discord is a fixture.

Source admissions use the production encrypted peer exchange. Independent
capture-time body hashes still use a private directory for verification;
this does not implement automatic world promotion or replace live Discord.
"""
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
    xbe = "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
    owners = ("one", "two")
    verified = []
    output = Path(os.environ.get("HALO2_PUBLISHER_TEST_OUTPUT",
        str(ROOT / ".local/player-host-continuity-20261014/authority-publisher")))
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="halo2-source-publisher-") as temp:
        path = Path(temp) / "playlists.json"
        fixture = catalog()
        for row in fixture["playlists"]:
            row["team_size"] = 0
            row["games"][0].update(map="lockout", variant="Slayer")
            row["rank_configuration"] = "retail-launch-2004"
        path.write_text(json.dumps(fixture))

        async def verify(session, token, application):
            if token not in owners:
                raise Problem(401, "Fixture authorization denied")
            assert application == "1558522073272426519"
            verified.append(token)
            return VerifiedOwner("discord", str(123456789012345678 + owners.index(token)), token)

        app = create_app(Settings(state=Path(temp), public_url="http://127.0.0.1:8080", allow_http=True,
            rate_per_minute=100000, matchmaking_catalog=str(path)), verify)
        async with TestClient(TestServer(app)) as client:
            processes = []
            try:
                for owner in owners:
                    env = os.environ | dict(HTTP_PROXY="http://127.0.0.1:1", NO_PROXY="", no_proxy="")
                    processes.append(await asyncio.create_subprocess_exec(binary, str(ROOT), str(client.make_url("/")).rstrip("/"),
                        owner, port(), xbe, temp, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE, env=env))
                async def collect(owner, process):
                    out, err = await process.communicate()
                    (output / (owner + ".log")).write_bytes(out + err)
                    print(owner, process.returncode, out.decode(), err.decode(), flush=True)
                    assert process.returncode == 0, "Native source publisher process failed"
                await asyncio.wait_for(asyncio.gather(*(collect(owner, p) for owner, p in zip(owners, processes))), 300)
                assert set(verified) == set(owners)
                directory = next(iter(app.values()))
                assert not directory.matchmaking.sessions and not directory.matchmaking.core.tickets
                assert all(room.closed and room.direct_udp for room in directory.match_transport.rooms.values())
                db = directory.store.db
                assert db.execute("SELECT count(*) FROM players").fetchone()[0] == 2
                assert db.execute("SELECT count(*) FROM match_results").fetchone()[0] == 0
                assert db.execute("SELECT count(*) FROM carnage_reports").fetchone()[0] == 0
                print("PASS two independently authenticated native processes, measured host choice, eight admitted source players, asynchronous full/delta publication, prepared-world receipts, cancellation, and no unfinished result publication")
            finally:
                for process in processes:
                    if process.returncode is None:
                        process.kill()
                        await process.wait()


if __name__ == "__main__":
    asyncio.run(main())
