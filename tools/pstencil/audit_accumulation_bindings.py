#!/usr/bin/env python3
"""Retain primary-checked resident accumulation bindings and implementation rows.

These are original resident resources, not an observed original frame. Every
decoded source field is checked against the pinned cache before summarizing it.
The actual per-frame implementation selector and target contents remain open.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from oracle_smg_firing_1_1 import Xbe


def field(row, name):
    return next(f for f in row['fields'] if f['name'] == name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise RuntimeError('Refusing to replace retained reference evidence')
    base = ROOT / 'analysis/pstencil/2003-presentation'
    spec = importlib.util.spec_from_file_location('resident', base / 'extract_resident_environment.py')
    resident = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(resident)
    if hashlib.sha256(resident.DATA).hexdigest() != \
       '1c40d851a19dd9077b1a5cc8d84319e00150678dbfed5d6713e7e786973cc4b1':
        raise RuntimeError('Primary resident cache differs')
    reference = resident.CACHE.parent.parent / 'halo2debug.xbe'
    if hashlib.sha256(reference.read_bytes()).hexdigest() != \
       '5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18':
        raise RuntimeError('Primary executable differs')
    sources = [base / 'resident-environment.json', base / 'resident-material-passes/passes.json']
    materials, programs = [json.loads(path.read_text()) for path in sources]
    checked = 0

    def verify(value):
        nonlocal checked
        if isinstance(value, dict):
            if 'raw_hex' in value and 'file_offset' in value:
                offset = int(value['file_offset'], 16)
                raw = bytes.fromhex(value['raw_hex'])
                if resident.DATA[offset:offset + len(raw)] != raw:
                    raise RuntimeError(f'Decoded source field differs at {offset:x}')
                checked += 1
            if value.get('reference'):
                tag = value['reference']
                if resident.TAGS[int(tag['datum'], 16)] != tag:
                    raise RuntimeError('Decoded source tag reference differs')
            for child in value.values():
                verify(child)
        elif isinstance(value, list):
            for child in value:
                verify(child)

    verify(materials)
    verify(programs)
    indexed = {}
    for program in programs['programs']:
        raw = struct.pack('<60I', *(int(w, 16) for w in program['program_words']))
        start = int(program['definition']['file_offset'], 16) + 0x14
        if resident.DATA[start:start + 240] != raw or \
           hashlib.sha256(raw).hexdigest() != program['program_sha256']:
            raise RuntimeError('Resident compiled program differs')
        indexed[(program['tag']['datum'], program['implementation'])] = program

    shader_rows = []
    for material in materials['shaders']:
        layers = []
        for lighting in material['postprocess_lighting']:
            model = field(lighting, 'lighting model')['value']
            for layer, row in enumerate(field(lighting, 'runtime layers*')['rows']):
                for selected in field(row, 'passes*')['rows']:
                    tag = field(selected, 'pass*')['reference']
                    for implementation, overrides in enumerate(field(selected, 'implementations*')['rows']):
                        program = indexed.get((tag['datum'], implementation))
                        if program is None:
                            continue  # Only the independently exported pass inventory is in scope.
                        textures = []
                        for stage, binding in enumerate(field(overrides, 'textures*')['rows']):
                            bitmap = field(binding, 'source bitmap*')['reference']
                            textures.append(dict(stage=stage, bitmap=bitmap,
                                frame=field(binding, 'source bitmap index*')['value'],
                                override=field(binding, 'source override*')['value'],
                                flags=field(binding, 'pass parameter flags')['value'],
                                file_offset=binding['file_offset']))
                        states = [{f['name']: f['value'] for f in state['fields']}
                                  for state in field(program['definition'], 'states*')['rows']]
                        layers.append(dict(lighting_model=model, layer=layer, pass_=tag,
                            implementation=implementation, textures=textures,
                            builtin=field(program['definition'], 'hack pixel shader index*')['value'],
                            resident_program_sha256=program['program_sha256'], states=states,
                            override_file_offset=overrides['file_offset']))
        shader_rows.append(dict(shader=material['tag'], selections=layers))

    image = Xbe(reference)
    table = struct.unpack('<11I', image.read(0x96478, 44))
    if table[:4] != (0x962b9, 0x962de, 0x962e4, 0x962ea) or \
       image.read(0x962de, 18).hex() != '6a006a09eb1e6a006a0beb186a006a0aeb12':
        raise RuntimeError('Original texture override dispatch differs')
    result = dict(scope=__doc__, checked_fields=checked,
        inputs={str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in sources},
        cache_sha256=hashlib.sha256(resident.DATA).hexdigest(),
        original_texture_dispatch=dict(entry='00096200', table='00096478',
            sha256=hashlib.sha256(image.read(0x96200, 0x2a4)).hexdigest(),
            targets={'2': 9, '3': 11, '4': 10},
            jumps={str(i + 1): f'{address:08x}' for i, address in enumerate(table)}),
        shaders=shader_rows, full_presentation_verified=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(checked_fields=checked, shaders=len(shader_rows),
                         selections=sum(len(s['selections']) for s in shader_rows))))


if __name__ == '__main__':
    main()
