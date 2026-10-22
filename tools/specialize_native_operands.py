#!/usr/bin/env python3
"""Fold literal executable operands using the canonical generated mapping.

Only a build-local copy changes. Dynamic/unmapped calls retain the original
checked function, and each replacement is recorded in a bounded receipt. The
original instruction order, state operations and service calls stay intact.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re

HEADER = 'std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image)'
MAPPING = re.compile(re.escape(HEADER) +
    r'\{if\(image==SourcePhysicsContactImage::MapPack11\)return p;switch\(p\)\{'
    r'(?P<cases>(?:case 0x[0-9a-f]+u:return 0x[0-9a-f]+u;)+)'
    r'default:throw std::runtime_error\("[^"\n]*"\);}}')
CASE = re.compile(r'case (0x[0-9a-f]+)u:return (0x[0-9a-f]+)u;')
CALL = re.compile(r'\bselected\(\s*(0x[0-9a-f]+)u\s*,\s*'
                  r'([A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*)\s*\)')
INSTRUCTION = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')
NON_CODE = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
CURSOR_STATE_HEADERS = ('physics/runtime/physics_contact_runtime_internal.hpp', 'physics/runtime/physics_world_runtime_internal.hpp',
    'physics/vehicles/physics_vehicle_runtime_internal.hpp', 'physics/vehicles/physics_vehicle_motion_internal.hpp',
    'physics/vehicles/physics_vehicle_motion_math_internal.hpp')


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def specialize(text):
    mapping = MAPPING.search(text)
    if not mapping or text.count(HEADER) != 1:
        raise ValueError('Unsupported canonical native operand mapping')
    pairs = [(int(a, 16), int(b, 16)) for a, b in CASE.findall(mapping['cases'])]
    addresses = dict(pairs)
    if len(addresses) != len(pairs):
        raise ValueError('Duplicate canonical native operand mapping')
    masked = NON_CODE.sub(lambda match: ' ' * len(match[0]), text)
    result, cursor, unknown = [], 0, 0
    counts = Counter()
    for call in CALL.finditer(masked):
        literal = int(call[1], 16)
        if literal not in addresses:
            unknown += 1
            continue
        translated = addresses[literal]
        replacement = (f'0x{literal:x}u' if literal == translated else
                       f'({call[2]}==SourcePhysicsContactImage::MapPack11?'
                       f'0x{literal:x}u:0x{translated:x}u)')
        result.extend((text[cursor:call.start()], replacement))
        cursor = call.end()
        counts[(literal, translated)] += 1
    result.append(text[cursor:])
    output = ''.join(result)
    cursor_calls = 0
    if any(f'#include "{header}"' in text for header in CURSOR_STATE_HEADERS):
        # These canonical states implement/inherit the same cursor contract.
        # Other generated modules keep their existing callback expression.
        masked = NON_CODE.sub(lambda match: ' ' * len(match[0]), output)
        pieces, cursor = [], 0
        for call in INSTRUCTION.finditer(masked):
            pieces.extend((output[cursor:call.start()], f's.instruction_literal({call[1]});'))
            cursor = call.end()
            cursor_calls += 1
        pieces.append(output[cursor:])
        output = ''.join(pieces)
    receipt = dict(schema='native-operand-specialization-v1',
        canonical_sha256=digest(text), specialized_sha256=digest(output),
        mapping_sha256=digest(mapping[0]), folded_calls=sum(counts.values()),
        unmapped_literal_calls=unknown,
        instruction_cursor_calls=cursor_calls,
        bindings=[dict(canonical=a, tu15=b, uses=n) for (a, b), n in sorted(counts.items())])
    return output, receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.source.resolve() == args.output.resolve():
        parser.error('Specialized operands must use a separate build-local output')
    output, receipt = specialize(args.source.read_text())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(output)
    args.output.with_suffix('.operands.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({key: value for key, value in receipt.items() if key != 'bindings'}))


if __name__ == '__main__':
    main()
