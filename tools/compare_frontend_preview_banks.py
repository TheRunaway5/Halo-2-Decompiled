#!/usr/bin/env python3
"""Compare frozen native preparation with the exact retained original pass.

Checks program words and registers read by the selected original vertex
program. Per-vertex attributes0..9 are excluded: the retained global bank is
not a capture of the geometry stream. Register-level differences may include
components that the final output does not consume, and are not pixel proof.
"""
from pathlib import Path
import argparse, json, struct, sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from audit_rendering import Cache
ROOT = Path(__file__).resolve().parents[1]
D = ROOT / 'analysis/frontend/customization'
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--arms-file',default='native-arms-material-bank.jsonl')
parser.add_argument('--output',default='preview-plasma-arms-consumed-bank-comparison.json')
args=parser.parse_args()
c = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
original = json.loads((D / 'original-preview-shader-banks-elite.json').read_text())
files = {'e325017a': args.arms_file,
         'e3c001c4': 'native-plasma-lod-material-bank.jsonl',
         'e3c401c8': 'native-plasma-luminous-material-bank.jsonl'}
def words(data):
    return struct.unpack('<' + str(len(data) // 4) + 'I', data)
def f32(word):
    return struct.unpack('<f', struct.pack('<I', word))[0]
report = []
for row in original['rows']:
    if row['shader'] not in files:
        continue
    shader = int(row['shader'], 16)
    _, sd = c.tag(shader, 'shad'); sb = c.block(sd + 32, 124)[0]
    _, td = c.tag(c.u32(sb), 'stem'); tb = c.block(td + 88, 40)[0]
    header = words(bytes.fromhex(row['state_header_hex']))
    lod = next(i for i, p in enumerate(c.block(tb, 10)) if p + c.magic == header[5])
    pi = next(i for i, p in enumerate(c.block(tb + 16, 10)) if p + c.magic == header[6])
    p = c.block(tb + 16, 10)[pi]
    _, pd = c.tag(c.u32(p + 4), 'spas')
    pp = next(pp for pp in c.block(pd + 28, 88) if pp + c.magic == int(row['postprocess'], 16))
    ii = (int(row['implementation'], 16) - c.u32(pp + 4)) // 306
    native = next(n for n in map(json.loads, (D / files[row['shader']]).read_text().splitlines())
        if n['lod'] == lod and n['template_pass'] == pi and n['implementation'] == ii)
    differences = []
    for kind, values, expected in [
        ('program', native['program'], words(bytes.fromhex(row['pixel_program_hex']))),
        ('attributes', sum(native['attributes'], []), words(bytes.fromhex(row['vertex_attributes_hex']))),
        ('constants', sum(native['constants'], []), words(bytes.fromhex(row['vertex_constants_hex'])))]:
        for i, (a, b) in enumerate(zip(values, expected)):
            if kind == 'attributes' and (i // 4 < 10 or i // 4 not in native['read_attributes']):
                continue
            if kind == 'constants' and i // 4 + 18 not in native['read_constants']:
                continue
            if a != b:
                differences.append(dict(bank=kind, word=i,
                    register=i // 4 + (18 if kind == 'constants' else 0),
                    native=f'{a:08x}', original=f'{b:08x}',
                    native_float=f32(a) if kind != 'program' else None,
                    original_float=f32(b) if kind != 'program' else None))
    record = dict(shader=row['shader'], name=c.tags[shader]['name'], source_lod=lod,
        template_pass=pi, implementation=ii, read_attributes=native['read_attributes'],
        read_constants=native['read_constants'], differences=differences)
    report.append(record)
(D / args.output).write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
