#!/usr/bin/env python3
"""Build the compact immutable raw-vertex dependency table without SPIR-V work."""
import argparse
import hashlib
import json
from pathlib import Path

from compile_native_world_kernels import atomic
from native_cpu_vertex_kernel import _program


def values(value, limit, name, length=None):
    if (not isinstance(value, list) or (length is not None and len(value) != length) or
            any(type(item) is not int or not 0 <= item <= limit for item in value)):
        raise ValueError('Raw native metadata has invalid '+name)
    return value


def rows(catalogs, programs):
    sources = {}
    for row in programs['cpu_vertex_programs']:
        raw, _ = _program(row);digest = hashlib.sha256(raw).hexdigest()
        if type(row.get('relative_constants')) is not bool:
            raise ValueError('Raw native metadata has no explicit source relative-constant flag')
        sources[digest] = (len(raw), row['relative_constants'])
    output = [];seen = set()
    for path in catalogs:
        catalog = json.loads(path.read_bytes())
        if (catalog.get('schema') != 'halo2-native-menu-world-vertex-source-v2' or
                catalog.get('abi') not in (2, 3, 4) or catalog.get('target') != 'vulkan1.3'):
            raise ValueError('Raw native metadata has an unknown catalog contract')
        for row in catalog['entries']:
            classification = row['class_index'];digest = row['program_sha256'];extent = row['program_bytes']
            if (type(classification) is not int or classification not in range(7) or digest not in sources or
                    type(extent) is not int or extent != sources[digest][0]):
                raise ValueError('Raw native metadata lacks its complete original program identity')
            semantics = values(row['semantics'], 65535, 'ordered declaration')
            if not 1 <= len(semantics) <= 16:
                raise ValueError('Raw native metadata declaration has an invalid extent')
            inputs = values(row['consumed_inputs'], 15, 'input registers')
            constants = values(row['consumed_constants'], 191, 'constant registers')
            lanes = values(row['consumed_input_components'], 15, 'input components', 16)
            default_w = row['default_texture_w_mask']
            if inputs != sorted(set(inputs)) or constants != sorted(set(constants)):
                raise ValueError('Raw native metadata dependency sets are not canonical')
            if any(mask and index not in inputs for index, mask in enumerate(lanes)):
                raise ValueError('Raw native metadata lanes have no consumed input owner')
            if type(default_w) is not int or not 0 <= default_w <= 15:
                raise ValueError('Raw native metadata has an invalid default texture W mask')
            key = (classification, digest, extent, tuple(semantics))
            if key in seen:
                raise ValueError('Raw native metadata duplicates a complete declaration/program identity')
            seen.add(key)
            output.append({**row, 'relative_constants': sources[digest][1]})
    if not output:
        raise ValueError('Raw native metadata contains no complete source identities')
    return sorted(output, key=lambda row: (row['class_index'], row['program_sha256'], row['semantics']))


def header(records):
    result = ['#pragma once', '#include "halo2/menu_world_vertex_catalog.hpp"', '#include <array>',
        'namespace halo2::generated_menu_world_vertex_metadata {']
    for index, row in enumerate(records):
        for field, kind in (('semantics', 'std::uint16_t'), ('consumed_inputs', 'unsigned'), ('consumed_constants', 'unsigned')):
            data = row[field]
            result.append(f'inline constexpr std::array<{kind},{len(data)}> row_{index}_{field}{{{{'+','.join(str(value)+'u' for value in data)+'}};')
    result.append(f'inline constexpr std::array<MenuWorldVertexMetadataEntry,{len(records)}> entries{{{{')
    for index, row in enumerate(records):
        lanes = ','.join(str(value)+'u' for value in row['consumed_input_components'])
        result.append('    {'+str(row['class_index'])+'u,"'+row['program_sha256']+'",'+str(row['program_bytes'])+'u,'+
            ','.join(f'row_{index}_{field}' for field in ('semantics', 'consumed_inputs', 'consumed_constants'))+
            ',{{'+lanes+'}},'+str(row['default_texture_w_mask'])+'u,'+str(row['relative_constants']).lower()+'},')
    return '\n'.join([*result, '}};', '}', ''])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sources', type=Path, action='append', required=True)
    parser.add_argument('--programs', type=Path, required=True)
    parser.add_argument('--header', type=Path, required=True)
    parser.add_argument('--manifest', type=Path)
    args = parser.parse_args()
    records = rows(args.sources, json.loads(args.programs.read_bytes()))
    data = header(records).encode();atomic(args.header, data)
    if args.manifest:
        atomic(args.manifest, (json.dumps({'schema': 'HALO2_NATIVE_RAW_VERTEX_METADATA_V1', 'entries': len(records),
            'source_sha256': {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in [*args.sources, args.programs]},
            'header_sha256': hashlib.sha256(data).hexdigest(), 'header_bytes': len(data),
            'runtime_opcode_analysis': False}, indent=2)+'\n').encode())
    print('Native raw vertex metadata:', len(records), 'complete identities;', len(data), 'bytes')


if __name__ == '__main__':
    main()
