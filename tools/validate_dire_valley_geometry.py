#!/usr/bin/env python3
"""Require the compiler's complete BSP resource graph after native publication."""
import argparse
import hashlib
import json
from pathlib import Path

from mcc_content import MccCache


def geometry(cache):
    bsp = next(t for t in cache.tags.values() if t['group'] == 'sbsp')
    result = []
    for kind, offset, stride in [('cluster', 156, 176), ('definition', 312, 200)]:
        for ordinal, section in enumerate(cache.block(bsp['offset'] + offset, stride)):
            if not cache.u16(section):
                continue
            pointer, header = cache.u32(section + 40), cache.u32(section + 48)
            base = pointer + header + 8
            resources = []
            for row in cache.block(section + 56, 16):
                start, length = base + cache.u32(row + 12), cache.u32(row + 8)
                resources.append(dict(type=cache.u16(row + 4), channel=cache.u16(row + 6),
                    bytes=length, sha256=hashlib.sha256(cache.data[start:start + length]).hexdigest()))
            result.append(dict(kind=kind, ordinal=ordinal, vertices=cache.u16(section),
                triangles=cache.u16(section + 2), subparts=cache.u16(section + 36),
                header_sha256=hashlib.sha256(cache.data[pointer:base]).hexdigest(), resources=resources))
    return result


def validate(source, published):
    expected, actual = geometry(MccCache(source)), geometry(MccCache(published))
    if len(expected) != len(actual):
        raise ValueError('Native publication changed the compiler geometry section count')
    for before, after in zip(expected, actual):
        if before != after:
            raise ValueError('Native publication changed compiler geometry: '
                + before['kind'] + ' ' + str(before['ordinal']))
    return dict(compiler_geometry_preserved=True, sections=len(actual),
        resources=sum(len(row['resources']) for row in actual),
        subparts=sum(row['subparts'] for row in actual), section_hashes=actual)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--published', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    result = validate(args.source, args.published)
    args.report.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({key: value for key, value in result.items() if key != 'section_hashes'}))
