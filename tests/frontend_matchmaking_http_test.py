"""Native menus -> real authenticated HTTP API -> coordinator -> second HTTP player.

Only Discord's external endpoints and native connection measurements are test
fixtures. No private credential, rank or network score comes from a UI field.
"""
import asyncio
import contextlib
import hashlib
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


def lobby_gpu_receipt(binary):
    output=Path(os.environ['HALO2_TEST_SCRATCH'])/'lobby-buttons-gpu'
    observations=json.loads((output/'observations.json').read_text())
    assert len(observations)==23 and len({row['phase'] for row in observations})==23
    stages={row['phase']:row for row in observations}
    assert stages['privacy-open']['screen']==25 and stages['privacy-open']['privacy']['mode']==0
    assert stages['privacy-invite-only']['screen']==14 and stages['privacy-invite-only']['privacy']['mode']==1
    assert stages['privacy-current']['selected']==1
    assert stages['privacy-same-mode-return']['privacy']['mode']==1
    assert stages['privacy-same-mode-return']['privacy']['revision']>stages['privacy-current']['privacy']['revision']
    assert stages['privacy-open-restored']['privacy']['mode']==0
    for row in observations:
        path=output/row['file'];assert path.is_file()
        row.update(raw_bytes=path.stat().st_size,raw_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
    evidence=os.environ.get('HALO2_ONLINE_FLOW_EVIDENCE')
    if not evidence:return
    destination=Path(evidence);assert destination.is_absolute()
    destination=destination/'lobby-buttons';destination.mkdir(parents=True,exist_ok=True)
    from PIL import Image,ImageDraw
    # Every full-resolution frame remains lossless. Only the navigational
    # contact sheet uses thumbnails;23 full-size panels exceeded its1MiB cap.
    cell_width,cell_height,caption_height=256,192,22
    sheet=Image.new('RGB',(3*cell_width,((len(observations)+2)//3)*(cell_height+caption_height)),'#071321');draw=ImageDraw.Draw(sheet)
    retained=0;pngs=[]
    for i,row in enumerate(observations):
        with Image.open(output/row['file']) as original:
            frame=original.convert('RGB');target=destination/(row['phase']+'.png');frame.save(target,optimize=True,compress_level=9)
            size=target.stat().st_size
            assert size<=1024*1024 and retained+size<=7*1024*1024
            retained+=size;pngs.append(dict(path=str(target),bytes=size,sha256=hashlib.sha256(target.read_bytes()).hexdigest(),phase=row['phase'],raw_ppm_sha256=row['raw_sha256'],
                dimensions=list(frame.size),decoded_rgb_sha256=hashlib.sha256(frame.tobytes()).hexdigest()))
            frame.thumbnail((cell_width,cell_height));sheet.paste(frame,((i%3)*cell_width,(i//3)*(cell_height+caption_height)))
        draw.text(((i%3)*cell_width+4,(i//3)*(cell_height+caption_height)+cell_height+2),row['phase'],fill='white')
    contact=destination/'contact-sheet.png';sheet.save(contact,optimize=True,compress_level=9);retained+=contact.stat().st_size
    assert retained<=8*1024*1024 and contact.stat().st_size<=1024*1024
    report=dict(passed=True,scope='Actual SDL key delivery and Vulkan-rendered Setup23/privacy25 with acknowledged same-mode setter/listing237/lobby/shared-countdown/search/cancel; no gameplay launch',
        original_xbe_sha256='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd',
        native_binary_sha256=hashlib.sha256(Path(binary).read_bytes()).hexdigest(),
        ui_profile_source_sha256=hashlib.sha256((ROOT/'src/ui/frontend_og_matchmaking_layout_data.inc').read_bytes()).hexdigest(),
        observations=observations,pngs=pngs,contact_sheet=dict(path=str(contact),bytes=contact.stat().st_size,sha256=hashlib.sha256(contact.read_bytes()).hexdigest(),
            dimensions=list(sheet.size),thumbnail_dimensions=[cell_width,cell_height]),
        retained_bytes=retained,raw_frames_scratch_owned=True,production_changed=False)
    (destination/'report.json').write_text(json.dumps(report,indent=2)+'\n')


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
                rank_configuration="retail-launch-2004",minimum_players=6,maximum_players=8,
                gathering=dict(retail_hopper=1))
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
        native_session_tokens = set()
        native_account_tokens=set()
        account_observations=[]
        observation_start=time.monotonic()
        native_queue_posts = []
        async def verify(session, token, application):
            assert application == "1558522073272426519"
            if token not in ("t", "peer-token"):
                raise Problem(401, "Test authorization denied")
            verified.append(token)
            return "discord", "123456789012345678" if token == "t" else "999999999999999999"
        app = create_app(Settings(state=Path(temp), public_url="http://127.0.0.1:8080", allow_http=True,
                                  rate_per_minute=100000, matchmaking_catalog=str(path)), verify)
        legacy_countdown = os.environ.get("H2_TEST_LEGACY_PARTY_COUNTDOWN") == "1"
        if legacy_countdown:
            from aiohttp import web
            @web.middleware
            async def legacy_party_view(request, handler):
                response = await handler(request)
                if request.path in ("/v1/party", "/v1/party/commands") and response.status == 200:
                    body = json.loads(response.body)
                    if body.get("party") is not None:
                        body["party"].pop("countdown", None)
                    return web.json_response(body, status=response.status)
                return response
            app.middlewares.append(legacy_party_view)
        async def observe_session(request, response):
            if request.method=='POST' and request.path=='/v1/account/session' and request.headers.get('Authorization')=='Bearer t' and response.status==201:
                native_account_tokens.add(json.loads(response.body)['token'])
            if request.headers.get('Authorization','').removeprefix('Bearer ') in native_account_tokens:
                account_observations.append(dict(method=request.method,path=request.path,status=response.status,seconds=round(time.monotonic()-observation_start,4)))
            if (request.method=="POST" and request.path=="/v1/matchmaking/session" and
                    request.headers.get("Authorization")=="Bearer t" and response.status==201):
                session_creations.append(True)
                native_session_tokens.add(json.loads(response.body)["token"])
            elif request.method=="POST" and request.path=="/v1/matchmaking/ticket" and response.status==201:
                authorization=request.headers.get("Authorization", "")
                token=authorization.removeprefix("Bearer ")
                if token in native_session_tokens:
                    native_queue_posts.append((token,json.loads(response.body)["playlist_id"]))
        app.on_response_prepare.append(observe_session)
        api = next(iter(app.values())).matchmaking
        async def fail_current_search(request):
            # Deliberate isolated integration fault at the real room owner.
            # The native client receives only its authenticated service result.
            room = next((r for r in api.transport.rooms.values() if not r.closed
                         and r.phase in ("connecting", "loading")), None)
            assert room is not None
            await room.close("Fixture connection setup failed.")
            from aiohttp import web
            return web.json_response({"failed": True})
        app.router.add_post("/fixture/fail-current-search", fail_current_search)
        consent_responses_pending = 0
        queue_during_consent = []
        async def delay_consent_response(request, response):
            nonlocal consent_responses_pending
            if request.method!="POST":
                return
            if request.path=="/v1/party/commands" and response.status==200:
                consent_responses_pending+=1
                try:
                    # The handler has consumed its bounded request stream.
                    # Delay every accepted party operation, including ready,
                    # until its native worker can observe the response.
                    await asyncio.sleep(.12)
                finally:
                    consent_responses_pending-=1
            elif request.path=="/v1/matchmaking/ticket" and consent_responses_pending:
                queue_during_consent.append(True)
        app.on_response_prepare.append(delay_consent_response)
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
                queued_playlist = None
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
                        socket=None;queued=False;queued_playlist=None
                    native_playlist=next((t.playlist for t in api.core.tickets.values()
                        if t.players[0].screen_name!="Peer Pilot"),None)
                    if queued_playlist=="ranked" and native_playlist!="ranked":
                        await client.delete("/v1/matchmaking/ticket",headers=peer)
                        queued=False;queued_playlist=None
                    desired="ranked" if gathering_run and native_playlist=="ranked" else "casual"
                    if not queued and native_playlist==desired:
                        import uuid
                        response = await client.post("/v1/matchmaking/ticket", headers=peer,
                            json=dict(playlist_id=desired, revision="lab-1", request_id=str(uuid.uuid4())))
                        assert response.status == 201
                        queued = True;queued_playlist=desired
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
                    300 if "--capture" in native_options or os.environ.get('H2_TEST_LOBBY_BUTTONS_GPU')=='1' else 120)
                print(stdout.decode(), end="")
                print(stderr.decode(), end="", file=sys.stderr)
                if process.returncode not in (0,77) and os.environ.get('H2_TEST_LOBBY_BUTTONS')=='1':
                    print('Sanitized account HTTP observations: '+json.dumps(account_observations),file=sys.stderr)
                assert process.returncode in (0, 77), "Native menu/queue integration failed"
                if process.returncode == 77: return 77
                if os.environ.get("H2_TEST_LOBBY_BUTTONS") == "1":
                    assert verified.count("t") >= 1 and len(session_creations) == 1, "Lobby buttons bypassed their authenticated session exchange"
                    assert [playlist for _,playlist in native_queue_posts] == ([] if legacy_countdown else ["ranked"]), "Lobby buttons queued an unsupported or unintended search"
                    assert not queue_during_consent, "Start queued before accepted playlist consent"
                    assert not api.core.proposals and not api.core.tickets, "Lobby button cancellation retained a live proposal/ticket"
                    assert all(s.screen_name == "Peer Pilot" for s in api.sessions.values()), "Lobby button fixture retained its native session after sign-out"
                    assert not any(row['status']==429 for row in account_observations),'Native lobby polling exceeded the actual account service budget'
                    if not legacy_countdown:
                        countdown_reads=[r['seconds'] for r in account_observations if r['method']=='GET' and r['path']=='/v1/party/countdown']
                        assert countdown_reads,'Native shared lobby never observed its dedicated countdown endpoint'
                        assert max(sum(t<=v<t+1 for v in countdown_reads) for t in countdown_reads)<=11,'Native countdown polling exceeded its ten-per-second cadence'
                        social_reads=[r['seconds'] for r in account_observations if r['method']=='GET' and r['path'] in ('/v1/clans','/v1/friends','/v1/party')]
                        assert max((sum(t<=v<t+60 for v in social_reads) for t in social_reads),default=0)<=120,'Native social refresh consumed all120/minute account reads'
                        print('PASS bounded native account reads: '+json.dumps(dict(countdown=len(countdown_reads),social=len(social_reads),elapsed=round(time.monotonic()-observation_start,2))))
                    if os.environ.get('H2_TEST_LOBBY_BUTTONS_GPU')=='1':lobby_gpu_receipt(binary)
                    print("PASS authenticated native lobby buttons, legacy countdown rejection" if legacy_countdown else
                          "PASS authenticated native source Game Setup and shared-clock Start")
                    return 0
                if "--return-lobby-ui" in native_options:
                    assert verified.count("t") >= 1 and len(session_creations) == 1, "Lobby return bypassed its authenticated session exchange"
                    assert [playlist for _, playlist in native_queue_posts] == ["ranked"], "Return control did not abandon exactly its intended search"
                    assert {token for token, _ in native_queue_posts} == native_session_tokens, "Return control replaced its authenticated queue session"
                    assert all(t.players[0].screen_name == "Peer Pilot" for t in api.core.tickets.values()), "Return retained an abandoned ticket"
                    assert all(s.screen_name == "Peer Pilot" for s in api.sessions.values()), "Return fixture retained its queue session after teardown"
                    assert not api.core.proposals and not queue_during_consent, "Return retained a proposal or queued before consent"
                    print("PASS authenticated abandoned-search return to original pregame lobby with retained party, playlist and profile")
                    return 0
                # The account grant authenticates one queue session. Lobby
                # and hopper transitions retain it; explicit final teardown
                # revokes it rather than forcing another identity exchange.
                assert verified.count("t") >= 1 and len(session_creations) == 1, "Native client did not retain its single authenticated matchmaking session"
                assert len(native_session_tokens) == 1, "Hopper transitions replaced the authenticated queue token"
                assert [playlist for _,playlist in native_queue_posts] == ["ranked","casual","casual","casual","casual"], "Native client did not perform the five intended authenticated searches/recoveries exactly once"
                assert {token for token,_ in native_queue_posts} == native_session_tokens, "Hopper searches did not share the authenticated session"
                assert all(t.players[0].screen_name == "Peer Pilot" for t in api.core.tickets.values()), "Native queue survived cancellation or sign-out"
                assert all(s.screen_name == "Peer Pilot" for s in api.sessions.values()), "Native session survived teardown"
                assert not api.core.proposals
                assert not queue_during_consent, "Queue worker enqueued before the party worker observed its consent response"
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
