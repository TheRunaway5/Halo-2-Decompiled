#!/usr/bin/env python3
"""Prove native local/skin→WORLD position composition without algebraic rewrites.

The production AOT prefix already retains each complete original identity.
This audit verifies that its exact WORLD output followed by the corresponding
world-class native program has the original class1..6 ordered oPos dependency.
"""
import argparse
import json
from pathlib import Path
import struct
from audit_source_world_depth_position import position_dag, sha, audit as world_audit
from native_cpu_vertex_kernel import _program, position_prefix


def audit(registry):
    _, world_admitted = world_audit(registry)
    if len({item['dag']['sha256'] for item in world_admitted.values()}) != 1:
        raise ValueError('World projection admissions no longer share an exact ordered position dependency')
    by_family = {}
    for row in registry['cpu_vertex_programs']:
        code, words = _program(row)
        for alias in row.get('aliases', []):
            if alias.get('kind') != 'raw-vertex-class':
                continue
            key = (alias['name'], alias['classification'])
            by_family.setdefault(key, {})[sha(code)] = (code, words)
    # Raw POSITION is a three-component native fetch. Its missing W is one,
    # independently of palette/weight components which remain symbolic here.
    one = sha(json.dumps(('f32-bits', 0x3f800000), separators=(',', ':')).encode())
    position = [sha(json.dumps(('input', 0, lane), separators=(',', ':')).encode()) for lane in range(3)] + [one]
    def prefix_dag(words):
        prefix = position_prefix(words)
        if prefix is None:return None
        code = b'\x78\x20' + struct.pack('<H', len(prefix)) + b''.join(struct.pack('<4I', *w) for w in prefix)
        return position_dag(code, output_index=9, input_lanes={0: position})
    apply_prefixes = {};apply_inputs = {}
    for (family, classification), programs in by_family.items():
        if classification not in range(1, 7) or family.rsplit('\\',1)[-1] not in (
                'shadow_buffer_application_1tap','shadow_buffer_application_3tap','shadow_buffer_application_lightmap'):continue
        for _, words in programs.values():
            local = prefix_dag(words)
            if local is None:raise ValueError('Actual Apply program lost its native WORLD boundary')
            apply_prefixes.setdefault(classification,set()).add(tuple(local['lanes'][:3]))
            apply_inputs.setdefault(classification,set()).update(local['input_registers'])
    if any(len(values)!=1 for values in apply_prefixes.values()):
        raise ValueError('Apply variants of one class no longer have an identical native WORLD prefix')
    entries, rejected = [], []
    for (family, classification), programs in sorted(by_family.items()):
        if classification not in range(1, 7):
            continue
        for program_hash, (code, words) in sorted(programs.items()):
            local = prefix_dag(words)
            if local is None:
                rejected.append({'family': family, 'class': classification, 'reason': 'no authenticated WORLD prefix'})
                continue
            original = position_dag(code, input_lanes={0: position})
            matches = []
            for world_hash, item in sorted(world_admitted.items())[:1]:
                world = item['code']
                composed = position_dag(world, input_lanes={0: local['lanes'][:3] + [one]})
                if composed['lanes'] == original['lanes']:
                    # Equality of the original symbolic WORLD DAG also proves
                    # equality under this identical input-lane substitution.
                    matches = [world_hash]
            item = {'family': family, 'class': classification, 'program_sha256': program_hash,
                    'world_prefix_w_one': local['lanes'][3] == one, 'world_programs': matches,
                    'ordered_world_xyz_dag':sha(''.join(local['lanes'][:3]).encode()),
                    'matches_apply_prefix':tuple(local['lanes'][:3]) in apply_prefixes[classification],
                    'ordered_position_dag': original['sha256']}
            (entries if matches else rejected).append(item)
    return {'schema': 'HALO2_SOURCE_MODEL_DEPTH_POSITION_V1', 'admitted': entries, 'rejected': rejected,
            'equivalent_world_programs': sorted(world_admitted),
            'apply_prefix_input_registers':{str(key):sorted(value) for key,value in sorted(apply_inputs.items())},
            'world_position_equivalence_class': next(iter(world_admitted.values()))['dag']['sha256'],
            'algebraic_simplification': False, 'runtime_instruction_interpreter': False}


def header(registry, result):
    # The runtime consumes actual Apply bindings. Generation, water and other
    # raw programs cannot acquire receiver ownership through a shared prefix.
    admitted = {}
    for item in result['admitted']:
        if item['family'].rsplit('\\', 1)[-1] not in (
                    'shadow_buffer_application_1tap', 'shadow_buffer_application_3tap',
                    'shadow_buffer_application_lightmap'):continue
        admitted.setdefault(item['program_sha256'],[]).append(item)
    codes = {sha(_program(row)[0]): _program(row)[0] for row in registry['cpu_vertex_programs']}
    lines = ['#pragma once', '#include <algorithm>', '#include <array>', '#include <cstddef>',
             '#include <span>', 'namespace halo2::source_model_depth_private {',
             'inline constexpr unsigned position_input_mask(unsigned classification) {',
             '    switch(classification) {case 0:return 1u;']
    for classification,inputs in result['apply_prefix_input_registers'].items():
        lines += [f'    case {classification}:return {sum(1<<value for value in inputs)}u;']
    lines += ['    default:return 0u;}', '}',
             '// Generated ordered raw Apply -> fixed WORLD prefix -> class0 proof.',
             '// Complete original tokens and the actual compiled class are mandatory.',
             'inline bool position_program_admitted(std::span<const std::byte> code,unsigned classification) {']
    for n, (key, items) in enumerate(sorted(admitted.items())):
        code = codes[key]
        classes = sorted({item['class'] for item in items})
        condition = '('+' || '.join('classification=='+str(value) for value in classes)+')'
        lines += ['    // '+items[0]['family']+' classes'+','.join(map(str,classes))+' SHA256 '+key,
                  f'    static constexpr std::array<unsigned char,{len(code)}> p{n}{{'+','.join(map(str,code))+'};',
                  f'    if({condition} && code.size()==p{n}.size() && std::equal(code.begin(),code.end(),p{n}.begin(),[](std::byte a,unsigned char b){{return std::to_integer<unsigned char>(a)==b;}}))return true;']
    lines += ['    return false;', '}',
              '// The ordinary material must also own the identical skin/decompression',
              '// prefix. A WORLD variant alone cannot establish that native relation.',
              'inline bool ordinary_position_program_admitted(std::span<const std::byte> code,unsigned classification) {']
    ordinary = {}
    for item in result['admitted']:
        if item['matches_apply_prefix']:
            ordinary.setdefault(item['program_sha256'],[]).append(item)
    for n, (key, items) in enumerate(sorted(ordinary.items())):
        code = codes[key];classes=sorted({item['class'] for item in items})
        condition='('+' || '.join('classification=='+str(value) for value in classes)+')'
        lines += ['    // Native ordinary/Apply ordered WORLD xyz SHA256 '+items[0]['ordered_world_xyz_dag'],
                  '// Complete original program SHA256 '+key,
                  f'    static constexpr std::array<unsigned char,{len(code)}> p{n}{{'+','.join(map(str,code))+'};',
                  f'    if({condition} && code.size()==p{n}.size() && std::equal(code.begin(),code.end(),p{n}.begin(),[](std::byte a,unsigned char b){{return std::to_integer<unsigned char>(a)==b;}}))return true;']
    return '\n'.join([*lines, '    return false;', '}', '}', ''])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--registry', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--header', type=Path)
    args = parser.parse_args()
    data = args.registry.read_bytes()
    registry = json.loads(data)
    result = audit(registry);result['registry_sha256'] = sha(data)
    if args.header:args.header.write_text(header(registry, result))
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'admitted': len(result['admitted']), 'rejected': len(result['rejected'])}))


if __name__ == '__main__':
    main()
