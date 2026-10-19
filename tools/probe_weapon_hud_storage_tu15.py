#!/usr/bin/env python3
"""Minimize the live dual-drop NULL HUD binding in unchanged original code.

Run through run_test.py. The arena/clock inputs below are declared diagnostic
inputs. This verifies storage admission only, not new-map or HUD presentation.
"""
import argparse
import json
import os
import struct
from pathlib import Path
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_INVALID
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP
from oracle_smg_firing_1_1 import Xbe
from original_xbox_pose_case import sha
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def run(image, initialize, fill):
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x600000)
    for section in image.sections:
        raw = image.section_bytes(section)
        if section.va + len(raw) <= 0x610000:
            cpu.mem_write(section.va, raw)
    stack, stop, arena = 0x2008000, 0x2100000, 0x81000000
    cpu.mem_map(0x2000000, 0x10000)
    cpu.mem_map(stop, 0x1000)
    cpu.mem_map(arena, 0x10000)
    cpu.mem_write(arena, bytes([fill]) * 0x10000)
    put = lambda at, value: cpu.mem_write(at, struct.pack('<I', value))
    word = lambda at: struct.unpack('<I', cpu.mem_read(at, 4))[0]
    put(0x4f2950, arena)
    put(0x4f2954, 0x100)
    put(0x4f2958, 0x200)
    put(0x4f295c, 0x12345678)
    reached, faults = {}, []
    def instruction(machine, pc, size, _):
        assert bytes(machine.mem_read(pc, size)) == image.read(pc, size)
        reached[f'{pc:08x}'] = image.read(pc, size).hex()
    def invalid(machine, access, address, size, value, _):
        faults.append(dict(pc=machine.reg_read(UC_X86_REG_EIP), access=access, address=address, bytes=size))
        return False
    cpu.hook_add(UC_HOOK_CODE, instruction)
    cpu.hook_add(UC_HOOK_MEM_INVALID, invalid)
    def call(entry):
        put(stack, stop)
        put(stack + 4, 0)
        put(stack + 8, 0)
        cpu.reg_write(UC_X86_REG_ESP, stack)
        cpu.emu_start(entry, stop, count=100000)
        return cpu.reg_read(UC_X86_REG_EIP) == stop
    if initialize:
        assert call(0x25a71c), dict(pc=hex(cpu.reg_read(UC_X86_REG_EIP)),sp=hex(cpu.reg_read(UC_X86_REG_ESP)))
    binding = word(0x50eecc)
    cpu.reg_write(UC_X86_REG_EAX, 0)  # Actual live failure's local user index.
    try:
        returned = call(0x25aa57)
    except UcError:
        returned = False
        assert faults
    if initialize:
        assert binding == arena + 0x100 and returned and not faults
        assert word(0x4f2954) == 0x100 + 0x13a8
    else:
        assert binding == 0 and not returned
        assert faults == [dict(pc=0x25aa73, access=19, address=0x82, bytes=1)], faults
    return dict(original_constructor_executed=initialize, declared_arena_fill=fill,
        hud_binding=binding, returned=returned, faults=faults, reached_instruction_bytes=reached,
        arena_cursor=word(0x4f2954), original_crc=word(0x4f295c))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    cases = [run(image, False, 0)] + [run(image, True, fill) for fill in (0, 0x5a, 0xa5)]
    report = dict(passed=True, component='Minimized original dual-drop HUD storage admission',
        original_xbe_sha256=NEW_PIN, cases=cases, original_instruction_patches=[], supplied_callee_results=[],
        native_allocator_results_supplied=False, original_allocator_and_crc_executed=True,
        new_map_initialization_verified=False, hud_or_framebuffer_parity_verified=False,
        runner_scratch=os.environ['HALO2_TEST_SCRATCH'], tool_sha256=sha(Path(__file__)), large_artifacts_retained=[])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print('Original NULL binding reproduces25AA73/82; original25A71C allocator admits the same consumer for three arena fills.')


if __name__ == '__main__':
    main()
