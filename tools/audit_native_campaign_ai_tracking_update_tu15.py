#!/usr/bin/env python3
"""Pin the real perception-update owner graph without executing gameplay."""
import argparse, hashlib, json, os
from pathlib import Path
from capstone.x86 import X86_OP_IMM
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ENTRIES = {0x26e040:'broad vision/update frontier',0x264b50: 'property visibility/chase update',0x26fa40: 'perception tracking eligibility',0x2676c0: 'tracking extension target predicate',0x1ed240: 'actor authored character policy',0x27c710: 'existing authored character/order policy',0x2653c0:'264B50 direct dependency',0x264e30:'264B50 direct dependency',0x2649f0:'264B50 direct dependency',0x1ed450:'existing character perception policy',0x1ea2e0:'26FA40 direct dependency',0x1ed4f0:'26FA40 direct dependency',0x27c7e0:'26FA40 direct dependency',0x1c1d70:'26FA40 direct dependency',0x1ea1d0:'actor behavior stack lookup',0x1ed530:'character inherited policy block'}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    owners = {}
    for entry, role in ENTRIES.items():
        body = _trace(code, entry, set(ENTRIES))
        listing, calls = [], []
        for pc in sorted(body.code):
            instruction = code.at(pc)
            row = dict(pc=hex(pc), bytes=instruction.bytes.hex(),
                       operation=instruction.mnemonic, operands=instruction.op_str)
            listing.append(row)
            if instruction.mnemonic == 'call':
                calls.append(dict(site=hex(pc), target=instruction.op_str,
                                  direct_target=(hex(instruction.operands[0].imm)
                                      if instruction.operands[0].type == X86_OP_IMM else None)))
        owners[hex(entry)] = dict(role=role, sites=len(listing), calls=calls,
            listing_sha256=hashlib.sha256(json.dumps(listing, sort_keys=True).encode()).hexdigest(),
            listing=listing)
    report = dict(passed=True, original_sha256=NEW_PIN, owners=owners,
                  unit_action_begin_closure=False, ai_set_orders_closed=False,
                  gameplay_execution=False, retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: dict(sites=value['sites'], calls=value['calls'])
                      for key, value in owners.items()}, indent=2))

if __name__ == '__main__':
    main()
