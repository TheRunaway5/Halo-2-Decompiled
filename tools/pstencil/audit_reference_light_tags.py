#!/usr/bin/env python3
"""Decode original loose light definitions with the matching primary tool schema.

Authoring fields and constructor read offsets are retained separately from
per-frame selection. This does not assert current-tag compatibility, current
GPU implementation, or an original framebuffer match.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise RuntimeError('Refusing to replace retained reference evidence')
    path = ROOT / 'analysis/pstencil/2003-presentation/decode_passes.py'
    spec = importlib.util.spec_from_file_location('primary_pass_schema', path)
    schema = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(schema)
    digest = hashlib.sha256(schema.TOOL.read_bytes()).hexdigest()
    if digest != '043fd958e12bbacb76aadc5c589dc3f2fc2586dfec80bbd4b6fd8d8b426b66d5':
        raise RuntimeError('Matching original tool differs')
    definition = schema.schema(0x7e2d58)
    if definition['name'] != 'light' or definition['size'] != 0x260:
        raise RuntimeError('Original light schema differs')
    expected = {0xdc: 'specular mask', 0x174: 'falloff function',
                0x176: 'diffuse contrast', 0x178: 'specular contrast',
                0x17a: 'falloff geometry'}
    selectors = []
    for offset, name in expected.items():
        field = next(f for f in definition['fields'] if f['offset'] == offset and f['kind'] == 9)
        if field['name'] != name:
            raise RuntimeError('Literal original light selector role differs')
        selectors.append(dict(offset=hex(offset), name=name,
            schema_address=field['schema_address'],
            values=[schema.enum(field['definition'], i)
                    for i in range(schema.u(field['definition']))]))
    rows, unresolved = [], []
    for path in sorted((schema.BUILD / 'tools/tags').rglob('*.light')):
        try:
            reader = schema.Reader(path)
            decoded = reader.block(0x7e2d58, 1)
            if reader.position != len(reader.data):
                raise RuntimeError(f'Consumed {reader.position} of {len(reader.data)} bytes')
            if struct.unpack_from('<4I', reader.data, 64) != (0x74626664, 0, 1, 0x260):
                raise RuntimeError('Original serialized root declaration differs')
            rows.append(dict(path=str(path), sha256=hashlib.sha256(reader.data).hexdigest(),
                             bytes=len(reader.data), fully_consumed=True, rows=decoded))
        except (AssertionError, ValueError, RuntimeError, StopIteration) as error:
            unresolved.append(dict(path=str(path), reason=str(error)))
    if not rows:
        raise RuntimeError('No complete original loose light definitions')
    result = dict(scope=__doc__, tool=str(schema.TOOL), tool_sha256=digest,
        root_schema='007e2d58', root_size=0x260, selectors=selectors,
        schemas=list(schema.SCHEMAS.values()), tags=rows, unresolved=unresolved,
        full_presentation_verified=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(complete=len(rows), unresolved=len(unresolved), selectors=selectors)))


if __name__ == '__main__':
    main()
