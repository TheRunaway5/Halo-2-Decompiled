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
from aiohttp import web
from collections import Counter
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
    # The separate MCC acceptance mode has no runtime executable input. The
    # older disc/reference mode remains an explicitly offline source control.
    mcc_map = os.environ.get("HALO2_SOURCE_AUTHORITY_MCC_MAP")
    if mcc_map:
        assert Path(mcc_map).is_file(), "Configured prepared MCC map is missing"
    xbe = "" if mcc_map else "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
    owners = ("one", "two")
    verified = []
    output = Path(os.environ.get("HALO2_PUBLISHER_TEST_OUTPUT",
        str(Path(os.environ["HALO2_TEST_SCRATCH"]) / "authority-publisher")))
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="halo2-source-publisher-") as temp:
        path = Path(temp) / "playlists.json"
        fixture = catalog()
        for row in fixture["playlists"]:
            row["team_size"] = 0
            if os.environ.get("HALO2_SHADOW_GPU_MODE"):
                row.update(minimum_players=2, maximum_players=2, maximum_party=1)
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
        observations = Counter()
        if os.environ.get("HALO2_SHADOW_TRANSPORT_ONLINE"):
            @web.middleware
            async def observe(request, handler):
                response = await handler(request)
                try:
                    value = json.loads(response.body)
                    state = value.get("state", "")
                    state = state if isinstance(state, str) else "nested"
                    error = str(value.get("error", ""))[:160]
                    route = request.match_info.route.resource.canonical
                    observations[(request.method, route, response.status, state, error)] += 1
                except (ValueError, TypeError, AttributeError):
                    pass
                return response
            app.middlewares.append(observe)
        async with TestClient(TestServer(app)) as client:
            processes = []
            drains = []
            tails = {}

            async def drain(owner, stream):
                # Keep real process output even when a different owner fails
                # first and the fixture tears down the remaining native PC.
                # Both readers share one bounded tail on this asyncio thread.
                tail = tails[owner]
                while chunk := await stream.read(8192):
                    tail.extend(chunk)
                    if len(tail) > 256 * 1024:
                        del tail[:-256 * 1024]
                    (output / (owner + ".log")).write_bytes(tail)
            try:
                for owner in owners:
                    env = os.environ | dict(HTTP_PROXY="http://127.0.0.1:1", NO_PROXY="", no_proxy="")
                    if os.environ.get("HALO2_SHADOW_GPU_MODE"):
                        private = Path(os.environ["HALO2_TEST_SCRATCH"]) / ("online-" + owner)
                        private.mkdir(parents=True, exist_ok=True)
                        env.update(XDG_CACHE_HOME=str(private / "cache"), XDG_CONFIG_HOME=str(private / "config"),
                            XDG_DATA_HOME=str(private / "data"), HALO2_PIPELINE_CACHE_DIRECTORY=str(private / "pipelines"),
                            MESA_SHADER_CACHE_DIR=str(private / "mesa"), __GL_SHADER_DISK_CACHE_PATH=str(private / "nvidia"))
                    prefix = ["xvfb-run", "-a", "-s", "-screen 0 800x600x24"] if os.environ.get("HALO2_SHADOW_GPU_MODE") else []
                    processes.append(await asyncio.create_subprocess_exec(*prefix, binary, str(ROOT), str(client.make_url("/")).rstrip("/"),
                        owner, port(), xbe, temp, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE, env=env))
                    tails[owner] = bytearray()
                    drains.extend((asyncio.create_task(drain(owner, processes[-1].stdout)),
                                   asyncio.create_task(drain(owner, processes[-1].stderr))))
                async def collect(owner, process):
                    await process.wait()
                    await asyncio.shield(asyncio.gather(*drains[2 * owners.index(owner):2 * owners.index(owner) + 2]))
                    print(owner, process.returncode, bytes(tails[owner]).decode(errors="replace"), flush=True)
                    assert process.returncode == 0, "Native source publisher process failed"
                await asyncio.wait_for(asyncio.gather(*(collect(owner, p) for owner, p in zip(owners, processes))), 300)
                if os.environ.get("HALO2_ADMITTED_CAMERA_ONGOING"):
                    receipts = list(Path(temp).glob("ongoing-*.json"))
                    assert len(receipts) == 1, "Actual ongoing receiver did not retain its bounded receipt"
                    for report in receipts:
                        payload = report.read_bytes()
                        assert len(payload) <= 1024 * 1024
                        result = json.loads(payload)
                        assert result["passed"] and len(result["adoptions"]) >= 3
                        assert result["default_terminal_recovery"] is False
                        (output / report.name).write_bytes(payload)
                if os.environ.get("HALO2_SHADOW_TRANSPORT_ONLINE"):
                    receipts = list(Path(temp).glob("native-shadow-one.json")) + list(Path(temp).glob("native-shadow-two.json"))
                    assert len(receipts) == 2, "Both actual ONLINE native transport endpoints must retain their receipt"
                    for report in receipts:
                        payload = report.read_bytes()
                        assert len(payload) <= 1024 * 1024 and json.loads(payload)["passed"]
                        (output / report.name).write_bytes(payload)
                assert set(verified) == set(owners)
                directory = next(iter(app.values()))
                assert not directory.matchmaking.sessions and not directory.matchmaking.core.tickets
                assert all(room.closed and room.direct_udp for room in directory.match_transport.rooms.values())
                db = directory.store.db
                assert db.execute("SELECT count(*) FROM players").fetchone()[0] == 2
                assert db.execute("SELECT count(*) FROM match_results").fetchone()[0] == 0
                assert db.execute("SELECT count(*) FROM carnage_reports").fetchone()[0] == 0
                print("PASS two independently authenticated native processes, measured host choice, eight admitted source players, native source-exchange fixture checks, and no unfinished result publication")
            finally:
                for process in processes:
                    if process.returncode is None:
                        process.kill()
                        await process.wait()
                # These readers are independent of collect()/gather(), so a
                # cancelled wait never discards communicate()'s private buffer.
                await asyncio.gather(*drains, return_exceptions=True)
                for owner, tail in tails.items():
                    (output / (owner + ".log")).write_bytes(tail)
                if os.environ.get("HALO2_SHADOW_TRANSPORT_ONLINE"):
                    print("Native ONLINE HTTP observations", json.dumps([
                        dict(method=key[0], route=key[1], status=key[2], state=key[3], error=key[4], count=count)
                        for key, count in observations.most_common(32)]), flush=True)


if __name__ == "__main__":
    asyncio.run(main())
