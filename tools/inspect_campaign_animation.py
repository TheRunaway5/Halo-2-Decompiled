#!/usr/bin/env python3
"""Extract retail script signatures and instruction evidence; no production edits."""
from pathlib import Path
import csv, hashlib, json, struct, xml.etree.ElementTree as ET
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

root = Path(__file__).resolve().parents[1]
blob = (root / 'assets/disc/default.xbe').read_bytes()
expected = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
assert hashlib.sha256(blob).hexdigest() == expected
base = struct.unpack_from('<I', blob, 0x104)[0]
count, table = struct.unpack_from('<II', blob, 0x11c)
sections = [struct.unpack_from('<5I', blob, table - base + i * 56)[1:] for i in range(count)]

def read(address, size):
    for start, _, offset, raw_size in sections:
        if start <= address < start + raw_size:
            assert address + size <= start + raw_size
            return blob[offset + address - start:offset + address - start + size]
    raise ValueError(hex(address))

with (root / 'tools/reference/halo2-decompiled/config/functions.csv').open() as file:
    records = {int(row['va'], 16): row for row in csv.DictReader(file)}
cs = Cs(CS_ARCH_X86, CS_MODE_32)

def dump(address):
    row = records[address]
    size = int(row['size'])
    path = root / f'analysis/campaign/animation-playback-{address:x}.txt'
    path.write_text(f'XBE SHA256 {expected}\nRetail {address:08x}, {size} bytes, manifest {row["status"]}\n' + ''.join(
        f'{instruction.address:08x} {instruction.bytes.hex():<20} {instruction.mnemonic} {instruction.op_str}\n'
        for instruction in cs.disasm(read(address, size), address)))

xml = ET.parse(root / 'tools/reference/assembly/src/Blamite/Formats/Halo2Xbox/H2X_Scripting.xml')
names = {int(function.attrib['opcode'], 16): function.attrib['name'] for function in xml.iter('function') if 'opcode' in function.attrib}
definitions = []
for opcode in [*range(0xb8, 0xbf), *range(0xc7, 0xd2)]:
    pointer = struct.unpack('<I', read(0x4744e0 + opcode * 4, 4))[0]
    ret, flags, evaluate, parse, argc = struct.unpack('<HHIIH', read(pointer, 14))
    definitions.append(dict(opcode=f'{opcode:03x}', name=names[opcode], definition=f'{pointer:08x}',
        return_type=ret, flags=flags, evaluator=f'{evaluate:08x}', parse=f'{parse:08x}',
        argument_types=list(struct.unpack('<' + 'H' * argc, read(pointer + 14, argc * 2)))))
    dump(evaluate)
for address in [0x10a660, 0x10a390, 0x10a460, 0x10a3f0, 0x11b520, 0x11b4a0, 0x11b6b0, 0x11b710,
        0x1101e0, 0x111650, 0x112dc0, 0x13bff0, 0x13ca80, 0x1c6340, 0x1c6440, 0x1c6470,
        0x1c6500, 0x1c6560, 0x1c6620, 0x1c66a0, 0x1c68c0, 0x1c69b0, 0x1c6bb0, 0x1c6ea0,
        0x1c6f60, 0x1c7450, 0x1cb5b0, 0x1ccb40, 0x1ccb80, 0x1daea0, 0x2798a0, 0x142a60,
        0x10f690, 0xb8bd0, 0xb8890]:
    if address in records:
        dump(address)
result = dict(xbe_sha256=expected, builtin_table='004744e0', functions=definitions,
    callbacks=dict(unit=dict(table='004679b0', matrix_slot='00467a20', callback='00112dc0'),
                   scenery=dict(table='00467ff0', matrix_slot='00468060', callback='0010a390')))
(root / 'analysis/campaign/animation-playback-retail.json').write_text(json.dumps(result, indent=2) + '\n')
print(f'Extracted {len(definitions)} script signatures and instruction dumps')
