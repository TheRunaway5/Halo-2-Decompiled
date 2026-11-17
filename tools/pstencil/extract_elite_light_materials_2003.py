#!/usr/bin/env python3
"""Recover exact Elite layer 16/17 bindings from authenticated 2003 caches.

Offline reference maintenance only. Runtime builds consume the generated bank;
the original executables and assets are never modified.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'analysis/pstencil/2003-presentation'))
import extract_resident_environment as resident

NAMES = {f'objects\\characters\\elite\\shaders\\{name}'
         for name in ('head', 'helmet', 'arms', 'torso', 'legs', 'teeth')}


def field(row, name):
    return next(value for value in row['fields'] if value['name'] == name)


def extract():
    inventory = ROOT / 'analysis/pstencil/2003-cache-inventory'
    records, unresolved = {}, []
    for entry in json.loads((inventory / 'summary.json').read_text())['maps']:
        metadata = json.loads((inventory / entry['report']).read_text())
        selected = [tag for tag in metadata['tags']
                    if tag['group'] == 'shad' and tag['name'] in NAMES and tag['payload_file_offset']]
        if not selected:
            continue
        data = Path(metadata['path']).read_bytes()
        if hashlib.sha256(data).hexdigest() != metadata['sha256']:
            raise RuntimeError('Original cache changed: ' + metadata['path'])
        resident.DATA = data
        resident.INDEX = int(metadata['tag_data_offset'], 16)
        resident.MAGIC = metadata['pointer_bias']
        resident.TAGS = {int(tag['datum'], 16): {
            **tag, 'payload_file_offset': int(tag['payload_file_offset'], 16)
            if tag['payload_file_offset'] else None} for tag in metadata['tags']}
        for tag in selected:
            try:
                lighting = resident.block(int(tag['payload_file_offset'], 16) + 0x30, 0x8112d0)
                if not lighting:
                    raise ValueError('No resident lighting model')
                for model in lighting:
                    layers = field(model, 'runtime layers*')['rows']
                    kinds, colors, evidence = {}, {}, []
                    for layer in (2, 16, 17):
                        passes = field(layers[layer], 'passes*')['rows']
                        if len(passes) != 1:
                            raise ValueError('Ambiguous resident pass selection')
                        selected_pass = passes[0]
                        reference = field(selected_pass, 'pass*')['reference']
                        definitions = resident.block(reference['payload_file_offset'] + 0x30, 0x811760)
                        implementations = field(selected_pass, 'implementations*')['rows']
                        if len(definitions) != 1 or len(implementations) != 1:
                            raise ValueError('Ambiguous resident implementation')
                        selector = field(definitions[0], 'hack pixel shader index*')
                        kinds[layer] = selector['value']
                        evidence.append({key: selector[key] for key in ('file_offset', 'raw_hex')})
                        if layer != 17:
                            continue
                        for constant in field(implementations[0], 'VS constants*')['rows']:
                            bank = field(constant, 'register bank*')['value']
                            index = field(constant, 'register index*')['value']
                            if bank != 0 or index not in (10, 11):
                                continue
                            if index in colors or field(constant, 'overlays*')['rows']:
                                raise ValueError('Duplicate or animated material color')
                            colors[index] = [field(constant, axis + '*')['value'] for axis in 'xyzw']
                            for source in constant['fields']:
                                if source['name'] in ('register bank*', 'register index*',
                                                      'pass constant data index*', 'x*', 'y*', 'z*', 'w*', 'overlays*'):
                                    evidence.append({key: source[key] for key in ('file_offset', 'raw_hex')})
                    if kinds[2] != 1 or kinds[16] not in (6, 9, 10) or kinds[17] != 7:
                        raise ValueError('Unsupported normal/utility/dependent producer chain')
                    if set(colors) != {10, 11}:
                        raise ValueError('Missing explicit facing/grazing color')
                    records.setdefault(tag['name'], []).append(dict(
                        map=entry['name'], cache_sha256=metadata['sha256'],
                        shader_file_offset=tag['payload_file_offset'],
                        lighting_model=field(model, 'lighting model')['value'],
                        parameters=dict(v10=colors[10], v11=colors[11], utility_builtin=kinds[16]),
                        fields=evidence))
            except (ValueError, KeyError, IndexError, StopIteration, AssertionError) as error:
                unresolved.append(dict(map=entry['name'], shader=tag['name'], reason=repr(error)))
    consensus, disagreements = [], []
    for name, sources in sorted(records.items()):
        row = dict(shader=name, parameters=sources[0]['parameters'], sources=sources)
        distinct = {json.dumps(source['parameters'], sort_keys=True) for source in sources}
        (consensus if len(distinct) == 1 else disagreements).append(row)
    if {row['shader'] for row in consensus} != NAMES or disagreements or unresolved:
        raise RuntimeError(json.dumps(dict(consensus=len(consensus), disagreements=disagreements,
                                           unresolved=unresolved)))
    return dict(scope=__doc__, primary_tool_sha256=hashlib.sha256(resident.defs.TOOL.read_bytes()).hexdigest(),
                materials=consensus, full_presentation_verified=False)


def generate(reference):
    def vector(values):
        return '{' + ','.join('std::bit_cast<float>(0x%08xu)' %
                             struct.unpack('<I', struct.pack('<f', value))[0] for value in values) + '}'
    rows = []
    for row in reference['materials']:
        parameters = row['parameters']
        rows.append('    {' + json.dumps(row['shader']) + ',' + vector(parameters['v10']) + ',' +
                    vector(parameters['v11']) + ',' + str(parameters['utility_builtin']) + 'u},')
    return ('// Generated by tools/pstencil/extract_elite_light_materials_2003.py.\n'
            'constexpr std::array<ReferenceLightMaterial,6> elite_light_materials{{\n' +
            '\n'.join(rows) + '\n}};\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--include', type=Path)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    reference = extract()
    content = generate(reference)
    if args.check:
        if json.loads(args.output.read_text()) != reference or not args.include or args.include.read_text() != content:
            raise RuntimeError('Generated Elite reference differs')
    else:
        if args.output.exists():
            if json.loads(args.output.read_text()) != reference:
                raise RuntimeError('Refusing to replace different retained evidence')
        else:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(json.dumps(reference, indent=2) + '\n')
        if args.include:
            args.include.write_text(content)
    print(json.dumps(dict(materials=len(reference['materials']),
                          source_records=sum(len(row['sources']) for row in reference['materials']))))


if __name__ == '__main__':
    main()
