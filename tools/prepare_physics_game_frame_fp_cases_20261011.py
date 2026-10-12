#!/usr/bin/env python3
"""Declare FP entry variations; unchanged original CPU execution supplies results.

Only the actual retained ABI and explicitly declared FP fields are inputs.
This is a component-domain experiment, not evidence of retail caller controls.
The normal matrix preserves the captured SSE rounding/denormal controls; the
optional rotate experiment deliberately explores other SSE rounding modes and
can leave the retained data domain. An unretained original read is a failure,
never a zero-filled successful fixture.
"""
import argparse
import copy
import hashlib
import json
import zlib
from pathlib import Path


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--input', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--sse-rounding', choices=('preserve', 'rotate'), default='preserve')
    ap.add_argument('--checkpoint', type=Path, help='Supply exact captured entry page reached by original case5')
    a = ap.parse_args()
    assert sha(a.input) == '1960794b2ba2c03fe4da9911914cc28db881f5060b7788b94999f94bfd1a2219'
    j = json.loads(a.input.read_text())
    assert len(j['cases']) == 1 and j['cases'][0]['complete']
    case = j['cases'][0]
    cases = [case]
    for pc in (0, 2, 3):
        for rc in range(4):
            for flags in (0, 0x3f):
                c = copy.deepcopy(case)
                c['id'] = len(cases)
                abi = c['entry_abi']
                abi['x87_control_word'] = (abi['x87_control_word'] & ~0xf00) | (pc << 8) | (rc << 10)
                abi['x87_status'] = (abi['x87_status'] & ~0x3f) | flags
                if a.sse_rounding == 'rotate':
                    abi['mxcsr'] = (abi['mxcsr'] & ~0x603f) | (((rc + (flags != 0)) & 3) << 13) | flags
                else:
                    abi['mxcsr'] = (abi['mxcsr'] & ~0x3f) | flags
                c['declared_abi_variation'] = dict(x87_precision_bits=pc, x87_rounding_bits=rc,
                    x87_exception_flags=flags, mxcsr=abi['mxcsr'], natural_capture=False)
                cases.append(c)
    j['cases'] = cases
    j['declared_abi_variations_only'] = True
    j['input_original_fixture_sha256'] = sha(a.input)
    if a.sse_rounding == 'rotate':
        j['scope'] = 'One actual retained game-frame entry plus 24 explicitly varied masked x87/SSE controls and sticky flags. Initial memory remains original input. Template expected results are replaced by independently executing unchanged original instructions on the host CPU.'
    else:
        j['scope'] = 'One actual retained game-frame entry plus 24 explicitly varied masked x87 controls and x87/SSE sticky flags. Captured SSE rounding and denormal controls and all initial memory remain unchanged. Template expected results are replaced by independently executing unchanged original instructions on the host CPU. This does not establish arbitrary retail caller controls.'
    if a.checkpoint:
        assert sha(a.checkpoint) == '6a64d10d3ac19526287985c2495d9c7262e8a5a881392b529eeb2f9aa74f230e'
        address = 0x82d88000
        capture = json.loads(a.checkpoint.read_text())
        region = next(r for r in capture['captured_entry']['regions'] if r['start'] <= address < r['start'] + r['size'])
        compressed = a.checkpoint.parent / 'memory' / region['file']
        assert sha(compressed) == region['compressed_sha256']
        raw = zlib.decompress(compressed.read_bytes())
        assert len(raw) == region['size'] and hashlib.sha256(raw).hexdigest() == region['sha256']
        page = raw[address-region['start']:address-region['start']+4096]
        assert len(page) == 4096
        for c in cases:
            memory = {b['address']+n:v for b in c['initial_banks'] for n,v in enumerate(bytes.fromhex(b['hex']))}
            for n, v in enumerate(page):
                p = address + n
                if p in memory:
                    assert memory[p] == v, 'Captured entry and first-before input disagree'
                memory[p] = v
            banks = []
            for p, v in sorted(memory.items()):
                if banks and banks[-1][0] + len(banks[-1][1]) == p:
                    banks[-1][1].append(v)
                else:
                    banks.append((p, bytearray([v])))
            c['initial_banks'] = [dict(address=p, hex=data.hex()) for p, data in banks]
            c['declared_additional_source_input'] = dict(checkpoint_sha256=sha(a.checkpoint),
                address=address, size=4096, source_initial_region_sha256=region['sha256'],
                page_sha256=hashlib.sha256(page).hexdigest(),
                reason='Original hardware case5 reached byte82D88081 outside minimal base-case observed banks; exact additional entry memory, no result bytes.')
        j['scope'] += ' Exact captured entry page82D88000 supplies the additional source input reached in case5; this is never an expected-state page.'
    assert not a.output.exists(), 'Preserve prior experiment evidence'
    a.output.write_text(json.dumps(j, sort_keys=True, separators=(',', ':')) + '\n')
    print(json.dumps(dict(cases=len(cases), sha256=sha(a.output))))


if __name__ == '__main__':
    main()
