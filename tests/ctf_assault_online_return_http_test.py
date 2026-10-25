"""Actual native queue/UDP/results/PCR return for all original objective presets.

Only the external Discord identity service is a fixture. Source position and
control inputs in the C++ completion driver are explicit test inputs.
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
from multiplayer.master_server.tests.test_matchmaking_http import catalog


def port():
    with socket.socket() as endpoint:
        endpoint.bind(("127.0.0.1", 0))
        return str(endpoint.getsockname()[1])


async def main():
    binary, map_file, presets, report_file = map(Path, sys.argv[1:])
    if not map_file.is_file():
        return 77
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    variants = [row["name"] for row in json.loads(presets.read_text())["variants"]
                if row["source_type"] in (1, 9)]
    if selected := os.environ.get("HALO2_OBJECTIVE_VARIANT"):
        variants = [name for name in variants if name == selected]
    assert variants
    report = dict(passed=False, cases=[], scope="Two independent native processes; real menu profiles, original unchanged objective variants, authenticated native room/UDP, genuine source completion, unanimous persisted report, world release, PCR Done and fresh requeue. Declared source position/control inputs are not played-route or physical-device proof.")
    for index, name in enumerate(variants):
        with tempfile.TemporaryDirectory(prefix="objective-online-", dir=scratch) as temporary:
            temp = Path(temporary)
            fixture = catalog()
            for row in fixture["playlists"]:
                row["team_size"] = 1
                row["games"][0].update(map=map_file.stem, variant=name)
                row["rank_configuration"] = "retail-launch-2004"
            path = temp / "playlists.json"
            path.write_text(json.dumps(fixture))
            verified = set()

            async def verify(session, token, application):
                if token not in ("one", "two"):
                    raise Problem(401, "Fixture authorization denied")
                assert application == "1558522073272426519"
                verified.add(token)
                return VerifiedOwner("discord", str(123456789012345678 + (token == "two")), token)

            app = create_app(Settings(state=temp, public_url="http://127.0.0.1:8080", allow_http=True,
                                     rate_per_minute=100000, matchmaking_catalog=str(path)), verify)
            row = dict(name=name, passed=False, processes=[])
            async with TestClient(TestServer(app)) as client:
                processes = []
                try:
                    for owner in ("one", "two"):
                        env = os.environ | dict(SDL_AUDIODRIVER="dummy", PYTHONDONTWRITEBYTECODE="1", HALO2_TEST_SCRATCH=str(temp))
                        processes.append(await asyncio.create_subprocess_exec(str(binary), str(map_file),
                            str(client.make_url("/")).rstrip("/"), owner, port(), name, str(temp / (owner+".json")),
                            stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE, env=env))
                    async def communicate(process):
                        result = await process.communicate()
                        if process.returncode:
                            for other in processes:
                                if other is not process and other.returncode is None:
                                    other.terminate()
                        return result
                    results = await asyncio.wait_for(asyncio.gather(*(communicate(p) for p in processes)), 480)
                    for owner, process, (out, err) in zip(("one", "two"), processes, results):
                        result = dict(owner=owner, exit_code=process.returncode,
                                      stdout=out.decode()[-4096:], stderr=err.decode()[-4096:])
                        if (receipt := temp / (owner+".json")).is_file():
                            result["receipt"] = json.loads(receipt.read_text())
                        row["processes"].append(result)
                    assert all(p.returncode == 0 for p in processes), "Native objective ONLINE process failed"
                    assert verified == {"one", "two"}
                    directory = next(iter(app.values()))
                    db = directory.store.db
                    row["committed_results"] = db.execute("SELECT count(*) FROM match_results WHERE validation='native-unanimous-v1'").fetchone()[0]
                    assert row["committed_results"] == 1
                    assert db.execute("SELECT count(*) FROM match_result_players").fetchone()[0] == 2
                    assert db.execute("SELECT count(*) FROM match_result_teams").fetchone()[0] == 2
                    assert all(room.closed for room in directory.match_transport.rooms.values())
                    row["passed"] = True
                except Exception as error:
                    row["error"] = str(error)
                finally:
                    for process in processes:
                        if process.returncode is None:
                            process.kill()
                            await process.wait()
            report["cases"].append(row)
            report_file.write_text(json.dumps(report, indent=2)+"\n")
            print(name, "PASS" if row["passed"] else "FAIL", flush=True)
    report["passed"] = all(row["passed"] for row in report["cases"])
    report_file.write_text(json.dumps(report, indent=2)+"\n")
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
