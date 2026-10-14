"""Three native C++ sessions through the real relay; world seals are fixtures."""
import asyncio
import json
from pathlib import Path
import sys
import tempfile
import uuid

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from aiohttp.test_utils import TestClient, TestServer
from multiplayer.master_server.service import Settings, create_app
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.tests.test_match_continuity_http import catalog


async def main():
    binary = str(Path(sys.argv[1]).resolve())
    legacy = len(sys.argv) > 2 and sys.argv[2] == "legacy"
    processes = []
    async def command(index, value):
        process = processes[index]
        process.stdin.write((json.dumps(value) + "\n").encode())
        await process.stdin.drain()
        line = await asyncio.wait_for(process.stdout.readline(), 8)
        if not line:
            raise AssertionError((await process.stderr.read()).decode())
        return json.loads(line)
    with tempfile.TemporaryDirectory(prefix="halo2-native-continuity-") as temp:
        path = Path(temp) / "catalog.json"
        path.write_text(json.dumps(catalog()))
        async def verify(session, token, application):
            if token not in ("one", "two", "three"):
                raise Problem(401, "Fixture authorization required")
            return "discord", "native-continuity-test-" + token
        app = create_app(Settings(state=Path(temp), public_url="http://127.0.0.1:8080",
            allow_http=True, matchmaking_catalog=str(path)), verify)
        async with TestClient(TestServer(app)) as client:
            try:
                headers = []
                for owner in ("one", "two", "three"):
                    response = await client.post("/v1/matchmaking/session",
                        headers={"Authorization": "Bearer " + owner},
                        json=dict(screen_name=owner, maps=["lockout"], language=0,
                                  compatibility="halo2-native-queue-v10" if legacy else "halo2-native-queue-v11"))
                    assert response.status == 201, await response.text()
                    header = {"Authorization": "Bearer " + (await response.json())["token"]}
                    headers.append(header)
                    response = await client.post("/v1/matchmaking/ticket", headers=header,
                        json=dict(playlist_id="casual", revision="lab-1", request_id=str(uuid.uuid4())))
                    assert response.status == 201, await response.text()
                descriptors = []
                for header in headers:
                    response = await client.get("/v1/matchmaking/transport", headers=header)
                    assert response.status == 200, await response.text()
                    descriptor = await response.json()
                    descriptors.append(descriptor)
                    process = await asyncio.create_subprocess_exec(binary, stdin=asyncio.subprocess.PIPE,
                        stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
                    processes.append(process)
                    process.stdin.write((json.dumps(dict(origin=str(client.make_url("/")).rstrip("/"),
                        descriptor=descriptor)) + "\n").encode())
                    await process.stdin.drain()
                initial = [json.loads(await asyncio.wait_for(p.stdout.readline(), 10)) for p in processes]
                assert all(p["phase"] == "running" and p["epoch"] == 0 for p in initial)
                host = initial[0]["host"]
                h = next(i for i, d in enumerate(descriptors) if d["slot"] == host)
                if legacy:
                    other = (h + 1) % 3
                    await command(other, dict(action="send"))
                    await command(h, dict(action="receive", peer=descriptors[other]["slot"]))
                    await command(other, dict(action="stop"))
                    assert await asyncio.wait_for(processes[other].wait(), 5) == 0
                    for i in range(3):
                        if i != other:
                            await command(i, dict(action="wait", phase="failed"))
                            await command(i, dict(action="stop"))
                            assert await asyncio.wait_for(processes[i].wait(), 5) == 0
                    print("PASS legacy native constructor, admission, raw authenticated routes and unchanged unsupported-continuation behavior")
                    return
                await command(h, dict(action="seal"))
                for i in range(3):
                    await command(i, dict(action="ack"))
                room = next(iter(app.values())).matchmaking.transport.rooms[descriptors[0]["id"]]
                async with asyncio.timeout(3):
                    while len(room.continuity.proofs) < 3:
                        await asyncio.sleep(.005)
                await command(h, dict(action="stop"))
                assert await asyncio.wait_for(processes[h].wait(), 5) == 0
                survivors = [i for i in range(3) if i != h]
                migration = [await command(i, dict(action="wait", phase="migrating")) for i in survivors]
                assert all(p["epoch"] == 1 and p["tick"] == 100 for p in migration)
                leader = migration[0]["host"]
                candidate = next(i for i in survivors if descriptors[i]["slot"] == leader)
                other = next(i for i in survivors if i != candidate)
                await command(candidate, dict(action="reject-wrong-world"))
                await command(candidate, dict(action="accept"))
                for i in survivors:
                    await command(i, dict(action="wait", phase="rebinding"))
                await command(other, dict(action="rebind"))
                for i in survivors:
                    resumed = await command(i, dict(action="wait", phase="running"))
                    assert resumed["host"] == leader and resumed["epoch"] == 1
                await command(other, dict(action="send"))
                await command(candidate, dict(action="receive", peer=descriptors[other]["slot"]))
                assert not room.closed
                for i in survivors:
                    await command(i, dict(action="stop"))
                    assert await asyncio.wait_for(processes[i].wait(), 5) == 0
                print("PASS three actual native C++ sessions: authority generation, validated handoff/rebind, wrong-world rejection and resumed authenticated routes")
                print("SCOPE: complete-world digests are declared transport fixtures; no game-world import or gameplay host migration claimed")
            finally:
                for process in processes:
                    if process.returncode is None:
                        process.kill()
                        await process.wait()


if __name__ == "__main__":
    asyncio.run(main())
