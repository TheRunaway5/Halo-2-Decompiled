#!/usr/bin/env python3
"""Pin the real vision/perception owner graph without executing gameplay."""
import argparse, hashlib, json, os
from pathlib import Path
from capstone.x86 import X86_OP_IMM
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ENTRIES = {2547776: 'vision/perception update parent', 2725984: 'actual vision dependency', 2517472: 'actual tracking dependency', 2519968: 'native perception position', 199632: 'actual RNG dependency', 2554176: 'actual vision dependency', 770560: 'root-owned object-motion dependency', 842944: 'actual object visibility dependency', 2547568: 'actual vision dependency', 2520352: 'native perception lookup', 769536: 'native object point', 2551712: 'actual scene-query dependency', 1122256: 'actual scene-query dependency', 2552416: 'actual scene-query dependency', 2551136: 'actual scene-query dependency', 2549328: 'actual vision state dependency', 2554432: 'native campaign score', 2512416: 'actual visibility/reaction policy dependency', 2551536: 'actual scene-query admission dependency', 2018496: 'actual inherited character visibility dependency', 2112160: 'actual look/action publication dependency', 1901568: 'actual scene/raycast dependency', 2566512: 'actual actor/clump reaction dependency'}

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
