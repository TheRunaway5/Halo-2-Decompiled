#!/usr/bin/env python3
"""Export authenticated original DLC map-preview descriptors, without pixels.

The source converter copies207 rows of220 DWORDs from a version3 LVL record
into an existing map-preview bitmap's actual pitch. Its selected mainmenu
resource supplies format11, one image and no sequences. Resource slot ordinals
depend on existing map registration and are deliberately not inferred here.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from audit_map_pack import level_catalog

ROOT = Path(__file__).resolve().parents[1]
SOURCE_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MENU_PIN = '1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
DEFAULT_ROOT = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/c')
CATALOG_PINS = {
    '4d53006400000001/patch_v3.lvl': '79aa649a0e9ea9f56d407ce02d6e8b5f08cf0549e33691b30051f30cc163d739',
    '4d53006400000002/patch_v3.lvl': '5ea88ad33dd6429fe633ade7adeea4d128fcc3516358ceea6fa2955d332f1a1d',
    '4d53006400000003/patch_v3.lvl': 'b4518faa1d892fbf17dde710881f57e7ea36f69883f1e430a8c476fc920963c7',
    '4d53006400000004/patch_v3.lvl': '65de3141c42c5d482a44683ffe0b59b2bd0cf305fdb37e4421e330dca1492417',
    '4d53006400000004old/patch_v3.lvl': 'd775b7825b218cd0aeabe1f5654d0423bdb17ed72b7fa64849b3981ede6d71a3',
}


def export_dlc_map_preview_descriptors(root, mainmenu=ROOT / 'assets/disc/maps/mainmenu.map'):
    """Return scenario-keyed source descriptors and exact LVL/CRC provenance.

    No original bitmap pixels, executable bytes, registration side effects or
    MCC scale assumption are emitted. Missing/changed original owners reject.
    """
    from audit_rendering import Cache
    root, mainmenu = Path(root), Path(mainmenu)
    assert hashlib.sha256(mainmenu.read_bytes()).hexdigest() == MENU_PIN
    cache = Cache(mainmenu)
    slots = []
    try:
        _, globals_at = cache.tag(next(t['datum'] for t in cache.tags.values() if t['group'] == 'matg'), 'matg')
        levels = cache.block(globals_at + 0x178, 24)
        for at in cache.block(levels[0] + 16, 0xc64):
            datum = cache.u32(at + 8)
            if datum == 0xffffffff:
                continue
            _, bitmap = cache.tag(datum, 'bitm')
            frames, sequences = cache.block(bitmap + 68, 116), cache.block(bitmap + 60, 60)
            assert len(frames) == 1 and not sequences
            frame = frames[0]
            shape = dict(width=cache.u16(frame + 4), height=cache.u16(frame + 6), format=cache.u16(frame + 12))
            assert shape == dict(width=220, height=207, format=11)
            if cache.u32(at) == 0xffffffff:
                slots.append(cache.tags[datum]['name'])
        assert len(slots) == 38
    finally:
        cache.close()
    result = {}
    paths = sorted(root.glob('*/patch_v3.lvl'))
    assert paths, 'No original version3 DLC map catalogs supplied'
    for path in paths:
        catalog_pin = hashlib.sha256(path.read_bytes()).hexdigest()
        assert CATALOG_PINS.get(str(path.relative_to(root))) == catalog_pin, 'Unidentified or changed original DLC catalog'
        for record in level_catalog(path):
            assert record['version'] == 3
            image = dict(frames=[dict(width=220, height=207, format=11)], sequences=[])
            provenance = dict(catalog=str(path.relative_to(root)), catalog_sha256=catalog_pin,
                              version=record['version'], index=record['index'],
                              record_offset=record['offset'], record_bytes=0x2d400,
                              checksum=record['checksum'], crc_verified=True,
                              preview=record['preview'])
            row = dict(map_id=record['map_id'], image=image, catalog_records=[provenance],
                       source_xbe_sha256=SOURCE_PIN, source_mainmenu_sha256=MENU_PIN,
                       source_converter='0x1A5590', source_copy_corridor=['0x1A56BD', '0x1A56F3'],
                       source_map_bitmap_provider='0x25842D frame0 -> original bitmap descriptor',
                       source_resource_slots=slots)
            scenario = record['scenario']
            if scenario in result:
                assert result[scenario]['map_id'] == row['map_id'] and result[scenario]['image'] == row['image']
                result[scenario]['catalog_records'].append(provenance)
            else:
                result[scenario] = row
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=DEFAULT_ROOT)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    descriptors = export_dlc_map_preview_descriptors(args.root)
    from oracle_tu15_team_balance import Xbe, NEW_DEFAULT
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EIP, UC_X86_REG_ESP
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == SOURCE_PIN
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0x10000, 0x800000)
    for section in image.sections:
        machine.mem_write(section.va, image.section_bytes(section))
    arena, destination, stack = 0x1100000, 0x1140000, 0x11ff000
    machine.mem_map(arena, 0x100000)
    executed = set()
    machine.hook_add(UC_HOOK_CODE, lambda _, address, size, user: executed.add(address))
    controls = []
    for scenario, descriptor in descriptors.items():
        for source in descriptor['catalog_records']:
            data = (args.root / source['catalog']).read_bytes()
            record = data[source['record_offset']:source['record_offset'] + source['record_bytes']]
            original = record[0xc70:]
            assert len(original) == 220 * 207 * 4
            for pitch in (880, 1024):
                machine.mem_write(arena, record)
                machine.mem_write(destination, b'\xcd' * (pitch * 207))
                machine.mem_write(stack + 0x18, struct.pack('<I', pitch))
                for register, value in ((UC_X86_REG_EAX, destination), (UC_X86_REG_EBP, arena), (UC_X86_REG_ESP, stack)):
                    machine.reg_write(register, value)
                machine.emu_start(0x1a56bd, 0x1a56f3, count=1000000)
                assert machine.reg_read(UC_X86_REG_EIP) == 0x1a56f3
                output = bytes(machine.mem_read(destination, pitch * 207))
                for row in range(207):
                    assert output[row*pitch:row*pitch+880] == original[row*880:(row+1)*880]
                    assert output[row*pitch+880:(row+1)*pitch] == b'\xcd' * (pitch-880)
                controls.append(dict(scenario=scenario, catalog=source['catalog'], index=source['index'], pitch=pitch,
                                     source_preview_sha256=hashlib.sha256(original).hexdigest(), copied_unchanged=True,
                                     destination_padding_unchanged=True))
    report = dict(passed=True, scope=__doc__, descriptors=descriptors, original_row_copy_controls=controls,
                  original_instruction_addresses=sorted(executed), source_sha256=SOURCE_PIN,
                  no_instruction_patches=True, no_replaced_callees=True,
                  pixels_retained=False, scratch=str(scratch), scratch_retained=False,
                  oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {len(descriptors)} DLC scenario descriptors, {len(controls)} unchanged original220x207 row-copy controls; no pixels retained')


if __name__ == '__main__':
    main()
