"""Independent native processes: real queue/QoS/admission/game state and voice relay.

Only external Discord identity endpoints are fixtures. No QoS score is injected.
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
from multiplayer.master_server.match_transport import Room
from multiplayer.master_server.tests.test_matchmaking_http import catalog


def port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return str(s.getsockname()[1])


async def main():
    binary = str(Path(sys.argv[1]).resolve())
    mode = sys.argv[2] if len(sys.argv) > 2 else "core"
    playlist = sys.argv[3] if len(sys.argv) > 3 else "ranked"
    clan = mode in ("clan", "clan-major", "clan-full")
    if clan:
        playlist = "clan"
    assert playlist in ("ranked", "casual", "clan")
    output = ROOT / ".local/matchmaking/captures" / (mode if playlist == "ranked" else mode+"-"+playlist)
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="halo2-real-match-") as temp:
        path = Path(temp)/"playlists.json"
        fixture = catalog()
        for row in fixture["playlists"]:
            row["rank_configuration"] = "retail-launch-2004"
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
        if mode in ("team", "team-render", "party"):
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
            try:
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
                    processes.append(await asyncio.create_subprocess_exec(binary, str(ROOT), str(client.make_url("/")).rstrip("/"),
                        owner, port(), str(output), mode, playlist, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE, env=env))
                results = await asyncio.wait_for(asyncio.gather(*(p.communicate() for p in processes)), 180)
                for owner, process, (out, err) in zip(owners, processes, results):
                    if process.returncode not in (0,77):
                        print(f"Native fixture {owner}: exit {process.returncode}",file=sys.stderr)
                    print(out.decode(), end="")
                    print(err.decode(), end="", file=sys.stderr)
                assert all(process.returncode in (0, 77) for process in processes), "Native match process failed"
                if any(p.returncode == 77 for p in processes):
                    return 77
                assert set(verified) == set(owners)
                directory = next(iter(app.values()))
                assert not directory.matchmaking.sessions and not directory.matchmaking.core.tickets
                if gatherings:
                    assert not directory.matchmaking.gatherings.sessions
                    assert host_admissions
                    print("PASS native preflight -> measured relay route -> retail query/countdown/selection -> retained gathering host")
                assert all(room.closed for room in directory.match_transport.rooms.values())
                db = directory.store.db
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
                print(f"PASS {len(owners)}-process Discord-authorized matchmaking -> measured relay -> real native match/voice -> unanimous persisted result -> cleanup")
            finally:
                for process in processes:
                    if process.returncode is None:
                        process.kill()
                        await process.wait()
    return 0

if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
