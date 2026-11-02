#!/usr/bin/env python3
"""Prepare every authenticated original destruction sound context in scratch.

V3 retains all pitch ranges, optional used storage, promotion rules/counters,
import StringIDs and literal chunks. No live owner is allocated or published.
Identical prepared payloads share owned hardlinks, never original asset files.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

from inspect_multiplayer import Cache, ROOT
from audit_vehicle_flyby_import_contract_tu15 import symbol


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def require(ok, why):
    if not ok:
        raise ValueError(why)


def entry(rows, index):
    require(0 <= index < len(rows), 'Original sound index escapes its owner')
    return rows[index]


def prepare(cache, tag, tables, resource, destination, payloads, expected, origin):
    g, classes, playbacks, scales, imports, intervals, ranges, permutations, flags, chunks, promotions = tables
    at = tag['offset']
    raw = lambda p, size: bytes(cache.data[p:p+size]).hex()
    root = bytes.fromhex(raw(at, 20))
    require(root[13] == 255 and cache.h(at+14) == -1, 'Original custom/extra sound owner needs explicit support')
    playback, scale = entry(playbacks, cache.h(at+6)), entry(scales, root[11])
    cls = entry(classes, root[2])
    normalized = dict(flags=int.from_bytes(root[:2], 'little'), sound_class=root[2],
        codec=list(root[3:6]), maximum_play_time=cache.u(at+16), playback_hex=raw(playback, 56),
        scale_hex=raw(scale, 20), sound_class_hex=raw(cls, 92),
        sound_classes_hex=[raw(row,92)for row in classes],pitch_ranges=[], promotion=None)
    require(len(classes)==54,'Original complete sound class owner has another extent')
    reference = dict(path=str(cache.path.resolve()), sound=at, playback=playback, scale=scale,
                     sound_class=cls,sound_classes=classes,gestalt=g,magic=cache.magic,pitch_ranges=[], promotion=None,origin=origin|dict(sound_datum=tag['datum']))
    index=cache.u(16);table=cache.u(index+8)-(cache.u(index)-(index+32))
    reference['sound_tag']=table+(int(tag['datum'],16)&65535)*16
    pitches, payload = [], bytearray()
    for pitch in range(root[10]):
        r = entry(ranges, cache.h(at+8)+pitch)
        first, count = cache.h(r+8), cache.h(r+10)
        require(first >= 0 and 1 <= count <= 32 and first+count <= len(permutations), 'Original permutation interval is invalid')
        imported = entry(imports, cache.h(r))
        interval = entry(intervals, cache.h(r+2))
        offset, size = int.from_bytes(cache.data[r+6:r+8], 'little') >> 3, (count+7)//8
        # Offset is the literal uint16 at+6. Preserve absence when no storage
        # is authored and the native count/class guard does not read it.
        used_at = flags[offset] if offset+size <= len(flags) else None
        require(used_at is not None or count == 1 or cache.data[cls+10] & 0x20,
                'Original sound lacks a required used-state owner')
        norm = dict(name=symbol(cache, cache.u(imported)), import_name_id=cache.u(imported),
                    interval_hex=raw(interval, 10), used_flags_hex=raw(used_at, size) if used_at is not None else None,
                    permutations=[])
        literal = dict(literal_hex=raw(r, 12), permutations=[])
        ref = dict(literal=r, interval=interval, import_name=imported,
                   used=used_at, used_bytes=size, permutations=[])
        for ordinal in range(count):
            p = permutations[first+ordinal]
            imported_perm = entry(imports, cache.h(p))
            first_chunk, chunk_count = cache.h(p+12), cache.h(p+14)
            require(first_chunk >= 0 and chunk_count > 0 and first_chunk+chunk_count <= len(chunks), 'Original sound chunk interval is invalid')
            parts, refs = [], []
            encoded = bytearray()
            for c in chunks[first_chunk:first_chunk+chunk_count]:
                pointer, descriptor = cache.u(c), cache.u(c+4)
                path = cache.path if pointer >> 30 == 0 else ROOT/'assets/disc/maps'/('', 'mainmenu.map', 'shared.map', 'single_player_shared.map')[pointer >> 30]
                source_offset, encoded_size = pointer & 0x3fffffff, descriptor & 0x3fffffff
                sector_size = (encoded_size+511) & ~511
                require(encoded_size > 0 and source_offset % 512 == 0, 'Original sound chunk is not an aligned nonempty sector')
                sector = resource(path, source_offset, sector_size)
                require(len(sector) == sector_size, 'Original sound sector is truncated')
                data = sector[:encoded_size]
                parts.append(dict(offset=len(payload), bytes=encoded_size, sector_bytes=sector_size,
                    source_flags=descriptor >> 30, literal_hex=raw(c, 12),
                    encoded_sha256=hashlib.sha256(data).hexdigest(), sector_sha256=hashlib.sha256(sector).hexdigest()))
                refs.append(dict(literal=c, path=str(path.resolve()), offset=source_offset,
                                 bytes=encoded_size, sector_bytes=sector_size))
                payload.extend(sector)
                encoded.extend(data)
            metadata = dict(ordinal=ordinal, name=symbol(cache, cache.u(imported_perm)),
                import_name_id=cache.u(imported_perm), chance_and_copy_hex=raw(p+2, 6),
                original_sample_size=cache.u(p+8), encoded_bytes=len(encoded), encoded_sha256=hashlib.sha256(encoded).hexdigest())
            key = (tag['name'], norm['name'], metadata['name'])
            require(key in expected and metadata['encoded_sha256'] == expected[key]['encoded_sha256'] and
                    tuple(root[3:6]) == (expected[key]['rate_index'], expected[key]['encoding'], expected[key]['codec']) and
                    metadata['original_sample_size'] == expected[key]['samples'], 'Prepared original payload differs from independent named audit')
            norm['permutations'].append(metadata)
            literal['permutations'].append(dict(**metadata, literal_hex=raw(p, 16), chunks=parts))
            ref['permutations'].append(dict(literal=p, import_name=imported_perm, chunks=refs))
        pitches.append(literal)
        normalized['pitch_ranges'].append(norm)
        reference['pitch_ranges'].append(ref)
    promotion = root[12]
    if promotion != 255:
        require(promotion < 128, 'Original signed promotion index is invalid')
        p = entry(promotions, promotion)
        rules, counters = cache.block(p, 16), cache.block(p+8, 4)
        normalized['promotion'] = dict(literal_hex=raw(p, 28), rules_hex=[raw(r, 16) for r in rules],
                                       counters_hex=[raw(c, 4) for c in counters])
        reference['promotion'] = dict(literal=p, rules=rules, counters=counters)
    digest = hashlib.sha256(payload).hexdigest()
    common = payloads/digest
    if not common.exists():
        common.write_bytes(payload)
    require(common.stat().st_size == len(payload) and sha(common) == digest, 'Owned prepared payload deduplication differs')
    destination.mkdir()
    os.link(common, destination/'resources.bin')
    manifest = dict(schema='H2_PREPARED_ORIGINAL_SOUND_BANK_V3', name=tag['name'],
        literal_sound_hex=root.hex(), original_contract=normalized, pitch_ranges=pitches,
        encoded_file=dict(path='resources.bin', bytes=len(payload), sha256=digest),
        provenance=origin|dict(sound_datum=tag['datum']),
        runtime_metadata_binding_complete=False, original_waveform_or_live_delivery_verified=False)
    (destination/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    return dict(directory=str(destination), sha256=sha(destination/'manifest.json'), reference=reference,
                name=tag['name'], map=cache.path.name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('inventory', 'reference-scope', 'payload-audit', 'destination', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    destination = args.destination.resolve()
    require(destination.is_relative_to(owner) and not destination.exists(), 'Prepared banks must use new owned scratch')
    inventory, scope, audit = (json.loads(p.read_text()) for p in (args.inventory, args.reference_scope, args.payload_audit))
    require(inventory['passed'] and inventory['vehicle_paths'] == 37 and audit['passed'] and audit['sound_count'] == 31,
            'Require complete authenticated destruction and payload audits')
    require(sha(args.inventory) == audit['inventory_sha256'], 'Destruction inventory changed since independent payload audit')
    paired = args.payload_audit.parent/'paired-metadata.json'
    paired_data=json.loads(paired.read_text())
    require(hashlib.sha256(json.dumps(paired_data).encode()).hexdigest() == audit['manifest_sha256'],
            'Independent paired metadata lost its original audit digest')
    expected = {}
    for row in paired_data['cases']:
        original = row['original']
        require(original['codec'] == 1 and original['channels'] in (1,2) and original['rate'] == 44100,
                'Original destruction codec needs a separately qualified importer')
        value = dict(encoded_sha256=original['encoded_sha256'], rate_index=1, encoding=original['channels']-1, codec=1, samples=original['samples'])
        key = tuple(row['key'])
        require(key not in expected or expected[key] == value, 'Independent original payload audit is ambiguous')
        expected[key] = value
    names = {name for row in inventory['owners'] for name in row['sounds']}
    maps = {row['name']: row['reference_sha256'] for row in inventory['maps']}
    pins = {str(p.resolve()): sha(p) for p in (args.inventory, args.reference_scope, args.payload_audit, paired,
        Path(__file__), ROOT/'tools/inspect_multiplayer.py', ROOT/'tools/audit_vehicle_flyby_import_contract_tu15.py')}
    streams = {}
    def pin(path):
        path = path.resolve()
        if str(path) not in pins:
            require(str(path) in audit['inputs'], 'Original input lacks independent authenticated provenance')
            pins[str(path)] = sha(path)
            require(pins[str(path)] == audit['inputs'][str(path)], 'Original authenticated source changed')
    def resource(path, offset, size):
        path = path.resolve();pin(path)
        if path not in streams:
            streams[path] = path.open('rb')
        streams[path].seek(offset)
        return streams[path].read(size)
    destination.mkdir();payloads = destination/'payloads';payloads.mkdir()
    cases = []
    try:
        for row in scope['maps']:
            path = Path(row['path'])
            if path.name not in maps:
                continue
            pin(path);require(pins[str(path.resolve())] == row['sha256'] == maps[path.name], 'Original map changed')
            cache = Cache(path)
            try:
                tags = [t for t in cache.tags.values() if t['group'] == 'snd!' and t['name'] in names and t['offset'] is not None]
                if not tags:
                    continue
                unique = lambda group: [t for t in cache.tags.values() if t['group'] == group and t['offset'] is not None]
                gestalts, classes = unique('ugh!'), unique('sncl')
                require(len(gestalts) == len(classes) == 1, 'Original sound metadata owner is ambiguous')
                g = gestalts[0]['offset']
                tables = (g, cache.block(classes[0]['offset'], 92), cache.block(g, 56), cache.block(g+8, 20),
                    cache.block(g+16, 4), cache.block(g+24, 10), cache.block(g+32, 12), cache.block(g+40, 16),
                    cache.block(g+56, 1), cache.block(g+64, 12), cache.block(g+72, 28))
                for tag in sorted(tags, key=lambda t:t['name']):
                    origin=dict(map=path.name,map_sha256=row['sha256'],gestalt_datum=gestalts[0]['datum'],class_datum=classes[0]['datum'])
                    cases.append(prepare(cache, tag, tables, resource, destination/f'bank-{len(cases):04d}', payloads, expected,origin))
            finally:
                cache.data.close();cache.stream.close()
    finally:
        for stream in streams.values():
            stream.close()
    require({row['name'] for row in cases} == names and len(names) == 31, 'Prepared bank cohort omitted original destruction names')
    require(all(sha(path) == digest for path, digest in pins.items()), 'Original preparation input changed during export')
    catalog = destination/'catalog.json';catalog.write_text(json.dumps(dict(cases=cases), separators=(',', ':'))+'\n')
    report = dict(passed=True, goal_complete=False, contexts=len(cases), sound_banks=len(names),
        catalog=str(catalog), catalog_sha256=sha(catalog), inputs=pins, inputs_unchanged=True,
        shared_owned_payload_files=len(list(payloads.iterdir())),
        owned_payload_bytes=sum(p.stat().st_size for p in payloads.iterdir()),
        native_loader_or_runtime_binding_verified=False, original_waveform_or_device_verified=False,
        retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({key:report[key] for key in ('passed', 'contexts', 'sound_banks', 'owned_payload_bytes')}), flush=True)


if __name__ == '__main__':
    main()
