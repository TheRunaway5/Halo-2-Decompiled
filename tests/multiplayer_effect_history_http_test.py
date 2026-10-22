"""Rendered direct ONLINE history: explicit selected MCC or offline Xbox reference."""
import argparse
import asyncio
import hashlib
import json
import os
from pathlib import Path
import stat
import struct
import sys
import tempfile
import uuid
from aiohttp.test_utils import TestClient, TestServer

ROOT = Path(__file__).resolve().parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT))
from multiplayer.master_server.service import Settings, create_app
from multiplayer.master_server.player_identity import VerifiedOwner
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.tests.test_matchmaking_http import catalog


def input_pin(path, *, manifest=False, mcc_map=False):
    """Bounded manifests/header evidence plus actual file owner, not payload certification."""
    path = Path(path)
    if path.is_symlink():
        raise ValueError('MCC history input must have a direct file owner')
    path = path.resolve(strict=True)
    with path.open('rb') as stream:
        before = os.fstat(stream.fileno())
        if not stat.S_ISREG(before.st_mode) or before.st_size <= 0:
            raise ValueError('MCC history input is not a nonempty regular file')
        if manifest and before.st_size > 1024 * 1024:
            raise ValueError('MCC history manifest exceeds its bounded input policy')
        data = stream.read() if manifest else stream.read(8)
        after = os.fstat(stream.fileno())
    identity = lambda value: [value.st_dev, value.st_ino, value.st_mode, value.st_size,
                              value.st_mtime_ns, value.st_ctime_ns]
    if identity(before) != identity(after) or identity(before) != identity(path.stat()):
        raise ValueError('MCC history input changed during admission')
    if mcc_map and (len(data) != 8 or struct.unpack('<II', data) != (0x68656164, 13)):
        raise ValueError('Selected ONLINE history requires an actual MCC version13 cache')
    return dict(path=str(path), identity=identity(before), checked_bytes=len(data),
                sha256=hashlib.sha256(data).hexdigest(), manifest=manifest, mcc_map=mcc_map)


def selected_mcc_inputs(requested, *, source_root=ROOT):
    """Require the explicit selected prepared cache; never discover, import or rehash MCC."""
    source_root = Path(source_root).resolve(strict=True)
    registry = source_root / 'assets/mcc-source.tsv'
    if registry.stat().st_size > 4096:
        raise ValueError('MCC source registry is excessive')
    registry_pin = input_pin(registry, manifest=True)
    rows = registry.read_text().splitlines()
    if (len(rows) != 2 or rows[0] != 'H2_MCC_ASSET_SOURCE_V1' or
            not rows[1].startswith('cache\t') or not rows[1][6:] or
            any(character in rows[1][6:] for character in '\t\r\n\0')):
        raise ValueError('Invalid selected MCC source registry')
    declared = Path(rows[1][6:])
    if not declared.is_absolute():
        declared = source_root / declared
    requested = Path(requested).expanduser()
    if requested.is_symlink() or declared.is_symlink():
        raise ValueError('Selected MCC cache must have a direct directory owner')
    cache = requested.resolve(strict=True)
    if (cache != declared.resolve(strict=True) or cache.name.startswith('.') or
            cache.parent != (source_root / 'assets/imported').resolve(strict=True)):
        raise ValueError('--mcc-root must be the selected owned prepared MCC cache')
    provenance = cache / 'provenance.json'
    provenance_pin = input_pin(provenance, manifest=True)
    preparation = json.loads(provenance.read_text())
    if (not isinstance(preparation, dict) or preparation.get('schema') != 'H2_MCC_TU15_PREPARATION_V1' or
            preparation.get('external_installation_requirement') != 'Halo MCC only' or
            not isinstance(preparation.get('source_files'), dict)):
        raise ValueError('Selected ONLINE history requires MCC-only preparation provenance')
    maps = cache / 'halo2/h2_maps_win64_dx11'
    if maps.is_symlink() or maps.resolve(strict=True) != maps:
        raise ValueError('Selected MCC maps lost their direct directory owner')
    pins = [registry_pin, provenance_pin]
    for name in ('lockout.map', 'mainmenu.map', 'shared.map', 'single_player_shared.map', 'textures.dat'):
        path = maps / name
        record = preparation['source_files'].get('h2_maps_win64_dx11/' + name)
        if (not isinstance(record, dict) or not isinstance(record.get('bytes'), int) or record['bytes'] <= 0 or
                not isinstance(record.get('sha256'), str) or len(record['sha256']) != 64 or
                any(character not in '0123456789abcdef' for character in record['sha256'])):
            raise ValueError('Prepared MCC provenance omitted a required source-file owner')
        if path.resolve(strict=True).parent != maps:
            raise ValueError('Selected MCC history input is outside its owned maps directory')
        pins.append(input_pin(path, mcc_map=name.endswith('.map')))
    unchanged_inputs(pins)
    return maps / 'lockout.map', pins


def unchanged_inputs(pins):
    for before in pins:
        after = input_pin(before['path'], manifest=before['manifest'], mcc_map=before['mcc_map'])
        if after != before:
            raise ValueError('Selected MCC history input ownership/header/manifest changed during the test')


async def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('--mcc-root', type=Path, help='Explicit selected prepared MCC cache; no auto discovery or Xbox/Vista fallback')
    args = parser.parse_args()
    binary = str(args.binary.resolve(strict=True))
    pins = []
    if args.mcc_root is not None:
        map_path, pins = selected_mcc_inputs(args.mcc_root)
        gameplay = '--mcc'
        print('SELECTED MCC INPUT ' + json.dumps(dict(route='selected-MCC-native', inputs=pins,
            external_xbe=False, full_payload_hash_verified=False, import_or_conversion_performed=False,
            complete_payload_provenance_or_release_acceptance=False), sort_keys=True), flush=True)
    else:
        xbe = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
        map_path = ROOT / 'assets/disc/maps/lockout.map'
        if not xbe.exists() or not map_path.exists():
            return 77
        gameplay = str(xbe)
        print('REFERENCE offline Xbox TU1.5 route; not MCC runtime acceptance', flush=True)
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
                str(client.make_url('/')).rstrip('/'), str(descriptor), gameplay, str(map_path),
                stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE,
                env=os.environ | dict(HTTP_PROXY='http://127.0.0.1:1', NO_PROXY='', no_proxy=''))
            try:
                out, err = await asyncio.wait_for(process.communicate(), 240)
                print(out.decode(), err.decode(), flush=True)
                assert process.returncode == 0, 'Rendered native ONLINE effect history failed'
                if pins:
                    assert out.count(b'PASS selected-MCC-native ONLINE receipt route:') == 1, 'MCC native route did not complete its original receipt/history gates'
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
                unchanged_inputs(pins)
    return 0


if __name__ == '__main__':
    raise SystemExit(asyncio.run(main()))
