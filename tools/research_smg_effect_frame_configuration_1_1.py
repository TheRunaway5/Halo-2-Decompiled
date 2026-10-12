#!/usr/bin/env python3
"""Pin configuration, memory-class callbacks and the actual DSP record owner."""
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
OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/smg-effect-frame-configuration/static'
ENTRIES = {0x663b0, 0x663a0, 0x663f0, 0x82190, 0x531a0, 0x53300, 0x55770, 0x55800, 0x190930, 0x192d30, 0x2dbda1, 0x2dbdbc}


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
    raw = image.read(0x45ad78, 0x40)
    values = struct.unpack('<16I', raw)
    callbacks = [dict(index=index, dispose=f'{values[2*index]:08x}', initialize=f'{values[2*index+1]:08x}') for index in range(8)]
    section = next(section for section in image.sections if section.name == 'DSPImage')
    dsp = image.read(section.va, section.rsize)
    report = dict(schema='source-smg-effect-frame-configuration-static-v1', tool_sha256=sha(__file__), source_xbe_sha256=hashlib.sha256(image.data).hexdigest(), routines=routines,
                  memory_class_callback_table=dict(address='0045ad78', raw_hex=raw.hex(), stride=8, callbacks=callbacks),
                  dsp_section=dict(name=section.name, original_virtual_address=f'{section.va:08x}', virtual_size=section.vsize, raw_size=section.rsize, raw_sha256=hashlib.sha256(dsp).hexdigest(), scope='Actual original XBE section bytes and name; section-loader/DSP/device initialization not executed by this static exporter.'),
                  original_name=dict(address='0045aea0', raw_hex=image.read(0x45aea0, 9).hex()),
                  capacity_stores=[dict(instruction='00066f21', original_bytes_hex=image.read(0x66f21, 10).hex(), address='004d6d24', word='00100000'), dict(instruction='00066f2b', original_bytes_hex=image.read(0x66f2b, 10).hex(), address='004d6d28', word='00010000')],
                  dsp_record_writer=dict(instruction='00192dbe', original_bytes_hex=image.read(0x192dbe, 6).hex(), address='00518480', value_register='esi', scope='Listed only: requires real original section-load path before binding actual48-byte arena record. No record/default device/result supplied.'),
                  scope='Complete unchanged original instruction bodies and table/section bytes. Static DSP ownership complements actual NULL-read stop; no positive audio, cache loader, frame-state writer or aging execution claim.')
    save(args.output, encoded(report))
    print(dict(output=str(args.output), sha256=sha(args.output), routines=len(routines)))


if __name__ == '__main__':
    main()
