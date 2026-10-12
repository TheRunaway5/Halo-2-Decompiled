#!/usr/bin/env python3
"""Derived exact-rational audit of observed original131ED0 results.

This does not execute original/native instructions or generate oracle expected
outputs. It checks captured x87 products/integers against the declared input
words, explicit rounding and signed32 bounds, then checks the source byte mask.
"""
import argparse
from fractions import Fraction
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fp80(row):
    field = row['fp80_top_exponent']
    exponent = (field & 0x7fff or 1) - 16383 - 63
    value = Fraction(row['fp80_top_mantissa'])
    value = value * 2**exponent if exponent >= 0 else value / 2**-exponent
    return -value if field & 0x8000 else value


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture', type=Path, default=ROOT / 'tests/fixtures/state_rgb_pack_original.json')
    ap.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/game-variants/state-rgb-pack-20261011/audit.json')
    args = ap.parse_args()
    raw = args.fixture.read_bytes()
    fixture = json.loads(raw)
    counts = dict(cases=0, products=0, conversions=0, conversion_overflow=0, rounded_byte_wrap=0)
    overflow_by_control = {str(control): 0 for control in (0x027f, 0x067f, 0x0a7f, 0x0e7f)}
    for case in fixture['cases']:
        channels = [None] * 3
        for row in case['fistp_observations']:
            word = case['rgb_bits'][row['channel_index']]
            scalar = struct.unpack('<f', struct.pack('<I', word))[0]
            product = Fraction.from_float(scalar) * 255
            assert product == fp80(row)
            assert row['fpcw'] == case['fpcw']
            mode = case['fpcw'] >> 10 & 3
            rounded = (round(product), product.numerator // product.denominator,
                       -(-product.numerator // product.denominator), int(product))[mode]
            if rounded < -(2**31) or rounded > 2**31 - 1:
                integer = -(2**31)
                counts['conversion_overflow'] += 1
                overflow_by_control[str(case['fpcw'])] += 1
            else:
                integer = rounded
                if integer < 0 or integer > 255:
                    counts['rounded_byte_wrap'] += 1
            assert row['produced_integer'] == integer
            assert row['produced_bits'] == integer & 0xffffffff
            channels[row['channel_index']] = integer & 255
            counts['products'] += 1
            counts['conversions'] += 1
        assert case['packed_rgb'] == channels[0] << 16 | channels[1] << 8 | channels[2]
        counts['cases'] += 1
    report = dict(schema='state-rgb-pack-derived-audit-v1', boundary=__doc__,
                  fixture_sha256=hashlib.sha256(raw).hexdigest(), auditor_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  counts=counts, overflow_by_control=overflow_by_control,
                  all_products_conversions_and_masks_match=True,
                  limits=['Audit of original observed data, not an independent original/native execution.',
                          'No architectural exception-status claim is made; the oracle records its emulator FPSW limitation.'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(counts, indent=2))


if __name__ == '__main__':
    main()
