"""Two independent native couches; only external Discord is a fixture."""
import asyncio
import json
import hashlib
import os
from pathlib import Path
import socket
import signal
import sys
import tempfile
import time
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


def kill_verified_eos_owner(process, binary):
    """Kill only this owned native PC and its exact adjacent SDK worker.

    Stop the parent first so its IPC descriptors remain open while the real
    worker is identified/stopped. This prevents EOF from converting abrupt
    loss into the SDK's ordinary CloseConnections teardown. PIDFDs prevent PID
    reuse; no new process group escapes the enclosing run_test supervisor.
    """
    assert sys.platform=="linux" and hasattr(os,"pidfd_open") and hasattr(signal,"pidfd_send_signal")
    parent=os.pidfd_open(process.pid);worker=None;killed=False
    try:
        signal.pidfd_send_signal(parent,signal.SIGSTOP)
        children=set()
        for path in Path("/proc",str(process.pid),"task").glob("*/children"):
            try:children.update(int(v) for v in path.read_text().split())
            except FileNotFoundError:pass
        expected=Path(binary).parent/"eos_sdk_worker";matched=[]
        for pid in sorted(children):
            candidate=None
            try:
                candidate=os.pidfd_open(pid)
                metadata=Path("/proc",str(pid),"status").read_text().splitlines()
                ppid=next(int(line.split()[1]) for line in metadata if line.startswith("PPid:"))
                if ppid==process.pid and Path("/proc",str(pid),"exe").samefile(expected):
                    matched.append(candidate);candidate=None
            except (FileNotFoundError,ProcessLookupError):pass
            finally:
                if candidate is not None:os.close(candidate)
        if len(matched)!=1:
            for candidate in matched:os.close(candidate)
            raise AssertionError("Abrupt EOS fixture did not find exactly its own real SDK worker")
        worker=matched[0];signal.pidfd_send_signal(worker,signal.SIGSTOP)
        signal.pidfd_send_signal(parent,signal.SIGKILL);killed=True
        signal.pidfd_send_signal(worker,signal.SIGKILL)
        return dict(native_owner_sigkill=True,verified_adjacent_sdk_worker_sigkill=True,worker_count=1)
    finally:
        if worker is not None:
            if not killed:
                try:signal.pidfd_send_signal(worker,signal.SIGCONT)
                except ProcessLookupError:pass
            os.close(worker)
        if not killed:
            try:signal.pidfd_send_signal(parent,signal.SIGCONT)
            except ProcessLookupError:pass
        os.close(parent)


async def main():
    binary = str(Path(sys.argv[1]).resolve())
    mode = sys.argv[2] if len(sys.argv) > 2 else "core"
    eos = mode in ("terminal-eos", "terminal-eos-abrupt")
    abrupt = mode in ("terminal-abrupt", "terminal-eos-abrupt")
    terminal = mode in ("terminal", "terminal-abrupt") or eos
    if eos and os.environ.get("H2_EOS_LIVE_TEST")!="1":return 77
    family = mode == "family"
    family_preset = sys.argv[4] if family and len(sys.argv)>4 else None
    family_source = None
    if family:
        assert family_preset in ("Slayer", "Oddball", "2 on 1", "King", "CTF Classic", "Multi Bomb", "3 Plots")
        fixture=json.loads((ROOT/"tests/fixtures/multiplayer_engines_original_tu15.json").read_text())
        family_source=next(row for row in fixture["variants"] if row["name"]==family_preset)
        raw=bytes.fromhex(family_source["raw"])
        assert int.from_bytes(raw[0x44:0x48],"little")==family_source["source_type"]
        family_team=bool(raw[0x48]&1)
    assets = ROOT / "assets"
    sources = dict(line.split("\t", 1) for line in (assets / "asset-sources.tsv").read_text().splitlines()[1:])
    vista = sources["vista"]
    xbe = os.environ.get("HALO2_COUCH_TEST_XBE", str(assets / "multiplayer-map-pack/demos/update/Updates/default.xbe.03"))
    mcc_maps = None
    if terminal or family:
        from tests.helpers.mcc_online_ui import validate_cache
        if len(sys.argv)<4:raise ValueError("Native source proof requires an explicit prepared MCC cache")
        mcc=validate_cache(ROOT,sys.argv[3]);vista=str(mcc);mcc_maps=str(mcc/"halo2/h2_maps_win64_dx11")
        xbe=os.environ.get("HALO2_COUCH_TEST_XBE",str(Path.home()/".local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"))
        assert hashlib.sha256(Path(xbe).read_bytes()).hexdigest()=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"
    output = Path(os.environ.get("HALO2_COUCH_TEST_OUTPUT",
                                str(Path(os.environ["HALO2_TEST_SCRATCH"]) / "couch-captures" / mode)))
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
            if family:
                row["team_size"]=4 if family_team else 0
                row["games"][0]["variant"]=family_preset
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
            departure = None
            try:
                for owner in owners[:2]:
                    env = os.environ | dict(SDL_AUDIODRIVER="dummy", HTTP_PROXY="http://127.0.0.1:1", NO_PROXY="", no_proxy="",HALO2_COUCH_RENDER_BARRIER=temp)
                    if eos:
                        for key in ("HTTP_PROXY","HTTPS_PROXY","ALL_PROXY","http_proxy","https_proxy","all_proxy",
                                    "LD_PRELOAD","HALO2_EOS_ACCOUNT_FIXTURE_OBSERVER","HALO2_EOS_ACCOUNT_FIXTURE_WORKER"):
                            env.pop(key,None)
                        device=Path(temp)/("eos-device-"+owner);device.mkdir(mode=0o700)
                        env.update(H2_EOS_FORCE_RELAY="1",H2_EOS_PROFILES="0",H2_EOS_CACHE_DIR=str(device/"eos-cache"),
                            XDG_CONFIG_HOME=str(device/"config"),XDG_DATA_HOME=str(device/"data"),XDG_CACHE_HOME=str(device/"cache"))
                    if mcc_maps:env["HALO2_COUCH_TEST_MCC_MAPS"]=mcc_maps
                    if family:env["HALO2_COUCH_FAMILY_PRESET"]=family_preset
                    processes.append(await asyncio.create_subprocess_exec(binary, str(ROOT), str(client.make_url("/")).rstrip("/"),
                        owner, port(), str(output), vista, xbe, mode,
                        stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE, env=env))
                killed_owner = None;abrupt_evidence=None
                async def abrupt_departure():
                    nonlocal killed_owner,abrupt_evidence
                    if not abrupt:return
                    end=time.monotonic()+210
                    while time.monotonic()<end:
                        hosts=[owner for owner in owners[:2] if (Path(temp)/("terminal-host-"+owner)).is_file()]
                        if (Path(temp)/"terminal-paired-ready").is_file() and len(hosts)==1:
                            killed_owner=hosts[0]
                            if eos:abrupt_evidence=kill_verified_eos_owner(processes[owners.index(killed_owner)],binary)
                            else:processes[owners.index(killed_owner)].kill()
                            return
                        await asyncio.sleep(.05)
                    raise AssertionError("No actual paired checkpoint/host for abrupt departure")
                departure = asyncio.create_task(abrupt_departure())
                results = await asyncio.wait_for(asyncio.gather(*(p.communicate() for p in processes)), 580 if terminal or family else 240)
                await departure
                for owner, process, (out, err) in zip(owners, processes, results):
                    (output / (owner+".log")).write_bytes(out+err)
                    print(owner, process.returncode, out.decode(), err.decode(), flush=True)
                # Cadence/freshness diagnostics remain useful on a correctly
                # rejected stale terminal cut. Keep only bounded typed reports
                # at the caller's explicit evidence location, never raw banks.
                if terminal and os.environ.get("HALO2_ONLINE_FLOW_EVIDENCE"):
                    target=Path(os.environ["HALO2_ONLINE_FLOW_EVIDENCE"])
                    target=target.parent if target.suffix else target
                    target.mkdir(parents=True,exist_ok=True)
                    for path in sorted(output.glob("*-terminal-timing.json")):
                        assert 0<path.stat().st_size<=1024*1024
                        value=json.loads(path.read_text())
                        assert value["schema"]=="H2_COLD_TERMINAL_RECOVERY_1"
                        (target/(mode+"-"+path.name)).write_text(json.dumps(value,indent=2)+"\n")
                assert all(p.returncode == (-9 if owner==killed_owner else 0) for owner,p in zip(owners,processes)), "Native couch process failed"
                directory = next(iter(app.values()))
                db = directory.store.db
                expected = 4 if mode == "mixed" else 2
                for table in ("players", "player_profiles", "match_result_players", "match_result_memberships", "match_result_skill"):
                    count = 0 if (mode == "render" or terminal) and table.startswith("match_result_") else expected
                    if table=="match_result_memberships" and (os.environ.get("HALO2_COUCH_TEST_VARIANT") or (family and not family_team)):
                        count=0  # These bundled custom modes use individual FFA standings.
                    assert db.execute("SELECT count(*) FROM " + table).fetchone()[0] == count, table
                assert db.execute("SELECT count(DISTINCT player) FROM playlist_skill").fetchone()[0] == expected
                assert db.execute("SELECT count(*) FROM playlist_skill WHERE player LIKE 'guest-%'").fetchone()[0] == 0
                assert db.execute("SELECT count(*) FROM match_results WHERE validation='native-unanimous-v1'").fetchone()[0] == (0 if mode == "render" or terminal else 1)
                if terminal:
                    assert db.execute("SELECT count(*) FROM carnage_reports").fetchone()[0] == 0
                    receipts=[json.loads(p.read_text()) for p in output.glob("*-terminal.json")]
                    assert len(receipts)==1 and receipts[0]["passed"] and receipts[0]["result_recorded"] is False
                    assert receipts[0]["admitted_players"]==8 and receipts[0]["retained_local_players"]==4
                    costs=[json.loads(p.read_text()) for p in output.glob("*-terminal-host-cost.json")]
                    assert len(costs)==1 and costs[0]["host"] is True and costs[0]["first_frame_ms"]>=0
                    assert receipts[0]["validated_checkpoint"]["tick"]==0
                    assert receipts[0]["rendered_pcr_panes"]==list(range(6)) and receipts[0]["same_lobby_prior_stats"] and receipts[0]["unknown_rank_hidden"]
                    receipts[0]["departed_host_cost"]=costs[0]
                    if eos:
                        room=next(iter(directory.match_transport.rooms.values()))
                        assert room.eos and not room.eos_account and room.compatibility=="halo2-native-queue-v13"
                        assert len(room.eos_users)==2 and len(set(room.eos_users.values()))==2
                        receipts[0].update(real_eos_device_id_owners=2,compatibility="halo2-native-queue-v13",
                            force_relay_requested=True,account_v14_qualified=False,abrupt_owner=abrupt_evidence)
                    destination=os.environ.get("HALO2_ONLINE_FLOW_EVIDENCE")
                    if destination:
                        from PIL import Image
                        target=Path(destination);target=target.parent if target.suffix else target
                        target.mkdir(parents=True,exist_ok=True)
                        receipt=receipts[0];receipt["binary_sha256"]=hashlib.sha256(Path(binary).read_bytes()).hexdigest()
                        receipt["original_xbe_sha256"]=hashlib.sha256(Path(xbe).read_bytes()).hexdigest()
                        receipt["retained_pngs"]=[]
                        for frame in sorted(output.glob("*-terminal*.ppm")):
                            png=target/(mode+"-"+frame.stem+".png");image=Image.open(frame);image.load();image.save(png)
                            if png.stat().st_size>1024*1024 or sum(p.stat().st_size for p in target.glob("*.png"))>8*1024*1024:
                                png.unlink();continue
                            receipt["retained_pngs"].append(dict(file=png.name,bytes=png.stat().st_size,sha256=hashlib.sha256(png.read_bytes()).hexdigest()))
                        (target/(mode+"-receipt.json")).write_text(json.dumps(receipt,indent=2)+"\n")
                    print("PASS actual terminal source PCR remains explicitly unrecorded; no competitive/history rows were committed")
                elif mode != "render":
                    history_counts = {table: db.execute("SELECT count(*) FROM " + table).fetchone()[0]
                                      for table in ("carnage_player_summaries", "carnage_players",
                                                    "carnage_general_stats", "carnage_medals",
                                                    "carnage_weapon_stats", "carnage_opponents", "carnage_team_stats")}
                    history_reports = [tuple(row) for row in db.execute("SELECT admitted_players,published_players,format FROM carnage_reports")]
                    assert history_counts["carnage_player_summaries"] == expected, dict(expected=expected, counts=history_counts, reports=history_reports)
                    assert all(count == 0 for table, count in history_counts.items() if table != "carnage_player_summaries"), history_counts
                    assert history_reports == [(8, expected, "native-verified-summary-v2")], history_reports
                    assert {row[0] for row in db.execute("SELECT player FROM carnage_player_summaries")} == {
                        row[0] for row in db.execute("SELECT player FROM match_result_players")}, "History summaries differ from the verified persistent roster"
                    public = db.execute("SELECT public_id FROM carnage_reports").fetchone()[0]
                    response = await client.get("/v1/history/matches/"+public)
                    assert response.status == 200
                    history = await response.json()
                    assert len(history["players"]) == expected
                    assert (history["admitted_players"], history["published_players"], history["format"]) == (8, expected, "native-verified-summary-v2")
                    for player in history["players"]:
                        assert not player["details_available"] and player["appearance"] is None
                        assert all(player[field] == [] for field in ("general", "medals", "weapons", "opponents"))
                    assert all(team["general"] == [] for team in history["teams"])
                    assert history["guest_details"] == "excluded"
                    for player in history["players"]:
                        response = await client.get("/v1/history/players",params={"discord_username":player["discord_username"]})
                        assert (await response.json())["players"][0]["id"] == player["id"]
                        response = await client.get("/v1/history/players/"+player["id"]+"/matches")
                        assert (await response.json())["matches"][0]["id"] == public
                    encoded = json.dumps(history)
                    assert all(p not in encoded for (p,) in db.execute("SELECT id FROM players"))
                    assert "guest-" not in encoded and "discord_token" not in encoded
                    print("PASS actual native PCR verified summaries are readable by Discord username; guests, private identities and unchecked detail banks excluded")
                    if family:
                        receipts=[json.loads(p.read_text()) for p in sorted(output.glob("*-family.json"))]
                        assert len(receipts)==2 and all(r["passed"] and r["preset"]==family_preset and r["source_engine"]==family_source["source_type"] and
                            r["team_mode"]==family_team and r["result_recorded"] is True and r["fresh_authenticated_requeue"] for r in receipts)
                        assert all(r["rendered_pcr_panes"]==list(range(0 if family_team else 1,6)) for r in receipts)
                        assert all(r["all4_source_fades"] and r["ending_world_black_captured"] for r in receipts)
                        destination=os.environ.get("HALO2_ONLINE_FLOW_EVIDENCE")
                        if destination:
                            from PIL import Image
                            target=Path(destination);target=target.parent if target.suffix else target;target.mkdir(parents=True,exist_ok=True)
                            label="family-"+family_preset.lower().replace(" ","-")
                            value=dict(passed=True,preset=family_preset,process_receipts=receipts,binary_sha256=hashlib.sha256(Path(binary).read_bytes()).hexdigest(),
                                original_xbe_sha256=hashlib.sha256(Path(xbe).read_bytes()).hexdigest(),retained_pngs=[],persistent_counts=history_counts)
                            for frame in sorted(output.glob("*-family*.ppm")):
                                png=target/(label+"-"+frame.stem+".png");image=Image.open(frame);image.load();image.save(png)
                                if png.stat().st_size>1024*1024 or sum(p.stat().st_size for p in target.glob("*.png"))>8*1024*1024:
                                    png.unlink();continue
                                value["retained_pngs"].append(dict(file=png.name,bytes=png.stat().st_size,sha256=hashlib.sha256(png.read_bytes()).hexdigest()))
                            (target/(label+"-receipt.json")).write_text(json.dumps(value,indent=2)+"\n")
                        print("PASS actual source family",family_preset,"rendered native end -> unanimous receipt -> original PCR pages/Done/prior stats -> retained authenticated requeue")
                assert not directory.matchmaking.sessions and not directory.matchmaking.core.tickets
                assert all(room.closed for room in directory.match_transport.rooms.values())
                assert set(verified) == set(owners[:expected])
                print("PASS finite rendered match exits without publishing incomplete stats" if mode == "render" else
                      "PASS authenticated player results saved; guests absent from all persistent player/stat tables; all eight remain in local PCR")
            finally:
                if departure and not departure.done():
                    departure.cancel()
                    try:await departure
                    except asyncio.CancelledError:pass
                for process in processes:
                    if process.returncode is None:
                        process.kill()
                        await process.wait()
    return 0


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
