#!/usr/bin/env python3
"""Build an immutable embedded raw class0..6 catalog; never a runtime compiler.

The native generator owns token arithmetic, declaration fetch, ABI and epilogue.
This stdlib tool invokes the explicitly supplied build compiler/validator, pins
literal inputs and outputs, checks the compact ABI in SPIR-V, and emits C++ data.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

SCHEMA = 'halo2-menu-world-vertex-source-v1'
PORT_SCHEMA = 'halo2-native-menu-world-vertex-source-v2'
ABI = 2


def sha(data):
    return hashlib.sha256(data).hexdigest()


def digest(value):
    if not isinstance(value, str) or len(value) != 64 or any(c not in '0123456789abcdef' for c in value):
        raise ValueError('catalog SHA-256 identity is invalid')
    return value


def atomic(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.tmp')
    temporary.write_bytes(data)
    os.replace(temporary, path)


def local(directory, name):
    if not isinstance(name, str) or Path(name).name != name or name in ('', '.', '..'):
        raise ValueError('catalog source name escapes its generated directory')
    path = directory / name
    if path.resolve().parent != directory.resolve():
        raise ValueError('catalog source symlink escapes its generated directory')
    return path


def integers(value, minimum, maximum, label):
    if not isinstance(value, list) or any(type(x) is not int or x < minimum or x > maximum for x in value):
        raise ValueError('invalid catalog ' + label)
    return value


def run(argv):
    result = subprocess.run([str(x) for x in argv], capture_output=True, text=True, check=False)
    if result.returncode:
        raise RuntimeError('catalog build tool failed: ' + ' '.join(map(str, argv)) + '\n' + result.stdout + result.stderr)
    return result.stdout + result.stderr


def spirv_contract(data, semantics, abi=ABI):
    if len(data) < 20 or len(data) % 4:
        raise ValueError('compiled catalog SPIR-V has invalid extent')
    words = struct.unpack('<' + 'I' * (len(data) // 4), data)
    if words[0] != 0x07230203 or words[1] != 0x00010600 or words[4] != 0:
        raise ValueError('compiled catalog is not the agreed Vulkan1.3/SPIR-V1.6 context')
    instructions = []
    at = 5
    while at < len(words):
        size, opcode = words[at] >> 16, words[at] & 65535
        if not size or at + size > len(words):
            raise ValueError('compiled catalog SPIR-V instruction escapes module')
        instructions.append((opcode, words[at + 1:at + size]))
        at += size
    # Shader + Vertex entry; the build validator owns all further SPIR-V rules.
    if not any(op == 15 and args[0] == 0 for op, args in instructions):
        raise ValueError('compiled catalog lacks a vertex entry point')
    decorations, offsets, pointers, variables, types, constants = {}, {}, {}, [], {}, {}
    for op, args in instructions:
        if op == 71 and len(args) >= 2:
            decorations[(args[0], args[1])] = tuple(args[2:])
        elif op == 72 and len(args) == 4 and args[2] == 35:
            offsets[(args[0], args[1])] = args[3]
        elif op == 32 and len(args) == 3:
            pointers[args[0]] = (args[1], args[2])
        elif op in (21, 22, 23, 28, 30):
            types[args[0]] = (op, args[1:])
        elif op == 43 and len(args) == 3:
            constants[args[1]] = args[2]
        elif op == 59 and len(args) >= 3:
            variables.append(args)
    uniforms = [args for args in variables if args[2] == 2]
    if len(uniforms) != 1:
        raise ValueError('compiled catalog must have exactly one uniform block')
    pointer, variable, _ = uniforms[0][:3]
    if decorations.get((variable, 34)) != (1,) or decorations.get((variable, 33)) != (0,):
        raise ValueError('compiled catalog changed set1/binding0')
    if pointer not in pointers or pointers[pointer][0] != 2:
        raise ValueError('compiled catalog uniform pointer type is invalid')
    block = pointers[pointer][1]
    expected_offsets = [0, 3072, 3088, 3096] + ([3104] if abi == 3 else [])
    if decorations.get((block, 2)) != () or [offsets.get((block, i)) for i in range(len(expected_offsets))] != expected_offsets:
        raise ValueError('compiled catalog changed compact std140 member offsets')
    def vector(type_id, size):
        item = types.get(type_id)
        return bool(item and item[0] == 23 and item[1][1] == size and
                    types.get(item[1][0]) == (22, (32,)))
    members = types.get(block)
    if not members or members[0] != 30 or len(members[1]) != len(expected_offsets):
        raise ValueError('compiled catalog uniform member count/type differs')
    bank, clip, fog, surface = members[1][:4]
    bank_type = types.get(bank)
    if not bank_type or bank_type[0] != 28 or not vector(bank_type[1][0], 4) or constants.get(bank_type[1][1]) != 192 or decorations.get((bank, 6)) != (16,):
        raise ValueError('compiled catalog c192 bank type/stride differs')
    if not vector(clip, 4) or not vector(fog, 2) or not vector(surface, 2):
        raise ValueError('compiled catalog compact clip/fog/surface types differ')
    if abi == 3:
        attributes = members[1][4]
        attribute_type = types.get(attributes)
        if not attribute_type or attribute_type[0] != 28 or not vector(attribute_type[1][0], 4) or constants.get(attribute_type[1][1]) != 16 or decorations.get((attributes, 6)) != (16,):
            raise ValueError('compiled catalog constant attribute bank type/stride differs')
    locations = {1: {}, 3: {}}
    for pointer, variable, storage, *_ in variables:
        location = decorations.get((variable, 30))
        if storage not in locations or location is None:
            continue
        if len(location) != 1 or location[0] in locations[storage] or pointer not in pointers:
            raise ValueError('compiled catalog has ambiguous input/output locations')
        locations[storage][location[0]] = pointers[pointer][1]
    if set(locations[1]) != set(range(len(semantics))) or set(locations[3]) != set(range(13)):
        raise ValueError('compiled catalog changed declared input/output locations')
    for location, semantic in enumerate(semantics):
        type_id = locations[1][location]
        if semantic in (4, 5, 6):
            if types.get(type_id) != (21, (32, 1)):
                raise ValueError('compiled catalog packed basis fetch is not signed int32')
        elif not vector(type_id, 4):
            raise ValueError('compiled catalog position/UV fetch is not vec4')
    for location, type_id in locations[3].items():
        if not ((types.get(type_id) == (22, (32,))) if location in (4, 12) else vector(type_id, 4)):
            raise ValueError('compiled catalog ordinary GS varying type differs')
    for _, variable, storage, *_ in variables:
        location = decorations.get((variable, 30))
        if storage == 3 and location and location[0] >= 9 and decorations.get((variable, 14)) != ():
            raise ValueError('compiled catalog ordered depth output lost Flat decoration')
    return words


def array(name, type_name, values, literal):
    lines = [f'inline constexpr std::array<{type_name},{len(values)}> {name}{{{{']
    for i in range(0, len(values), 8):
        lines.append('  ' + ','.join(literal(x) for x in values[i:i + 8]) + ',')
    return '\n'.join(lines + ['}};'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sources', type=Path, required=True, help='native generator sources.json')
    parser.add_argument('--additional-sources', type=Path, action='append', default=[],
                        help='extra immutable programs, retaining each manifest ABI; duplicate identities fail')
    parser.add_argument('--compiler', type=Path, required=True, help='build glslangValidator executable')
    parser.add_argument('--validator', type=Path, required=True, help='build spirv-val executable')
    parser.add_argument('--header', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    args = parser.parse_args()
    raw_manifest = args.sources.read_bytes()
    source_manifest = json.loads(raw_manifest)
    manifests = [(args.sources, raw_manifest, source_manifest)]
    for path in args.additional_sources:
        raw = path.read_bytes()
        manifests.append((path, raw, json.loads(raw)))
    entries, abis = [], set()
    for path, _, manifest in manifests:
        abi = manifest.get('abi')
        if manifest.get('schema') not in (SCHEMA, PORT_SCHEMA) or abi not in (ABI, 3) or manifest.get('target') != 'vulkan1.3':
            raise ValueError('native catalog generator schema/ABI/target differs')
        rows = manifest.get('entries')
        if not isinstance(rows, list) or not rows:
            raise ValueError('native catalog contains no generated entries')
        abis.add(abi)
        entries.extend((entry, manifest, path.parent, abi) for entry in rows)
    compiler = args.compiler.resolve(strict=True)
    validator = args.validator.resolve(strict=True)
    compiler_version = run([compiler, '--version'])
    validator_version = run([validator, '--version'])
    # Source-owned GLSL may be in a read-only checkout. All build outputs
    # belong beside the requested generated header.
    compiled = args.header.parent / 'compiled'
    compiled.mkdir(parents=True, exist_ok=True)
    records, seen = [], set()
    header = ['#pragma once', '#include "halo2/menu_world_vertex_catalog.hpp"', '#include <array>',
              'namespace halo2::generated_menu_world_vertex_catalog {',
              'static_assert(menu_world_vertex_catalog_abi==2);',
              *(['static_assert(menu_world_vertex_attribute_catalog_abi==3);'] if 3 in abis else [])]
    for index, (entry, source_manifest, directory, abi) in enumerate(entries):
        if entry.get('class_index') not in range(7):
            raise ValueError('catalog entry is not actual class0..6')
        semantics = integers(entry['semantics'], 0, 65535, 'declaration')
        if not semantics or len(semantics) > 16 or any(x not in (0, 1, 2, 3, 4, 5, 6) for x in semantics):
            raise ValueError('catalog declaration has unsupported raw semantic')
        inputs = integers(entry['consumed_inputs'], 0, 15 if abi == 3 else len(semantics) - 1, 'consumed inputs')
        if any(x >= len(semantics) and x not in (10, 11) for x in inputs):
            raise ValueError('catalog constant attribute input is outside the admitted V10/V11 corridor')
        constants = integers(entry['consumed_constants'], 0, 191, 'consumed constants')
        if inputs != sorted(set(inputs)) or constants != sorted(set(constants)):
            raise ValueError('catalog consumed sets are not canonical')
        source_path = local(directory, entry['source'])
        source = source_path.read_bytes()
        if source_manifest['schema'] == SCHEMA:
            program = local(directory, entry['program']).read_bytes()
            if len(program) < 20 or len(program) > 4 + 136 * 16 or program[:2] != b'\x78\x20' or len(program) != 4 + struct.unpack_from('<H', program, 2)[0] * 16:
                raise ValueError('catalog program extent/header is invalid')
            program_bytes, program_sha = len(program), sha(program)
        else:
            if sha(source) != digest(entry['source_sha256']):
                raise ValueError('preserved port GLSL differs from its declared source identity')
            program_bytes, program_sha = entry['program_bytes'], digest(entry['program_sha256'])
            if type(program_bytes) is not int or not 20 <= program_bytes <= 4 + 136 * 16 or (program_bytes - 4) % 16:
                raise ValueError('catalog program extent is invalid')
        key = (entry['class_index'], program_sha, program_bytes, tuple(semantics))
        if key in seen:
            raise ValueError('catalog has duplicate literal program/declaration identity')
        seen.add(key)
        output = compiled / f'menu-world-vs-{index}.spv'
        command = [compiler, '-V', '--target-env', 'vulkan1.3', '-S', 'vert', source_path, '-o', output]
        build_log = run(command)
        validate_log = run([validator, '--target-env', 'vulkan1.3', output])
        spirv = output.read_bytes()
        words = spirv_contract(spirv, semantics, abi)
        prefix = f'entry_{index}'
        header.append(array(prefix + '_semantics', 'std::uint16_t', semantics, str))
        header.append(array(prefix + '_spirv', 'std::uint32_t', words, lambda x: f'0x{x:08x}u'))
        header.append(array(prefix + '_inputs', 'unsigned', inputs, lambda x: f'{x}u'))
        header.append(array(prefix + '_constants', 'unsigned', constants, lambda x: f'{x}u'))
        records.append({**entry, 'abi': abi, 'source_sha256': sha(source), 'program_sha256': program_sha, 'program_bytes': program_bytes,
                        'spirv_sha256': sha(spirv), 'spirv_bytes': len(spirv),
                        'spirv': str(output.resolve()), 'command': [str(x) for x in command],
                        'compiler_log': build_log, 'validator_log': validate_log})
    header.append(f'inline constexpr std::array<MenuWorldVertexCatalogEntry,{len(records)}> entries{{{{')
    for i, record in enumerate(records):
        p = f'entry_{i}'
        header.append(f'  {{{record["abi"]},{record["class_index"]},"{record["program_sha256"]}",{record["program_bytes"]},{p}_semantics,{p}_spirv,{p}_inputs,{p}_constants,'
                      f'"{record["source_sha256"]}","{record["spirv_sha256"]}"}},')
    header += ['}};', '} // namespace halo2::generated_menu_world_vertex_catalog', '']
    header_bytes = '\n'.join(header).encode()
    source_manifest = manifests[0][2]
    report = {'schema': 'halo2-menu-world-vertex-compiled-v1', 'abi': next(iter(abis)) if len(abis) == 1 else 'mixed',
              'abi_versions': sorted(abis), 'target': 'vulkan1.3',
              'sources_manifest_sha256': sha(raw_manifest),
              'additional_sources': [{'path': str(path), 'sha256': sha(raw), 'abi': manifest['abi'],
                                      'cache_sha256': manifest.get('cache_sha256'), 'entry_count': len(manifest['entries'])}
                                     for path, raw, manifest in manifests[1:]],
              'cache_sha256': (sha(Path(source_manifest['cache']).read_bytes()) if source_manifest['schema'] == SCHEMA
                               else digest(source_manifest['cache_sha256'])),
              'original_asset_files_read': source_manifest['schema'] == SCHEMA,
              'compiler': {'path': str(compiler), 'sha256': sha(compiler.read_bytes()), 'version': compiler_version},
              'validator': {'path': str(validator), 'sha256': sha(validator.read_bytes()), 'version': validator_version},
              'header_sha256': sha(header_bytes), 'entries': records,
              'vrtx_count': source_manifest['vrtx_count'], 'rejected': source_manifest['rejected'],
              'runtime_compilation': False, 'live_renderer_admission': False}
    atomic(args.header, header_bytes)
    atomic(args.manifest, (json.dumps(report, indent=2, sort_keys=True) + '\n').encode())
    print(f'menu raw vertex compiled catalog: {len(records)} embedded modules; compact ABI checked; runtime compilation absent')


if __name__ == '__main__':
    main()
