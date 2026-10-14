#!/usr/bin/env python3
"""Preserve already translated port GLSL for asset-independent builds.

Offline maintenance only: inputs are the existing native vertex catalog and
particle generator outputs. Original tokens/maps stay private. No shaders
from MCC are accepted or translated by this tool.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct

from compile_menu_world_vertex_catalog import SCHEMA, PORT_SCHEMA, ABI, local


def sha(data):
    return hashlib.sha256(data).hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n')


def export(catalog, particles, output):
    source = json.loads(catalog.read_bytes())
    if source['schema'] != SCHEMA or source['abi'] != ABI or source['target'] != 'vulkan1.3':
        raise ValueError('expected the existing native source catalog')
    output.mkdir(parents=True, exist_ok=False)
    menu = output / 'menu'
    menu.mkdir()
    entries = []
    for entry in source['entries']:
        program = local(catalog.parent, entry['program']).read_bytes()
        if program[:2] != b'\x78\x20' or len(program) != 4 + struct.unpack_from('<H', program, 2)[0] * 16:
            raise ValueError('original vertex program extent/header differs')
        glsl = local(catalog.parent, entry['source'])
        shutil.copyfile(glsl, menu / glsl.name)
        entries.append({k: entry[k] for k in ('source', 'class_index', 'semantics', 'consumed_inputs', 'consumed_constants')} |
                       {'program_sha256': sha(program), 'program_bytes': len(program), 'source_sha256': sha(glsl.read_bytes())})
    save(menu / 'sources.json', {'schema': PORT_SCHEMA, 'abi': ABI, 'target': 'vulkan1.3',
         'cache_sha256': sha(Path(source['cache']).read_bytes()), 'vrtx_count': source['vrtx_count'],
         'entries': entries, 'rejected': source['rejected']})
    rows = []
    for name, classification in (('particle', 7), ('particle_directional', 8), ('particle_perpendicular', 9), ('particle_model', 1), ('particle_distortion', 7)):
        metadata = json.loads((particles / (name + '_program.json')).read_bytes())
        if metadata['classification'] != classification or metadata['expected_vertices_embedded']:
            raise ValueError('particle source metadata differs')
        if metadata.get('variant', 'standard') != ('distortion' if name == 'particle_distortion' else 'standard'):
            raise ValueError('particle source variant differs')
        glsl = particles / (name + '_program.glsl')
        if sha(glsl.read_bytes()) != metadata['source_sha256']:
            raise ValueError('particle GLSL differs from its existing generator output')
        header = (particles / (name + '_program.hpp')).read_text()
        match = re.fullmatch(r'#pragma once\n#include <cstdint>\nnamespace halo2 \{\ninline constexpr std::uint64_t ' + name + r'_program_hash=(0x[0-9a-f]+)ull;\n\}\n', header)
        if not match:
            raise ValueError('particle native program identity header differs')
        shutil.copyfile(glsl, output / glsl.name)
        rows.append(metadata | {'source': glsl.name, 'program_fnv64': match[1]})
    save(output / 'particles.json', {'schema': 'H2_NATIVE_PARTICLE_SOURCES_V1', 'entries': rows})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--catalog', type=Path, required=True)
    parser.add_argument('--particle-directory', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    export(args.catalog, args.particle_directory, args.output)


if __name__ == '__main__':
    main()
