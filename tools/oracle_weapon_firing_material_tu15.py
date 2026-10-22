#!/usr/bin/env python3
"""Run unchanged TU1.5 10DE40/1046C0 firing material callbacks.

Declared object, definition and clock inputs exercise both barrel slots,
signed tick wrap, authored counts and raw float controls. No runtime content,
game service, source instruction or callback result is substituted.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe
from oracle_weapon_heat_shader_tu15 import Original
from oracle_weapon_barrel_timing_tu15 import WEAPON, DEF, TIME, STACK, STOP, OUT, WI, pack
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import x86_const as R


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.fixture.resolve().is_relative_to(scratch)
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    original = Original('TU1.5', image)
    machine = original.g
    machine.mem_write(WEAPON + 0xaa, b'\x02')
    machine.mem_write(WEAPON + 0x14, pack([0xffffffff]))
    machine.mem_write(0x51df54, pack([TIME]))
    counts = [0xffffffff, 0, 1, 2]
    ticks = [[0, 0], [1, 0], [2, 0], [0, 1], [0, 0xffffffff],
             [0xffffffff, 0], [0x7fffffff, 0x80000000],
             [0x80000000, 0x7fffffff], [0, 0x80000000], [0x80000000, 0]]
    values = [0, 0x80000000, 0xbf800000, 0x3e800000, 0x3f000000,
              0x3f7fffff, 0x3f800000, 0x3f800001, 0x40000000,
              0x7f800000, 0xff800000, 0x7fc01234, 0xffc05678]
    selectors = [0x0e000572, 0x10000573]
    digest = hashlib.sha256()
    cases = 0
    for count, (now, last), primary, secondary, selector in itertools.product(
            counts, ticks, values, values, selectors):
        machine.mem_write(DEF + 0x2d0, pack([count]))
        machine.mem_write(TIME + 8, pack([now]))
        machine.mem_write(WEAPON + 0x248, pack([last]))
        machine.mem_write(WEAPON + 0x1b4, pack([primary]))
        machine.mem_write(WEAPON + 0x1e8, pack([secondary]))
        machine.mem_write(OUT, bytes(8))
        machine.mem_write(STACK, pack([STOP, WI, selector]))
        for register, value in [(R.UC_X86_REG_ESP, STACK), (R.UC_X86_REG_EBX, OUT),
                                (R.UC_X86_REG_EDI, OUT + 4), (R.UC_X86_REG_FPCW, 0x27f),
                                (R.UC_X86_REG_MXCSR, 0x1f80)]:
            machine.reg_write(register, value)
        machine.emu_start(0x10de40, STOP, count=10000)
        assert machine.reg_read(R.UC_X86_REG_EIP) == STOP
        assert machine.reg_read(R.UC_X86_REG_ESP) == STACK + 12
        assert machine.reg_read(R.UC_X86_REG_EAX) & 255 == 1
        result = struct.unpack('<I', machine.mem_read(OUT, 4))[0]
        enabled = machine.mem_read(OUT + 4, 1)[0]
        digest.update(struct.pack('<7IB', count, now, last, primary, secondary,
                                  selector, result, enabled))
        cases += 1
    fixture = dict(original_xbe_sha256=NEW_PIN, entry=0x10de40, weapon_callback=0x1046c0,
                   barrel_count_offset=0x2d0, current_tick_pointer=0x51df54,
                   current_tick_offset=8, last_fired_offset=0x248,
                   barrel_value_offsets=[0x1b4, 0x1e8], barrel_counts=counts,
                   tick_pairs=ticks, raw_values=values, selectors=selectors,
                   cases=cases, results_sha256=digest.hexdigest(),
                   digest_encoding='ordered count,now,last,primary,secondary,selector,result uint32 LE; enabled uint8')
    args.fixture.write_text(json.dumps(fixture, separators=(',', ':')) + '\n')
    report = dict(passed=True, scope=__doc__, original_xbe_sha256=NEW_PIN,
                  original_calls=cases, executed_original_instructions=original.instructions,
                  complete_weapon_callback=original.complete_body,
                  fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  results_sha256=digest.hexdigest(), original_instruction_patches=[],
                  supplied_services=[], supplied_results=[], scratch=str(scratch),
                  cleanup_pending_runner_exit=True, large_artifacts_retained=[])
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, original_calls=cases,
                         fixture_bytes=args.fixture.stat().st_size)), flush=True)


if __name__ == '__main__':
    main()
