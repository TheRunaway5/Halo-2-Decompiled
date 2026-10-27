#!/usr/bin/env python3
"""Audit actual renderer SPIR-V for executable Xbox instruction-bank reads.

This is a structural ABI check, not a source-name/substring check. It recognises
the complete source program-bank shapes wherever they occur in Uniform,
StorageBuffer, Private or Function storage, follows access-chain ownership and
checks actual memory reads. Retained ABI fields with no executable reads are
allowed. Packed dynamic colors are data; control words and vertex instructions
are not. A new shader ABI must extend this guard and its independent controls.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct

COLORS = frozenset(range(10, 26)) | {43, 44}
MAX_MODULE_BYTES = 32 * 1024 * 1024
MAX_REPORT_BYTES = 1024 * 1024
MAX_MODULES = 20000


class AuditError(ValueError):
    pass


def sha(data):
    return hashlib.sha256(data).hexdigest()


def instructions(data):
    if len(data) < 20 or len(data) % 4 or len(data) > MAX_MODULE_BYTES:
        raise AuditError('SPIR-V extent is invalid or exceeds its declared bound')
    words = struct.unpack('<' + 'I' * (len(data) // 4), data)
    if words[0] != 0x07230203 or not 0x10000 <= words[1] <= 0x10600 or words[4]:
        raise AuditError('SPIR-V header is invalid or unsupported')
    result = []
    at = 5
    while at < len(words):
        length, opcode = words[at] >> 16, words[at] & 65535
        if not length or at + length > len(words):
            raise AuditError('SPIR-V instruction escapes its module')
        result.append((opcode, words[at + 1:at + length]))
        at += length
    if not any(op == 15 for op, _ in result):
        raise AuditError('SPIR-V has no entry point')
    return result


def audit_spirv(data):
    ops = instructions(data)
    types, constants, variables, names = {}, {}, {}, {}
    for op, args in ops:
        if 19 <= op <= 39 and args:
            types[args[0]] = (op, args[1:])
        elif op == 43 and len(args) >= 3:
            constants[args[1]] = args[2] if len(args) == 3 else None
        elif op == 59 and len(args) >= 3:
            variables[args[1]] = args[0]
        elif op == 5 and len(args) >= 2:
            names[args[0]] = struct.pack('<' + 'I' * (len(args) - 1), *args[1:]).split(b'\0', 1)[0].decode('utf-8', 'replace')

    def bank_kind(type_id):
        op, args = types.get(type_id, (0, ()))
        if op != 28 or len(args) != 2:
            return None
        element, length = args[0], constants.get(args[1])
        eop, eargs = types.get(element, (0, ()))
        if eop == 23 and len(eargs) == 2 and eargs[1] == 4:
            scalar = types.get(eargs[0])
            if scalar == (21, (32, 0)):
                return ('vertex', 4) if length == 136 else ('pixel', 4) if length == 15 else None
        if (eop, eargs) == (21, (32, 0)):
            return ('vertex', 1) if length == 544 else ('pixel', 1) if length == 60 else None
        return None

    def has_bank(type_id, seen=None):
        seen = set() if seen is None else seen
        if type_id in seen:
            return False
        seen.add(type_id)
        if bank_kind(type_id):
            return True
        op, args = types.get(type_id, (0, ()))
        if op == 30:
            return any(has_bank(member, seen) for member in args)
        if op in (28, 29):
            return has_bank(args[0], seen)
        return False

    roots = {}
    for variable, pointer_type in variables.items():
        op, args = types.get(pointer_type, (0, ()))
        if op == 32 and len(args) == 2 and has_bank(args[1]):
            roots[variable] = args[1]
    # Pointer provenance is (root variable, source-constant indices). None means
    # an address is selected by runtime data, including specialization values.
    paths = {root: {(root, ())} for root in roots}
    for _ in range(len(ops) + 1):
        changed = False
        for op, args in ops:
            additions = set()
            if op in (65, 66, 67) and len(args) >= 3:
                for root, previous in paths.get(args[2], ()):
                    additions.add((root, previous + tuple(constants.get(i) for i in args[3:])))
            elif op == 83 and len(args) == 3:
                additions.update(paths.get(args[2], ()))
            elif op == 169 and len(args) == 5:
                additions.update(paths.get(args[3], ()))
                additions.update(paths.get(args[4], ()))
            elif op == 245 and len(args) >= 4:
                for incoming in args[2::2]:
                    additions.update(paths.get(incoming, ()))
            if additions:
                result = args[1]
                before = len(paths.get(result, ()))
                paths.setdefault(result, set()).update(additions)
                changed |= len(paths[result]) != before
        if not changed:
            break
    else:
        raise AuditError('SPIR-V pointer ownership did not reach a fixed point')

    def read_violation(root, indices):
        current = roots[root]
        remaining = list(indices)
        while True:
            bank = bank_kind(current)
            if bank:
                kind, width = bank
                if kind == 'vertex':
                    return 'vertex-instruction-bank-read'
                if not remaining or remaining[0] is None:
                    return 'pixel-control-bank-read'
                row = remaining.pop(0)
                if width == 1:
                    words = {row}
                elif remaining:
                    component = remaining.pop(0)
                    if component is None:
                        return 'pixel-control-bank-read'
                    words = {4 * row + component}
                else:
                    words = set(range(4 * row, 4 * row + 4))
                return None if words <= COLORS and not remaining else 'pixel-control-bank-read'
            op, args = types.get(current, (0, ()))
            if not remaining:
                return 'whole-instruction-bearing-value-read' if has_bank(current) else None
            index = remaining.pop(0)
            if op == 30:
                if index is None or not 0 <= index < len(args):
                    return 'unknown-instruction-bearing-member-read'
                current = args[index]
            elif op in (28, 29, 23, 24):
                current = args[0]
            else:
                raise AuditError('SPIR-V access chain is inconsistent with its reflected owner')

    violations = set()
    read_count = 0
    for op, args in ops:
        pointers = [args[2]] if op == 61 and len(args) >= 3 else [args[1]] if op in (63, 64) and len(args) >= 2 else []
        for pointer in pointers:
            for root, path in paths.get(pointer, ()):
                read_count += 1
                violation = read_violation(root, path)
                if violation:
                    violations.add((violation, root, path))
    rows = [{'kind': kind, 'owner_id': root, 'owner_debug_name': names.get(root),
             'constant_index_path': list(path)}
            for kind, root, path in sorted(violations, key=lambda row: (row[0], row[1], str(row[2])))]
    return {'passed': not rows, 'instruction_abi_owners': len(roots),
            'owned_memory_reads': read_count, 'violations': rows,
            'scope': 'Actual SPIR-V memory reads from complete source program ABI banks; no debug names required'}


def source_runtime_calls(root):
    """Reject shader execution/code generation in production source callers."""
    rows = []
    codegen = ('menu_world_vertex_source', 'menu_world_skinned_vertex_source',
               'menu_world_material_vertex_source', 'menu_world_renderer_vertex_source',
               'menu_world_vertex_shader_source', 'menu_world_material_vertex_shader_source',
               'menu_world_renderer_vertex_shader_source', 'menu_world_vertex_input_components',
               'menu_world_renderer_vertex_input_components', 'menu_world_vertex_default_texture_w_mask')
    for path in sorted((root / 'src').rglob('*.cpp')):
        source = path.read_text()
        # Blank comments and ordinary literals while preserving line numbers.
        clean = re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                       lambda match: '\n' * match.group().count('\n') + ' ', source, flags=re.S)
        names = ['execute_vertex_program']
        # This source is linked only to the offline codegen archive. The
        # production target-graph check separately enforces that boundary.
        if path.relative_to(root).as_posix() != 'src/ui/menu_world_vertex_source.cpp':
            names.extend(codegen)
        for match in re.finditer(r'\b('+'|'.join(names)+r')\s*\(', clean):
            rows.append({'path': path.relative_to(root).as_posix(),
                         'line': clean.count('\n', 0, match.start()) + 1,
                         'sha256': sha(path.read_bytes()), 'symbol': match.group(1),
                         'kind': 'cpu-vertex-program-execution' if match.group(1)=='execute_vertex_program'
                             else 'runtime-vertex-program-analysis-or-codegen'})
    return rows


def production_archive_violations(graph):
    """Follow actual CMake link edges, excluding build-only dependencies."""
    targets = {}
    for line in graph.read_text().splitlines():
        name, links, _dependencies, sources = line.split('|')
        targets[name] = (links.split(';'), sources.split(';'))
    if 'halo2_native' not in targets:
        raise AuditError('Production target graph lacks halo2_native')
    closure, pending = set(), ['halo2_native']
    while pending:
        name = pending.pop()
        if name in closure or name not in targets:
            continue
        closure.add(name)
        pending.extend(value.removeprefix('$<LINK_ONLY:').removesuffix('>')
                       for value in targets[name][0])
    forbidden = {'halo2_vertex_program_reference', 'halo2_vertex_program_codegen'}
    rows = [{'target':name,'kind':'offline-archive-linked-to-production'}
            for name in sorted(closure & forbidden)]
    suffixes = ('src/ui/menu_world_vertex_source.cpp',
                'tools/menu_world_vertex_catalog_codegen.cpp',
                'tests/reference/vertex_program_reference.cpp')
    for name in sorted(closure):
        for source in targets[name][1]:
            if any(source.replace('\\','/').endswith(suffix) for suffix in suffixes):
                rows.append({'target':name,'source':source,'kind':'offline-source-linked-to-production'})
    return rows


def direct_files(directory, inventory=None):
    directory = directory.resolve(strict=True)
    if not directory.is_dir():
        raise AuditError('Shader/runtime owner is not a directory')
    files = []
    if inventory is None:
        files = sorted(directory.rglob('*.spv'))
    else:
        if not isinstance(inventory, dict):
            raise AuditError('Runtime inventory is not an object')
        for name, expected in inventory.items():
            relative = Path(name)
            if relative.is_absolute() or '..' in relative.parts or relative.as_posix() != name:
                raise AuditError('Runtime inventory owner escapes its directory')
            if relative.suffix != '.spv':
                continue
            path = directory / relative
            if not path.is_file() or path.is_symlink() or path.resolve(strict=True) != path:
                raise AuditError('Runtime shader lost its direct file owner')
            data = path.read_bytes()
            if len(data) != expected['bytes'] or sha(data) != expected['sha256']:
                raise AuditError('Runtime shader differs from immutable inventory: ' + name)
            files.append(path)
        actual = {p.relative_to(directory).as_posix() for p in directory.rglob('*.spv')}
        declared = {p.relative_to(directory).as_posix() for p in files}
        if declared != actual:
            raise AuditError('Runtime contains an unlisted or missing SPIR-V module')
        files.sort()
    if not 1 <= len(files) <= MAX_MODULES:
        raise AuditError('Shader inventory is empty or exceeds its bound')
    for path in files:
        if path.is_symlink() or path.resolve(strict=True) != path:
            raise AuditError('Shader module is not a direct owned file')
        if path.stat().st_size > MAX_MODULE_BYTES:
            raise AuditError('Shader module exceeds its bound')
    return directory, files


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    owners = parser.add_mutually_exclusive_group(required=True)
    owners.add_argument('--runtime-dir', type=Path)
    owners.add_argument('--shader-dir', type=Path)
    parser.add_argument('--source-root', type=Path)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args(argv)
    scratch = os.environ.get('HALO2_TEST_SCRATCH')
    if not scratch or not Path(scratch).is_dir():
        parser.error('Run this audit through tools/run_test.py / tools/run_tests.py')
    if args.runtime_dir:
        owner = args.runtime_dir.resolve(strict=True)
        manifest = owner / 'runtime-files.json'
        if manifest.is_symlink() or manifest.stat().st_size > MAX_REPORT_BYTES:
            raise AuditError('Runtime inventory has no bounded direct owner')
        owner, files = direct_files(owner, json.loads(manifest.read_text()))
    else:
        owner, files = direct_files(args.shader_dir)
    report = {'schema': 'HALO2_RENDERER_NO_RUNTIME_SHADER_INTERPRETER_V1',
              'owner': str(owner), 'modules': [], 'cpu_runtime_calls': [],
              'scope': 'Complete supplied shader inventory and optional production CPU callers; not visual/precision parity'}
    for path in files:
        data = path.read_bytes()
        try:
            result = audit_spirv(data)
        except AuditError as error:
            result = {'passed': False, 'invalid_module': str(error)}
        report['modules'].append({'path': path.relative_to(owner).as_posix(),
                                  'bytes': len(data), 'sha256': sha(data), **result})
    if args.source_root:
        report['cpu_runtime_calls'] = source_runtime_calls(args.source_root.resolve(strict=True))
    report['module_count'] = len(report['modules'])
    report['rejected_modules'] = sum(not row['passed'] for row in report['modules'])
    report['passed'] = report['rejected_modules'] == 0 and not report['cpu_runtime_calls']
    report['scratch_owner'] = str(Path(scratch).resolve())
    encoded = (json.dumps(report, indent=2) + '\n').encode()
    if len(encoded) > MAX_REPORT_BYTES or args.report.is_symlink() or args.report.resolve() in files:
        raise AuditError('Report output lost its bounded independent owner')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_bytes(encoded)
    print(json.dumps({'passed': report['passed'], 'modules': report['module_count'],
                      'rejected_modules': report['rejected_modules'],
                      'cpu_runtime_calls': len(report['cpu_runtime_calls']),
                      'report': str(args.report)}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
