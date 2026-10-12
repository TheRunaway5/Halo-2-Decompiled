#!/usr/bin/env python3
"""Read weather draw dependencies from an immutable original RAM export.

No guest, display, QMP, replay controller or GPU is accessed. A saved bank is
not asserted to be the input of a weather draw; render-view cleanup is recorded.
The executable bytes and jump-table entries are independent source authority.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from capstone import Cs, CS_ARCH_X86, CS_MODE_32


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def float_field(raw, offset):
    return dict(word=f'{uint(raw, offset):08X}', value=struct.unpack_from('<f', raw, offset)[0])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.snapshot = args.snapshot.resolve()
    args.output = args.output.resolve()
    snapshot = Snapshot(args.snapshot)
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN
    exports = args.output.parent / (args.output.stem + '-inputs')
    exports.mkdir(parents=True, exist_ok=True)
    raw_banks = []

    def bank(name, address, size):
        raw = snapshot.read(address, size)
        path = exports / (name + '.bin')
        path.write_bytes(raw)
        raw_banks.append(dict(name=name, address=f'{address:08X}', bytes=size,
            sha256=sha(raw), file=str(path.relative_to(ROOT))))
        return raw

    bsp = uint(snapshot.read(0x4e0348, 4))
    count, palette = words(snapshot.read(bsp + 0x84, 8))
    assert count <= 16
    table = uint(snapshot.read(0x4e3b44, 4))
    view = bank('retained-view-485600', 0x485600, 244)
    globals_bank = bank('retained-render-globals-485890', 0x485890, 0x300)
    visibility = bank('retained-visibility-547F88', 0x547f88, 0x2ae0)
    queue = uint(visibility, 12)
    queue_header = bank('retained-visible-queue', queue, 24)
    visible_count = struct.unpack_from('<h', queue_header, 4)[0]
    assert 0 <= visible_count <= 512
    cluster_raw = bank('retained-visible-clusters', uint(queue_header, 12), visible_count * 4) if visible_count else b''
    clocks = bank('retained-weather-clock-509418', 0x509418, 24)
    rng_address = uint(snapshot.read(0x4e7408, 4))
    rng = bank('retained-primary-auxiliary-streams', rng_address, 8)
    stage_binding = bank('retained-stage-bindings-51F3D8', 0x51f3d8, 16)
    stage_dimensions = bank('retained-stage-dimensions-485AF4', 0x485af4, 32)
    stage_states = bank('retained-stage-state-404DD0', 0x404dd0, 4 * 128)
    depth_address = uint(snapshot.read(0x509368, 4))
    depth = bank('retained-depth-surface-509368', depth_address, 32) if depth_address else b''
    original_depth_address = uint(snapshot.read(0x509364, 4))
    original_depth = bank('retained-original-depth-surface-509364', original_depth_address, 24) if original_depth_address else b''
    target3 = bank('retained-raster-target3-row', 0x4b4b58 + 3 * 152, 152)
    records = []
    for ordinal in range(count):
        pal = bank(f'palette-{ordinal}', palette + ordinal * 0x88, 0x88)
        datum = uint(pal, 0x24)
        if datum == 0xffffffff:
            records.append(dict(ordinal=ordinal, datum='NONE'))
            continue
        tag_row = bank(f'weather-{ordinal}-tag-row', table + (datum & 65535) * 16, 16)
        root_address = uint(tag_row, 8)
        root = bank(f'weather-{ordinal}-root', root_address, 176)
        particles, particle_address, plates, plate_address = words(root[:16])
        assert particles <= 64 and plates <= 64
        entry = dict(ordinal=ordinal, datum=f'{datum:08X}', root=f'{root_address:08X}',
            particle_count=particles, particle_pointer=f'{particle_address:08X}',
            plate_count=plates, plate_pointer=f'{plate_address:08X}', plates=[])
        for index in range(plates):
            address = plate_address + index * 936
            raw = bank(f'weather-{ordinal}-plate-{index}', address, 936)
            bitmaps = []
            for layer in range(3):
                bitmap = uint(raw, layer * 8 + 4)
                bitmap_row = bank(f'weather-{ordinal}-plate-{index}-bitmap-{layer}-tag-row',
                    table + (bitmap & 65535) * 16, 16)
                definition = uint(bitmap_row, 8)
                metadata = bank(f'weather-{ordinal}-plate-{index}-bitmap-{layer}-definition', definition, 84)
                bitmaps.append(dict(stage=layer + 1, datum=f'{bitmap:08X}', definition=f'{definition:08X}',
                    image_count=uint(metadata, 0x44), image_pointer=f'{uint(metadata, 0x48):08X}'))
            views = []
            for slot in range(4):
                base = slot * 184
                fields = {f'+{offset:X}':float_field(raw, base + offset) for offset in
                    [0xec, 0xf0, 0xf4, 0x11c, 0x120, 0x124, 0x128, 0x12c, 0x130,
                     0x134, 0x138, 0x13c, 0x148, 0x14c]}
                views.append(dict(slot=slot, fields=fields))
            entry['plates'].append(dict(ordinal=index, address=f'{address:08X}',
                texture_scale=[float_field(raw, 0x48 + layer * 4) for layer in range(3)],
                depth_blend=[float_field(raw, 0x80), float_field(raw, 0x84)],
                tint0=[float_field(raw, 0x98 + axis * 4) for axis in range(3)],
                bitmaps=bitmaps, views=views))
        records.append(entry)

    sizes = {}
    with (ROOT / 'tools/reference/halo2-decompiled/config/functions.csv').open() as source:
        for row in csv.reader(source):
            if len(row) >= 2:
                try:
                    sizes[int(row[0], 16)] = int(row[1])
                except ValueError:
                    pass
    disassembler = Cs(CS_ARCH_X86, CS_MODE_32)
    routines = []
    for address in [0x36b50, 0x37a60, 0x391f0, 0x39780, 0x3a5c0, 0x143c0,
                    0x14bc0, 0x14f60, 0x14600, 0x1dd60, 0x12fa0, 0x3fb280, 0x3fb2d0]:
        size = sizes[address]
        raw = image.read(address, size)
        instructions = list(disassembler.disasm(raw, address))
        path = exports / f'source-{address:08X}-disassembly.txt'
        path.write_text('\n'.join(f'{i.address:08X} {i.mnemonic} {i.op_str}' for i in instructions) + '\n')
        routines.append(dict(address=f'{address:08X}', bytes=size, sha256=sha(raw),
            instructions=len(instructions), disassembly=str(path.relative_to(ROOT)),
            calls=[dict(pc=f'{i.address:08X}', target=i.op_str) for i in instructions if i.mnemonic == 'call']))

    index = image.read(0x15154 + 3, 1)[0]
    texture_target = uint(image.read(0x15114 + index * 4, 4))
    assert texture_target == 0x15020
    report = dict(scope=__doc__, snapshot=str(args.snapshot.relative_to(ROOT)),
        snapshot_manifest_sha256=sha((args.snapshot / 'export.json').read_bytes()),
        snapshot_ram_sha256=sha(snapshot.ram), original_executable_sha256=PIN,
        tool_sha256=sha(Path(__file__).read_bytes()),
        paused_before=snapshot.report['qmp_before'], paused_after=snapshot.report['qmp_after'],
        current_view=struct.unpack_from('<h', view)[0], current_mode=struct.unpack_from('<h', view, 2)[0],
        current_visibility_cluster=struct.unpack_from('<i', visibility, 8)[0],
        retained_visible_clusters=words(cluster_raw),
        render_time_double_wordpair=[f'{v:08X}' for v in words(globals_bank[16:24])],
        weather_elapsed=float_field(clocks, 0), weather_last_time=float_field(clocks, 4),
        weather_controls=[f'{v:08X}' for v in words(clocks[8:20])],
        rng=dict(address=f'{rng_address:08X}', primary=f'{uint(rng):08X}', auxiliary=f'{uint(rng,4):08X}'),
        stage_texture_words=[f'{v:08X}' for v in words(stage_binding)],
        stage_dimensions=[f'{v:08X}' for v in words(stage_dimensions)],
        plate_stage0_binding=dict(source='37F14: EDI=3, EBX=0 ->14F60; literal15154/15114 selects15020',
            jump_table_index=index, target=f'{texture_target:08X}', pointer_global='00509368',
            retained_depth_surface=f'{depth_address:08X}', retained_surface_words=[f'{v:08X}' for v in words(depth)],
            original_depth_surface=f'{original_depth_address:08X}', original_surface_words=[f'{v:08X}' for v in words(original_depth)],
            alias_producer='14600 constructs20-byte textureheader:Common40001,Data=509364surface+4,Lock0,Format11229,Size271DF27F. Source bitmap linearformat11 table also mapsNVformat12; the original depth header here usesNVformat2E.',
            target3_row_sha256=sha(target3), dimensions_authority='1DD60 reads valid target3 row, otherwise its literal target3 fallback is640x480; retained row is not a weather entry.'),
        weather=records, routines=routines, raw_banks=raw_banks,
        limitations=['The manifest PC determines whether these banks are actual36B50 entry inputs or post-view retained data; the tool does not promote them to37A60/391F0 entry inputs.',
            'Retained visible queue, initialized plate banks and weather clock have no newly inferred same-draw chronology.',
            'Stage1..3 authored bitmap identities are retained. Actual143C0/12310/12D5A0 residency/binding chronology is not substituted.',
            'This tool reads/export bytes and disassembles unchanged instructions; it executes no original draw routine and claims no raster appearance.'],
        guest_io=False, GPU=False, production_activation=False)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k:report[k] for k in ['snapshot_ram_sha256','current_view','current_visibility_cluster',
        'retained_visible_clusters','weather_elapsed','weather_last_time','rng','plate_stage0_binding']}, indent=2))


if __name__ == '__main__':
    main()
