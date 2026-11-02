#!/usr/bin/env python3
"""Authenticate sparse offline loop-location fixtures with untouched TU1.5 code.

The native regression consumes only declared data operands and original traces,
writes and host-x86 exits. The separately owned Xbox executable is an offline
authority for this generator and is never an input to the game or native test.
Run through tools/run_test.py; only the bounded JSON fixture leaves scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_charged_effect_lifetime_bank_tu15 import segment
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R


ROOT = Path(__file__).resolve().parents[1]
SCHEMA = 'HALO2_LOOP_AUDIO_LOCATION_ORIGINAL_V1'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path,
        default=ROOT/'tests/fixtures/loop_audio_locations_tu15_original.json')
    parser.add_argument('--mode3-recipes', type=Path)
    parser.add_argument('--mode2-recipes', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.output.resolve().is_relative_to(scratch), 'Generated fixtures must remain in owned scratch'
    assert bool(args.mode3_recipes) == bool(args.mode2_recipes)
    if args.mode3_recipes:
        cases = []
        for entry, path in ((0x17e160, args.mode3_recipes), (0x230b10, args.mode2_recipes)):
            for row in json.loads(path.read_text())['cases']:
                cases.append(dict(row, entry=entry,
                    stack_arguments=[0x60300000, 0x240a020, 0x240a040] if entry == 0x17e160 else
                                    [0x60300000, 0x2401000, 0x240a000, 0x240a040],
                    initial_fp=dict(x87_control_word=0x27f, mxcsr=0x1f80)))
    else:
        previous = json.loads(args.input.read_text())
        assert previous['schema'] == SCHEMA and previous['original_xbe_sha256'] == NEW_PIN
        cases = previous['cases']
    assert len(cases) == 22 and len({row['case'] for row in cases}) == 22
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    high = max(section.va + section.vsize for section in image.sections)
    pristine = bytearray(high - 0x10000)
    for section in image.sections:
        pristine[section.va-0x10000:section.va-0x10000+section.rsize] = image.section_bytes(section)
    registers = [getattr(R, 'UC_X86_REG_'+name) for name in
                 ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')]
    for row in cases:
        guest = bytearray(pristine)
        for address, value in row['source_pointer_overrides'].items():
            struct.pack_into('<I', guest, int(address, 16)-0x10000, value)
        private = row['private_bank']
        assert private['zero_fill'] and private['address'] == 0x2400000 and private['bytes'] == 0x40000
        bank = bytearray(private['bytes'])
        for address, payload in private['nonzero_blocks']:
            data = bytes.fromhex(payload); offset = address-private['address']
            assert offset >= 0 and offset+len(data) <= len(bank)
            bank[offset:offset+len(data)] = data
        caller = bytes.fromhex(row['caller_hex']); assert len(caller) == 168
        gp = struct.unpack_from('<8I', caller)
        assert gp[4] == 0x600f0000
        stack = bytearray(0x200000)
        struct.pack_into('<'+'I'*len(row['stack_arguments']), stack, gp[4]-0x60000000,
                         *row['stack_arguments'])
        vm = Uc(UC_ARCH_X86, UC_MODE_32); mapped = set()
        def mount(address, data):
            for page in range(address & ~4095, (address+len(data)+4095) & ~4095, 4096):
                if page not in mapped:
                    vm.mem_map(page, 4096); mapped.add(page)
            vm.mem_write(address, bytes(data))
        mount(0x10000, guest); mount(private['address'], bank); mount(0x60000000, stack)
        mount(0x60200000, bytes(4096)); mount(0x60300000, bytes(4096)); mount(0x70000000, bytes(4096))
        vm.mem_write(0x70000008, segment(0, 0x9b)+segment(0, 0x93)+segment(0x60200000, 0x93))
        vm.reg_write(R.UC_X86_REG_GDTR, (0, 0x70000000, 0xfff, 0))
        for register, value in ((R.UC_X86_REG_CS, 8), (R.UC_X86_REG_DS, 16),
                                (R.UC_X86_REG_ES, 16), (R.UC_X86_REG_SS, 16), (R.UC_X86_REG_FS, 24)):
            vm.reg_write(register, value)
        for register, value in zip(registers, gp): vm.reg_write(register, value)
        for index in range(8):
            vm.reg_write(getattr(R, f'UC_X86_REG_XMM{index}'),
                         int.from_bytes(caller[32+index*16:48+index*16], 'little'))
        for register, value in ((R.UC_X86_REG_EFLAGS, struct.unpack_from('<I', caller, 160)[0]),
                (R.UC_X86_REG_FPCW, row['initial_fp']['x87_control_word']),
                (R.UC_X86_REG_FPSW, 0), (R.UC_X86_REG_FPTAG, 0xffff),
                (R.UC_X86_REG_MXCSR, row['initial_fp']['mxcsr'])):
            vm.reg_write(register, value)
        trace = []; writes = []; image_reads = set(); current = [0]
        def instruction(uc, address, size, unused):
            assert code.inside(address) and bytes(uc.mem_read(address, size)) == image.read(address, size)
            current[0] = address; trace.append(address)
        def read(uc, access, address, size, value, unused):
            if 0x10000 <= address and address+size <= high:
                image_reads.update(range(address, address+size))
            else:
                assert (private['address'] <= address and address+size <= private['address']+len(bank)) or \
                    (0x60000000 <= address and address+size <= 0x60201000)
        def write(uc, access, address, size, value, unused):
            if 0x60000000 <= address < 0x60200000: return
            assert private['address'] <= address and address+size <= private['address']+len(bank)
            assert 1 <= size <= 8
            writes.append(struct.pack('<III', current[0], address, size) +
                bytes(uc.mem_read(address, size)) + (value & ((1 << (size*8))-1)).to_bytes(size, 'little'))
        vm.hook_add(UC_HOOK_CODE, instruction); vm.hook_add(UC_HOOK_MEM_READ, read)
        vm.hook_add(UC_HOOK_MEM_WRITE, write)
        vm.emu_start(row['entry'], 0x60300000, count=20000)
        assert vm.reg_read(R.UC_X86_REG_EIP) == 0x60300000
        assert vm.reg_read(R.UC_X86_REG_ESP) == gp[4]+4*len(row['stack_arguments'])
        assert b''.join(struct.pack('<I', pc) for pc in trace).hex() == row['original_trace_hex']
        assert b''.join(writes).hex() == row['original_ordered_nonstack_writes_hex']
        ordered = sorted(image_reads); ranges = []; position = 0
        while position < len(ordered):
            start = ordered[position]; end = start+1; position += 1
            while position < len(ordered) and ordered[position] == end:
                end += 1; position += 1
            data = bytes(guest[start-0x10000:end-0x10000])
            assert bytes(vm.mem_read(start, end-start)) == data
            ranges.append(dict(address=start, hex=data.hex()))
        row['image_data'] = ranges
        assert len(bytes.fromhex(row['expected_hardware_exit_hex'])) == 180
    report = dict(schema=SCHEMA, original_xbe_sha256=NEW_PIN, cases=cases,
        provenance='Declared standalone operand banks; exact untouched original traces/writes and host x86 hardware exits; no replaced game services or source instructions',
        source_instruction_patches=[], supplied_game_services=[], physical_xbox_verified=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(passed=True, cases=len(cases), bytes=args.output.stat().st_size,
        sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())))


if __name__ == '__main__': main()
