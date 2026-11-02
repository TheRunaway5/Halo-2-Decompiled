#!/usr/bin/env python3
"""Authenticate the remaining TU1.5 convex simplex/publication source frontier."""
import argparse
import hashlib
import json
import os
from pathlib import Path

from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from capstone.x86_const import X86_OP_IMM


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert os.environ.get('HALO2_TEST_SCRATCH'), 'Use tools/run_test.py'
    assert hashlib.file_digest(Path(NEW_DEFAULT).open('rb'), 'sha256').hexdigest() == NEW_PIN
    args.output.mkdir(parents=True, exist_ok=False)
    code = _Code(Xbe(NEW_DEFAULT), '.text')
    rows = []
    for owner in (0x30d830, 0x30de60):
        body = complete_body(code, owner)
        listing = ''.join(f'{pc:x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'
                          for pc in sorted(body.code))
        name = f'{owner:08x}.txt'
        (args.output / name).write_text(listing)
        calls = []
        for pc in sorted(body.code):
            instruction = code.at(pc)
            if instruction.mnemonic != 'call':
                continue
            op = instruction.operands[0]
            calls.append(dict(site=pc, assembly=instruction.op_str,
                              direct_owner=op.imm & 0xffffffff if op.type == X86_OP_IMM else None))
        rows.append(dict(owner=owner, sites=len(body.code), calls=calls, listing=name,
                         listing_sha256=hashlib.sha256(listing.encode()).hexdigest()))
    report = dict(passed=True, original_sha256=NEW_PIN, scope='Authenticated source inventory only; no native kernel or production admission qualification.',
                  owners=rows, retained_large_artifacts=[], owned_scratch=os.environ['HALO2_TEST_SCRATCH'],
                  temporary_allocated_bytes_before_cleanup=0,
                  script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output / 'report.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(rows, indent=2))


if __name__ == '__main__':
    main()
