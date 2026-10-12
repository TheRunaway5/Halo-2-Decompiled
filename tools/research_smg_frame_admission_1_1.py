#!/usr/bin/env python3
"""Pin original frame lifecycle writers, caller tables and virtual data scope."""
import csv
import hashlib
import json
from pathlib import Path
import struct
import sys
from capstone.x86 import X86_OP_MEM
ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from functions import _Code, _trace
from oracle_smg_effect_owner_1_1 import sha, save, encoded
OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/smg-effect-frame-admission/static'
ENTRIES = {0x12cbc0, 0x190240, 0x1902e0, 0x1904d0, 0x1905b0, 0x190650, 0x190690, 0x190730, 0x1907e0, 0x190890, 0x190a30, 0x190b30, 0x19e070, 0x124040, 0x12d950, 0x147dd0, 0x68300, 0x68460, 0x139640}


def main():
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest() == 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
    starts = {int(row['address'], 16) for row in csv.DictReader((ROOT / 'analysis/map-pack-update/functions.tsv').open(), delimiter='\t')}
    code = _Code(image, '.text')
    listings, references = [], []
    for entry in sorted(ENTRIES):
        flow = _trace(code, entry, starts | ENTRIES)
        instructions = [code.at(address) for address in sorted(flow.code)]
        for instruction in instructions:
            assert instruction.bytes == image.read(instruction.address, instruction.size)
            for operand in instruction.operands:
                if operand.type == X86_OP_MEM and operand.mem.disp == 0x4f4864 and operand.mem.base == operand.mem.index == 0:
                    references.append(dict(owner=f'{entry:08x}', address=f'{instruction.address:08x}', bytes_hex=instruction.bytes.hex(), mnemonic=instruction.mnemonic, operands=instruction.op_str, operand_access=operand.access, scope='Literal absolute operand in independently traced original owner; static, not executed coverage.'))
        path = OUT / 'instructions' / f'{entry:08x}.asm'
        save(path, ''.join(f'{instruction.address:08x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n' for instruction in instructions).encode())
        listings.append(dict(entry=f'{entry:08x}', file=str(path.relative_to(OUT)), sha256=sha(path), rows=len(instructions), direct_calls=[dict(site=f'{site:08x}', target=f'{target:08x}') for site, target, kind in flow.sites if kind == 'call']))
    section = next(section for section in image.sections if section.va <= 0x4f4864 < section.va + section.vsize)
    tables = [dict(address=f'{address:08x}', raw_hex=image.read(address, count * 4).hex(), targets=[f'{value:08x}' for value in struct.unpack('<' + 'I' * count, image.read(address, count * 4))]) for address, count in [(0x190598, 5), (0x190638, 6)]]
    report = dict(schema='source-smg-frame-admission-static-v1', tool_sha256=sha(__file__), source_xbe_sha256=hashlib.sha256(image.data).hexdigest(), routines=listings, literal_global_references=references, tables=tables, image_global_scope=dict(address='004f4864', section=section.name, section_va=f'{section.va:08x}', virtual_size=section.vsize, raw_size=section.rsize, delta=0x4f4864 - section.va, within_raw_file_bytes=0x4f4864 - section.va + 4 <= section.rsize, reader_zero_filled4_hex=image.read(0x4f4864, 4).hex(), scope='Virtual data tail beyond raw section bytes; zero observed in prepared mapped backing. This is not a recovered whole-title initializer or Xbox loader execution.'), previous_initialization_evidence=['docs/MP_LOCAL_COMMAND_SCHEDULING_RESEARCH.md', 'docs/MP_LOCAL_MAP_CLOCK_RESEARCH.md', 'docs/SMG_AUDIO_STARTUP_1_1_RESEARCH.md', 'docs/MP_GRENADE_ACTION_SOURCE_RESEARCH.md'], scope='Original bytes own lifecycle writes and request/elapsed gates. No constant state, AL, readiness, cached result, device admission or elapsed is executed/supplied by this static report.')
    save(OUT / 'report.json', encoded(report))
    print(json.dumps(dict(output=str(OUT / 'report.json'), sha256=sha(OUT / 'report.json'), routines=len(listings), global_sites=len(references))))


if __name__ == '__main__':
    main()
