#!/usr/bin/env python3
"""Execute unchanged TU1.5 primary-intent fragments, not the whole producer.

Run through tools/run_test.py. Only the bounded JSON receipt is retained;
the emulator and all generated scratch belong to that invocation.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_EFLAGS,
)

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
DEFAULT_XBE = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
LISTINGS = ROOT / 'analysis/multiplayer/integration/requested-controls/owned-update-functions/instructions'
STACK, STATE, OUTPUT = 0x02000800, 0x02100000, 0x02200000
FRAGMENTS = (
    ('primary_fire_bit16', 0x1895AC, 0x1895C8, 0x10000),
    ('primary_action_bit6', 0x189735, 0x18974F, 0x40),
    ('output_flags_copy', 0x189E21, 0x189E26, None),
)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, default=DEFAULT_XBE)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert scratch.is_dir(), 'run through tools/run_test.py'
    image = Xbe(args.xbe)
    assert sha(image.data) == PIN
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    instructions = {}
    fragments = []
    for name, start, end, mask in FRAGMENTS:
        raw = image.read(start, end - start)
        decoded = list(md.disasm(raw, start))
        assert decoded and decoded[-1].address + decoded[-1].size == end
        for ins in decoded:
            assert ins.mnemonic != 'call', 'no supplied callees are permitted'
            instructions[ins.address] = ins
        fragments.append({'name': name, 'start': f'{start:08x}', 'stop_before': f'{end:08x}',
                          'mask': None if mask is None else f'{mask:08x}',
                          'sha256': sha(raw), 'instruction_bytes': raw.hex(),
                          'instructions': [f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}' for i in decoded]})
    listing_receipts = []
    for name in ('00187610-later-001889d0.asm', '001884e0-later-001898a0.asm'):
        path = LISTINGS / name
        lines = path.read_text().splitlines()
        for line in lines:
            address, raw, *_ = line.split()
            raw = bytes.fromhex(raw)
            assert image.read(int(address, 16), len(raw)) == raw, line
        listing_receipts.append({'path': str(path.relative_to(ROOT)), 'sha256': sha(path.read_bytes()),
                                 'instructions_authenticated': len(lines)})
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x189000, 0x1000)
    cpu.mem_write(0x189000, image.read(0x189000, 0x1000))
    for base in (0x02000000, STATE, OUTPUT):
        cpu.mem_map(base, 0x1000)
    current = {}
    reads, writes, pcs = set(), set(), set()

    def on_code(g, address, size, _):
        assert current['start'] <= address < current['end'], hex(address)
        ins = instructions[address]
        assert size == ins.size and bytes(g.mem_read(address, size)) == ins.bytes
        pcs.add(address)

    def on_read(g, _access, address, size, _value, _):
        allowed = ({(STATE + 4, 4)} if current['copy'] else
                   {(STACK + 0x27, 1), (STATE + 0x18, 4)})
        assert (address, size) in allowed, (hex(address), size)
        reads.add((address, size))

    def on_write(g, _access, address, size, value, _):
        assert (address, size) == ((OUTPUT, 4) if current['copy'] else (STATE + 0x18, 4))
        writes.add((address, size))

    cpu.hook_add(UC_HOOK_CODE, on_code)
    cpu.hook_add(UC_HOOK_MEM_READ, on_read)
    cpu.hook_add(UC_HOOK_MEM_WRITE, on_write)
    patterns = sorted({0, 0xFFFFFFFF, 0xAAAAAAAA, 0x55555555, 0x10040} |
                      {1 << bit for bit in range(32)} |
                      {0xFFFFFFFF ^ (1 << bit) for bit in range(32)})
    digest = hashlib.sha256()
    examples = []
    count = 0
    step_count = 0
    for held in range(256):
        for flags in patterns:
            expected = flags
            intermediate = []
            for _name, start, end, mask in FRAGMENTS:
                current.update(start=start, end=end, copy=mask is None)
                cpu.reg_write(UC_X86_REG_EAX, 0x81828384)
                cpu.reg_write(UC_X86_REG_EBX, 0x91929394)
                cpu.reg_write(UC_X86_REG_ECX, 0xA1A2A3A4)
                cpu.reg_write(UC_X86_REG_EDX, flags ^ 0xA5A5A5A5)
                cpu.reg_write(UC_X86_REG_ESI, OUTPUT)
                cpu.reg_write(UC_X86_REG_EDI, STATE)
                cpu.reg_write(UC_X86_REG_EBP, STATE)
                cpu.reg_write(UC_X86_REG_ESP, STACK)
                cpu.reg_write(UC_X86_REG_EFLAGS, 0x202)
                cpu.mem_write(STACK + 0x27, bytes([held]))
                cpu.mem_write(STATE + 0x18, struct.pack('<I', expected))
                cpu.mem_write(STATE + 4, struct.pack('<I', expected))
                cpu.mem_write(OUTPUT, b'\xC7' * 4)
                cpu.emu_start(start, end, count=32)
                assert cpu.reg_read(UC_X86_REG_EIP) == end
                if mask is not None:
                    expected = (expected | mask) if held else (expected & ~mask)
                actual = struct.unpack('<I', cpu.mem_read(OUTPUT if mask is None else STATE + 0x18, 4))[0]
                assert actual == expected, (held, hex(flags), hex(start), hex(actual), hex(expected))
                intermediate.append(actual)
                step_count += 1
            assert expected == ((flags & ~0x10040) | (0x10040 if held else 0))
            digest.update(struct.pack('<BIIII', held, flags, *intermediate))
            if held in (0, 1, 2, 127, 128, 255) and flags in (0, 0xFFFFFFFF, 0xAAAAAAAA, 0x55555555):
                examples.append({'held_byte': held, 'input_flags': f'{flags:08x}',
                                 'after_bit16': f'{intermediate[0]:08x}', 'after_bit6': f'{intermediate[1]:08x}',
                                 'copied_output': f'{intermediate[2]:08x}'})
            count += 1
    report = {
        'passed': True, 'xbe_path': str(args.xbe), 'xbe_sha256': PIN,
        'helper_sha256': sha(Path(__file__).read_bytes()), 'scratch_path': str(scratch),
        'scope': 'Unchanged original TU1.5 fragments only: primary held-byte to bit16, same held-byte to bit6, then ordinary output flag copy. Not whole 1889D0/1898A0 execution, original game input polling, target acquisition or a rapid-sword-flight result.',
        'supplied_callees': [], 'instruction_fetches_authenticated': True,
        'listing_receipts': listing_receipts, 'fragments': fragments,
        'held_values_exhausted': 256, 'flag_patterns': [f'{p:08x}' for p in patterns],
        'cases': count, 'fragment_executions': step_count,
        'all_flags_other_than_bits6_16_preserved': True,
        'truth': {'held_zero': '(input_flags & ~0x00010040)', 'held_1_through_255': '(input_flags | 0x00010040)'},
        'case_transcript_sha256': digest.hexdigest(),
        'observed_data_reads': [{'address': f'{a:08x}', 'bytes': n} for a, n in sorted(reads)],
        'observed_data_writes': [{'address': f'{a:08x}', 'bytes': n} for a, n in sorted(writes)],
        'executed_instruction_addresses': [f'{p:08x}' for p in sorted(pcs)],
        'weapon_independence_scope': 'These exact fragments read only the supplied trigger-state byte, input flags, and copy source. No object, weapon, weapon definition or weapon type is read. Earlier producer logic may change the held byte before fragment entry.',
        'output_copy_context_static': {
            'inhibition_branch': '18990F calls 14BB3C; 189916 calls 12E9A0; their OR with [4F556A+local_user*140] branches at 18992D. Nonzero zeros the 4C-byte local command at 18993D..189967, preserving an unrelated byte flag. This exceptional path is not executed here.',
            'normal_flags_copy': '189D04 loads local command flags [esp+30], 189D2E stores player-state+4; 189E21..189E24 copies player-state+4 unchanged to output+0.',
            'additional_flags': 'The ordinary path can OR bit14 at 189CE9. No Sword-specific mask of bits6/16 appears in the authenticated 1898A0 listing.',
        },
        'examples': examples,
        'large_artifacts_retained': [],
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'passed': True, 'cases': count, 'fragment_executions': step_count,
                      'report': str(args.report), 'scratch_path': str(scratch)}))


if __name__ == '__main__':
    main()
