#!/usr/bin/env python3
"""Pin the real perception-score caller and policy graph without executing gameplay."""
import argparse, hashlib, json, os
from pathlib import Path
from capstone.x86 import X86_OP_IMM
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ENTRIES = {0x26fa40:'perception score',0x266d30:'actual perception update caller',0x26e040:'actual vision/update caller',0x160a60:'real multiplayer virtual team relation dispatch'}

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
    score = {row['pc']: row for row in owners['0x26fa40']['listing']}
    assert score['0x26faec']['operands'] == 'word ptr [ebp + 0x24], cx'
    assert score['0x26faf0']['operands'] == '0x26ffec'
    assert score['0x26fd19']['operands'] == 'cx, word ptr [ebp + 0x24]'
    assert score['0x26fd21']['operands'] == '0x26fd81'
    assert score['0x26fe52']['operands'] == 'ecx, 1'
    assert score['0x26fe7c']['operands'] == '0xc1e90'
    assert score['0x26fe8c']['operands'] == 'ecx, 2'
    assert score['0x26fe94']['operands'] == '0x160a60'
    report['ordinary_readonly_private_profile'] = dict(
        unreachable_site='0x26fd81',
        invariant='26FAEC/26FAF0 admits signed property+24>0; the same WORD is tested at26FD19/26FD21 after only retained-read-only dependencies. No serial retained write can make it<1.',
        dependency_proof='analysis/campaign-native-kernels-20261028/ai-tracking-update/policy-component-proof.json',
        asynchronous_external_memory_mutation_admitted=False)
    report['campaign_policy_profile'] = dict(
        selector='read(read(4F3218)+8)',campaign_value=1,
        original_bitset_call='26FE7C ->C1E90(words=read(5020C4)+C4,index=signed_team*16+1)',
        multiplayer_value=2,required_owner_call='26FE94 ->160A60',
        required_owner_source_continuations=['26FE99','26FE9C'],
        guessed_virtual_policy_result=False)
    report['actual_callers'] = []
    for entry in ('0x266d30','0x26e040'):
        listing = owners[entry]['listing']
        for index, row in enumerate(listing):
            if row['operation'] == 'call' and row['operands'] == '0x26fa40':
                continuation = listing[index+1]
                assert continuation['operation'] == 'movss' and continuation['operands'].endswith(', xmm0')
                report['actual_callers'].append(dict(entry=entry,call=row,
                    argument_and_continuation_listing=listing[max(0,index-6):index+11],
                    immediate_typed_result='scalar XMM0 bits ->actual tracked property+3C',
                    broader_parent_transport_qualified=False))
    assert len(report['actual_callers']) == 2
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: dict(sites=value['sites'], calls=value['calls'])
                      for key, value in owners.items()}, indent=2))

if __name__ == '__main__':
    main()
