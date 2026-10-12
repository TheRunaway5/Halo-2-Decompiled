#!/usr/bin/env python3
"""Pin original DSP loader/arena/device owners and actual XBE section inputs."""
import argparse
import csv
import hashlib
import struct
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from functions import _Code, _trace
from xbe import Xbe
from oracle_smg_effect_owner_1_1 import sha, save, encoded
OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/static'
ENTRIES = {0x12000, 0x12080, 0x126b20, 0x192d30, 0x192e50, 0x2205c0, 0x2dbcfb, 0x2dbd62, 0x2dbd83, 0x2dbda1, 0x2dbdbc, 0x2dc2d8, 0x2dc9c7, 0x33785c, 0x381ead}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT / 'report.json')
    args = parser.parse_args()
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest() == 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
    starts = {int(row['address'], 16) for row in csv.DictReader((ROOT / 'analysis/map-pack-update/functions.tsv').open(), delimiter='\t')} | ENTRIES
    code = _Code(image, '.text')
    routines = []
    for entry in sorted(ENTRIES):
        flow = _trace(code, entry, starts)
        instructions = [code.at(address) for address in sorted(flow.code)]
        for instruction in instructions:
            assert image.read(instruction.address, instruction.size) == instruction.bytes
        path = args.output.parent / 'instructions' / f'{entry:08x}.asm'
        save(path, ''.join(f'{instruction.address:08x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n' for instruction in instructions).encode())
        routines.append(dict(entry=f'{entry:08x}', file=str(path.relative_to(args.output.parent)), rows=len(instructions), sha256=sha(path)))
    count, pointer = struct.unpack_from('<II', image.data, 0x11c)
    table = image.data[pointer - image.base:pointer - image.base + count * 0x38]
    sections = []
    for index, section in enumerate(image.sections):
        address = pointer + index * 0x38
        raw = table[index * 0x38:(index + 1) * 0x38]
        words = struct.unpack('<9I', raw[:0x24])
        assert tuple(words[:6]) == (section.flags, section.va, section.vsize, section.raw, section.rsize, struct.unpack_from('<I', image.data, address - image.base + 0x14)[0])
        sections.append(dict(index=index, name=section.name, header_pointer=f'{address:08x}', header38_hex=raw.hex(), virtual_address=f'{section.va:08x}', virtual_size=section.vsize, raw_file_offset=section.raw, raw_file_size=section.rsize, raw_section_sha256=sha_bytes(image.section_bytes(section)), original_reference_count=words[6], head_reference_pointer=f'{words[7]:08x}', tail_reference_pointer=f'{words[8]:08x}', head_reference4_hex=image.data[words[7] - image.base:words[7] - image.base + 4].hex(), tail_reference4_hex=image.data[words[8] - image.base:words[8] - image.base + 4].hex()))
    original_name = image.read(0x45aea0, 9)
    assert original_name == b'DSPImage\0'
    primary = ROOT / 'analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/provider/primary-source'
    files = {str(path.relative_to(ROOT)): sha(path) for path in [primary / 'EmuKrnlXe.cpp', primary / 'Xbe.cpp', ROOT / 'analysis/multiplayer/integration/spawn-source/local-map-protection/primary-source/VMManager.cpp', ROOT / 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/Cxbx-Reloaded-EmuKrnlRtl.cpp', ROOT / 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h']}
    report = dict(schema='source-smg-effect-dsp-owner-static-v1', tool_sha256=sha(__file__), source_xbe_sha256=sha_bytes(image.data), routines=routines, primary_source_files=files,
                  original_header2000_sha256=sha_bytes(image.data[:0x2000]), section_count=count, section_table_pointer=f'{pointer:08x}', section_table_stride=0x38, section_table_hex=table.hex(), sections_in_original_header_order=sections,
                  authored_section_loader_call=dict(argument_push='00192d39', original_argument_bytes_hex=image.read(0x192d39, 5).hex(), caller='00192d40', original_call_bytes_hex=image.read(0x192d40, 5).hex(), name_pointer='0045aea0', name9_hex=original_name.hex()),
                  source_arena_owner=dict(entry='00012000', alloc_call='00012015', output_store='0001201a', cursor_store='0001201f', original_request=[0, 0x23000, 0x1000, 4], source_bytes_hex=image.read(0x12008, 0x1d).hex(), scope='Listed original whole title constructor; includes earlier2DC2D8 disk/cache work. Successful OS allocation and complete constructor return are not inferred.'),
                  source_dsp_record_owner=dict(entry='00192d30', alloc_size=0x48, pointer_store='00192dbe', download_call='00192dd4', source_pointer_store_bytes_hex=image.read(0x192dbe, 6).hex(), device_provider='00381ead', scope='Natural caller220720 requires real original DirectSound device/caps/policy first. This pointer/record is not supplied.'),
                  scope='Literal original headers, authored names, original routines and pinned primary provider bodies; dynamic executed coverage is retained separately. No loader referencecount, OS memory, audio device, DSP record, game admission or elapsed value is inferred from these static bytes.')
    save(args.output, encoded(report))
    print(dict(output=str(args.output), sha256=sha(args.output), routines=len(routines), sections=len(sections)))


def sha_bytes(raw):
    return hashlib.sha256(raw).hexdigest()


if __name__ == '__main__':
    main()
