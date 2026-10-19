"""Project two authenticated MCC scenario catalogs into blank menu slots.

Only decoded metadata is changed; the caller owns transactional cache writing.
All text, logical IDs, order, flags, team counts and pixels come from MCC.
No Xbox/Vista file, runtime alias, fallback title or filename inference is used.
"""
from dataclasses import dataclass
import hashlib
from pathlib import Path
import struct

from audit_rendering import FormatError, file_sha
from mcc_content import MccCache, texture_resource

PROFILE = 'H2_MCC_AUTHORED_MULTIPLAYER_CATALOG_V1'
STRIDE = 0xC64
TARGETS = {
    r'scenarios\multi\halo\highplains\highplains': (
        'deb3b4f64fc27d54ed40f34be374a13f1a37e83203dbb7aa51ffa0262bfd48a3',
        '04e983bcbaf0cf8d4e9dbd3b8c98a195f60055c3be2c6eed8a68bb81d543b414',
        '94d8a4487566651b5bf3b406057df7603013ea6d38109cd46175481f8d9732d3',
        'dabc9945ecb84801b2b484ad1e5e36bc9d2c8e251858b07f013eded7c4fbedde'),
    r'scenarios\multi\halo\derelict\derelict': (
        '5312ae96cfdc7d5ba1286d1a59275b162cfc98d36def164d5a5e5c81307763cb',
        '9c0d29134ec06a56f6af72faad949452083618c0145f3b421d7247ba08f2fd84',
        '12ef80c371185c4be14d678ce3481585e7e42e77b89248c6f9fbc7027f0047de',
        '7433cc2daef3bbdcda34cdebf3f135ea177802f2422e0b3ddb4c4215ba4aa9c6'),
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def require(value, message):
    if not value:
        raise FormatError('MCC multiplayer catalog: ' + message)


def owned(cache, tag, at, size):
    require(tag['offset'] is not None and tag['offset'] <= at and
            at + size <= tag['offset'] + tag['size'], 'metadata escapes its complete tag owner')
    cache.require(at, size)


def block(cache, tag, at, stride):
    owned(cache, tag, at, 8)
    result = cache.block(at, stride)
    require(len(result) <= 256, 'excessive authored catalog')
    for start in result:
        owned(cache, tag, start, stride)
    return result


def catalog(cache, tag, at):
    headers = block(cache, tag, at, 24)
    require(len(headers) == 1, 'catalog requires one actual level-data header')
    return block(cache, tag, headers[0] + 16, STRIDE)


def text(record, at, size):
    return record[at:at + size].decode('utf-16le').split('\0')[0]


def summary(record):
    return dict(map_id=struct.unpack_from('<i', record)[0],
                titles=[text(record, 12 + i * 64, 64) for i in range(9)],
                descriptions=[text(record, 0x24C + i * 256, 256) for i in range(9)],
                scenario=record[0xB4C:0xC4C].split(b'\0')[0].decode('ascii'),
                sort_order=struct.unpack_from('<i', record, 0xC4C)[0],
                flags=record[0xC50], team_counts=list(record[0xC54:0xC64]))


def bitmap(cache, datum):
    tag, at = cache.tag(datum, 'bitm')
    frames = block(cache, tag, at + 68, 168)
    require(len(frames) == 1, 'preview requires its one authored frame')
    frame = bytes(cache.data[frames[0]:frames[0] + 168])
    require(struct.unpack_from('<2H', frame, 4) == (220, 207) and frame[8] == 1 and
            struct.unpack_from('<3H', frame, 10) == (0, 11, 0x210) and
            struct.unpack_from('<H', frame, 20)[0] == 0,
            'preview is outside the qualified220x207 BGRA domain')
    require(struct.unpack_from('<I', frame, 28)[0] == datum,
            'preview frame does not retain its full-salt bitmap owner')
    require(frame[60:68] == bytes(8) and frame[84:92] == bytes(8),
            'preview has unqualified secondary resource partitions')
    return tag, frames[0], frame


def resource(stream, pointer, size):
    pixels = texture_resource(stream, pointer, size)
    stream.seek(pointer & 0x3FFFFFFF)
    stored = stream.read(size)
    require(len(stored) == size, 'preview compressed resource is truncated')
    return pixels, dict(pointer=pointer, stored_bytes=size, stored_sha256=digest(stored),
                        decoded_bytes=len(pixels), sha256=digest(pixels))


@dataclass(frozen=True)
class CatalogPlan:
    mainmenu_decoded_sha256: str
    stores: tuple
    records: tuple


def plan_catalog(mainmenu, maps_directory, textures_stream):
    """Read-only plan over owned MCC maps sharing the same textures.dat bank.

    Whole C64 input fingerprints select the qualified vanilla domain. A mixed,
    partial or customized target fails closed rather than acquiring guessed data.
    An exact completed projection is idempotent.
    """
    maps_directory = Path(maps_directory)
    require(mainmenu.u32(4) == 13, 'mainmenu must remain MCC version13')
    globals_tag, globals_at = mainmenu.tag(mainmenu.u32(mainmenu.index + 16), 'matg')
    entries = catalog(mainmenu, globals_tag, globals_at + 0x178)
    by_path = {}
    for at in entries:
        path = mainmenu.string(at + 0xB4C, 256)
        if path in TARGETS:
            require(path not in by_path, 'duplicate mainmenu scenario identity')
            by_path[path] = at
    require(set(by_path) == set(TARGETS), 'qualified vanilla menu slots are missing')
    sources = {}
    # Match the actual cache header scenario, never a display name or leaf alias.
    for path in maps_directory.glob('*.map'):
        require(not path.is_symlink(), 'map source must have an owned regular file')
        with path.open('rb') as file:
            header = file.read(464)
        require(len(header) == 464 and struct.unpack_from('<I', header, 4)[0] == 13,
                'map source must remain MCC version13')
        scenario = header[208:464].split(b'\0')[0].decode('ascii')
        if scenario in TARGETS:
            require(scenario not in sources, 'ambiguous cache header scenario identity')
            sources[scenario] = path
    require(set(sources) == set(TARGETS), 'authored MCC scenario cache is missing')
    stores, records, bitmap_owners = [], [], set()
    for path, (before_pin, source_pin, frame_pin, source_frame_pin) in TARGETS.items():
        at = by_path[path]
        before = bytes(mainmenu.data[at:at + STRIDE])
        target_datum = mainmenu.u32(at + 8)
        require(target_datum not in bitmap_owners, 'preview slot has multiple catalog owners')
        bitmap_owners.add(target_datum)
        require(sum(mainmenu.u32(p + 8) == target_datum for p in entries) == 1,
                'preview slot is shared by unrelated catalog records')
        target_tag, target_at, target_frame = bitmap(mainmenu, target_datum)
        require(target_tag['name'].startswith('ui\\code_global_bitmaps\\multiplayer'),
                'destination is not its preallocated code-global preview slot')
        source = MccCache(sources[path])
        try:
            scenario_tag, scenario_at = source.tag(source.u32(source.index + 12), 'scnr')
            require(scenario_tag['name'] == path and source.string(208, 256) == path and
                    source.u16(scenario_at + 16) == 1, 'header/scenario typed ownership differs')
            local = catalog(source, scenario_tag, scenario_at + 0x398)
            require(len(local) == 1, 'scenario must own one complete multiplayer definition')
            authored = bytes(source.data[local[0]:local[0] + STRIDE])
            require(digest(authored) == source_pin and summary(authored)['scenario'] == path,
                    'whole authored MCC scenario definition differs from the qualified source')
            source_tag, source_at, source_frame = bitmap(source, source.u32(local[0] + 8))
            require(digest(source_frame) == source_frame_pin,
                    'whole authored preview descriptor differs from the qualified source')
            require(source_tag['name'].startswith('ui\\global_bitmaps\\map_shots\\'),
                    'source is not the authored map-shot bitmap')
            allowed = set(range(16, 20)) | set(range(28, 32)) | set(range(56, 60)) | set(range(80, 84))
            require(all(a == b for i, (a, b) in enumerate(zip(target_frame, source_frame)) if i not in allowed),
                    'preview descriptors differ outside the qualified ownership/resource fields')
            old_pointer, old_size = struct.unpack_from('<I', target_frame, 56)[0], struct.unpack_from('<I', target_frame, 80)[0]
            pointer, size = struct.unpack_from('<I', source_frame, 56)[0], struct.unpack_from('<I', source_frame, 80)[0]
            projected = bytearray(authored)
            # TU1.5 register1A5590 preserves runtime record+4/+8 (bitmap group
            # and datum), then copies207rows of220pixels. It supplies no new
            # registration point. Keep destination descriptor+10/+12 and+1C.
            projected[4:12] = before[4:12]
            projected = bytes(projected)
            state = 'already-converted' if before == projected else 'stock-placeholder'
            require(state == 'already-converted' or digest(before) == before_pin,
                    'partial/customized menu record is outside the qualified whole-owner states')
            if state == 'already-converted':
                require((old_pointer, old_size) == (pointer, size), 'converted metadata has a stale preview resource')
                original_frame = bytearray(target_frame)
                struct.pack_into('<I', original_frame, 56, 0x02399A80)
                struct.pack_into('<I', original_frame, 80, 1986)
                require(digest(original_frame) == frame_pin, 'converted preview has changed destination ownership or registration metadata')
            else:
                require(digest(target_frame) == frame_pin, 'partial/customized blank preview is outside the qualified whole-owner states')
            old_pixels, old_resource = resource(textures_stream, old_pointer, old_size)
            pixels, authored_resource = resource(textures_stream, pointer, size)
            require(len(old_pixels) == len(pixels) == 220 * 207 * 4, 'preview resource has a partial pixel bank')
            if state == 'stock-placeholder':
                def store(tag, offset, previous, value, operation):
                    stores.append(dict(map=mainmenu.path.name, datum=f"{tag['datum']:08x}", group=tag['group'],
                        tag=tag['name'], address=tag['address'] + offset - tag['offset'], file_offset=offset,
                        tag_offset=offset - tag['offset'], before=previous.hex(), after=value.hex(), operation=operation))
                store(globals_tag, at, before, projected, 'whole-authored-C64-with-destination-bitmap')
                store(target_tag, target_at + 56, target_frame[56:60], source_frame[56:60], 'owned-MCC-preview-partition-pointer')
                store(target_tag, target_at + 80, target_frame[80:84], source_frame[80:84], 'owned-MCC-preview-partition-size')
            records.append(dict(state=state, scenario=path, before=summary(before), after=summary(projected),
                before_record_sha256=digest(before), source_record_sha256=digest(authored), projected_record_sha256=digest(projected),
                source_cache=str(source.path), source_cache_sha256=file_sha(source.path),
                destination_bitmap=dict(datum=f'{target_datum:08x}', tag=target_tag['name'], frame_offset=target_at),
                source_bitmap=dict(datum=f"{source_tag['datum']:08x}", tag=source_tag['name'], frame_offset=source_at),
                registration_point=dict(destination=list(struct.unpack_from('<2h', target_frame, 16)),
                    source=list(struct.unpack_from('<2h', source_frame, 16)), preserved=True),
                prior_resource=old_resource, authored_resource=authored_resource))
        finally:
            source.close()
    return CatalogPlan(digest(mainmenu.data), tuple(stores), tuple(records))


def apply_catalog_plan(mainmenu, plan):
    """Validate the entire before-image, then apply only planned metadata bytes."""
    require(digest(mainmenu.data) == plan.mainmenu_decoded_sha256, 'mainmenu changed after planning')
    ranges = []
    for store in plan.stores:
        at, previous = store['file_offset'], bytes.fromhex(store['before'])
        value = bytes.fromhex(store['after'])
        require(len(previous) == len(value) and bytes(mainmenu.data[at:at + len(previous)]) == previous,
                'store before-image differs')
        require(all(at + len(previous) <= lo or at >= hi for lo, hi in ranges), 'planned metadata stores overlap')
        ranges.append((at, at + len(previous)))
    for store in plan.stores:
        at, value = store['file_offset'], bytes.fromhex(store['after'])
        mainmenu.data[at:at + len(value)] = value
    return dict(profile=PROFILE, records=list(plan.records), stores=list(plan.stores),
                source_decoded_sha256=plan.mainmenu_decoded_sha256, output_decoded_sha256=digest(mainmenu.data),
                metadata_store_blocks=len(plan.stores), metadata_store_bytes=sum(hi - lo for lo, hi in ranges),
                changed_store_blocks=sum(s['before'] != s['after'] for s in plan.stores),
                policy='MCC authored data only; no runtime aliases or Xbox/Vista runtime assets; no gameplay readiness claim')
