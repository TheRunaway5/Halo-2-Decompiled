#!/usr/bin/env python3
"""Execute original TU1.5 normal start-countdown blocks and timer helpers.

Run through tools/run_test.py. No original instruction or callee is replaced.
The complete readiness helper, readiness-deferral producer, countdown publication
and mode-transition callees are outside these explicitly bounded controls.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from oracle_game_variants import Source, STACK
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from functions import _Code, _trace
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESI, UC_X86_REG_ESP
from capstone import Cs, CS_ARCH_X86, CS_MODE_32


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, default=NEW_DEFAULT)
    parser.add_argument('--report-output', type=Path, required=True)
    args = parser.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = Source({'path': str(args.xbe.resolve()), 'pin': NEW_PIN, 'globals': 0x51d048})
    memory = source.g
    state = 0x1100000
    memory.mem_map(state, 0x10000)
    executed = set()

    def write(address, value):
        memory.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def read(address):
        return struct.unpack('<I', memory.mem_read(address, 4))[0]

    def boundary(start, stops, registers):
        hit = []

        def observe(_memory, address, _size, _context):
            if address in stops:
                hit.append(address)
                memory.emu_stop()
            else:
                executed.add(address)

        hook = memory.hook_add(UC_HOOK_CODE, observe)
        try:
            for register, value in registers:
                memory.reg_write(register, value & 0xffffffff)
            memory.emu_start(start, 0x3000000, count=10000)
        finally:
            memory.hook_del(hook)
        assert hit, (hex(start), hex(memory.reg_read(UC_X86_REG_EIP)))
        return hit[0]

    controls = []
    for elapsed in (0, 1, 999, 1000, 3001, 6500, 9999, 10000, 10001, 60000):
        # The original helpers' own clock-data path, not a stubbed import.
        memory.mem_write(0x51d048, b'\x01')
        write(0x51d04c, 1000 + elapsed)
        write(state + 0x18, 1000)
        boundary(0x76481, {0x764af}, [(UC_X86_REG_ESP, STACK),
                 (UC_X86_REG_EBP, state), (UC_X86_REG_ESI, state + 0x1000)])
        remaining = memory.reg_read(UC_X86_REG_EDI)
        assert remaining == max(0, 10000 - elapsed)
        boundary(0x764b4, {0x764d3}, [(UC_X86_REG_ESP, STACK),
                 (UC_X86_REG_EDI, remaining), (UC_X86_REG_ESI, state + 0x1000)])
        seconds = memory.reg_read(UC_X86_REG_EBX)
        assert seconds == (max(0, 10000 - elapsed) + 999) // 1000
        write(STACK + 0x14, state + 0x2000)
        boundary(0x764d8, {0x764fb}, [(UC_X86_REG_ESP, STACK), (UC_X86_REG_EDI, remaining)])
        ready = bytes(memory.mem_read(STACK + 0x12, 1))[0]
        assert ready == int(elapsed >= 10000)
        controls.append(dict(elapsed_ms=elapsed, remaining_ms=remaining, seconds=seconds, ready=bool(ready)))

    write(state + 0x18, 0)
    write(0x51d04c, 123456)
    boundary(0x76481, {0x764af}, [(UC_X86_REG_ESP, STACK),
             (UC_X86_REG_EBP, state), (UC_X86_REG_ESI, state + 0x1000)])
    assert read(state + 0x18) == 123456 and memory.reg_read(UC_X86_REG_EDI) == 10000
    write(state + 0x18, 123456)
    boundary(0x764f4, {0x764fb}, [(UC_X86_REG_EBP, state)])
    assert read(state + 0x18) == 0

    dispatch = []
    for readiness in (0, 1):
        for deferral in (0, 1):
            memory.mem_write(state + 0x16, bytes([deferral]))
            write(STACK + 0x14, state + 0x2000)
            stop = boundary(0x76473, {0x76481, 0x764fb}, [(UC_X86_REG_ESP, STACK),
                            (UC_X86_REG_EBP, state), (UC_X86_REG_EBX, state + 0x16),
                            (UC_X86_REG_EAX, readiness)])
            assert stop == (0x76481 if readiness and not deferral else 0x764fb)
            dispatch.append(dict(readiness_result=readiness, deferral_byte=deferral, timer_entered=stop == 0x76481))

    code = _Code(source.image, '.text')
    with (ROOT / 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open() as inventory:
        starts = {int(row['address'], 16) for row in csv.DictReader(inventory, delimiter='\t')}
    body = _trace(code, 0x76360, starts)
    listing = ''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'
                      for pc in sorted(body.code))
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    executed_listing = ''.join(f'{pc:08x} ' + next(decoder.disasm(source.image.read(pc, 15), pc)).bytes.hex() + '\n'
                               for pc in sorted(executed))
    report = dict(passed=True, tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  original_xbe_sha256=NEW_PIN, retail_source_locator='0x72700', tu15_owner='0x76360',
                  tu15_instruction_count=len(body.code), timer_entry='0x76481', arithmetic_entry='0x76499',
                  publish_countdown_call='0x764d3 -> 0x5cbd0', set_game_mode_call='0x7650e -> 0x5aed0',
                  scope=__doc__, controls=controls, initial_clock_stored=123456,
                  readiness_loss_cleared_clock=True, dispatch_controls=dispatch,
                  owner_listing_sha256=hashlib.sha256(listing.encode()).hexdigest(),
                  executed_listing_sha256=hashlib.sha256(executed_listing.encode()).hexdigest(),
                  executed_distinct_instruction_count=len(executed), owner_listing=listing,
                  executed_instructions=executed_listing)
    encoded = json.dumps(report, indent=2) + '\n'
    assert len(encoded.encode()) <= 1024 * 1024
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(encoded)
    print(json.dumps(dict(passed=True, tu15_owner=report['tu15_owner'],
                          executed_instructions=len(executed), controls=len(controls) + len(dispatch) + 2,
                          report=str(args.report_output))))


if __name__ == '__main__':
    main()
