"""Import the exact original loading bank from supported H2EK data without execution.

The Halo 2 Editing Kit ships the original PC normal-mode loading art in
loading_dx11.bin (slots 0,1,3,4,5,11) and the original four-voice ambience in
its tool executables' .rdata. Setup stages both into the private cache; the
native logo/sweep loading presentation selects this bank when it is present and
falls back to MCC V3 otherwise. No PE is executed and no Xbox/Vista input is read.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import uuid

from audit_rendering import FormatError
from installation_paths import resource_root
from mcc_movies import _cancellation, _lock, _owned, _publish_new

POLICY = 'tools/data/h2ek-original-loading-v2.json'
POLICY_SCHEMA = 'H2_H2EK_ORIGINAL_LOADING_SOURCES_V2'
SCHEMA = 'H2_H2EK_ORIGINAL_LOADING_V2'
BANK = 'presentation/original-loading-v2'
# Earlier ambience-only banks remain references; they are verified, never activated.
LEGACY_SCHEMA = 'H2_H2EK_LOADING_AMBIENCE_V1'
LEGACY_BANK = 'presentation/original-loading-v1'
TONE = 'ambience.xbox-adpcm'
SOURCE_FILES = ('guerilla.exe', 'halo2_tag_test.exe', 'sapien.exe', 'tool.exe')
TEXTURE_SOURCE = 'loading_dx11.bin'
TEXTURE_SLOTS = (0, 1, 3, 4, 5, 11)
STEPS = (7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,
    97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,
    796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,
    4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,
    16818,18500,20350,22385,24623,27086,29794,32767)
CHANGES = (-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8)
SHA256 = re.compile(r'[0-9a-f]{64}')


def _sha(data):
    return hashlib.sha256(data).hexdigest()


def _record(path):
    before = path.stat()
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    after = path.stat()
    if (before.st_size, before.st_mtime_ns, before.st_ino) != (after.st_size, after.st_mtime_ns, after.st_ino):
        raise FormatError('H2EK source changed during verification')
    return dict(bytes=after.st_size, sha256=digest)


def _dds_bytes(row):
    return 128 + ((row['width'] + 3) // 4) * ((row['height'] + 3) // 4) * (8 if row['fourcc'] == 'DXT1' else 16)


def _policy(root):
    root = resource_root(root)
    path = _owned(root / POLICY, root)
    if not path.is_file() or not 0 < path.stat().st_size <= 64 * 1024:
        raise FormatError('H2EK original loading source policy is missing or too large')
    raw = path.read_bytes()
    value = json.loads(raw)
    if (not isinstance(value, dict) or value.get('schema') != POLICY_SCHEMA or
            not isinstance(value.get('audio'), dict) or not isinstance(value.get('sources'), list) or
            not 1 <= len(value['sources']) <= 16 or not isinstance(value.get('textures'), dict)):
        raise FormatError('Invalid H2EK original loading policy')
    audio = value['audio']
    if (type(audio.get('bytes')) is not int or not 0 < audio['bytes'] <= 1024**2 or audio['bytes'] % 36 or
            audio.get('channels') != 1 or audio.get('sample_rate') != 22050 or
            audio.get('decoded_frames') != audio['bytes'] // 36 * 64 or
            any(SHA256.fullmatch(audio.get(key, '')) is None for key in ('sha256', 'decoded_pcm_sha256'))):
        raise FormatError('Invalid H2EK loading ambience audio identity')
    seen = set()
    for source in value['sources']:
        if (not isinstance(source, dict) or source.get('file') not in SOURCE_FILES or source['file'] in seen or
                type(source.get('bytes')) is not int or not 0 < source['bytes'] <= 128 * 1024**2 or
                SHA256.fullmatch(source.get('sha256', '')) is None or
                type(source.get('offset')) is not int or not 0 <= source['offset'] <= source['bytes'] - audio['bytes'] or
                not isinstance(source.get('section'), dict)):
            raise FormatError('Invalid H2EK loading ambience executable identity')
        seen.add(source['file'])
    textures = value['textures']
    images = textures.get('images')
    if (textures.get('file') != TEXTURE_SOURCE or type(textures.get('bytes')) is not int or
            not 0 < textures['bytes'] <= 64 * 1024**2 or SHA256.fullmatch(textures.get('sha256', '')) is None or
            not isinstance(images, list) or
            [row.get('slot') if isinstance(row, dict) else None for row in images] != list(TEXTURE_SLOTS)):
        raise FormatError('Invalid H2EK original loading texture source identity')
    for row in images:
        if (row.get('path') != f"loading-{row['slot']:02d}.dds" or row.get('fourcc') not in ('DXT1', 'DXT3') or
                any(type(row.get(key)) is not int for key in ('offset', 'bytes', 'width', 'height')) or
                not 0 < row['width'] <= 4096 or not 0 < row['height'] <= 4096 or row['bytes'] != _dds_bytes(row) or
                not 0 <= row['offset'] <= textures['bytes'] - row['bytes'] or
                SHA256.fullmatch(row.get('sha256', '')) is None):
            raise FormatError('Invalid H2EK original loading texture extent')
    return raw, value


def _payload_names(policy):
    return [TONE, *(row['path'] for row in policy['textures']['images'])]


def inventory(root):
    """Packages carry source policy only, never a tool executable or ripped bank."""
    if not _owned(resource_root(root) / POLICY, resource_root(root)).exists():
        return {}
    raw, _ = _policy(root)
    return {POLICY: dict(bytes=len(raw), sha256=_sha(raw))}


def required_bytes(root):
    if not _owned(resource_root(root) / POLICY, resource_root(root)).exists():
        return 0
    policy = _policy(root)[1]
    return policy['audio']['bytes'] + sum(row['bytes'] for row in policy['textures']['images']) + 16 * 1024


def discover(*, home=None, environment=None, extra_libraries=()):
    """Bounded Steam library discovery; explicit H2EK roots take precedence."""
    env = os.environ if environment is None else environment
    home = Path.home() if home is None else Path(home)
    if env.get('HALO2_H2EK_ROOT'):
        path = Path(env['HALO2_H2EK_ROOT']).expanduser().resolve()
        if not path.is_dir() or not any((path / name).is_file() for name in SOURCE_FILES):
            raise FormatError('HALO2_H2EK_ROOT does not contain extracted Halo 2 Editing Kit tools')
        return path
    # Reuse ordinary installed-game discovery to find an already configured
    # MCC library, then include Steam's other configured libraries as well.
    direct = []
    from mcc_content import discover as discover_mcc
    try:
        direct.append(discover_mcc(home=home, environment=env, extra_libraries=extra_libraries).parent.parent / 'H2EK')
    except (FormatError, OSError):
        pass
    libraries = [home / '.local/share/Steam', home / '.steam/steam', home / '.steam/root',
        home / '.var/app/com.valvesoftware.Steam/.local/share/Steam',
        home / 'Library/Application Support/Steam', *map(Path, extra_libraries)]
    for key in ('PROGRAMFILES(X86)', 'ProgramFiles(x86)', 'PROGRAMFILES', 'ProgramFiles', 'LOCALAPPDATA'):
        if env.get(key):
            libraries.append(Path(env[key]) / 'Steam')
    visited = set()
    for candidate in libraries:
        library = candidate.expanduser().resolve()
        if library in visited:
            continue
        visited.add(library)
        for config in (library / 'steamapps/libraryfolders.vdf', library / 'config/libraryfolders.vdf'):
            if config.is_file() and config.stat().st_size <= 1024**2:
                paths = re.findall(r'"(?:path|[0-9]+)"\s+"([^"\r\n]+)"', config.read_text(errors='replace'))
                libraries.extend(Path(path.replace('\\\\', '\\')) for path in paths)
        names = ['H2EK']
        manifest = library / 'steamapps/appmanifest_1613450.acf'
        if manifest.is_file() and manifest.stat().st_size <= 1024**2:
            match = re.search(r'"installdir"\s+"([^"\r\n]+)"', manifest.read_text(errors='replace'))
            if match and Path(match[1]).name == match[1] and match[1] not in ('.', '..') and '\\' not in match[1]:
                names.insert(0, match[1])
        direct.extend(library / 'steamapps/common' / name for name in names)
    for path in direct:
        if path.is_dir() and any((path / name).is_file() for name in SOURCE_FILES):
            return path.resolve()
    return None


def decode_mono_adpcm(data):
    """Match native vista_loading_audio_decode's bounded mono36→64 branch."""
    if not data or len(data) % 36 or len(data) > 1024**2:
        raise FormatError('Unsupported loading ambience ADPCM block extent')
    pcm = bytearray()
    for offset in range(0, len(data), 36):
        predictor, = struct.unpack_from('<h', data, offset)
        index = data[offset + 2]
        if index > 88:
            raise FormatError('Unsupported loading ambience ADPCM step index')
        pcm.extend(struct.pack('<h', predictor))
        for ordinal in range(63):
            nibble = (data[offset + 4 + ordinal // 2] >> (4 * (ordinal % 2))) & 15
            step = STEPS[index]
            delta = (step >> 3) + (step if nibble & 4 else 0) + (step >> 1 if nibble & 2 else 0) + (step >> 2 if nibble & 1 else 0)
            predictor = max(-32768, min(32767, predictor + (-delta if nibble & 8 else delta)))
            index = max(0, min(88, index + CHANGES[nibble]))
            pcm.extend(struct.pack('<h', predictor))
    return bytes(pcm)


def _section(data, source, extent):
    def unpack(fmt, offset):
        size = struct.calcsize(fmt)
        if not 0 <= offset <= len(data) - size:
            raise FormatError('Truncated H2EK PE metadata')
        return struct.unpack_from(fmt, data, offset)
    if data[:2] != b'MZ':
        raise FormatError('H2EK source lacks its DOS signature')
    pe, = unpack('<I', 0x3c)
    if data[pe:pe + 4] != b'PE\0\0':
        raise FormatError('H2EK source lacks its PE signature')
    count, = unpack('<H', pe + 6)
    optional_bytes, = unpack('<H', pe + 20)
    optional = pe + 24
    if not 1 <= count <= 96 or optional_bytes < 96 or unpack('<H', optional)[0] != 0x10b:
        raise FormatError('Unsupported H2EK PE32 section layout')
    matches = []
    for index in range(count):
        at = optional + optional_bytes + index * 40
        name, virtual_bytes, virtual_at, raw_bytes, raw_at, _, _, _, _, characteristics = unpack('<8s6I2HI', at)
        if raw_at <= source['offset'] and source['offset'] + extent <= raw_at + raw_bytes <= len(data):
            matches.append(dict(name=name.split(b'\0', 1)[0].decode('ascii'), virtual_address=virtual_at,
                virtual_size=virtual_bytes, raw_offset=raw_at, raw_bytes=raw_bytes,
                characteristics_hex=f'0x{characteristics:08x}'))
    if matches != [source['section']] or matches[0]['name'] != '.rdata' or matches[0]['characteristics_hex'] != '0x40000040':
        raise FormatError('H2EK loading ambience is outside its supported read-only file-backed section')


def _texture(data, row):
    """One complete DDS extent; header fields match the native loader's checks."""
    image = data[row['offset']:row['offset'] + row['bytes']]
    if (len(image) != row['bytes'] or image[:4] != b'DDS ' or struct.unpack_from('<2I', image, 4) != (124, 0x1007) or
            struct.unpack_from('<2I', image, 12) != (row['height'], row['width']) or
            struct.unpack_from('<2I', image, 76) != (32, 4) or image[84:88] != row['fourcc'].encode() or
            _sha(image) != row['sha256']):
        raise FormatError('H2EK loading texture differs from the accepted original bank: ' + row['path'])
    return image


def _extract(directory, policy):
    audio = policy['audio']
    textures = policy['textures']
    for source in policy['sources']:
        path = _owned(directory / source['file'], directory)
        if not path.exists():
            continue
        if path.stat().st_size != source['bytes'] or _record(path) != {key: source[key] for key in ('bytes', 'sha256')}:
            continue
        data = path.read_bytes()
        if len(data) != source['bytes'] or _sha(data) != source['sha256']:
            raise FormatError('H2EK executable changed during loading ambience import')
        _section(data, source, audio['bytes'])
        bank = data[source['offset']:source['offset'] + audio['bytes']]
        if len(bank) != audio['bytes'] or _sha(bank) != audio['sha256']:
            raise FormatError('H2EK loading ambience raw bytes differ from the accepted bank')
        pcm = decode_mono_adpcm(bank)
        if len(pcm) != audio['decoded_frames'] * 2 or _sha(pcm) != audio['decoded_pcm_sha256']:
            raise FormatError('H2EK loading ambience decoded PCM identity differs')
        break
    else:
        raise FormatError('Installed H2EK executables do not match a supported loading ambience source; update its verified source policy')
    path = _owned(directory / TEXTURE_SOURCE, directory)
    if not path.is_file() or _record(path) != {key: textures[key] for key in ('bytes', 'sha256')}:
        raise FormatError('Installed H2EK loading_dx11.bin does not match the supported original loading bank; update its verified source policy')
    data = path.read_bytes()
    if len(data) != textures['bytes'] or _sha(data) != textures['sha256']:
        raise FormatError('H2EK loading_dx11.bin changed during import')
    images = {row['path']: _texture(data, row) for row in textures['images']}
    return bank, source, images


def _unavailable(*, policy_available=True):
    return dict(schema=SCHEMA, complete=False, available=False, runtime_activation=False,
        policy_available=policy_available,
        reason=('Halo 2 Editing Kit is not installed; MCC loading remains selected' if policy_available else
                'H2EK loading source policy is unavailable; the original bank was not prepared and MCC loading remains selected'),
        xbox_or_vista_input_required=False)


def _verify_legacy(cache, entry):
    """Ambience-only V1 banks stay byte-checked references; they never activate."""
    files = entry.get('files')
    if entry.get('complete') is not True:
        return False
    if not isinstance(files, dict) or set(files) != {LEGACY_BANK + '/' + TONE, LEGACY_BANK + '/manifest.json'}:
        raise FormatError('Invalid installed H2EK loading ambience file inventory')
    for name, row in files.items():
        if _record(_owned(cache / name, cache)) != row:
            raise FormatError('Installed H2EK loading ambience payload differs')
    return False


def verify(cache, entry):
    cache = Path(cache)
    if isinstance(entry, dict) and entry.get('schema') == LEGACY_SCHEMA:
        return _verify_legacy(cache, entry)
    if (isinstance(entry, dict) and entry.get('schema') == SCHEMA and entry.get('complete') is False and
            entry.get('available') is False and entry.get('runtime_activation') is False and 'files' not in entry):
        return False
    if not isinstance(entry, dict):
        raise FormatError('Invalid installed H2EK original loading declaration')
    if (entry.get('schema') != SCHEMA or entry.get('complete') is not True or entry.get('available') is not True or
            entry.get('runtime_activation') is not True):
        raise FormatError('Invalid installed H2EK original loading declaration')
    files = entry.get('files')
    expected = {BANK + '/' + name for name in (TONE, *(f'loading-{slot:02d}.dds' for slot in TEXTURE_SLOTS))}
    if not isinstance(files, dict) or set(files) != expected | {BANK + '/manifest.json'}:
        raise FormatError('Invalid installed H2EK original loading file inventory')
    for name, row in files.items():
        if _record(_owned(cache / name, cache)) != row:
            raise FormatError('Installed H2EK original loading payload differs')
    metadata = json.loads((cache / BANK / 'manifest.json').read_bytes())
    if (metadata.get('schema') != SCHEMA or metadata.get('complete') is not True or
            metadata.get('runtime_activation') is not True or
            {BANK + '/' + name: row for name, row in metadata.get('files', {}).items()} !=
            {name: row for name, row in files.items() if name in expected}):
        raise FormatError('Installed H2EK original loading manifest differs')
    audio = metadata['audio']
    raw = (cache / BANK / TONE).read_bytes()
    if (_sha(raw) != audio['sha256'] or len(raw) != audio['bytes'] or
            _sha(decode_mono_adpcm(raw)) != audio['decoded_pcm_sha256']):
        raise FormatError('Installed H2EK loading ambience encoded/decoded identity differs')
    if [row.get('slot') for row in metadata.get('textures', [])] != list(TEXTURE_SLOTS):
        raise FormatError('Installed H2EK original loading texture slots differ')
    for row in metadata['textures']:
        _texture((cache / BANK / row['path']).read_bytes(), dict(row, offset=0))
    return True


def _prepare(root, cache, previous, progress):
    if previous is not None:
        if (not isinstance(previous, dict) or previous.get('schema') not in (SCHEMA, LEGACY_SCHEMA) or
                type(previous.get('complete')) is not bool):
            raise FormatError('Customized H2EK original loading provenance retained unchanged')
        if previous['schema'] == LEGACY_SCHEMA:
            # Preserve the verified V1 reference bank; prepare V2 beside it.
            _verify_legacy(cache, previous)
            legacy, previous = (previous if previous['complete'] else None), None
        else:
            legacy = None
        if previous is not None and previous['complete']:
            old_files = previous.get('files')
            if not isinstance(old_files, dict) or not all(name.startswith(BANK + '/') for name in old_files):
                raise FormatError('Invalid previous H2EK original loading inventory')
            missing = False
            for name, row in old_files.items():
                path = _owned(cache / name, cache)
                if not path.exists():
                    missing = True
                elif _record(path) != row:
                    raise FormatError('Customized original-loading source bank retained unchanged')
            if not missing:
                verify(cache, previous)
                return previous
    else:
        legacy = None
    keep = previous if previous is not None and previous.get('complete') else None
    if not _owned(resource_root(root) / POLICY, resource_root(root)).exists():
        if os.environ.get('HALO2_H2EK_ROOT') or keep is not None:
            raise FormatError('An explicit H2EK source or bank repair requires the supported original loading policy')
        progress('H2EK original loading source policy is unavailable; MCC loading remains selected.')
        return legacy or _unavailable(policy_available=False)
    policy_raw, policy = _policy(root)
    directory = discover()
    if directory is None:
        if keep is not None:
            raise FormatError('H2EK is unavailable; the missing original-loading source bank could not be repaired')
        progress('H2EK original loading bank is unavailable; MCC loading remains selected.')
        return legacy or _unavailable()
    tone, source, images = _extract(directory, policy)
    payloads = {BANK + '/' + TONE: tone, **{BANK + '/' + name: data for name, data in images.items()}}
    records = {name.removeprefix(BANK + '/'): dict(bytes=len(data), sha256=_sha(data)) for name, data in payloads.items()}
    metadata = dict(schema=SCHEMA, complete=True, available=True,
        source_kind='H2EK read-only PE .rdata and loading_dx11.bin', source=source,
        texture_source={key: policy['textures'][key] for key in ('file', 'bytes', 'sha256')},
        policy_sha256=_sha(policy_raw), audio=dict(path=TONE, **policy['audio']),
        textures=[{key: row[key] for key in ('slot', 'path', 'bytes', 'width', 'height', 'fourcc', 'sha256')}
                  for row in policy['textures']['images']],
        files=records, decoded_pcm_verified=True, extraction_only=True, pe_executed=False,
        runtime_activation=True, xbox_or_vista_input_required=False)
    payloads[BANK + '/manifest.json'] = (json.dumps(metadata, indent=2) + '\n').encode()
    files = {name: dict(bytes=len(data), sha256=_sha(data)) for name, data in payloads.items()}
    destination = _owned(cache / BANK, cache, directory=True)
    old_files = previous.get('files', {}) if previous is not None else {}
    if destination.exists():
        for path in destination.rglob('*'):
            _owned(path, cache, directory=path.is_dir())
            name = path.relative_to(cache).as_posix()
            if name not in files or _record(path) not in (files[name], old_files.get(name)):
                raise FormatError('Customized original-loading source bank retained unchanged')
    if shutil.disk_usage(cache).free < sum(row['bytes'] for row in files.values()) + 1024**2:
        raise FormatError('Insufficient free space for the H2EK original loading bank')
    progress('Preparing the exact original loading screen and ambience from verified H2EK data…')
    destination.mkdir(parents=True, exist_ok=True)
    for name, data in payloads.items():
        path = _owned(cache / name, cache)
        if path.exists() and _record(path) == files[name]:
            continue
        pending = destination / ('.h2ek-loading-' + uuid.uuid4().hex)
        try:
            pending.write_bytes(data)
            if path.exists():
                if _record(path) != old_files.get(name):
                    raise FormatError('H2EK original loading bank changed during repair')
                os.replace(pending, path)
            else:
                _publish_new(pending, path)
        finally:
            pending.unlink(missing_ok=True)
    if _policy(root)[0] != policy_raw:
        raise FormatError('H2EK source policy changed during preparation')
    entry = dict(schema=SCHEMA, complete=True, available=True, files=files,
        source_kind=metadata['source_kind'], decoded_pcm_verified=True,
        runtime_activation=True, xbox_or_vista_input_required=False)
    if legacy is not None:
        entry['legacy_reference'] = legacy
    verify(cache, entry)
    return entry


def prepare(root, cache, *, progress=print):
    cache = Path(cache)
    with _cancellation(), _lock(cache):
        return _prepare(root, cache, None, progress)


def repair(root, cache, *, progress=print):
    cache = Path(cache)
    with _cancellation(), _lock(cache):
        path = _owned(cache / 'provenance.json', cache)
        before = path.read_bytes()
        report = json.loads(before)
        if (report.get('schema') != 'H2_MCC_TU15_PREPARATION_V1' or
                report.get('external_installation_requirement') != 'Halo MCC only'):
            raise FormatError('H2EK original loading requires an owned MCC cache')
        entry = _prepare(root, cache, report.get('h2ek_loading_ambience'), progress)
        updated = {**report, 'h2ek_loading_ambience': entry}
        if report != updated:
            pending = cache / ('.h2ek-loading-provenance-' + uuid.uuid4().hex)
            try:
                pending.write_text(json.dumps(updated, indent=2) + '\n')
                if path.read_bytes() != before:
                    raise FormatError('MCC provenance changed during H2EK original loading repair')
                os.replace(pending, path)
            finally:
                pending.unlink(missing_ok=True)
        return cache, updated
