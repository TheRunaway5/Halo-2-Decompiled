#!/usr/bin/env python3
"""Execute whole original1AFA90 on every authored behavior-child list.

Each20-byte row is original engine metadata, not an actor outcome. Optional
24-byte property commands are declared component inputs. No instruction is
patched, no call return is supplied, and the original owns ordering, filtering
and the bounded output pointer list. Later1B0020 behavior callbacks are outside
this constructor comparison and require actual campaign qualification.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from export_ai_behavior_descriptor_data_tu15 import descriptor_inputs, child_selection_inputs
from oracle_campaign_ai_placement_settings_tu15 import GP

BASE, STOP, ENTRY = 0x2000000, 0x3000000, 0x1afa90
PROPERTY, COMMANDS, OUTPUT, STACK = BASE+0x1000, BASE+0x1100, BASE+0x4000, BASE+0xff00


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    ap.add_argument('--verify', action='store_true')
    args = ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        ap.error('Run through tools/run_test.py')
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    records, _, _ = descriptor_inputs(image)
    parents, ranges = child_selection_inputs(image, records)
    code = _Code(image, '.text'); body = _trace(code, ENTRY, {ENTRY})
    cases, covered = [], set()
    for parent in parents:
        for operation in (None, 0, 1, 2, 3, 4):
            g = Uc(UC_ARCH_X86, UC_MODE_32); g.mem_map(0x10000, 0x600000)
            for section in image.sections:
                g.mem_write(section.va, image.section_bytes(section))
            g.mem_map(BASE, 0x10000); g.mem_map(STOP, 4096)
            patches = []
            def put(address, value, size=4):
                raw = (value & ((1 << (size*8))-1)).to_bytes(size, 'little')
                g.mem_write(address, raw); patches.append([address, raw.hex()])
            if operation is not None:
                seed_child = parent['children'][0] if parent['children'] else parents[0]['children'][0]
                put(PROPERTY+4, COMMANDS); put(PROPERTY+8, 1, 2)
                put(COMMANDS, operation, 2)
                put(COMMANDS+2, seed_child['kind'], 2)
                embedded = image.read(seed_child['address'], 20)
                g.mem_write(COMMANDS+4, embedded); patches.append([COMMANDS+4, embedded.hex()])
            for at, value in ((STACK, STOP), (STACK+4, OUTPUT),
                    (STACK+8, 0 if operation is None else PROPERTY),
                    (STACK+12, parent['table']), (STACK+16, parent['count'])):
                put(at, value)
            g.mem_write(OUTPUT, bytes([0xa5])*200)
            initial = [0x11223344, 0x22334455, 0x33445566, 0x44556677,
                STACK, 0x55667788, 0x66778899, 0x778899aa]
            xmm = [[0x3f000000+i*4+lane for lane in range(4)] for i in range(8)]
            for reg, value in zip(GP, initial): g.reg_write(reg, value)
            for i, lanes in enumerate(xmm):
                g.reg_write(R.UC_X86_REG_XMM0+i, int.from_bytes(struct.pack('<4I', *lanes), 'little'))
            g.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
            g.reg_write(R.UC_X86_REG_FPCW, 0x27f); g.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
            trace, stores = [], []
            def instruction(_g, pc, size, _user):
                assert pc in body.code and bytes(g.mem_read(pc, size)) == image.read(pc, size), hex(pc)
                trace.append(pc); covered.add(pc)
            def store(_g, _kind, address, size, value, _user):
                assert BASE <= address < address+size <= BASE+0x10000, hex(address)
                stores.append([g.reg_read(R.UC_X86_REG_EIP), address,
                    bytes(g.mem_read(address, size)).hex(),
                    (value & ((1 << (size*8))-1)).to_bytes(size, 'little').hex()])
            g.hook_add(UC_HOOK_CODE, instruction); g.hook_add(UC_HOOK_MEM_WRITE, store)
            completed = True
            try:
                g.emu_start(ENTRY, STOP, count=10000)
            except UcError:
                # The original opcode2 command reads an existing child even
                # for an empty/null table. Retain this real failure as a
                # negative control; do not supply a row or a return value.
                assert parent['count'] == 0 and operation == 2 and g.reg_read(R.UC_X86_REG_EIP) == 0x1afb1c
                completed = False
            if completed:
                assert g.reg_read(R.UC_X86_REG_EIP) == STOP
                count = g.reg_read(R.UC_X86_REG_EAX) & 0xffff
                assert count <= 50
                for at in range(OUTPUT, OUTPUT+count*4, 4):
                    pointer = int.from_bytes(g.mem_read(at, 4), 'little')
                    assert pointer in {row['address'] for row in parent['children']} | {COMMANDS+4}, hex(pointer)
            cases.append(dict(parent=parent['address'], operation=operation, patches=patches,
                completed=completed, fault_pc=None if completed else g.reg_read(R.UC_X86_REG_EIP),
                gp=initial, xmm=xmm, final_gp=[g.reg_read(reg) for reg in GP],
                final_xmm=[list(struct.unpack('<4I', g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16, 'little'))) for i in range(8)],
                final_flags=g.reg_read(R.UC_X86_REG_EFLAGS), trace=trace, stores=stores,
                output=bytes(g.mem_read(OUTPUT, 200)).hex()))
    fixture = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, instruction_patches=[],
        supplied_dependency_returns=[], entry=ENTRY, native_component_banks=[[BASE, 0x10000]],
        output=OUTPUT, parents=parents, selection_ranges=[[at, image.read(at, n).hex()] for at, n in ranges], cases=cases)
    raw = (json.dumps(fixture, separators=(',', ':'))+'\n').encode()
    if args.verify or args.fixture.exists():
        assert args.fixture.read_bytes() == raw, 'Preserve previous independent original evidence'
    else:
        args.fixture.parent.mkdir(parents=True, exist_ok=True); args.fixture.write_bytes(raw)
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, cases=len(cases), parent_lists=len(parents),
        completed_cases=sum(row['completed'] for row in cases), original_missing_input_controls=sum(not row['completed'] for row in cases),
        authored_child_rows=sum(row['count'] for row in parents), complete_constructor_instructions=len(body.code),
        executed_constructor_instructions=len(covered), unexecuted_instructions=[pc for pc in body.code if pc not in covered],
        fixture_bytes=len(raw), fixture_sha256=hashlib.sha256(raw).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), runtime_reference_required=False,
        full_selector_or_decision_parity_verified=False)
    args.report.parent.mkdir(parents=True, exist_ok=True); args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report))


if __name__ == '__main__': main()
