#!/usr/bin/env python3
"""Execute unchanged TU1.5 PCR tab-selection blocks with explicit widget data.

Stops before virtual focus and sound services. No executable instructions or
callees are replaced. This establishes the linked visible-tab selection only,
not widget focus side effects, sound playback, Live services or framebuffer.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_ESI

ENTRY = 0x2BDF46
FOCUS = 0x2BDF83
SOUND = 0x2BDF8E
OWNER, EVENT, TABS, STACK = 0x1000000, 0x1001000, 0x1002000, 0x100F000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert os.environ.get('HALO2_TEST_SCRATCH'), 'Run through tools/run_test.py'
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0x10000, 0x800000)
    for section in image.sections:
        machine.mem_write(section.va, image.section_bytes(section))
    machine.mem_map(OWNER, 0x10000)
    trace = []

    def hook(uc, address, size, _):
        if address in (FOCUS, SOUND):
            uc.emu_stop()
        else:
            trace.append(address)

    machine.hook_add(UC_HOOK_CODE, hook)

    def put(address, value):
        machine.mem_write(address, struct.pack('<I', value))

    cases = []
    for teams in (False, True):
        for pane in range(6):
            for direction, event_type in ((-1, 2), (1, 4)):
                machine.mem_write(OWNER, bytes(0x4000))
                for tab in range(6):
                    address = TABS + tab * 0x100
                    put(address + 0x18, address + 0x100 if tab < 5 else 0)
                    put(address + 0x1C, address - 0x100 if tab else 0)
                    machine.mem_write(address + 0x6E, bytes([int(teams or tab > 0)]))
                put(OWNER + 0x70, TABS + pane * 0x100)
                put(EVENT, event_type)
                put(STACK, 0)
                put(STACK + 4, EVENT)
                machine.reg_write(UC_X86_REG_ECX, OWNER)
                machine.reg_write(UC_X86_REG_ESP, STACK)
                trace.clear()
                machine.emu_start(ENTRY, ENTRY + 0x8A, count=1000)
                assert machine.reg_read(UC_X86_REG_EIP) in (FOCUS, SOUND)
                selected = (machine.reg_read(UC_X86_REG_ESI) - TABS) // 0x100
                assert 0 <= selected < 6 and (teams or selected > 0)
                cases.append(dict(pane=pane, direction=direction, teams=teams,
                                  selected=selected, instruction_count=len(trace),
                                  executed_instructions=list(trace)))
    report = dict(schema='H2_TU15_PCR_TAB_SELECTION_1', source_sha256=NEW_PIN,
                  original_entry=ENTRY, source_locator='retail 0x2B41EA c_y_menu_tab_bar::v10',
                  original_body_sha256=hashlib.sha256(image.read(ENTRY, 0x8A)).hexdigest(),
                  oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  scope=__doc__, cases=cases)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS unchanged TU1.5 PCR tab selection: {len(cases)} cases, '
          f'{sum(c["instruction_count"] for c in cases)} instructions')


if __name__ == '__main__':
    main()
