#!/usr/bin/env python3
"""Attach exact offline dependency analysis to immutable native source records."""
import argparse
import hashlib
import json
from pathlib import Path

from compile_native_world_kernels import atomic


def identity(row):
    return row['class_index'], row['program_sha256'], row['program_bytes'], tuple(row['semantics'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--analysis', type=Path, required=True)
    parser.add_argument('--sources', type=Path, action='append', required=True)
    args = parser.parse_args()
    raw = args.analysis.read_bytes();analysis = json.loads(raw)
    if analysis.get('schema') != 'HALO2_RAW_VERTEX_ANALYSIS_V1':
        raise ValueError('Unknown native vertex dependency analysis')
    rows = {}
    for row in analysis['entries']:
        key = identity(row)
        if key in rows:
            raise ValueError('Offline native metadata duplicates a complete identity')
        rows[key] = row
    updates = []
    for path in args.sources:
        catalog = json.loads(path.read_bytes())
        for row in catalog['entries']:
            source = rows[identity(row)]
            if row['consumed_inputs'] != source['consumed_inputs'] or row['consumed_constants'] != source['consumed_constants']:
                raise ValueError('Offline native dependency set differs from compiled source record')
            shader = path.parent/row['source']
            if hashlib.sha256(shader.read_bytes()).hexdigest() != row['source_sha256']:
                raise ValueError('Native source GLSL differs from immutable identity')
            row['consumed_input_components'] = source['consumed_input_components']
            row['default_texture_w_mask'] = source['default_texture_w_mask']
        catalog['offline_metadata_analysis_sha256'] = hashlib.sha256(raw).hexdigest()
        updates.append((path, (json.dumps(catalog, indent=2, sort_keys=True)+'\n').encode()))
    for path, data in updates:
        atomic(path, data)
    print('Updated immutable native vertex metadata:', len(rows), 'complete identities')


if __name__ == '__main__':
    main()
