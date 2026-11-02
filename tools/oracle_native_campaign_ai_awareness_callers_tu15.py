#!/usr/bin/env python3
"""Physical caller packets at original2AF0F0/2AF13F, without adapter input.

Start at each real ready-argument MOV, execute real PUSH/CALL and the complete
original gameplay body, then stop at the actual return before its caller XOR.
No callee return, instruction, GP result or floating-point outcome is supplied.
The canonical gameplay corpus remains immutable; no full-bank copy is retained.
"""
import argparse
import copy
import gc
import gzip
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

from capstone import CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_awareness_tu15 as gameplay
import validate_native_geometry_tu15 as geometry

ARGUMENTS = 0x1073000
sha = common.sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--fixture', type=Path,
                        default=ROOT / 'tests/fixtures/native_campaign_ai_awareness_tu15.json.gz')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    assert scratch.is_dir() and sha(NEW_DEFAULT) == NEW_PIN
    fixture_pin = sha(args.fixture)
    fixture = json.loads(gzip.decompress(args.fixture.read_bytes()))
    assert fixture['image_sha256'] == NEW_PIN
    assert fixture['backend'] == 'physical-unchanged-i386-offline'
    assert not fixture['instruction_patches'] and not fixture['supplied_original_outcomes']
    whole = [row for row in fixture['cases'] if row['completed'] and not row.get('fault')]
    selected = []
    names = set()

    def add(row):
        if row['name'] not in names:
            selected.append(row)
            names.add(row['name'])

    # The complete source caller argument kinds, not supplied iterator results.
    for first_kind in range(4):
        for second_kind in range(4):
            add(next(row for row in whole if row['entry'] == 0x27d7d0 and
                     row['first_ai'] >> 30 == first_kind and
                     row['second_reference'] >> 30 == second_kind))
        add(next(row for row in whole if row['entry'] == 0x27da40 and
                 row['first_ai'] >> 30 == first_kind))
    for name in ('twenty_real_clumps', 'object_twenty_real_clumps',
                 'live_top_0_depth_2', 'clock_alias_state_word',
                 'object_clock_alias_state_word', 'property_alias_actor_next',
                 'property_alias_clock_pointer'):
        add(next(row for row in whole if row['name'] == name))
    for depth in range(8):
        add(next(row for row in whole if row['name'] == f'live_x87_{depth}'))
    expected = {row['name']: row for row in selected}
    inputs = []
    for index, row in enumerate(selected):
        ai = row['entry'] == 0x27d7d0
        packet = struct.pack('<II', row['first_ai'], row['second_reference'])
        gp = list(row['gp'])
        gp[0] = ARGUMENTS
        inputs.append(dict(
            id=index, name=row['name'], entry=0x2af0e9 if ai else 0x2af139,
            gameplay_entry=row['entry'], call_instruction=0x2af0f0 if ai else 0x2af13f,
            return_pc=0x2af0f5 if ai else 0x2af144,
            first_ai=row['first_ai'], second_reference=row['second_reference'],
            gp=gp, eflags=0x206, before_fp=row['before_fp'], live_x87=row['live_x87'],
            input_changes=row.get('input_changes', []),
            banks=[dict(address=b['address'], before=b['before']) for b in row['banks']] +
                  [dict(address=ARGUMENTS, before=packet.hex())]))
    del whole, fixture
    gc.collect()
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    common.ENTRIES = gameplay.ENTRIES
    metadata = common.metadata(image)
    metadata['required_owners'] = {}
    registers = {name: n for n, name in enumerate(
        ('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi'))}
    prefixes = []
    for begin, stop in ((0x2af0e9, 0x2af0f5), (0x2af139, 0x2af144)):
        pc = begin
        instructions = []
        while pc < stop:
            instruction = code.at(pc)
            operands = []
            for op in instruction.operands:
                if op.type != X86_OP_MEM:
                    continue
                base, index = instruction.reg_name(op.mem.base), instruction.reg_name(op.mem.index)
                operands.append(dict(base=registers[base] if base else -1,
                                     index=registers[index] if index else -1,
                                     scale=op.mem.scale, displacement=op.mem.disp & 0xffffffff,
                                     bytes=op.size, read=bool(op.access & CS_AC_READ),
                                     write=bool(op.access & CS_AC_WRITE)))
            metadata['sites'].append(dict(instruction=pc, bytes=instruction.bytes.hex(), operands=operands))
            instructions.append(dict(pc=hex(pc), bytes=instruction.bytes.hex(),
                                     text=instruction.mnemonic + ' ' + instruction.op_str))
            pc += instruction.size
        assert pc == stop
        prefixes.append(dict(begin=hex(begin), stop=hex(stop), instructions=instructions))
    # Reuse only unchanged original code/declared input memory preparation.
    # Its temporary generic observer is replaced before any case execution.
    owned = gameplay.prepare(image, metadata, scratch)
    observer = ROOT / 'tools/observe_native_campaign_ai_awareness_callers_tu15.cpp'
    command = ['c++', '-std=c++20', '-O2', str(observer), '-o', str(owned / 'observer')]
    result = subprocess.run(command, text=True, capture_output=True)
    assert result.returncode == 0, result.stderr
    observed = geometry.physical(owned, inputs, 'actual-caller')
    packets = []
    for row in observed:
        original = expected[row['name']]
        assert row['completed'] and row['callee_entry'] is not None
        entry = row['callee_entry']
        ai = row['gameplay_entry'] == 0x27d7d0
        sp = row['gp'][4]
        words = list(struct.unpack('<' + ('III' if ai else 'II'), bytes.fromhex(entry['stack_hex'])))
        assert words == [row['return_pc']] + ([row['first_ai']] if ai else []) + [row['second_reference']]
        assert entry['gp'][4] == sp - (12 if ai else 8)
        assert entry['gp'][2 if ai else 0] == row['first_ai']
        assert entry['gp'][1] == row['second_reference']
        assert row['boundary_gp'][4] == sp
        assert entry['fp'] == row['before_fp']
        assert row['after_fp'] == original['after_fp']
        for reg in (3, 5, 6, 7):
            assert row['boundary_gp'][reg] == row['gp'][reg]
        actual_banks = {b['address']: b for b in row['banks']}
        assert all(actual_banks[b['address']]['after'] == b['after'] for b in original['banks'])
        assert actual_banks[ARGUMENTS]['before'] == actual_banks[ARGUMENTS]['after']
        root_reads = [read for read in row['reads'] if read['instruction'] not in
                      (0x2af0e9, 0x2af0ec, 0x2af139, 0x2af13c)]
        assert root_reads == original['reads']
        assert row['writes'] == original['writes']
        changed_gp = [reg for reg in (0, 1, 2) if row['boundary_gp'][reg] != entry['gp'][reg]]
        changed_xmm = [reg for reg in range(8) if row['returned_xmm'][reg] != entry['xmm'][reg]]
        xmm_before, xmm_after = entry['xmm'], row['returned_xmm']
        entry = dict(entry)
        entry.pop('xmm')
        packets.append(dict(
            canonical_case=row['name'], gameplay_entry=hex(row['gameplay_entry']),
            actual_call_instruction=hex(row['call_instruction']), actual_return_pc=hex(row['return_pc']),
            first_ai=row['first_ai'], second_reference=row['second_reference'],
            caller_ready_packet_address=ARGUMENTS, caller_ready_packet_hex=actual_banks[ARGUMENTS]['before'],
            initial_caller_gp=row['gp'], callee_entry=entry,
            returned_gp=row['boundary_gp'], returned_eflags=row['returned_eflags'],
            simd=dict(changed_registers=[dict(index=n, before=xmm_before[n], after=xmm_after[n])
                                         for n in changed_xmm],
                      unchanged_indices=[n for n in range(8) if n not in changed_xmm],
                      all128_bytes_before_sha256=hashlib.sha256(struct.pack('<32I', *sum(xmm_before, []))).hexdigest(),
                      all128_bytes_after_sha256=hashlib.sha256(struct.pack('<32I', *sum(xmm_after, []))).hexdigest()),
            before_fp=row['before_fp'], after_fp=row['after_fp'],
            live_x87_depth=len(row['live_x87']), live_x87_unchanged=row['live_x87_unchanged'],
            nonvolatile_gp_unchanged=True, returned_sp_equals_pre_push_caller_sp=True,
            actual_original_persistent_banks_equal_canonical=True,
            exact_root_reads_equal_canonical=True, exact_stores_equal_canonical=True,
            changed_volatile_gp_indices=changed_gp, changed_xmm_indices=changed_xmm,
            original_instructions=row['original_instructions'],
            ordered_root_read_count=len(root_reads), ordered_store_count=len(row['writes'])))
    assert sha(args.fixture) == fixture_pin
    continuation = ROOT / 'analysis/campaign-native-ai-awareness-20261029/hs-void-continuation.json'
    report = dict(
        passed=True, scope=__doc__, original_sha256=NEW_PIN,
        backend='physical-unchanged-i386-offline', instruction_patches=[],
        supplied_callee_returns=[], supplied_gameplay_results=[],
        canonical_fixture=dict(path=str(args.fixture.relative_to(ROOT)), bytes=args.fixture.stat().st_size,
                               sha256=fixture_pin, unchanged=True),
        cases=len(packets), actual_gameplay_calls=len(packets), packets=packets,
        source_prefix_instructions=prefixes,
        original_instructions=sum(row['original_instructions'] for row in observed),
        ordered_root_reads=sum(p['ordered_root_read_count'] for p in packets),
        ordered_stores=sum(p['ordered_store_count'] for p in packets),
        declared_live_abi=['actual callee arguments', 'real CALL return word',
                           'RET8/RET4 caller SP', 'EBX/EBP/ESI/EDI', 'older x87 values',
                           'CW/SW/MXCSR continuation'],
        volatile_retirement_scope='Actual root differences are recorded, not fabricated. Caller continuation liveness is authenticated by the separate primary receipt; no binding source was inspected.',
        continuation_receipt=dict(path=str(continuation.relative_to(ROOT)), sha256=sha(continuation)),
        source_hashes={str(Path(__file__).relative_to(ROOT)): sha(__file__),
                       str(observer.relative_to(ROOT)): sha(observer),
                       'tools/oracle_native_campaign_ai_awareness_tu15.py': sha(Path(gameplay.__file__))},
        scope_limits=['Starts at a genuine ready argument pointer, not at212010 decoder entry; no decoder outcome is supplied or claimed executed.',
                      'Stops after actual gameplay RET before caller XOR/publisher; whole HS evaluator execution is not claimed.',
                      'No native adapter comparison, runtime HS scheduling, FP authority owner or campaign/GPU acceptance.'],
        owned_scratch=str(scratch), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, separators=(',', ':')) + '\n')
    print(f'{len(packets)} actual original caller packets: held arguments, realRET/SP, nonvolatile/liveFP and canonical persistent effects passed')


if __name__ == '__main__':
    main()
