"""Native menus -> real authenticated HTTP API -> coordinator -> second HTTP player.

Only Discord's external endpoints and native connection measurements are test
fixtures. No private credential, rank or network score comes from a UI field.
"""
import asyncio
import contextlib
import json
import os
from pathlib import Path
import sys
import tempfile
import time
from frontend_test_ports import callback_port
from helpers.mcc_online_ui import ui_arguments

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
try:
    from aiohttp.test_utils import TestClient, TestServer
    from multiplayer.master_server.service import Settings, create_app
    from multiplayer.master_server.protocol import Problem
    from multiplayer.master_server.tests.test_matchmaking_http import catalog
except ImportError:
    print("Install multiplayer/master_server/requirements.txt for native HTTP integration")
    sys.exit(77)


async def main():
    binary = str(Path(sys.argv[1]).resolve())
    try:
        assets = ui_arguments(ROOT, sys.argv[2:])
    except ValueError as error:
        print("ONLINE UI configuration failed: " + str(error), file=sys.stderr)
        return 1
    native_options = assets.forwarded
    with tempfile.TemporaryDirectory(prefix="halo2-native-matchmaking-") as temp:
        path = Path(temp) / "playlists.json"
        fixture = catalog()
        gathering_run = "--gathering-ui" in native_options
        if gathering_run:
            fixture["playlists"][0].update(candidate_selection="retail-candidates-native-relay-v1",
                rank_configuration="retail-launch-2004",gathering=dict(retail_hopper=2))
        from multiplayer.master_server.tests.test_clan_matchmaking_http import clan_catalog
        for major in (False,True):
            row = clan_catalog(major)["playlists"][0]
            row["id"] = "major-clan" if major else "minor-clan"
            row["name"] = "Major Clan Lab Slayer" if major else "Minor Clan Lab Slayer"
            row["description"] = ("Two clan parties of 6 to 8; opposing sizes differ by at most two. " if major else
                "Two clan parties of 3 or 4; equal opposing sizes. ") + "Every member belongs to the same clan and chooses this playlist. Clan rank applies. Lockout Team Slayer validation rotation; custom search timing and radius."
            fixture["playlists"].append(row)
        path.write_text(json.dumps(fixture))
        verified, session_creations = [], []
        async def verify(session, token, application):
            assert application == "1558522073272426519"
            if token not in ("t", "peer-token"):
                raise Problem(401, "Test authorization denied")
            verified.append(token)
            return "discord", "123456789012345678" if token == "t" else "999999999999999999"
        app = create_app(Settings(state=Path(temp), public_url="http://127.0.0.1:8080", allow_http=True,
                                  rate_per_minute=100000, matchmaking_catalog=str(path)), verify)
        async def observe_session(request, response):
            if (request.method=="POST" and request.path=="/v1/matchmaking/session" and
                    request.headers.get("Authorization")=="Bearer t" and response.status==201):
                session_creations.append(True)
        app.on_response_prepare.append(observe_session)
        api = next(iter(app.values())).matchmaking
        rank_run = "--rank-ui" in native_options
        def set_display_rank(player, level):
            with api.directory.store.db:
                api.directory.store.db.execute("INSERT OR REPLACE INTO playlist_ranks VALUES(?,?,?)",
                                               (player,"ranked",level))
        if rank_run:
            async def seed_rank(request, response):
                if request.path=="/v1/matchmaking/session" and request.method=="POST" and response.status==201:
                    for session in api.sessions.values():
                        set_display_rank(session.player,37 if session.screen_name!="Peer Pilot" else 19)
            app.on_response_prepare.append(seed_rank)
        async with TestClient(TestServer(app)) as client:
            response = await client.post("/v1/matchmaking/session", headers={"Authorization": "Bearer peer-token"},
                json=dict(screen_name="Peer Pilot", maps=["lockout"],
                    compatibility="halo2-native-queue-v8" if gathering_run else "halo2-native-queue-v7",language=0))
            assert response.status == 201
            peer = {"Authorization": "Bearer " + (await response.json())["token"]}
            probe = await client.ws_connect("/v1/matchmaking/qos",headers=peer) if gathering_run else None
            async def probes():
                async for message in probe:
                    data=json.loads(message.data)
                    if "ping" in data: await probe.send_json({"pong":data["ping"]})
                    if "probe" in data: await probe.send_json({"echo":data["probe"]})
            probe_task=asyncio.create_task(probes()) if probe else None
            async def partner():
                queued = False
                socket = None
                reader = None
                async def answer(socket):
                    async for message in socket:
                        from aiohttp import WSMsgType
                        if message.type == WSMsgType.TEXT:
                            data=json.loads(message.data)
                            if "ping" in data: await socket.send_json({"pong":data["ping"]})
                try:
                  while True:
                    if socket and socket.closed:
                        socket=None;queued=False
                    if not queued and any(t.playlist == "casual" for t in api.core.tickets.values()):
                        import uuid
                        response = await client.post("/v1/matchmaking/ticket", headers=peer,
                            json=dict(playlist_id="casual", revision="lab-1", request_id=str(uuid.uuid4())))
                        assert response.status == 201
                        queued = True
                    if queued:
                        await client.get("/v1/matchmaking/ticket", headers=peer)
                        if api.core.proposals and not socket:
                            response=await client.get("/v1/matchmaking/transport",headers=peer)
                            if response.status==200:
                                room=await response.json()
                                socket=await client.ws_connect("/v1/matches/"+room["id"]+"/relay",headers={"Authorization":"Bearer "+room["token"]})
                                reader=asyncio.create_task(answer(socket))
                    await asyncio.sleep(.02)
                finally:
                    if reader:
                        reader.cancel()
                        with contextlib.suppress(asyncio.CancelledError):await reader
                    if socket:await socket.close()
            task = asyncio.create_task(partner())
            # Poison ambient proxies: native identity and queue transport must
            # stay on the explicitly pinned endpoint and fixed Discord URLs.
            env = os.environ | dict(http_proxy="http://127.0.0.1:1", HTTP_PROXY="http://127.0.0.1:1", NO_PROXY="", no_proxy="")
            process = await asyncio.create_subprocess_exec(binary, *assets.source,
                "--matchmaking-url", str(client.make_url("/")).rstrip("/"), "--callback-port",callback_port(),*native_options,
                stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE, env=env)
            try:
                stdout, stderr = await asyncio.wait_for(process.communicate(),
                    300 if "--capture" in native_options else 45)
                print(stdout.decode(), end="")
                print(stderr.decode(), end="", file=sys.stderr)
                assert process.returncode in (0, 77), "Native menu/queue integration failed"
                if process.returncode == 77: return 77
                # Verification may legitimately hit the directory's token
                # cache; count successful native exchanges independently.
                assert verified.count("t") >= 1 and len(session_creations) >= 2, "Native client never re-exchanged existing Discord authorization"
                assert all(t.players[0].screen_name == "Peer Pilot" for t in api.core.tickets.values()), "Native queue survived cancellation or sign-out"
                assert all(s.screen_name == "Peer Pilot" for s in api.sessions.values()), "Native session survived teardown"
                assert not api.core.proposals
                print("PASS real native menus, Discord playlists, Quickmatch, casual roster, measured relay, authenticated gameplay launch, cancel/rejoin/sign-out and " +
                      ("MCC-only imported UI inputs" if assets.mcc_cache else "Vista assets"))
            finally:
                task.cancel()
                with contextlib.suppress(asyncio.CancelledError): await task
                if probe_task:
                    probe_task.cancel()
                    with contextlib.suppress(asyncio.CancelledError):await probe_task
                    await probe.close()
                if process.returncode is None: process.kill(); await process.wait()
    return 0


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
