#!/usr/bin/env python3
"""Execute unchanged10DE40/BEC50 alive input over all16-bit object flags.

Only declared object/tag/stack inputs are supplied. The actual image owns its
descriptor chain and complete callbacks. No cache gameplay or pixels are tested.
Generated fixtures stay in HALO2_TEST_SCRATCH; retain the small reference only
after this complete comparison returns successfully.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from oracle_weapon_heat_shader_tu15 import Original
from oracle_weapon_barrel_timing_tu15 import WEAPON, STACK, STOP, OUT, WI, pack
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
    selector, entry = 0x0500055b, 0x10de40
    period, digest = [], hashlib.sha256()
    for flags in range(65536):
        machine.mem_write(WEAPON + 0x10a, struct.pack('<H', flags))
        machine.mem_write(OUT, bytes(8))
        machine.mem_write(STACK, pack([STOP, WI, selector]))
        for register, value in [(R.UC_X86_REG_ESP, STACK), (R.UC_X86_REG_EBX, OUT),
                                (R.UC_X86_REG_EDI, OUT + 4), (R.UC_X86_REG_FPCW, 0x27f),
                                (R.UC_X86_REG_MXCSR, 0x1f80)]:
            machine.reg_write(register, value)
        machine.emu_start(entry, STOP, count=10000)
        assert machine.reg_read(R.UC_X86_REG_EIP) == STOP
        assert machine.reg_read(R.UC_X86_REG_ESP) == STACK + 12
        assert machine.reg_read(R.UC_X86_REG_EAX) & 255 == 1
        value = struct.unpack('<I', machine.mem_read(OUT, 4))[0]
        active = machine.mem_read(OUT + 4, 1)[0]
        row = [value, active]
        if flags < 8:
            period.append(row)
        assert row == period[flags % 8], flags
        digest.update(struct.pack('<HIB', flags, value, active))
    code = _Code(image, '.text')
    complete = {}
    for owner in [entry, 0xbec50]:
        body = _trace(code, owner, {owner})
        complete[f'{owner:08x}'] = {f'{pc:08x}': code.at(pc).bytes.hex() for pc in sorted(body.code)}
    fixture = dict(original_xbe_sha256=NEW_PIN, generic_entry=entry, base_owner=0xbec50,
                   selector=selector, flag_offset=0x10a, flag_words_verified=65536,
                   result_period=period, exhaustive_results_sha256=digest.hexdigest(),
                   digest_encoding='65536 ordered little-endian uint16 flags, uint32 value bits, uint8 enabled')
    args.fixture.write_text(json.dumps(fixture, separators=(',', ':')) + '\n')
    report = dict(passed=True, scope=__doc__, original_xbe_sha256=NEW_PIN,
                  original_calls=65536, complete_original_bodies=complete,
                  executed_original_instructions=original.instructions,
                  fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  exhaustive_results_sha256=digest.hexdigest(),
                  original_instruction_patches=[], supplied_services=[], supplied_results=[],
                  scratch=str(scratch), cleanup_pending_runner_exit=True, large_artifacts_retained=[])
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, original_calls=65536, fixture_bytes=args.fixture.stat().st_size)), flush=True)


if __name__ == '__main__':
    main()
