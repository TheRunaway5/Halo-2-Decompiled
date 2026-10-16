#!/usr/bin/env python3
"""Execute original TU1.5 surface selection up to the first unowned binder.

Surface handles are declared inputs. This proves the selector's color/depth
choices, not SDK binding, image alias storage, draw behavior or live frames.
The original instructions are unchanged and no game service is supplied.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    arguments = parser.parse_args()
    if arguments.output.exists():
        raise ValueError('Preserve existing evidence; choose a new output directory')
    image = Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest() != NEW_PIN:
        raise ValueError('Original TU1.5 executable identity differs')
    stack, stop = 0x2008000, 0x2100000
    handles = (0x1100000, 0x1101000, 0x1102000, 0x1103000)
    executed, cases = {}, []
    for mode in (0, 3, 4):
        for depth_enabled in (0, 1):
            cpu = Uc(UC_ARCH_X86, UC_MODE_32)
            cpu.mem_map(0x10000, 0x600000)
            for section in image.sections:
                raw = image.section_bytes(section)
                if section.va + len(raw) <= 0x610000:
                    cpu.mem_write(section.va, raw)
            cpu.mem_map(0x2000000, 0x20000)
            cpu.mem_map(stop, 0x1000)
            for cell, handle in zip((0x515e3c, 0x515e40, 0x515e44, 0x515e50), handles):
                cpu.mem_write(cell, struct.pack('<I', handle))
            cpu.mem_write(stack, struct.pack('<III', stop, mode, 0))
            cpu.reg_write(UC_X86_REG_ESP, stack)
            cpu.reg_write(UC_X86_REG_EAX, depth_enabled)

            def instruction(machine, pc, size, _):
                if pc == 0x1cd10:
                    machine.emu_stop()
                    return
                raw = bytes(machine.mem_read(pc, size)).hex()
                if executed.setdefault(f'{pc:08x}', raw) != raw:
                    raise ValueError('An original instruction changed')

            cpu.hook_add(UC_HOOK_CODE, instruction)
            cpu.emu_start(0x14ba0, stop, count=500)
            if cpu.reg_read(UC_X86_REG_EIP) != 0x1cd10:
                raise ValueError('Selector did not reach its first unowned target binder')
            color = cpu.reg_read(UC_X86_REG_ESI)
            depth = cpu.reg_read(UC_X86_REG_EAX)
            sp = cpu.reg_read(UC_X86_REG_ESP)
            returned_to, flag = struct.unpack('<II', cpu.mem_read(sp, 8))
            if (color != handles[{0: 0, 3: 3, 4: 1}[mode]] or
                    depth != (handles[2] if depth_enabled and mode != 4 else 0) or
                    flag != 1 or returned_to != 0x14d27):
                raise ValueError('Original target selection or binder call frame differs')
            cases.append(dict(mode=mode, depth_enabled=bool(depth_enabled),
                              color_surface=f'{color:08x}', depth_surface=f'{depth:08x}',
                              flag=flag, return_site=f'{returned_to:08x}'))
    arguments.output.mkdir(parents=True)
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN,
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  declared_surface_inputs=dict(zip(('515E3C', '515E40', '515E44', '515E50'), handles)),
                  entry='00014ba0', stop_before_unowned_target_binder='0001cd10',
                  case_count=len(cases), cases=cases, executed_instruction_bytes=executed,
                  supplied_game_services=[], instruction_patches=[], passed=True,
                  renderable_alias_mode=3, sdk_binding_verified=False,
                  within_draw_alias_feedback_verified=False, live_frame_verified=False)
    (arguments.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(cases)} untouched selector prefixes passed; {len(executed)} instruction addresses')


if __name__ == '__main__':
    main()
