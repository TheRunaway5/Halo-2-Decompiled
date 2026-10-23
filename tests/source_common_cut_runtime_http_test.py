"""Three authenticated direct sessions and real native recovery-pin owners.

Identity and playlist data are local fixtures. This does not authorize a live
login, enable migration, or qualify renderer/device/headset/WAN continuity.
"""
import sys
sys.dont_write_bytecode = True

import argparse
import asyncio
import json
import os
from pathlib import Path
import tempfile
import uuid
from aiohttp.test_utils import TestClient, TestServer

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from multiplayer.master_server.service import Settings, create_app
from multiplayer.master_server.player_identity import VerifiedOwner
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.tests.test_matchmaking_http import catalog
from multiplayer_effect_history_http_test import selected_mcc_inputs, unchanged_inputs

LOG_LIMIT = 1024 * 1024


async def bounded_output(stream):
    tail, total = bytearray(), 0
    while block := await stream.read(65536):
        total += len(block)
        tail.extend(block)
        if len(tail) > LOG_LIMIT:
            del tail[:-LOG_LIMIT]
    return bytes(tail), total


async def native(binary, origin, descriptor, gameplay, map_path, *, mcc=False):
    process = await asyncio.create_subprocess_exec(str(binary), origin, str(descriptor), gameplay, str(map_path),
        stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE,
        env=os.environ | dict(HTTP_PROXY='http://127.0.0.1:1', NO_PROXY='', no_proxy=''))
    readers = [asyncio.create_task(bounded_output(stream)) for stream in (process.stdout, process.stderr)]
    timed_out = False
    try:
        try:
            await asyncio.wait_for(process.wait(), 270)
        except asyncio.TimeoutError:
            timed_out = True
    finally:
        # Keep the child in the enclosing run_test process group as a second
        # cancellation boundary. Wait before the private backend/files retire.
        if process.returncode is None:
            process.kill()
        await process.wait()
        output = await asyncio.gather(*readers)
        for label, (tail, total) in zip(('stdout', 'stderr'), output):
            if total > len(tail):
                print(f'{label}: retained final {len(tail)} of {total} bytes', flush=True)
            print(tail.decode(errors='replace'), flush=True)
    assert not timed_out, 'Native common-cut runtime exceeded 270 seconds'
    assert process.returncode == 0, 'Native common-cut runtime failed'
    text = output[0][0].decode(errors='replace')
    inventory_mode = os.environ.get('HALO2_COMMON_INVENTORY', '')
    if inventory_mode == 'abort':
        markers = ('PASS native common-cut runtime owners: two independently validated receiver pins and exact foreground receipts',
            'PASS genuine retained inventory Abort:')
    elif inventory_mode == 'preparation':
        markers = ('PASS native common-cut runtime owners: two independently validated receiver pins and exact foreground receipts',
            'PASS genuine native preparation scheduling:')
    else:
        markers = (
        'PASS native common-cut runtime owners: two independently validated receiver pins and exact foreground receipts',
        'PASS native common-cut delayed association survives history eviction:',
        'PASS native common-cut rejection/Abort repair preserves prior native/foreground owners',
        'PASS native common-cut two released rounds retain exact proofs without live activation',
        )
    if inventory_mode == 'freeze':
        markers += ('PASS genuine retained inventory Rust freeze:',)
    elif not inventory_mode:
        markers += (
        'PASS genuine retained inventory rotation:',
        'PASS native common-cut composed receipt adoption:',
        'PASS native common-cut peer audio envelopes:',
        )
    for marker in markers:
        assert text.count(marker) == 1, 'Missing/duplicated native control: ' + marker
    if mcc and not inventory_mode:
        marker = 'PASS selected-MCC-native common-cut receipt route:'
        assert text.count(marker) == 1, 'Missing/duplicated selected MCC route: ' + marker
    if not inventory_mode:
        marker = ('PASS Rust prospective native role:' if os.environ.get('HALO2_COMMON_PROSPECTIVE')
            else 'PASS decision-free exact native import:')
        assert text.count(marker) == 1, 'Missing/duplicated exact native preparation control: ' + marker
    if os.environ.get('HALO2_COMMON_PROSPECTIVE'):
        marker = 'PASS prospective held foreground boundary:'
        assert text.count(marker) == 1, 'Missing/duplicated bound prospective foreground control: ' + marker
    if os.environ.get('HALO2_COMMON_CLIENT'):
        marker = 'PASS genuine prospective client alternatives:'
        assert text.count(marker) == 1, 'Missing/duplicated actual prospective client control: ' + marker
    if os.environ.get('HALO2_COMMON_CLIENT_ABORT'):
        marker = 'PASS genuine prospective client Abort:'
        assert text.count(marker) == 1, 'Missing/duplicated actual prospective client Abort control: ' + marker


async def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('reference', nargs='*', metavar='REFERENCE_XBE_MAP',
        help='Optional offline Xbox reference XBE and map; never used by the MCC route')
    parser.add_argument('--mcc-root', type=Path,
        help='Explicit selected prepared MCC cache; no Xbox/Vista fallback or import')
    parser.add_argument('--map', type=Path,
        help='Explicit selected MCC Lockout map, within --mcc-root')
    args = parser.parse_args()
    if args.reference and len(args.reference) != 2:
        parser.error('the reference route takes both XBE and MAP')
    if args.mcc_root is not None and args.reference:
        parser.error('--mcc-root cannot use reference XBE/MAP arguments')
    if args.map is not None and args.mcc_root is None:
        parser.error('--map requires --mcc-root')
    pins = []
    if args.mcc_root is not None:
        map_path, pins = selected_mcc_inputs(args.mcc_root)
        if args.map is not None:
            if args.map.is_symlink() or args.map.resolve(strict=True) != map_path:
                parser.error('--map must be the selected prepared MCC Lockout map')
        gameplay = '--mcc'
        print('SELECTED MCC INPUT ' + json.dumps(dict(route='selected-MCC-native-common-cut', inputs=pins,
            external_xbe=False, full_payload_hash_verified=False, import_or_conversion_performed=False,
            complete_payload_provenance_or_release_acceptance=False), sort_keys=True), flush=True)
    else:
        xbe = Path(args.reference[0] if args.reference and args.reference[0] else
            '/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
        map_path = Path(args.reference[1]) if args.reference else ROOT/'assets/disc/maps/lockout.map'
        if not xbe.is_file() or not map_path.is_file():
            return 77
        gameplay = str(xbe)
        print('REFERENCE offline Xbox TU1.5 common-cut route; not MCC runtime acceptance', flush=True)
    binary = args.binary.resolve(strict=True)
    scratch = os.environ.get('HALO2_TEST_SCRATCH')
    assert scratch and Path(scratch).is_dir(), 'Run through tools/run_test.py to own disposable storage'
    assert binary.is_file(), 'Native common-cut test executable is missing'
    with tempfile.TemporaryDirectory(prefix='halo2-common-cut-', dir=scratch) as temp:
        path = Path(temp)
        fixture = catalog()
        for row in fixture['playlists']:
            row.update(minimum_players=12, maximum_players=12, maximum_party=4, team_size=0)
        playlist = path/'playlists.json'
        playlist.write_text(json.dumps(fixture))
        owners, verified = ('one', 'two', 'three'), []

        async def verify(session, token, application):
            if token not in owners or application != '1558522073272426519':
                raise Problem(401, 'Fixture authorization denied')
            verified.append(token)
            return VerifiedOwner('discord', str(123456789012345678 + owners.index(token)), token)

        app = create_app(Settings(state=path, public_url='http://127.0.0.1:8080', allow_http=True,
            rate_per_minute=100000, matchmaking_catalog=str(playlist)), verify)
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
            descriptor = path/'descriptors.json'
            descriptor.write_text(json.dumps(descriptors))
            descriptor.chmod(0o600)
            try:
                await native(binary, str(client.make_url('/')).rstrip('/'), descriptor, gameplay, map_path,
                    mcc=args.mcc_root is not None)
            finally:
                unchanged_inputs(pins)
            assert set(verified) == set(owners), 'Not all three physical identities authenticated independently'
            directory = next(iter(app.values()))
            rooms = tuple(directory.match_transport.rooms.values())
            assert rooms and all(room.direct_udp for room in rooms), 'Fixture used a master gameplay relay'
            db = directory.store.db
            assert db.execute('SELECT count(*) FROM match_results').fetchone()[0] == 0
            assert db.execute('SELECT count(*) FROM carnage_reports').fetchone()[0] == 0
            print('PASS native common-cut master fixture: three identities, direct player transport and no unfinished PCR publication')
    return 0


if __name__ == '__main__':
    raise SystemExit(asyncio.run(main()))
