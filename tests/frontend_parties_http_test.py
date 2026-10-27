"""Real native Vista party UI, authenticated HTTP membership and a peer."""
import asyncio
import contextlib
import json
import os
from pathlib import Path
import sys
import tempfile
import uuid
from frontend_test_ports import callback_port
from helpers.mcc_online_ui import ui_arguments

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from aiohttp.test_utils import TestClient, TestServer
from multiplayer.master_server.service import Settings, create_app
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.player_identity import VerifiedOwner
from multiplayer.master_server.tests.test_matchmaking_http import catalog


async def main():
    try:
        assets = ui_arguments(ROOT, sys.argv[2:])
    except ValueError as error:
        print("ONLINE UI configuration failed: " + str(error), file=sys.stderr)
        return 1
    native_options = assets.forwarded
    captures = Path(native_options[native_options.index("--capture")+1]) if "--capture" in native_options else None
    if captures:
        captures.mkdir(parents=True, exist_ok=True)
        for name in ("request-full-party", "full-party-complete"):
            (captures/name).unlink(missing_ok=True)
    with tempfile.TemporaryDirectory(prefix="halo2-party-ui-") as temp:
        async def verify(session, token, application):
            assert application == "1558522073272426519"
            if token not in ("t", "peer") and not token.startswith("layout_"):
                raise Problem(401, "Fixture authorization denied")
            return VerifiedOwner("discord", "222222222222222222" if token == "peer" else "private-"+token,
                                 "verified_user" if token == "t" else "peer_pilot" if token == "peer" else token)
        # A two-member party cannot fill this four-player playlist, so its
        # search stays in progress until a member cancels it.
        playlists = catalog()
        playlists["playlists"] = [playlists["playlists"][1] | dict(id="party-search", name="Party Search lab",
                                  minimum_players=4, maximum_players=4, maximum_party=2)]
        (Path(temp)/"playlists.json").write_text(json.dumps(playlists))
        app = create_app(Settings(state=Path(temp), public_url="http://127.0.0.1:8080", allow_http=True,
                                  rate_per_minute=100000, matchmaking_catalog=str(Path(temp)/"playlists.json")), verify)
        async with TestClient(TestServer(app)) as client:
            async def sign_in(token, name):
                response = await client.post("/v1/account/session", headers={"Authorization":"Bearer "+token}, json={"screen_name":name})
                assert response.status == 201, await response.text()
                return {"Authorization":"Bearer "+(await response.json())["token"]}
            peer = await sign_in("peer", "Pilot")  # Deliberate same chosen name.
            blocked_peers = [await sign_in("layout_blocked_"+str(i), "Duplicate Blocked") for i in range(2)]
            response = await client.put("/v1/account/profile", headers=peer,
                json={"appearance": "080c0901011a098100"})
            assert response.status == 200, await response.text()
            guests = [await sign_in("layout_"+str(i), "W"*31 if i == 13 else "Party Member "+str(i).zfill(2))
                      for i in range(14)] if captures else []
            async def view(headers):
                response = await client.get("/v1/party", headers=headers)
                assert response.status == 200, await response.text()
                return await response.json()
            async def command(headers, action, **kw):
                response = await client.post("/v1/party/commands", headers=headers,
                    json=dict(action=action, request_id=str(uuid.uuid4())) | kw)
                assert response.status == 200, await response.text()
                return await response.json()
            api = next(iter(app.values())).matchmaking
            async def partner():
                joined = reinvited = stress_done = cancelled = False
                queue = None
                while True:
                    friends=await (await client.get('/v1/friends',headers=peer)).json()
                    for friend in friends['friends']:
                        if friend['state']=='incoming':
                            accepted=await client.post('/v1/friends/commands',headers=peer,json=dict(
                                action='accept',request_id=str(uuid.uuid4()),target=friend['id']))
                            assert accepted.status==200,await accepted.text()
                    data = await view(peer)
                    if not joined and data["invitations"]:
                        invite = data["invitations"][0]
                        data = await command(peer, "accept", target=invite["id"], version=invite["version"])
                        joined = True
                    party = data["party"]
                    leader = next((m for m in party["members"] if m["leader"]), None) if party else None
                    me = next((m for m in party["members"] if m["self"]), None) if party else None
                    if leader and me and not leader["self"] and leader["ready_playlist"] == "party-search":
                        # The member consents to the leader's playlist, then
                        # cancels the leader's first party search.
                        if queue is None:
                            response = await client.post("/v1/matchmaking/session", headers={"Authorization": "Bearer peer"},
                                json=dict(screen_name="Pilot", maps=["lockout"], compatibility="halo2-native-queue-v7"))
                            assert response.status == 201, await response.text()
                            queue = {"Authorization": "Bearer "+(await response.json())["token"]}
                        if me["ready_playlist"] != "party-search":
                            # A concurrent party change makes this stale; the next poll retries.
                            await client.post("/v1/party/commands", headers=peer, json=dict(action="ready",
                                request_id=str(uuid.uuid4()), version=party["version"], playlist_id="party-search", revision="lab-1"))
                        if not cancelled and any(t.playlist == "party-search" for t in api.core.tickets.values()):
                            response = await client.delete("/v1/matchmaking/ticket", headers=queue)
                            assert response.status == 200, await response.text()
                            cancelled = True
                    if guests and not stress_done:
                        for guest in guests:
                            guest_view = await view(guest)
                            if not guest_view["party"] and guest_view["invitations"]:
                                invitation = guest_view["invitations"][0]
                                await command(guest, "accept", target=invitation["id"], version=invitation["version"])
                    if guests and not stress_done and (captures/"full-party-complete").exists():
                        for guest in guests:
                            current = (await view(guest))["party"]
                            await command(guest, "leave", version=current["version"])
                        stress_done = True
                    if joined and not reinvited and party and party["leader"] and len(party["members"]) == 1:
                        await command(peer, "invite", version=party["version"], username="verified_user")
                        reinvited = True
                    await asyncio.sleep(1)
            task = asyncio.create_task(partner())
            env = os.environ | dict(SDL_AUDIODRIVER="dummy", HTTP_PROXY="http://127.0.0.1:1", NO_PROXY="", no_proxy="")
            env['H2_TEST_PEER_FRIEND_TAG']=(await (await client.get('/v1/friends',headers=peer)).json())['friend_tag']
            env['H2_TEST_BLOCKED_FRIEND_TAGS']=json.dumps([
                (await (await client.get('/v1/friends',headers=headers)).json())['friend_tag']
                for headers in blocked_peers])
            process = await asyncio.create_subprocess_exec(str(Path(sys.argv[1]).resolve()), *assets.source,
                "--party-url", str(client.make_url("/")).rstrip("/"),
                "--callback-port",callback_port(),*native_options, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE, env=env)
            try:
                out, err = await asyncio.wait_for(process.communicate(), 600)
                print(out.decode(), end="")
                print(err.decode(), end="", file=sys.stderr)
                assert process.returncode in (0, 77), "Native party UI failed"
                if process.returncode == 77:
                    return 77
                party = (await view(peer))["party"]
                assert party and party["leader"] and len(party["members"]) == 1
                await command(peer, "leave", version=party["version"])
                directory = next(iter(app.values()))
                assert directory.store.db.execute("SELECT count(*) FROM parties").fetchone()[0] == 0
                print("PASS native party creation/invite/accept/leader transfer/leave, duplicate chosen names, private identity and " +
                      ("MCC-only imported UI inputs" if assets.mcc_cache else "Vista asset rendering"))
            finally:
                task.cancel()
                with contextlib.suppress(asyncio.CancelledError):
                    await task
                if process.returncode is None:
                    process.kill()
                    await process.wait()
    return 0

if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
