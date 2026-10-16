"""Actual native session workers; only Discord authorization is a fixture.

The coordinator selects a physical host from real peer measurements. No score,
RTT, capacity, loss, host identity or prepared-world certificate is injected.
"""
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
    output = Path(os.environ.get('HALO2_PEER_QUALITY_OUTPUT',
        str(ROOT / '.local/player-host-continuity-20261015/peer-quality')))
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='halo2-peer-quality-') as temp:
        path = Path(temp)
        fixture = catalog()
        for row in fixture['playlists']:
            row.update(minimum_players=8, maximum_players=8, maximum_party=4)
        (path / 'playlists.json').write_text(json.dumps(fixture))
        async def verify(session, token, application):
            if token not in ('one', 'two') or application != '1558522073272426519':
                raise Problem(401, 'Fixture authorization denied')
            return VerifiedOwner('discord', str(123456789012345678 + (token == 'two')), token)
        app = create_app(Settings(state=path, public_url='http://127.0.0.1:8080', allow_http=True,
            rate_per_minute=100000, matchmaking_catalog=str(path / 'playlists.json')), verify)
        async with TestClient(TestServer(app)) as client:
            headers = []
            for owner in ('one', 'two'):
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
            processes = []
            try:
                for d in descriptors:
                    descriptor = path / ('descriptor-' + str(d['slot']) + '.json')
                    descriptor.write_text(json.dumps(d)); descriptor.chmod(0o600)
                    processes.append(await asyncio.create_subprocess_exec(binary,
                        str(client.make_url('/')).rstrip('/'), str(descriptor),
                        stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE,
                        env=os.environ | dict(HTTP_PROXY='http://127.0.0.1:1', NO_PROXY='', no_proxy='')))
                async def collect(d, process):
                    out, err = await process.communicate()
                    (output / ('session-' + str(d['slot']) + '.log')).write_bytes(out + err)
                    print(process.returncode, out.decode(), err.decode(), flush=True)
                    assert process.returncode == 0, 'Native direct quality session failed'
                await asyncio.wait_for(asyncio.gather(*(collect(d, p) for d, p in zip(descriptors, processes))), 50)
                api = next(iter(app.values())).matchmaking
                room = api.transport.rooms[descriptors[0]['id']]
                measured_hosts = room.measured_hosts()
                assert measured_hosts and room.index(min(measured_hosts, key=measured_hosts.__getitem__)) == room.host
                rows = [json.loads((path / ('result-' + str(d['slot']) + '.json')).read_text()) for d in descriptors]
                assert all(r['host'] == room.host and r['received'] > 50 and len(r['candidates']) == 2 for r in rows)
                assert all(q['upload'] > 0 and q['peer'] in (0, 4) for r in rows for q in r['candidates'])
                ready = [{str(peer): metrics for peer, metrics in reports.items()} for reports in room.direct_ready.values()]
                assert all(row['upload_bytes_per_second'] > 0 for reports in ready for row in reports.values())
                evidence = dict(host=room.host, selected_host_score=list(measured_hosts[min(measured_hosts, key=measured_hosts.__getitem__)]),
                    native=rows, admitted_direct_summaries=ready,
                    gameplay_master_relay=False, world_promotion=False, discord='fixture')
                (output / 'native-session-results.json').write_text(json.dumps(evidence, indent=2) + '\n')
                print('PASS actual measured setup, physical host selection, eight-seat admission and concurrent direct datagrams')
            finally:
                for process in processes:
                    if process.returncode is None:
                        process.kill()
                await asyncio.gather(*(p.wait() for p in processes), return_exceptions=True)


if __name__ == '__main__':
    asyncio.run(main())
