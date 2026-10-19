"""Rendered four-view native ONLINE history through two direct sessions; fixture identity."""
import asyncio
import json
import os
from pathlib import Path
import sys
import tempfile
import uuid
from aiohttp.test_utils import TestClient, TestServer

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from multiplayer.master_server.service import Settings, create_app
from multiplayer.master_server.player_identity import VerifiedOwner
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.tests.test_matchmaking_http import catalog


async def main():
    binary = str(Path(sys.argv[1]).resolve())
    xbe = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
    map_path = ROOT / 'assets/disc/maps/lockout.map'
    if not xbe.exists() or not map_path.exists():
        return 77
    with tempfile.TemporaryDirectory(prefix='halo2-native-takeover-') as temp:
        path = Path(temp)
        fixture = catalog()
        for row in fixture['playlists']:
            row.update(minimum_players=8, maximum_players=8, maximum_party=4, team_size=0)
        (path / 'playlists.json').write_text(json.dumps(fixture))
        owners = ('one', 'two')

        async def verify(session, token, application):
            if token not in owners or application != '1558522073272426519':
                raise Problem(401, 'Fixture authorization denied')
            return VerifiedOwner('discord', str(123456789012345678 + owners.index(token)), token)

        app = create_app(Settings(state=path, public_url='http://127.0.0.1:8080', allow_http=True,
            rate_per_minute=100000, matchmaking_catalog=str(path / 'playlists.json')), verify)
        async with TestClient(TestServer(app)) as client:
            headers = []
            for owner in owners:
                response = await client.post('/v1/matchmaking/session', headers={'Authorization': 'Bearer ' + owner},
                    json=dict(screen_name='Pilot ' + owner, maps=['lockout'], language=0,
                        compatibility='halo2-native-queue-v12',
                        local_players=[dict(screen_name='Guest ' + str(n)) for n in (2, 3, 4)]))
                assert response.status == 201, await response.text()
                headers.append({'Authorization': 'Bearer ' + (await response.json())['token']})
            for header in headers:
                response = await client.post('/v1/matchmaking/ticket', headers=header,
                    json=dict(playlist_id='casual', revision='lab-1', request_id=str(uuid.uuid4())))
                assert response.status == 201, await response.text()
            descriptors = []
            for header in headers:
                response = await client.get('/v1/matchmaking/transport', headers=header)
                assert response.status == 200, await response.text()
                descriptors.append(await response.json())
            descriptor = path / 'descriptors.json'
            descriptor.write_text(json.dumps(descriptors)); descriptor.chmod(0o600)
            process = await asyncio.create_subprocess_exec(binary,
                str(client.make_url('/')).rstrip('/'), str(descriptor), str(xbe), str(map_path),
                stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE,
                env=os.environ | dict(HTTP_PROXY='http://127.0.0.1:1', NO_PROXY='', no_proxy=''))
            try:
                out, err = await asyncio.wait_for(process.communicate(), 240)
                print(out.decode(), err.decode(), flush=True)
                assert process.returncode == 0, 'Rendered native ONLINE effect history failed'
                directory = next(iter(app.values()))
                db = directory.store.db
                assert db.execute('SELECT count(*) FROM match_results').fetchone()[0] == 0
                assert db.execute('SELECT count(*) FROM carnage_reports').fetchone()[0] == 0
                assert all(room.direct_udp for room in directory.match_transport.rooms.values())
                print('PASS rendered native ONLINE effect history with no master gameplay/media relay or unfinished PCR publication')
            finally:
                if process.returncode is None:
                    process.kill()
                await process.wait()
    return 0


if __name__ == '__main__':
    raise SystemExit(asyncio.run(main()))
