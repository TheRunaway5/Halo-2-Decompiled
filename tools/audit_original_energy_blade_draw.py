#!/usr/bin/env python3
"""Identify retained original20 draw8787 from literal geometry and owner banks.

No GPU replay, guest access, native rendering, pose fitting or owner proximity
search is performed. The earlier immutable object snapshot is corroborated
against the actual draw's complete consumed root palette, not relabelled as
the same frame.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def words(data):
    assert len(data) % 4 == 0
    return [f'{v:08X}' for v in struct.unpack('<' + 'I' * (len(data) // 4), data)]


def resources(cache, section):
    pointer, size = cache.u32(section + 0x38), cache.u32(section + 0x3c)
    names = [cache.path.name, 'mainmenu.map', 'shared.map', 'single_player_shared.map']
    path = cache.path.parent / names[pointer >> 30]
    with path.open('rb') as f:
        f.seek(pointer & 0x3fffffff)
        raw = f.read(size)
    assert len(raw) == size
    base = size - cache.u32(section + 0x44) - 4
    entries = cache.block(section + 0x48, 16)
    def block(entry):
        first, length = base + cache.u32(entry + 12), cache.u32(entry + 8)
        assert first >= 0 and first + length <= len(raw)
        return raw[first:first + length]
    parts = block(entries[0])
    indices = block(next(p for p in entries if cache.u16(p + 4) == 32))
    assert len(parts) % 72 == 0 and len(indices) % 2 == 0
    return parts, struct.unpack('<' + 'H' * (len(indices) // 2), indices)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer-shading/original-energy-blade-draw8787.json')
    args = parser.parse_args()
    source = ROOT / 'analysis/multiplayer-shading/appearance-residual-replay/source'
    snapshot = ROOT / 'analysis/multiplayer-shading/registration-snapshot-restore-20261008'
    cache = Cache(ROOT / 'assets/disc/maps/zanzibar.map')
    indices = struct.unpack('<32I', (source / '8787-indices.bin').read_bytes())
    vp = (source / '8787-vertex-block0.bin').read_bytes()
    model, root = cache.tag(0xe2d90164, 'mode')
    materials = [cache.u32(p + 12) for p in cache.block(root + 0x60, 32)]
    parts_report = []
    for section_number, section in enumerate(cache.block(root + 0x24, 92)):
        part_bytes, index_words = resources(cache, section)
        for part_number in range(len(part_bytes) // 72):
            p = part_number * 72
            typ, flags, material, first, count = struct.unpack_from('<5H', part_bytes, p)
            strip = index_words[first:first + count]
            assert len(strip) == count
            parts_report.append(dict(section=section_number, part=part_number, type=typ,
                flags=flags, material=material, shader=f'{materials[material]:08X}',
                index_first=first, index_count=count, indices=list(strip),
                matches_captured_strip=strip == indices))
    matches = [p for p in parts_report if p['matches_captured_strip']]
    assert len(matches) == 1 and (matches[0]['section'], matches[0]['part']) == (0, 0)
    assert matches[0]['shader'] == 'E2DA0165'
    limits_at = cache.block(root + 0x14, 56)[0]
    limits = struct.unpack_from('<10f', cache.data, limits_at)
    scales = [f32(f32(limits[2*i+1] - limits[2*i]) * .5) for i in range(5)]
    centers = [f32(f32(limits[2*i+1] + limits[2*i]) * .5) for i in range(5)]
    constants = [scales[:3] + [1], centers[:3] + [0], scales[3:] + centers[3:]]
    packed = struct.pack('<12f', *sum(constants, []))
    assert packed == vp[170*16:173*16]
    # The FP asset is an independent negative control, including authored UV.
    _, fp_root = cache.tag(0xe30b0196, 'mode')
    fp_at = cache.block(fp_root + 0x14, 56)[0]
    assert bytes(cache.data[fp_at:fp_at+40]) != bytes(cache.data[limits_at:limits_at+40])
    exported = json.loads((snapshot / 'export.json').read_text())
    owner_matches = []
    for owner in exported['objects']:
        datum = int(owner['definition'], 16)
        tag = cache.tags.get(datum)
        if not tag or tag['group'] != 'weap':
            continue
        hlmt = cache.u32(tag['address'] - cache.magic + 0x38)
        if hlmt not in cache.tags:
            continue
        mode = cache.u32(cache.tags[hlmt]['address'] - cache.magic + 4)
        if mode != model['datum']:
            continue
        body_path = snapshot / 'exports' / f"object-{owner['handle']}.bin"
        body = body_path.read_bytes()
        assert struct.unpack_from('<I', body)[0] == datum
        # Source node0 is retained as scale, column-basis, translation; the
        # bound c50..52 bank is the transpose with scale multiplied separately.
        world = struct.unpack_from('<13f', body, owner['world_offset'])
        native_palette = [[f32(world[1+r+3*k] * world[0]) for k in range(3)] + [world[10+r]] for r in range(3)]
        palette_bytes = struct.pack('<12f', *sum(native_palette, []))
        if palette_bytes != vp[50*16:53*16]:
            continue
        row = body[0xaf]
        _, scenario = cache.tag(cache.scenario, 'scnr')
        netgame = cache.block(scenario + 0x120, 0x90)
        assert row < len(netgame)
        record = netgame[row]
        collection = cache.u32(record + 0x5c)
        collection_tag, collection_root = cache.tag(collection, 'itmc')
        item_definitions = [cache.u32(p + 8) for p in cache.block(collection_root, 16)]
        assert datum in item_definitions
        owner_matches.append(dict(handle=owner['handle'], definition=owner['definition'],
            model=f'{hlmt:08X}', render_model=f'{mode:08X}', raw_body_sha256=sha(body_path),
            captured_palette_words=words(vp[50*16:53*16]), raw_node0_words=words(body[owner['world_offset']:owner['world_offset']+52]),
            full_consumed_palette_word_matches=12, parent=owner['parent'],
            item_owner=f'{struct.unpack_from("<I",body,0x154)[0]:08X}',
            actual_runtime_netgame_row=row, created_tick=struct.unpack_from('<I', body, 0x150)[0],
            collection=f'{collection:08X}', collection_name=collection_tag['name'],
            collection_item_definitions=[f'{d:08X}' for d in item_definitions],
            authored_row_hex=bytes(cache.data[record:record+0x90]).hex()))
    assert len(owner_matches) == 1 and owner_matches[0]['handle'] == 'E2F70088'
    shader_key = cache.shader(0xe2da0165)
    shader = cache.shaders[shader_key]
    pp = shader['postprocess'][0]
    textures = []
    for bitmap in pp['bitmaps']:
        key = cache.bitmap(bitmap['datum'])
        textures.append(dict(datum=f"{bitmap['datum']:08X}", **cache.bitmaps[key]))
    fragment = (source / '8787-fragment-block0.bin').read_bytes()
    inputs = {str(p.relative_to(ROOT)): sha(p) for p in sorted(source.glob('8787-*')) if p.is_file()}
    inputs[str((snapshot / 'export.json').relative_to(ROOT))] = sha(snapshot / 'export.json')
    provenance = json.loads((ROOT / 'analysis/multiplayer-shading/original-xemu/zanzibar-world-interval20-provenance.json').read_text())
    report = dict(passed=True, event=8787, capture_sha256=provenance['rdc_sha256'],
        cache_sha256=sha(cache.path), tool_sha256=sha(Path(__file__)), source_files=inputs,
        identity=dict(render_model=f"{model['datum']:08X}", name=model['name'], section=0, part=0,
            shader='E2DA0165', shader_name=shader['name'], matched_indices=32,
            negative_other_parts=len(parts_report)-1, captured_vertex_count=22),
        all_authored_model_parts=parts_report, compression_words=words(bytes(cache.data[limits_at:limits_at+40])),
        captured_c170_c172_words=words(vp[170*16:173*16]), matched_compression_words=12,
        first_person_compression_negative_control=True, owner_corroboration=owner_matches,
        authored_shader=shader, authored_template=cache.templates[pp['template']], authored_bitmaps=textures,
        captured_ps_constants={str(i):words(fragment[544+i*16:560+i*16]) for i in range(17)},
        snapshot_clock=exported['clock_tick'], snapshot_label=exported['snapshot'],
        provenance_boundary='Object snapshot is earlier independent baseline12474, not original20 capture interval. Its exact definition->hlmt->mode identity, runtime netgame row5 and all12 consumed root palette words independently corroborate this draw. No owner is selected by position/proximity.',
        remaining_boundaries=['No preVS raw vertex buffer was exported; input decompression bank/index identity is proved, not full raw-input equivalence.',
            'Bound texture dimensions/mips are retained, but authored BC blocks versus host-decoded original texture samples are not byte-compared here.',
            'Captured animated pixel tint is independent of authored default colors; source object-function/time producer remains required.',
            'Native multiplayer currently has no world/netgame weapon allocation/pose/render submission; this is not a shader-selection-only discrepancy.'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'Original8787: energy sword top section0/part0;32 indices+12 compression words exact;{len(parts_report)-1} other parts rejected; owner E2F70088 row5 corroborated by12 palette words.')


if __name__ == '__main__':
    main()
