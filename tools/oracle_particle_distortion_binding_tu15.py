#!/usr/bin/env python3
"""Execute the untouched Xbox TU1.5 external23 texture binder.

Framebuffer-bank handles, counter and target readiness are declared inputs.
This proves selection/dimension behavior, not the original frame producer,
surface contents, live scheduling or Xbox pixels. No game service is supplied.
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

ROOT = Path(__file__).resolve().parents[1]
STACK, STOP, WIDTH, HEIGHT = 0x2008000, 0x2100000, 0x2001000, 0x2001004
BANK = (0x1100000, 0x1101000)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError('preserve existing evidence; choose a new output directory')
    image = Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest() != NEW_PIN:
        raise ValueError('original TU1.5 executable identity differs')
    args.output.mkdir(parents=True)
    executed, cases = {}, []
    for counter in (1, 2, 3, 4, 255, 256, 2**32-1, 2**32, 2**32+1, 2**63-1):
        for stage in range(4):
            for readiness in ('ready', 'busy', 'absent'):
                uc = Uc(UC_ARCH_X86, UC_MODE_32)
                uc.mem_map(0x10000, 0x600000)
                for section in image.sections:
                    raw = image.section_bytes(section)
                    if section.va + len(raw) <= 0x610000:
                        uc.mem_write(section.va, raw)
                uc.mem_map(0x2000000, 0x20000)
                uc.mem_map(STOP, 0x1000)
                uc.mem_write(0x4922c0, struct.pack('<Q', counter))
                uc.mem_write(0x515e34, struct.pack('<II', *BANK))
                target = 0x4c1378 + 23 * 0x98
                uc.mem_write(target + 0x80, struct.pack('<III', 320, 240, 0 if readiness == 'absent' else 0x1200000))
                uc.mem_write(target + 0x95, bytes([readiness == 'busy']))
                uc.mem_write(0x492314, struct.pack('<8I', *([0xdeadbeef] * 8)))
                uc.mem_write(0x52dfa8, struct.pack('<4I', *([0xdeadbeef] * 4)))
                uc.mem_write(STACK, struct.pack('<5I', STOP, 0, stage, WIDTH, HEIGHT))
                uc.reg_write(UC_X86_REG_EAX, 23)
                uc.reg_write(UC_X86_REG_ESP, STACK)
                def code(cpu, address, size, _):
                    raw = bytes(cpu.mem_read(address, size)).hex()
                    old = executed.setdefault(f'{address:08x}', raw)
                    if old != raw:
                        raise ValueError('original instruction changed')
                uc.hook_add(UC_HOOK_CODE, code)
                uc.emu_start(0x19510, STOP, count=1000)
                if uc.reg_read(UC_X86_REG_EIP) != STOP or uc.reg_read(UC_X86_REG_ESP) != STACK + 20:
                    raise ValueError('original external23 call did not complete its ABI')
                dimensions = struct.unpack('<II', uc.mem_read(WIDTH, 8))
                texture = struct.unpack('<I', uc.mem_read(0x52dfa8 + stage * 4, 4))[0]
                bound_dimensions = tuple(struct.unpack('<I', uc.mem_read(base + stage * 4, 4))[0] for base in (0x492314, 0x492324))
                expected_dimensions = (320, 240) if readiness == 'ready' else (0, 0)
                if texture != BANK[(counter-1) % 2] or dimensions != expected_dimensions:
                    raise ValueError(f'source texture bank or external dimensions differ: counter={counter} stage={stage} readiness={readiness} texture={texture:x} dimensions={dimensions}')
                if bound_dimensions != ((320, 240) if readiness == 'ready' else (0, 0)):
                    raise ValueError('source bound texture dimensions differ')
                if bytes(uc.mem_read(0x4922c0, 8)) != struct.pack('<Q', counter):
                    raise ValueError('external binder advanced the original frame counter')
                for other in set(range(4)) - {stage}:
                    if struct.unpack('<I', uc.mem_read(0x52dfa8 + other*4, 4))[0] != 0xdeadbeef:
                        raise ValueError('external binder overwrote another texture stage')
                cases.append(dict(counter=counter, stage=stage, readiness=readiness,
                                  texture=texture, external_dimensions=dimensions,
                                  bound_dimensions=bound_dimensions))
    # The texture headers keep the physical addresses copied at145E0;
    # untouched SDK403B50 rotates the surface addresses at presentation.
    # Counter values are declared here, with13EF0's increment retained in
    # the companion listing. No original present/scheduling claim follows.
    device, surfaces, physical, push = 0x1000000, (0x1010000, 0x1011000), (0x2200000, 0x2300000), 0x1200000
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(0x10000, 0x600000)
    for section in image.sections:
        raw = image.section_bytes(section)
        if section.va + len(raw) <= 0x610000:
            uc.mem_write(section.va, raw)
    uc.mem_map(0x1000000, 0x300000)
    uc.mem_map(0x2000000, 0x20000)
    uc.mem_map(STOP, 0x1000)
    def word(address, value):
        uc.mem_write(address, struct.pack('<I', value))
    def read_word(address):
        return struct.unpack('<I', uc.mem_read(address, 4))[0]
    word(0x411568, device)
    word(device, push)
    word(device+4, push+0x10000)
    word(device+0x1a10, 2)
    word(device+0x1a04, surfaces[0])
    word(0x411774, 1)
    for index in range(2):
        word(device+0x1a14+index*4, surfaces[index])
        word(surfaces[index]+4, physical[index])
        word(BANK[index]+4, physical[index])
        word(0x515e34+index*4, BANK[index])
    uc.mem_write(0x4c21a0, struct.pack('<III', 320, 240, 0x1200000))
    uc.mem_write(0x4c21b5, b'\0')
    uc.hook_add(UC_HOOK_CODE, code)
    rotations = []
    for frame in range(1, 9):
        uc.mem_write(0x4922c0, struct.pack('<Q', frame))
        uc.mem_write(STACK, struct.pack('<5I', STOP, 0, 1, WIDTH, HEIGHT))
        uc.reg_write(UC_X86_REG_EAX, 23)
        uc.reg_write(UC_X86_REG_ESP, STACK)
        uc.emu_start(0x19510, STOP, count=1000)
        texture = read_word(0x52dfac)
        selected_data, current_surface_data = read_word(texture+4), read_word(surfaces[0]+4)
        if selected_data != current_surface_data:
            raise ValueError('declared bank selector does not match the SDK-rotated render surface')
        uc.mem_write(STACK, struct.pack('<I', STOP))
        uc.reg_write(UC_X86_REG_ESI, device)
        uc.reg_write(UC_X86_REG_ESP, STACK)
        uc.emu_start(0x403b50, STOP, count=1000)
        if uc.reg_read(UC_X86_REG_EIP) != STOP:
            raise ValueError('original SDK surface rotation did not complete')
        if read_word(surfaces[0]+4) == current_surface_data or tuple(read_word(p+4) for p in BANK) != physical:
            raise ValueError('original SDK rotation did not preserve fixed texture-bank aliases')
        rotations.append(dict(declared_frame_counter=frame, selected_texture=texture,
                              selected_data=selected_data, current_render_surface_data=current_surface_data,
                              next_render_surface_data=read_word(surfaces[0]+4)))
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN,
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  entry='00019510', external=23, case_count=len(cases), cases=cases,
                  supplied_game_services=[], instruction_patches=[],
                  executed_instruction_bytes=executed, passed=True,
                  sdk_rotation_entry='00403b50', sdk_rotation_cases=rotations,
                  same_physical_render_surface_selected_in_declared_rotation=True,
                  live_frame_producer_verified=False, xbox_pixels_verified=False,
                  full_map_goal_complete=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(f'{len(cases)} original external23 calls and {len(rotations)} SDK surface rotations passed; {len(executed)} untouched instruction addresses')


if __name__ == '__main__':
    main()
