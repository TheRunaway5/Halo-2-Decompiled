"""Real four-endpoint session/DTLS workers; account and native readiness fixtures."""
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
    output = Path(os.environ.get('HALO2_PEER_MIGRATION_OUTPUT',
        str(ROOT / '.local/peer-session-migration-20261018/session')))
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='halo2-peer-migration-') as temp:
        path = Path(temp)
        fixture = catalog()
        for row in fixture['playlists']:
            row.update(minimum_players=16, maximum_players=16, maximum_party=4)
        (path / 'playlists.json').write_text(json.dumps(fixture))
        async def verify(session, token, application):
            if token not in ('one', 'two', 'three', 'four') or application != '1558522073272426519':
                raise Problem(401, 'Fixture authorization denied')
            return VerifiedOwner('discord', str(123456789012345678 + ('one', 'two', 'three', 'four').index(token)), token)
        app = create_app(Settings(state=path, public_url='http://127.0.0.1:8080', allow_http=True,
            rate_per_minute=100000, matchmaking_catalog=str(path / 'playlists.json')), verify)
        async with TestClient(TestServer(app)) as client:
            headers = []
            for owner in ('one', 'two', 'three', 'four'):
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
                str(client.make_url('/')).rstrip('/'), str(descriptor),
                stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE,
                env=os.environ | dict(HTTP_PROXY='http://127.0.0.1:1', NO_PROXY='', no_proxy=''))
            try:
                out, err = await asyncio.wait_for(process.communicate(), 80)
                (output / 'session.log').write_bytes(out + err)
                print(process.returncode, out.decode(), err.decode(), flush=True)
                assert process.returncode == 0, 'Native direct migration session failed'
                result = json.loads((path / 'result.json').read_text())
                automatic = bool(os.environ.get('HALO2_PEER_AUTO_SELECT'))
                assert [r['epoch'] for r in result['handoffs']] == ([1, 2, 3] if automatic else [1, 2])
                assert [r['seats'] for r in result['handoffs']] == ([12, 8, 4] if automatic else [12, 8])
                assert not result['master_gameplay_relay']
                assert result['decoded_synthetic_audio'] == bool(os.environ.get('HALO2_PEER_AUDIO_HANDOFF'))
                assert result['automatic_measured_host_choice'] == bool(os.environ.get('HALO2_PEER_AUTO_SELECT'))
                assert result['automatic_peer_proposals']
                assert result['native_commit_callback_guard']
                (output / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
                print('PASS real direct session handoffs, media barriers and physical/couch/identity continuity')
            finally:
                if process.returncode is None:
                    process.kill()
                await process.wait()


if __name__ == '__main__':
    repeat = int(os.environ.get('HALO2_PEER_MIGRATION_REPEAT', '1'))
    if not 1 <= repeat <= 3:
        raise ValueError('Migration repetition count must be 1..3')
    base = Path(os.environ.get('HALO2_PEER_MIGRATION_OUTPUT',
        str(ROOT / '.local/peer-session-migration-20261018/session')))
    for index in range(repeat):
        if repeat > 1:
            os.environ['HALO2_PEER_MIGRATION_OUTPUT'] = str(base / ('attempt-' + str(index + 1)))
        asyncio.run(main())
