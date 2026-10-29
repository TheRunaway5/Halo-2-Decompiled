"""Independent native processes: real queue/QoS/admission/game state and direct voice.

Only external Discord identity endpoints are fixtures. No QoS score is injected.
"""
import asyncio
import argparse
import json
import hashlib
import io
import os
from pathlib import Path
import socket
import signal
import sys
import tempfile
import time
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from aiohttp import web
from aiohttp.test_utils import TestClient, TestServer
from multiplayer.master_server.service import Settings, create_app
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.player_identity import VerifiedOwner
from multiplayer.master_server.match_transport import Room
from multiplayer.master_server.tests.test_matchmaking_http import catalog


def retain_party_frames(output, destination):
    """Explicit, bounded retention of pixels written by the actual native draw."""
    from PIL import Image, ImageDraw
    destination=Path(destination)
    assert destination.is_absolute(), "Use an absolute explicit evidence directory"
    destination.mkdir(parents=True,exist_ok=True)
    assert not (destination/"evidence.json").exists(), "Select a fresh evidence directory for this completed run"
    reports=[json.loads(path.read_text()) for path in sorted(output.glob("*-ui-frames.json"))]
    frames=[dict(row,owner=report["owner"],behavior_source=report["behavior_source"],
                 source_xbe_sha256=report["source_xbe_sha256"])
            for report in reports for row in report["frames"] if (output/row["file"]).is_file()]
    def pick(predicate):
        return next((row for row in frames if predicate(row)),None)
    leader=pick(lambda r:r.get("leader") and r["screen"]==14 and r["ui_countdown"]==3
                and r.get("countdown") and r["countdown"]["seconds"]==3)
    assert leader, "No actual leader frame at the published stage3"
    member=pick(lambda r:not r.get("leader",True) and r["screen"]==14 and r["ui_countdown"]==3
                and r.get("countdown") and r["countdown"]["seconds"]==3
                and r["countdown"]["serial"]==leader["countdown"]["serial"])
    assert member, "No actual member frame sharing the leader's published countdown"
    candidates=[("leader-shared3",leader),("member-shared3",member)]
    shared_pairs=[]
    for pair in (("one","two"),("three","four")):
        observed=[]
        for owner in pair:
            row=pick(lambda r:r["owner"]==owner and r["screen"]==14 and r["ui_countdown"]==3
                     and r.get("countdown") and r["countdown"]["seconds"]==3)
            assert row, f"Missing actual shared-stage3 view for {owner}"
            observed.append(row)
            if row not in (leader,member):candidates.append((owner+"-shared3",row))
        assert observed[0]["leader"] and not observed[1]["leader"]
        assert observed[0]["countdown"]["serial"]==observed[1]["countdown"]["serial"]
        shared_pairs.append(dict(owners=pair,serial=observed[0]["countdown"]["serial"],seconds=3))
    for owner in ("one","two","three","four"):
        for phase in ("left","fresh-consent"):
            row=pick(lambda r:r["owner"]==owner and r["file"]==owner+"-party-back-"+phase+".ppm")
            assert row,f"Missing actual SDL B/rejoin {phase} view for {owner}"
            candidates.append((owner+"-back-"+phase,row))
    for phase in ('black','fade','clear'):
        row=pick(lambda r:r['owner']=='one' and r['file']=='one-pcr-intro-'+phase+'.ppm')
        assert row,'Missing actual PCR open '+phase+' frame'
        candidates.append(('pcr-intro-'+phase,row))
    mandatory_labels={label for label,_ in candidates}
    for label,predicate in (
        ("leader-start",lambda r:r.get("leader") and r["screen"]==14 and r["ui_countdown"]==10),
        ("leader-cancel",lambda r:r.get("leader") and r["screen"]==14 and r["ui_countdown"]<0 and r.get("countdown")==None and "cancel" in r["file"]),
        ("late-member",lambda r:not r.get("leader",True) and r["screen"]==14 and 0<r["ui_countdown"]<10 and "late" in r["file"]),
        ("member-request",lambda r:r["screen"]==14 and r.get("countdown") and r["countdown"]["mode"]==1),
        ("shared2",lambda r:r.get("leader") and r["screen"]==14 and r["ui_countdown"]==2),
        ("shared1",lambda r:r.get("leader") and r["screen"]==14 and r["ui_countdown"]==1),
        ("zero-transition",lambda r:r["screen"]==210 and r.get("countdown") and r["countdown"]["complete"]),
        ("searching",lambda r:r["screen"]==210 and r["progress_pane"]==0),
        ("configuring",lambda r:r["screen"]==210 and r["progress_pane"]==2),
        ("team-stats",lambda r:r.get("leader") and r.get("results") and r["pane"]==0),
        ("player-stats",lambda r:r.get("leader") and r.get("results") and r["pane"]==1),
        ("hit-stats",lambda r:r.get("leader") and r.get("results") and r["pane"]==5)):
        row=pick(predicate)
        if row:candidates.append((label,row))
    receipt=dict(schema="H2_OG_PARTY_VISIBLE_FLOW_1",behavior_source="OG Xbox TU1.5",
                 captures=[],not_retained=[],input_receipts=reports,all_frame_receipts=[],
                 shared_clock_pairs=shared_pairs,
                 limits=["Native SDL input harness and authenticated service; physical-controller proof is separate.",
                         "Original unchanged TU1.5 behavior and actual native framebuffer; no original Xbox framebuffer comparison is claimed.",
                         "A shared zero is the actual transition to matchmaking; no synthetic lobby zero frame is inserted."])
    for row in frames:
        raw=(output/row["file"]).read_bytes()
        with Image.open(io.BytesIO(raw)) as source:
            receipt["all_frame_receipts"].append(dict(owner=row["owner"],file=row["file"],
                bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest(),width=source.width,height=source.height,observed=row))
    total=0;thumbs=[]
    for label,row in candidates:
        raw=(output/row["file"]).read_bytes()
        with Image.open(io.BytesIO(raw)) as source:
            source.load();image=source.convert("RGB");buffer=io.BytesIO();image.save(buffer,"PNG",compress_level=9)
            png=buffer.getvalue()
            if len(png)>1024*1024 or total+len(png)>6*1024*1024:
                receipt["not_retained"].append(dict(label=label,png_bytes=len(png),reason="explicit PNG/report budget"));continue
            target=destination/(label+".png");target.write_bytes(png);total+=len(png)
            receipt["captures"].append(dict(label=label,file=target.name,bytes=len(png),sha256=hashlib.sha256(png).hexdigest(),
                original_ppm_sha256=hashlib.sha256(raw).hexdigest(),width=image.width,height=image.height,observed=row))
            image.thumbnail((384,216));thumbs.append((label,image.copy()))
    assert mandatory_labels<={r["label"] for r in receipt["captures"]},"Required four-owner shared-clock and B/rejoin PNGs exceeded the declared evidence budget"
    sheet=Image.new("RGB",(384*3,240*((len(thumbs)+2)//3)),"#071323");draw=ImageDraw.Draw(sheet)
    for index,(label,thumb) in enumerate(thumbs):
        x,y=(index%3)*384,(index//3)*240;sheet.paste(thumb,(x,y));draw.text((x+6,y+218),label,fill="white")
    buffer=io.BytesIO();sheet.save(buffer,"PNG",compress_level=9);png=buffer.getvalue()
    assert len(png)<=1024*1024 and total+len(png)<=8*1024*1024
    (destination/"contact-sheet.png").write_bytes(png)
    receipt["contact_sheet"]=dict(file="contact-sheet.png",bytes=len(png),sha256=hashlib.sha256(png).hexdigest())
    receipt["total_png_bytes"]=total+len(png)
    encoded=json.dumps(receipt,indent=2)+"\n"
    assert len(encoded.encode())<=1024*1024 and receipt["total_png_bytes"]+len(encoded.encode())<=8*1024*1024
    (destination/"evidence.json").write_text(encoded)
    print("Retained actual native party flow:",destination,"PNG bytes:",receipt["total_png_bytes"])


def port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return str(s.getsockname()[1])


async def main():
    # run_test cancellation must enter finally and retire only this helper's
    # private Pulse device, as well as its actual native child processes.
    loop=asyncio.get_running_loop();task=asyncio.current_task()
    for sig in (signal.SIGTERM,signal.SIGHUP):loop.add_signal_handler(sig,task.cancel)
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary",type=Path)
    parser.add_argument("mode",nargs="?",default="core")
    parser.add_argument("playlist",nargs="?",default="ranked")
    parser.add_argument("--mcc-root",type=Path)
    parser.add_argument("--asset-root",type=Path,default=ROOT,
                        help="Original read-only asset owner for a frozen native build")
    args=parser.parse_args()
    binary = str(args.binary.resolve())
    asset_root=args.asset_root.resolve(strict=True)
    mode,playlist=args.mode,args.playlist
    ui_root=args.mcc_root
    if ui_root is None and os.environ.get("HALO2_ONLINE_FLOW_UI_ROOT"):
        ui_root=Path(os.environ["HALO2_ONLINE_FLOW_UI_ROOT"])
    if ui_root is not None:
        ui_root=ui_root.resolve(strict=True)
        assert (ui_root/"halo2/h2_maps_win64_dx11/mainmenu.map").is_file(),"Explicit MCC root has no admitted frontend cache"
    assert mode!="party" or ui_root is not None,"Party UI fixture requires --mcc-root with prepared imported MCC assets"
    clan = mode in ("clan", "clan-major", "clan-full")
    if clan:
        playlist = "clan"
    assert playlist in ("ranked", "casual", "clan")
    with tempfile.TemporaryDirectory(prefix="halo2-real-match-") as temp:
        # A root-owned CTest session may invoke this same mode again after a
        # failure. Raw frame/barrier files must never satisfy the next run.
        output = Path(os.environ.get('HALO2_TEST_SCRATCH', temp)) / 'match-captures' / (mode if playlist == 'ranked' else mode+'-'+playlist) / Path(temp).name
        output.mkdir(parents=True, exist_ok=True)
        path = Path(temp)/"playlists.json"
        fixture = catalog()
        for row in fixture["playlists"]:
            row["rank_configuration"] = "retail-launch-2004"
        if mode == "source-render":
            for row in fixture["playlists"]:
                row.update(minimum_players=8,maximum_players=8,maximum_party=4)
        owners = ("one", "two", "three", "four") if mode == "party" else ("one", "two")
        if clan:
            owners = ("one","two","three","four","five","six")
            major = mode in ("clan-major", "clan-full")
            if major:
                owners += ("seven","eight","nine","ten","eleven","twelve","thirteen","fourteen")
                if mode == "clan-full":
                    owners += ("fifteen","sixteen")
            from multiplayer.master_server.tests.test_clan_matchmaking_http import clan_catalog
            fixture = clan_catalog()
            if major:
                fixture["playlists"][0].update(team_size=8,minimum_players=12,maximum_players=16,
                    maximum_party=8,clan_match="major-retail-launch-2004")
        if mode in ("team", "team-render", "team-render-departure", "party"):
            for row in fixture["playlists"]:
                row["team_size"] = 2 if mode == "party" else 1
                if mode == "party" or clan:
                    row["minimum_players"] = row["maximum_players"] = 4
                    row["maximum_party"] = 2
                row["games"][0]["variant"] = "Team Slayer"
        gatherings = os.environ.get("HALO2_MATCHMAKING_GATHERINGS") == "1"
        if gatherings:
            for row in fixture["playlists"]:
                row["candidate_selection"] = "retail-candidates-native-relay-v1"
                if clan:
                    row["gathering"] = dict(retail_hopper=5 if major else 4)
                elif row["minimum_players"] == row["maximum_players"] == 2:
                    row["gathering"] = dict(retail_hopper=2)
                else:
                    row["gathering"] = dict(countdown_ms=[2147483647]*16,timeout_ms=150000,
                        provenance="Native four-player validation dimensions; launch at full capacity")
        path.write_text(json.dumps(fixture))
        verified = []
        async def verify(session, token, application):
            if token not in owners:
                raise Problem(401, "Fixture authorization denied")
            assert application == "1558522073272426519"
            verified.append(token)
            return VerifiedOwner("discord", str(123456789012345678 + owners.index(token)), token)
        app = create_app(Settings(state=Path(temp), public_url="http://127.0.0.1:8080", allow_http=True,
                                  rate_per_minute=100000, matchmaking_catalog=str(path)), verify)
        countdown_outages = set()
        if mode == "core":
            @web.middleware
            async def interrupt_countdown_once(request, handler):
                # Leave the real matched room/connection alive while one
                # authenticated countdown read fails. The native queue must
                # restore its admitted match and wait for the room's zero.
                if request.path == "/v1/matchmaking/ticket" and request.query.get("start") == "1":
                    _, session = next(iter(app.values())).matchmaking.session(request)
                    room = next((room for room in next(iter(app.values())).match_transport.rooms.values()
                                 if room.phase == "loading" and not room.closed
                                 and room.active_member(session.player)), None)
                    if room and session.player not in countdown_outages:
                        countdown_outages.add(session.player)
                        return web.json_response({"error": "Temporary countdown read failure"}, status=503)
                return await handler(request)
            app.middlewares.insert(0, interrupt_countdown_once)
        outage_until = 0.
        if os.environ.get("HALO2_TEST_MASTER_OUTAGE"):
            @web.middleware
            async def outage(request, handler):
                if request.path.endswith('/signal') and time.monotonic() < outage_until:
                    return web.Response(status=503)
                return await handler(request)
            app.middlewares.insert(0, outage)
        host_admissions = set()
        if gatherings:
            real_advance = Room.advance
            async def check_admission(room):
                before = room.phase
                await real_advance(room)
                if before == 'connecting' and room.phase == 'loading':
                    assert room.players[room.host][0] == room.proposal.gathering_host
                    host_admissions.add(room.id)
            Room.advance = check_admission
            async def restore_admission_check(app):
                Room.advance = real_advance
            app.on_cleanup.append(restore_admission_check)
        if clan:
            directory = next(iter(app.values()))
            real_rate = directory.rate
            # Independent fixture accounts share localhost; only its source
            # exchange bucket is enlarged. Real per-identity limits remain.
            directory.rate = lambda key,limit=None: real_rate(key,100000 if key=="owner:127.0.0.1" else limit)
        async with TestClient(TestServer(app)) as client:
            processes = []
            audio_capture = None
            audio_proof_attempted = False
            outage_task = None
            if os.environ.get("HALO2_TEST_MASTER_OUTAGE"):
                async def interrupt_master():
                    nonlocal outage_until
                    directory = next(iter(app.values()))
                    async with asyncio.timeout(30):
                        while not any(r.phase == 'running' for r in directory.match_transport.rooms.values()):
                            await asyncio.sleep(.01)
                    room = next(r for r in directory.match_transport.rooms.values() if r.phase == 'running')
                    assert room.direct_udp
                    await asyncio.sleep(.8)
                    outage_until = time.monotonic() + 2.5
                    await asyncio.gather(*(p.socket.close(code=1001, message=b'Fixture master outage') for p in tuple(room.peers.values())))
                    async with asyncio.timeout(15):
                        while time.monotonic() < outage_until or len(room.peers) != len(room.tokens):
                            await asyncio.sleep(.01)
                    assert not room.closed and room.phase == 'running'
                outage_task = asyncio.create_task(interrupt_master())
            try:
                audio_env={}
                if os.environ.get('HALO2_ONLINE_MATCH_AUDIO'):
                    assert mode=='party' and os.environ.get('DISPLAY'),'Actual matched audio requires the rendered party flow'
                    from frontend_audio_output import NativeFrontendAudioCapture
                    audio_capture=NativeFrontendAudioCapture(output)
                    audio_env=await audio_capture.start()
                seeded = []
                if mode == "party":
                    # All invite recipients have previously signed in, as real
                    # ONLINE users must. The native workers own party commands.
                    for owner in owners:
                        response = await client.post("/v1/account/session", headers={"Authorization":"Bearer "+owner}, json={"screen_name":"Same Pilot"})
                        assert response.status == 201, await response.text()
                        seeded.append((await response.json())["token"])
                for owner in owners:
                    env = os.environ | dict(HALO2_NATIVE_FIXTURE_BARRIER=str(Path(temp)/"native-barriers"), SDL_AUDIODRIVER="dummy", HTTP_PROXY="http://127.0.0.1:1", NO_PROXY="", no_proxy="")
                    if audio_capture and owner=='one':env.update(audio_env)
                    if ui_root is not None:env["HALO2_ONLINE_FLOW_UI_ROOT"]=str(ui_root)
                    processes.append(await asyncio.create_subprocess_exec(binary, str(asset_root), str(client.make_url("/")).rstrip("/"),
                        owner, port(), str(output), mode, playlist, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE, env=env))
                if audio_capture:audio_capture.watch_completion(processes)
                results = await asyncio.wait_for(asyncio.gather(*(p.communicate() for p in processes)), 300 if mode=="party" else 180)
                if outage_task:
                    await outage_task
                for owner, process, (out, err) in zip(owners, processes, results):
                    if process.returncode not in (0,77):
                        print(f"Native fixture {owner}: exit {process.returncode}",file=sys.stderr)
                    print(out.decode(), end="")
                    print(err.decode(), end="", file=sys.stderr)
                assert all(process.returncode in (0, 77) for process in processes), "Native match process failed"
                if any(p.returncode == 77 for p in processes):
                    return 77
                if mode == "source-render":
                    assert all(b"PASS actual rendered TU1.5 ONLINE source capture workload" in out for out, err in results)
                    assert all(room.direct_udp for room in next(iter(app.values())).match_transport.rooms.values())
                    if os.environ.get("HALO2_RUNTIME_RETENTION"):
                        assert sum(b"PASS real rendered runtime disconnect handoff" in out for out, err in results) == 1
                        assert next(iter(app.values())).store.db.execute("SELECT count(*) FROM match_results").fetchone()[0] == 0
                    print("PASS actual rendered eight-seat native source workload; replication qualification requires its separate evidence")
                    return 0
                assert set(verified) == set(owners)
                if mode=="party" and (evidence:=os.environ.get("HALO2_ONLINE_FLOW_EVIDENCE")):
                    retain_party_frames(output,evidence)
                if audio_capture:
                    audio_proof_attempted = True
                    await audio_capture.prove()
                directory = next(iter(app.values()))
                if mode == "core":
                    assert len(countdown_outages) == len(owners)
                    print("PASS native matched-room countdown recovery after one failed service poll per player")
                assert not directory.matchmaking.sessions and not directory.matchmaking.core.tickets
                if gatherings:
                    assert not directory.matchmaking.gatherings.sessions
                    assert host_admissions
                    print("PASS native preflight -> measured relay route -> retail query/countdown/selection -> retained gathering host")
                assert all(room.closed for room in directory.match_transport.rooms.values())
                db = directory.store.db
                if mode in ("render-departure", "team-render-departure"):
                    assert db.execute("SELECT count(*) FROM match_results").fetchone()[0] == 0
                    print("PASS real native last-opponent GAME OVER and framebuffer; departed-owner consent prevents competitive history")
                    return 0
                if mode == "nonhost-loss":
                    assert db.execute("SELECT count(*) FROM match_results").fetchone()[0] == 0
                    print("PASS real native direct nonhost departure retained the running host; no unfinished report was committed")
                    return 0
                assert db.execute("SELECT count(*) FROM match_results WHERE validation='native-unanimous-v1'").fetchone()[0] == (0 if mode == "mismatch" else 1)
                assert db.execute("SELECT count(*) FROM match_result_players").fetchone()[0] == (0 if mode == "mismatch" else len(owners))
                assert {tuple(row) for row in db.execute("SELECT playlist,ranked FROM match_results")} == (
                    set() if mode == "mismatch" else {(playlist, int(playlist != "casual"))})
                assert {row[0] for row in db.execute("SELECT rank FROM playlist_ranks")} <= ({1, 2} if mode == "party" else {1})
                assert db.execute("SELECT count(*) FROM match_result_skill").fetchone()[0] == (0 if mode == "mismatch" or clan else len(owners))
                assert {row[0] for row in db.execute("SELECT xp FROM playlist_skill")} <= ({0, 100} if mode == "party" else {0})
                if mode == "party":
                    places = {row[0] for row in db.execute("SELECT placement FROM match_result_teams")}
                    expected = [0, 0, 0, 0] if len(places) == 1 else [0, 0, 100, 100]
                    assert sorted(row[0] for row in db.execute("SELECT delta FROM match_result_skill")) == expected
                if clan:
                    assert db.execute("SELECT count(*) FROM playlist_skill").fetchone()[0] == 0
                    places = {row[0] for row in db.execute("SELECT placement FROM match_result_teams")}
                    expected = [0,0] if len(places)==1 else [0,100]
                    assert sorted(row[0] for row in db.execute("SELECT delta FROM match_result_clan_skill")) == expected
                    assert db.execute("SELECT count(*) FROM clan_playlist_skill").fetchone()[0] == 2
                    admitted = db.execute("SELECT c.clan,m.team FROM clan_members c JOIN match_result_memberships m ON m.player=c.player").fetchall()
                    assert len(admitted)==len(owners) and len({c for c,t in admitted})==2
                    assert all(len({t for c,t in admitted if c==selected})==1 for selected,t in admitted)
                team = mode in ("team", "team-render", "party") or clan
                assert db.execute("SELECT count(*) FROM match_result_teams").fetchone()[0] == (2 if team else 0)
                assert db.execute("SELECT count(*) FROM match_result_memberships").fetchone()[0] == (len(owners) if team else 0)
                if mode == "party" or clan:
                    assert db.execute("SELECT count(*) FROM party_members").fetchone()[0] == len(owners)
                    groups = db.execute("SELECT p.party,m.team FROM party_members p JOIN match_result_memberships m ON m.player=p.player ORDER BY p.party").fetchall()
                    assert len({p for p, _ in groups}) == 2
                    for party in {p for p, _ in groups}:
                        assert len({t for p, t in groups if p == party}) == 1
                    print(f"PASS {len(owners)} independent native accounts -> accepted invitations -> per-member playlist consent -> whole parties on teams -> native gameplay/voice/results/PCR -> group requeue")
                print(f"PASS {len(owners)}-process Discord-authorized matchmaking -> measured preflight -> {'legacy relay' if gatherings else 'direct peer UDP'} native match/voice -> unanimous persisted result -> cleanup")
            finally:
                for process in processes:
                    if process.returncode is None:
                        process.kill()
                        await process.wait()
                if audio_capture:
                    # A later B/requeue assertion must not erase valid actual
                    # initial UI audio evidence. Preserve the main failure;
                    # this independent proof never makes the full flow pass.
                    try:
                        if not audio_proof_attempted and (output/'one-ui-audio-complete').is_file():
                            audio_proof_attempted = True
                            await audio_capture.prove()
                    finally:await audio_capture.close()
    return 0

if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
