#!/usr/bin/env python3
"""Replay unchanged TU1.5 weighted hopper selection after block-file read.

The entry at198EDB receives an explicitly synthetic block-read result, block,
installed-map registry and RNG state. The original map predicate, clamp, RNG,
ordered selector,304-byte copy and return paths execute without substitutions.
This does not execute filesystem admission, original downloaded hopper data,
upstream random-state ownership or a complete Live matchmaking call.
Run through tools/run_test.py; generated work belongs to HALO2_TEST_SCRATCH.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EIP,
                              UC_X86_REG_ESP, UC_X86_REG_FPCW,
                              UC_X86_REG_MXCSR)

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ENTRY, STOP = 0x198EDB, 0x3000000
ESP, GLOBALS, TABLE, MAPS = 0x2020000, 0x1100000, 0x1100400, 0x1110000
RNG, OUTPUT_MAP, OUTPUT_SETTINGS = 0x1100600, 0x1100800, 0x1100900
BLOCK = ESP + 0x198


def sha(value):
    return hashlib.sha256(value).hexdigest()


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n")


def settings(token):
    return bytes((token * 31 + index * 17) & 255 for index in range(304))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tu15', type=Path, default=NEW_DEFAULT)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert sha(args.tu15.read_bytes()) == NEW_PIN
    image = Xbe(args.tu15)
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0x10000, 0x800000)
    for section in image.sections:
        machine.mem_write(section.va, image.section_bytes(section))
    machine.mem_map(0x1100000, 0x100000)
    machine.mem_map(0x2000000, 0x50000)
    machine.mem_map(STOP, 0x1000)
    executed, predicates = set(), []

    def observe(uc, address, size, unused):
        executed.add(address)
        if address == 0x1A5E30:
            at = uc.reg_read(UC_X86_REG_ESP)
            predicates.append(struct.unpack('<i', uc.mem_read(at + 4, 4))[0])

    machine.hook_add(UC_HOOK_CODE, observe)

    def put(at, value):
        machine.mem_write(at, struct.pack('<I', value & 0xFFFFFFFF))

    inputs = []
    seeds = [0, 1, 2, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF]
    banks = [[], [(50, 1)], [(50, 0)], [(50, -1)], [(50, -2147483648)],
             [(50, 2147483647)], [(50, 1), (20, 1)],
             [(50, 14), (80, 10), (80, 3), (20, 1)],
             [(50, 0), (20, 1001), (80, -1)],
             [(index + 1, 1000) for index in range(32)] + [(99, 766)],
             [(index + 1, 1000) for index in range(32)] + [(99, 767)],
             [(index + 1, 1000) for index in range(32)] + [(99, 768)],
             [(index + 1, 1000) for index in range(65)] + [(99, 535)],
             [(index + 1, 1000) for index in range(100)]]
    for rows in banks:
        for seed in seeds:
            maps = list(dict.fromkeys(row[0] for row in rows))
            for available in (maps, maps[1:], []):
                inputs.append((seed, rows, available, True))
    inputs.extend((seed, [(50, 2), (20, 3)], [50, 20], False) for seed in seeds)
    generator = random.Random(0x192EB0)
    for _ in range(128):
        rows = [(generator.randrange(1, 12), generator.randrange(-2, 1010))
                for _ in range(generator.randrange(1, 20))]
        available = [value for value in range(1, 12) if generator.randrange(3)]
        inputs.append((generator.getrandbits(32), rows, available, True))
    # These are chosen high16 draw values, reached through inverse LCG seeds.
    # They exercise both sides of every original cumulative row boundary.
    inverse = pow(1664525, -1, 1 << 32)
    for high in [0, 1, 32767, 32768, 65534, 65535,
                 *range(0, 65536, 1024)]:
        seed = ((high << 16) - 1013904223) * inverse & 0xFFFFFFFF
        inputs.append((seed, [(50, 2), (20, 3), (80, 1)], [50, 20, 80], True))
    cases = []
    for seed, rows, available, read_ok in inputs:
        predicates.clear()
        machine.mem_write(BLOCK, bytes(0x15CB8))
        put(BLOCK + 0x44, len(rows))
        tokens = []
        for index, (map_id, weight) in enumerate(rows):
            at = BLOCK + 0x48 + index * 0x37C
            put(at + 4, map_id)
            put(at + 0x248, weight)
            token = index + 1
            tokens.append(token)
            machine.mem_write(at + 0x24C, settings(token))
        put(0x4ECC1C, GLOBALS)
        put(GLOBALS + 0x178, 1)
        put(GLOBALS + 0x17C, TABLE)
        put(TABLE + 0x10, len(available))
        put(TABLE + 0x14, MAPS)
        for index, map_id in enumerate(available):
            put(MAPS + index * 0xC64, map_id)
        put(0x4F3CD8, RNG)
        put(RNG + 4, seed)
        put(ESP + 0x15E50, STOP)
        put(ESP + 0x15E54, OUTPUT_MAP)
        put(ESP + 0x15E58, OUTPUT_SETTINGS)
        put(OUTPUT_MAP, 0xA5A5A5A5)
        machine.mem_write(OUTPUT_SETTINGS, bytes([0xA5]) * 304)
        machine.reg_write(UC_X86_REG_ESP, ESP)
        machine.reg_write(UC_X86_REG_EAX, int(read_ok))
        machine.reg_write(UC_X86_REG_FPCW, 0x027F)
        machine.reg_write(UC_X86_REG_MXCSR, 0x1F80)
        machine.emu_start(ENTRY, STOP, count=1000000)
        assert machine.reg_read(UC_X86_REG_EIP) == STOP
        success = bool(machine.reg_read(UC_X86_REG_EAX) & 255)
        output = bytes(machine.mem_read(OUTPUT_SETTINGS, 304))
        chosen = next((i for i, token in enumerate(tokens) if settings(token) == output), None)
        assert (chosen is not None) == success
        if not success:
            assert output == bytes([0xA5]) * 304
            assert bytes(machine.mem_read(OUTPUT_MAP, 4)) == bytes([0xA5]) * 4
        assert predicates == ([row[0] for row in rows] if read_ok else [])
        total = sum(min(1000, max(1, weight)) for map_id, weight in rows if map_id in available)
        cases.append(dict(seed=seed, rows=[dict(map_id=m, weight=w, settings_token=t)
                                          for (m, w), t in zip(rows, tokens)],
                          installed=available, block_read=read_ok, total=total,
                          safe_total=total <= 32766,
                          updated_seed=struct.unpack('<I', machine.mem_read(RNG + 4, 4))[0],
                          selected=chosen, selected_map=rows[chosen][0] if success else None,
                          settings_sha256=sha(output) if success else None,
                          outputs_unchanged=not success))
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    instructions = []
    for address in sorted(executed):
        instruction = next(decoder.disasm(image.read(address, 15), address))
        assert bytes(machine.mem_read(address, instruction.size)) == instruction.bytes
        instructions.append(f'{address:08x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}')
    fixture = dict(schema='H2_TU15_HOPPER_SELECTION_ORIGINAL_V1', original_sha256=NEW_PIN,
                   scope=__doc__, entry=hex(ENTRY), map_predicate='0x1A5E30',
                   settings_pattern='byte[i]=(settings_token*31+i*17)&255; 304 bytes', cases=cases)
    save(args.fixture, fixture)
    report = dict(passed=True, cases=len(cases), safe_cases=sum(c['safe_total'] for c in cases),
                  signed_short_overflow_cases=sum(not c['safe_total'] for c in cases),
                  original_sha256=NEW_PIN, entry=hex(ENTRY), stop=hex(STOP),
                  original_instructions=len(executed), instruction_trace_sha256=sha('\n'.join(instructions).encode()),
                  tool_sha256=sha(Path(__file__).read_bytes()), fixture_sha256=sha(args.fixture.read_bytes()),
                  instruction_patches=[], replaced_callees=[], scope=__doc__,
                  scratch=str(scratch), scratch_retained=False)
    save(args.report, report)
    print(json.dumps(report))


if __name__ == '__main__':
    main()
