"""Steam metadata and file selection for the existing MCC preparation inputs.

Public app metadata is never treated as ownership or download authorization.
The content session must separately obtain each required depot key/manifest.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import unicodedata

PROFILE = Path(__file__).with_name('steam') / 'content-profile.json'
UINT64_MAX = (1 << 64) - 1


class SteamContentError(ValueError):
    pass


def profile():
    return json.loads(PROFILE.read_text(encoding='utf-8'))


def unsigned(value, field, *, maximum=UINT64_MAX, zero=False):
    if isinstance(value, bool) or not isinstance(value, (str, int)):
        raise SteamContentError('Invalid Steam ' + field)
    if isinstance(value, str) and not re.fullmatch(r'[0-9]+', value):
        raise SteamContentError('Invalid Steam ' + field)
    number = int(value)
    if number < (0 if zero else 1) or number > maximum:
        raise SteamContentError('Invalid Steam ' + field)
    return number


def depot_plan(metadata):
    """Bind known required roles to Valve's current public manifest IDs.

    A new base/Halo 2 depot layout requires an inspected profile update rather
    than silently downloading another game's assets or omitting a new owner.
    """
    rules = profile()
    try:
        if metadata['schema'] != 'H2_STEAM_APPINFO_V1':
            raise SteamContentError('Unrecognized Steam metadata')
        app = metadata['apps'][str(rules['app_id'])]
        depots = app['depots']
        expected = rules['depots']
        required_dlc = {rules['halo2_dlc_id'], rules['halo2_multiplayer_dlc_id']}
        found = {}
        for key, value in depots.items():
            if not key.isdecimal() or not isinstance(value, dict):
                continue
            manifest = value.get('manifests', {}).get(rules['branch'])
            if not manifest or value.get('depotfromapp'):
                continue
            dlc = unsigned(value['dlcappid'], 'DLC ID') if value.get('dlcappid') else 0
            if dlc and dlc not in required_dlc:
                continue
            config = value.get('config', {})
            if config.get('language', '') not in ('', rules['language']):
                continue
            if config.get('oslist', '') and rules['platform'] not in config['oslist'].split(','):
                continue
            if config.get('osarch', '') not in ('', rules['architecture']):
                continue
            if key not in expected:
                raise SteamContentError('Steam changed the MCC/Halo 2 depot layout; update its download profile')
            role = expected[key]
            if dlc != rules['depot_dlc'][key]:
                raise SteamContentError('Steam changed the required depot ownership relationship')
            found[key] = dict(depot_id=unsigned(key, 'depot ID'), role=role,
                manifest_id=str(unsigned(manifest['gid'], 'manifest ID')),
                depot_bytes=unsigned(manifest['size'], 'depot size'),
                depot_download_bytes=unsigned(manifest['download'], 'depot download size'))
        if found.keys() != expected.keys():
            raise SteamContentError('Steam did not return every required MCC/Halo 2 depot')
        return dict(schema='H2_STEAM_DEPOT_PLAN_V1', app_id=rules['app_id'],
            halo2_dlc_id=rules['halo2_dlc_id'], branch=rules['branch'],
            halo2_multiplayer_dlc_id=rules['halo2_multiplayer_dlc_id'],
            platform=rules['platform'], architecture=rules['architecture'], language=rules['language'],
            change_number=unsigned(app['change_number'], 'change number'),
            depots=[found[key] for key in expected], entitlement_verified=False)
    except (KeyError, TypeError, AttributeError) as error:
        raise SteamContentError('Incomplete Steam application metadata') from error


def manifest_path(name):
    if not isinstance(name, str) or not name or len(name) > 4096:
        raise SteamContentError('Invalid Steam manifest filename')
    name = name.replace('\\', '/')
    if name.startswith('/') or any(ord(c) < 32 for c in name) or ':' in name:
        raise SteamContentError('Unsafe Steam manifest filename')
    parts = name.split('/')
    if any(part in ('', '.', '..') or part.endswith((' ', '.')) for part in parts):
        raise SteamContentError('Unsafe Steam manifest filename')
    return PurePosixPath(name).as_posix()


def selected_file(name, rules=None):
    rules = profile() if rules is None else rules
    name = manifest_path(name).casefold()
    if name in {p.casefold() for p in rules['shared_files']}:
        return True
    if not name.startswith(rules['halo2_prefix'].casefold()):
        return False
    if PurePosixPath(name).suffix in rules['excluded_suffixes']:
        return False
    return not any((name + '/').startswith(prefix.casefold()) for prefix in rules['excluded_prefixes'])


def file_plan(depots, manifests):
    """Select importer inputs from authenticated, decrypted manifest inventories.

    This describes bytes and roles only. It deliberately carries no entitlement
    verdict; the Steam session remains responsible for authorization.
    """
    rules = profile()
    expected = {p['depot_id']: p for p in depots['depots']}
    seen_depots, seen_names, files = set(), set(), []
    for manifest in manifests:
        depot = unsigned(manifest['depot_id'], 'depot ID')
        identity = str(unsigned(manifest['manifest_id'], 'manifest ID'))
        if depot not in expected or depot in seen_depots or identity != expected[depot]['manifest_id']:
            raise SteamContentError('Steam manifest differs from the selected depot plan')
        seen_depots.add(depot)
        for row in manifest['files']:
            name = manifest_path(row['name'])
            if row.get('directory') or not selected_file(name, rules):
                continue
            key = unicodedata.normalize('NFC', name).casefold()
            # Mac volumes are commonly case insensitive. Reject aliases before
            # writing either file, including aliases across depot inventories.
            if key in seen_names or row.get('symlink'):
                raise SteamContentError('Ambiguous or linked Steam manifest filename')
            seen_names.add(key)
            if (expected[depot]['role'] == 'shared') != (key in {p.casefold() for p in rules['shared_files']}):
                raise SteamContentError('Steam file is assigned to the wrong content owner')
            sha1 = row['sha1']
            if not isinstance(sha1, str) or not re.fullmatch('[0-9a-fA-F]{40}', sha1):
                raise SteamContentError('Invalid Steam manifest file hash')
            files.append(dict(path=name, bytes=unsigned(row['bytes'], 'file size', zero=True),
                download_bytes=unsigned(row['compressed_bytes'], 'compressed file size', zero=True),
                sha1=sha1.lower(), depot_id=depot, manifest_id=identity))
    if seen_depots != expected.keys():
        raise SteamContentError('A required Steam manifest is missing')
    paths = {p['path'].casefold() for p in files}
    missing = [p for p in rules['required_files'] if p.casefold() not in paths]
    if missing:
        raise SteamContentError('Steam manifests lack required MCC preparation files: ' + ', '.join(missing))
    files.sort(key=lambda p: p['path'].casefold())
    digest = hashlib.sha256(json.dumps(files, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    return dict(schema='H2_STEAM_FILE_PLAN_V1', plan_sha256=digest, files=files,
        bytes=sum(p['bytes'] for p in files), download_bytes=sum(p['download_bytes'] for p in files),
        entitlement_verified=False)
