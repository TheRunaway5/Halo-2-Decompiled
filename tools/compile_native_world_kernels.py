#!/usr/bin/env python3
"""Compile the source-owned water/sky registry into direct native Vulkan kernels.

Only this build tool reads program words. The runtime receives immutable SPIR-V
and performs an exact complete-identity lookup; no original assets or runtime
shader compiler are required. Generated source, objects and SPIR-V live below
the caller's build directories.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import time

from native_world_vertex_kernel import lower_vertex_program
from native_world_fragment_kernel import lower_fragment_program, build_fragment_source
from native_cpu_vertex_kernel import build_position_header

SCHEMA = 'HALO2_NATIVE_WORLD_KERNEL_PROGRAMS_V1'
COLORS = frozenset((*range(10, 26), 43, 44))


def sha(data):
    return hashlib.sha256(data).hexdigest()


def atomic(path, data):
    try:
        if path.read_bytes() == data:
            return
    except FileNotFoundError:
        pass
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.tmp')
    try:
        temporary.write_bytes(data)
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def uints(value, length, label):
    if (not isinstance(value, list) or len(value) != length or
            any(type(x) is not int or not 0 <= x <= 0xffffffff for x in value)):
        raise ValueError('Native world registry has invalid ' + label)
    return value


def validate_entry(entry):
    value = entry.get('vertex_program_hex')
    if not isinstance(value, str) or not value or value.lower() != value:
        raise ValueError('Native world registry has invalid vertex bytes')
    try:
        program = bytes.fromhex(value)
    except ValueError as error:
        raise ValueError('Native world registry has invalid vertex hex') from error
    if program.hex() != value or len(program) < 20 or program[:2] != b'\x78\x20':
        raise ValueError('Native world registry changed original vertex header')
    count = struct.unpack_from('<H', program, 2)[0]
    if not 1 <= count <= 136 or len(program) != 4 + count * 16:
        raise ValueError('Native world registry changed original vertex extent')
    instructions = [list(struct.unpack_from('<4I', program, 4 + 16 * i)) for i in range(count)]
    if entry.get('vertex_instructions') != instructions:
        raise ValueError('Native world decoded instructions differ from complete source bytes')
    controls = uints(entry.get('controls'), 60, 'control words')
    if any(controls[index] for index in COLORS):
        raise ValueError('Native world control identity retained animated colors')
    declaration = uints(entry.get('packed_declaration'), 4, 'packed declaration')
    semantics = [(declaration[i // 4] >> (8 * (i % 4))) & 255 for i in range(16)]
    if entry.get('semantics') != semantics:
        raise ValueError('Native world declaration differs from complete packed identity')
    extended = entry.get('extended_vertex_bank')
    if type(extended) is not bool:
        raise ValueError('Native world extended bank must be an explicit boolean')
    key = program + struct.pack('<64I?', *controls, *declaration, extended)
    vertex_key = program + struct.pack('<4I?', *declaration, extended)
    fragment_key = struct.pack('<60I', *controls)
    return dict(program=program, controls=controls, declaration=declaration,
                semantics=semantics, extended=extended, instructions=instructions,
                key=sha(key), vertex_key=sha(vertex_key), fragment_key=sha(fragment_key))


VERTEX_WRAPPERS = {'world': 'world.vert',
                   'source-depth': 'world.vert',
                   'normal': 'pstencil/normal_accumulation.vert',
                   'normal-utility': 'pstencil/normal_light_utility.vert'}


def vertex_source(shader_root, row, wrapper='world'):
    base = (shader_root / VERTEX_WRAPPERS[wrapper]).read_text()
    if wrapper == 'source-depth':
        base = base.replace('#version 450', '#version 450\n#define H2_SOURCE_WORLD_DEPTH 1', 1)
    marker = '#include "world_vertex.glsl"'
    if base.count(marker) != 1:
        raise ValueError('Native world vertex wrapper ABI changed')
    body = (shader_root / 'world_vertex.glsl').read_text()
    lowered = lower_vertex_program(row['instructions'], declaration=row['declaration'],
                                   extended_vertex_bank=row['extended'])
    marker = '#include "vertex_program.glsl"'
    if body.count(marker) != 1:
        raise ValueError('Native world vertex helper ABI changed')
    body = body.replace(marker, '#define H2_NATIVE_VERTEX_PROGRAM\n#define H2_NATIVE_WORLD_VERTEX_KERNEL\n' + marker + '\n' + lowered['glsl'])
    # Declaration-specific post-skin coordinates are native arithmetic too.
    replacement = ''
    if row['semantics'][0] == 192:
        lines = []
        for stage in range(2):
            for axis in range(3):
                name = f'native_skin_{stage}_{axis}'
                lines += [f'        vec4 {name}_row=vpConstant({18+stage*3+axis}u);',
                          f'        precise vec4 {name}_p=vpProduct(vec4(sourceLocalPosition,1),{name}_row);',
                          f'        precise float {name}_xy={name}_p.x+{name}_p.y;',
                          f'        precise float {name}_xyz={name}_xy+{name}_p.z;',
                          f'        vpOut[{9+stage}][{axis}]={name}_xyz+{name}_row.w;']
        replacement = '\n'.join(lines)
    if body.count('applySourceCPUSkinTextureCoordinates();') != 1 or body.count('if(material.flags.w!=0u)') != 1:
        raise ValueError('Native world vertex execution ABI changed')
    body = body.replace('applySourceCPUSkinTextureCoordinates();', replacement)
    body = body.replace('if(material.flags.w!=0u)', 'if(true)')
    base = base.replace('#include "world_vertex.glsl"', body)
    base = base.replace('#define H2_EXTENDED_VERTEX_BANK\n', '')
    if row['extended']:
        base = base.replace('#extension GL_GOOGLE_include_directive : require',
                            '#extension GL_GOOGLE_include_directive : require\n#define H2_EXTENDED_VERTEX_BANK')
    return base, lowered


def spirv_contract(data, stage):
    """Reject executable reads of instruction/declaration/control-word banks."""
    if len(data) < 20 or len(data) % 4:
        raise ValueError('Native world SPIR-V has invalid extent')
    words = struct.unpack('<' + 'I' * (len(data) // 4), data)
    if words[0] != 0x07230203 or words[1] > 0x00010600 or words[4] != 0:
        raise ValueError('Native world output is not supported SPIR-V')
    ops, at = [], 5
    while at < len(words):
        size, opcode = words[at] >> 16, words[at] & 65535
        if not size or at + size > len(words):
            raise ValueError('Native world SPIR-V instruction escapes module')
        ops.append((opcode, words[at+1:at+size]));at += size
    entry_model = 0 if stage == 'vert' else 4
    if not any(op == 15 and args[0] == entry_model for op, args in ops):
        raise ValueError('Native world output has no matching stage entry point')
    constants, decorations, pointer_types, variable_types, member_names = {}, {}, {}, {}, {}
    for op, args in ops:
        if op == 43 and len(args) == 3:
            constants[args[1]] = args[2]
        elif op == 71 and len(args) >= 3:
            decorations[(args[0], args[1])] = args[2:]
        elif op == 32 and len(args) == 3:
            pointer_types[args[0]] = args[2]
        elif op == 59 and len(args) >= 3:
            variable_types[args[1]] = args[0]
        elif op == 6 and len(args) >= 3:
            text = struct.pack('<'+'I'*(len(args)-2), *args[2:]).split(b'\0', 1)[0].decode()
            member_names[(args[0], args[1])] = text
        elif 48 <= op <= 52:
            raise ValueError('Native world output retained specialization-time instructions')
    material = [variable for (variable, kind), value in decorations.items()
                if kind == 33 and value == (4,) and decorations.get((variable, 34)) == (0,)]
    if len(material) > 1:
        raise ValueError('Native world output has ambiguous material owners')
    block = pointer_types.get(variable_types.get(material[0])) if material else None
    accesses = {}
    for op, args in ops:
        if op in (65, 66):
            result, base = args[1], args[2]
            if base in material:
                accesses[result] = tuple(constants.get(index) for index in args[3:])
            elif base in accesses:
                accesses[result] = accesses[base] + tuple(constants.get(index) for index in args[3:])
        elif op == 61:
            pointer = args[2]
            if pointer in material:
                raise ValueError('Native world shader reads the complete instruction-bearing material block')
            if pointer not in accesses:
                continue
            path = accesses[pointer]
            member = member_names.get((block, path[0])) if path else None
            if not path or path[0] is None or member is None:
                raise ValueError('Native world material access has no complete static member owner')
            if member in ('vertexInstructions', 'vertexDeclaration'):
                raise ValueError('Native world shader reads an instruction or declaration bank')
            if member == 'flags' and (len(path) < 2 or path[1] in (None, 3)):
                raise ValueError('Native world shader reads the runtime instruction count')
            if member == 'program':
                if stage == 'vert' or len(path) != 3 or None in path or path[1] * 4 + path[2] not in COLORS:
                    raise ValueError('Native world shader decodes a runtime program control word')
    return dict(no_runtime_instruction_reads=True, no_runtime_declaration_reads=True,
                no_runtime_program_control_reads=True, no_specialization_instructions=True,
                instructions=len(ops))


def spirv_interface_locations(data, storage):
    """Inspect actual entry-point interface variables after optimization."""
    words = struct.unpack('<'+'I'*(len(data)//4), data)
    variables, locations, interfaces = {}, {}, set()
    at = 5
    while at < len(words):
        count, opcode = words[at] >> 16, words[at] & 65535
        if not count or at+count > len(words):
            raise ValueError('Native world interface instruction escapes module')
        args = words[at+1:at+count]
        if opcode == 59 and len(args) >= 3:
            variables[args[1]] = args[2]
        elif opcode == 71 and len(args) == 3 and args[1] == 30:
            locations[args[0]] = args[2]
        elif opcode == 15:
            end = 2
            while end < len(args) and all((args[end] >> shift)&255 for shift in (0,8,16,24)):
                end += 1
            interfaces.update(args[end+1:])
        at += count
    return sorted(locations[value] for value in interfaces
                  if variables.get(value) == storage and value in locations)


def run(argv):
    result = subprocess.run([str(value) for value in argv], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError('Native world compiler failed: ' + ' '.join(map(str, argv)) + '\n' +
                           (result.stdout + result.stderr)[-12000:])
    return (result.stdout + result.stderr)[-4000:]


def literal_array(name, kind, values, literal):
    result = [f'inline constexpr std::array<{kind},{len(values)}> {name}{{{{']
    result += ['    ' + ','.join(literal(x) for x in values[i:i+12]) + ',' for i in range(0, len(values), 12)]
    return '\n'.join(result + ['}};'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--registry', type=Path, required=True)
    parser.add_argument('--header', type=Path)
    parser.add_argument('--manifest', type=Path)
    parser.add_argument('--cpu-header', type=Path,
        help='AOT CPU world-position kernels; defaults beside the world catalog header')
    parser.add_argument('--shader-output-dir', type=Path, required=True)
    parser.add_argument('--shader-root', type=Path, default=Path(__file__).resolve().parents[1] / 'shaders')
    parser.add_argument('--compiler', type=Path)
    parser.add_argument('--optimizer', type=Path)
    parser.add_argument('--validator', type=Path)
    parser.add_argument('--list-outputs', action='store_true', help='Print the complete declared SPIR-V output set for CMake')
    args = parser.parse_args()
    registry_raw = args.registry.read_bytes();registry = json.loads(registry_raw)
    if registry.get('schema') != SCHEMA or registry.get('abi') != 1 or registry.get('target') != 'vulkan1.3':
        raise ValueError('Native world registry schema/ABI/target differs')
    if not isinstance(registry.get('entries'), list) or not registry['entries']:
        raise ValueError('Native world registry contains no admitted source kernels')
    rows = [validate_entry(entry) for entry in registry['entries']]
    if len({row['key'] for row in rows}) != len(rows):
        raise ValueError('Native world registry duplicates a complete program identity')
    if args.list_outputs:
        paths = set()
        for row in rows:
            for wrapper in VERTEX_WRAPPERS:
                suffix = '' if wrapper == 'world' else '-'+wrapper
                paths.add(str(args.shader_output_dir / 'native_world' / ('v-'+row['vertex_key']+suffix+'.vert.spv')))
            paths.add(str(args.shader_output_dir / 'native_world' / ('f-'+row['fragment_key']+'.frag.spv')))
        print(';'.join(sorted(paths)))
        return
    if any(value is None for value in (args.header, args.manifest, args.compiler, args.optimizer, args.validator)):
        parser.error('Compilation requires header, manifest, compiler, optimizer and validator')
    private = args.header.parent / 'compiled';private.mkdir(parents=True, exist_ok=True)
    output = args.shader_output_dir / 'native_world';output.mkdir(parents=True, exist_ok=True)
    stages, entries, pins = {}, [], {}
    for path in sorted(args.shader_root.rglob('*.glsl')):
        pins[path.relative_to(args.shader_root).as_posix()] = sha(path.read_bytes())
    for relative in (*VERTEX_WRAPPERS.values(), 'world.frag'):
        pins[relative] = sha((args.shader_root/relative).read_bytes())
    for row in rows:
        variants = [('vert', row['vertex_key'] + ('' if wrapper == 'world' else '-'+wrapper), wrapper)
                    for wrapper in VERTEX_WRAPPERS]
        variants.append(('frag', row['fragment_key'], None))
        for stage, key, wrapper in variants:
            identity = (stage, key)
            if identity in stages:
                continue
            if stage == 'vert':
                source, metadata = vertex_source(args.shader_root, row, wrapper)
            else:
                metadata = lower_fragment_program(row['controls'])
                source = build_fragment_source((args.shader_root/'world.frag').read_text(), metadata)
            stem = ('v-' if stage == 'vert' else 'f-') + key
            source_path = private / (stem + '.' + stage)
            raw_spv = private / (stem + '.unoptimized.spv')
            result_path = output / (stem + '.' + stage + '.spv')
            atomic(source_path, source.encode())
            run([args.compiler, '-V', '--target-env', 'vulkan1.3', '-S', stage,
                 '-I'+str(args.shader_root), source_path, '-o', raw_spv])
            run([args.optimizer, '--target-env=vulkan1.3', '--preserve-interface', '-O', raw_spv, '-o', result_path])
            run([args.validator, '--target-env', 'vulkan1.3', result_path])
            data = result_path.read_bytes()
            inputs = spirv_interface_locations(data,1)
            if stage == 'vert':
                expected = sorted([*range(9),11,12,13] + ([9,10] if row['extended'] else []))
                if inputs != expected:
                    raise ValueError('Native world compiled input ABI differs from its complete Vulkan vertex declaration')
            stages[identity] = dict(path='native_world/'+result_path.name, sha256=sha(data),
                bytes=len(data), source_sha256=sha(source.encode()), contract=spirv_contract(data, stage),
                input_locations=inputs,
                output_locations=spirv_interface_locations(data,3))
        entries.append(row)
    header = ['#pragma once', '#include "halo2/native_world_kernel.hpp"', '#include <array>', '#include <string_view>',
              'namespace halo2::generated_native_world_kernels {',
              'using namespace std::string_view_literals;']
    programs = {}
    for index, row in enumerate(entries):
        if row['program'] not in programs:
            programs[row['program']] = 'vertex_'+str(index)
            header.append(literal_array(programs[row['program']], 'std::uint8_t', row['program'], lambda x: str(x)))
    header += [f'inline constexpr std::array<NativeWorldKernelEntry,{len(entries)}> entries{{{{']
    for index, row in enumerate(entries):
        vertex = stages[('vert', row['vertex_key'])]['path'];fragment = stages[('frag', row['fragment_key'])]['path']
        normal = stages[('vert', row['vertex_key']+'-normal')]['path']
        utility = stages[('vert', row['vertex_key']+'-normal-utility')]['path']
        source_depth = stages[('vert', row['vertex_key']+'-source-depth')]['path']
        metadata = lower_vertex_program(row['instructions'],declaration=row['declaration'],
                                        extended_vertex_bank=row['extended'])
        header += [f'    {{{programs[row["program"]]}, {{{",".join(str(x)+"u" for x in row["controls"])}}}, '
                   f'{{{",".join(str(x)+"u" for x in row["declaration"])}}}, '
                   f'{str(row["extended"]).lower()}, "{vertex}"sv, "{fragment}"sv, "{normal}"sv, "{utility}"sv, '
                   f'{metadata["default_texture_w_mask"]}u,{str(metadata["relative_constants"]).lower()}, "{source_depth}"sv}},']
    header += ['}};', '} // namespace halo2::generated_native_world_kernels', '']
    report = dict(schema='HALO2_NATIVE_WORLD_KERNEL_COMPILED_V1', abi=1, target='vulkan1.3',
        registry_sha256=sha(registry_raw), source_sha256=pins, entries=len(entries),
        vertex_kernels=sum(stage == 'vert' for stage, _ in stages),
        fragment_kernels=sum(stage == 'frag' for stage, _ in stages),
        stages=list(stages.values()), runtime_compiler=False, runtime_instruction_interpreter=False)
    report['preserve_shader_interface'] = True
    position_header, position_report = build_position_header(registry)
    atomic(args.cpu_header or args.header.parent/'native_position_kernels.generated.hpp',position_header.encode())
    report['native_cpu_position_kernels'] = position_report
    atomic(args.header, '\n'.join(header).encode())
    atomic(args.manifest, (json.dumps(report, indent=2, sort_keys=True)+'\n').encode())
    # These are all declared CMake OUTPUTs, with the catalog header first.
    # An identical successful revalidation must finish newer than its changed
    # inputs, or Make runs the entire kernel bank again for every target.
    # Preserve unchanged header bytes/inodes; refresh completion timestamps
    # only after every shader, CPU kernel, header and manifest has succeeded.
    completed_ns = time.time_ns()
    completed_paths = [args.header, args.cpu_header or args.header.parent/'native_position_kernels.generated.hpp',
                       args.manifest, *(args.shader_output_dir/row['path'] for row in stages.values())]
    for path in completed_paths:
        os.utime(path, ns=(completed_ns, completed_ns))
    print(f'Native water/sky kernels: {len(entries)} exact identities, '+
          f'{report["vertex_kernels"]} vertex and {report["fragment_kernels"]} fragment kernels')


if __name__ == '__main__':
    main()
