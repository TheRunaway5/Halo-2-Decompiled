#!/usr/bin/env python3
"""Retain complete native VS/CPU sources and compact all-map census evidence."""
import argparse
from collections import Counter
import hashlib
import json
import lzma
from pathlib import Path

from compile_native_world_kernels import atomic, validate_entry
from native_cpu_vertex_kernel import _program


def project(corpus, source_reference):
    if (corpus.get('schema') != 'HALO2_NATIVE_WORLD_KERNEL_PROGRAMS_V1' or
            corpus.get('abi') != 1 or corpus.get('target') != 'vulkan1.3' or
            corpus.get('all_materials') is not True or not corpus.get('entries')):
        raise ValueError('Native world source projection requires the complete offline census')
    if any(row.get('complete') is not True for row in corpus.get('prepared_inventories', [])):
        raise ValueError('Native world source projection has incomplete map ownership')
    for row in corpus['entries']:
        validate_entry(row)
    for row in corpus.get('cpu_vertex_programs', []):
        _program(row)
    result = {key: corpus[key] for key in ('schema', 'abi', 'target', 'source_authority',
        'dynamic_color_words', 'all_materials', 'prepared_inventories', 'rejected')}
    aggregates = {}
    for kind in ('entries', 'cpu_vertex_programs'):
        totals = Counter();result[kind] = []
        for row in corpus.get(kind, []):
            evidence = row['source_owner_evidence']
            map_counts = evidence['map_counts']
            digest = evidence.get('sha256')
            if (type(evidence.get('count')) is not int or evidence['count'] <= 0 or
                    any(type(value) is not int or value < 0 for value in map_counts.values()) or
                    sum(map_counts.values()) != evidence['count'] or
                    len(row.get('aliases', [])) > evidence['count'] or
                    not isinstance(digest, str) or len(digest) != 64 or
                    any(char not in '0123456789abcdef' for char in digest)):
                raise ValueError('Native source owner evidence has inconsistent complete counts or identity')
            totals.update(evidence['map_counts'])
            result[kind].append({**row, 'source_owner_evidence': {
                **{key: value for key, value in evidence.items() if key != 'map_counts'},
                'complete_map_counts_in_source_census': True}})
        aggregates[kind] = totals
    result['maps'] = []
    for row in corpus.get('maps', []):
        identity = row['source']+':'+row['sha256']+':'+row['map']
        result['maps'].append({**{key: value for key, value in row.items() if key != 'particle_fragment_census'},
            'particle_fragment_census': {key: value for key, value in row.get('particle_fragment_census', {}).items() if key != 'rejected'},
            'native_world_tuple_owner_records': aggregates['entries'][identity],
            'native_cpu_vertex_owner_records': aggregates['cpu_vertex_programs'][identity]})
    result['source_census'] = {**source_reference, 'complete_maps': len(corpus.get('maps', [])),
        'fragment_control_identities_in_separate_projection': len(corpus.get('fragment_programs', [])),
        'metadata_rejection_count': len(corpus.get('metadata_rejected', [])),
        'metadata_rejection_classes': dict(Counter(row.get('kind', 'unknown')+': '+row.get('error', 'unknown')
            for row in corpus.get('metadata_rejected', []))),
        'non_world_input_owners': len(corpus.get('non_world_inputs', [])),
        'scope': 'Complete vertex/position compilation sources; unresolved source-owner evidence and per-identity map counts remain in the authenticated census archive'}
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.input.resolve() == args.output.resolve():
        parser.error('The complete offline census cannot be overwritten')
    encoded = args.input.read_bytes();compressed = args.input.suffix == '.xz'
    raw = lzma.decompress(encoded) if compressed else encoded
    try:
        reference_path = args.input.resolve().relative_to(Path(__file__).resolve().parents[1]).as_posix()
    except ValueError:
        reference_path = str(args.input.resolve())
    reference = {'path': reference_path, 'encoding': 'xz-compressed original JSON' if compressed else 'original JSON',
        'decoded_sha256': hashlib.sha256(raw).hexdigest(), 'decoded_bytes': len(raw)}
    if compressed:
        reference['archive_sha256'] = hashlib.sha256(encoded).hexdigest()
    result = project(json.loads(raw), reference)
    output = (json.dumps(result, indent=2, sort_keys=True)+'\n').encode()
    atomic(args.output, output)
    print(json.dumps({'world_tuple_identities': len(result['entries']), 'cpu_programs': len(result['cpu_vertex_programs']),
        'source_bytes': len(output), 'source_sha256': hashlib.sha256(output).hexdigest()}, sort_keys=True))


if __name__ == '__main__':
    main()
