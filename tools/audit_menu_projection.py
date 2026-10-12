#!/usr/bin/env python3
"""Execute unchanged original frontend view setup and compare the captured lens.

The standard-video path is isolated from game services. This establishes the
render angle and clip rectangle, not complete scene or widescreen UI parity.
"""
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import *

x = Xbe(ROOT/'assets/disc/default.xbe')
pin = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
assert hashlib.sha256(x.data).hexdigest() == pin
guest = Uc(UC_ARCH_X86, UC_MODE_32)
guest.mem_map(0x10000, 0x600000)
for section in x.sections:
    guest.mem_write(section.va, x.section_bytes(section))
guest.mem_map(0x1000000, 0x10000)
guest.mem_map(0x2000000, 0x10000)
guest.mem_map(0x3000000, 0x1000)

def u32(at, value):
    guest.mem_write(at, struct.pack('<I', value))

def floats(at, values):
    guest.mem_write(at, struct.pack('<'+'f'*len(values), *values))

def call(address, arguments=(), registers=(), stop=0x3000000):
    stack = 0x200ff00
    u32(stack, 0x3000000)
    for i, value in enumerate(arguments):
        u32(stack+4+i*4, value)
    guest.reg_write(UC_X86_REG_ESP, stack)
    guest.reg_write(UC_X86_REG_FPCW, 0x027f)
    guest.reg_write(UC_X86_REG_MXCSR, 0x1f80)
    for register, value in registers:
        guest.reg_write(register, value)
    guest.emu_start(address, stop, count=100000)
    assert guest.reg_read(UC_X86_REG_EIP) == stop, hex(guest.reg_read(UC_X86_REG_EIP))

SOURCE, VIEW, CLIP, SCALARS = 0x1000000, 0x1001000, 0x1002000, 0x1003000
GRID, SPAN, POSITION, OUTER, INNER = 0x1004000, 0x1004010, 0x1004020, 0x1004030, 0x1004040
call(0x377670)  # The original 70-degree baseline initializer.
call(0x377690)  # The original radians-per-degree initializer.
guest.mem_write(0x485a88, b'\0')
guest.mem_write(0x485ac5, b'\0\0')  # Standard-video startup branch.
u32(0x4e6948, 0)
u32(0x4e0350, 0)
u32(0x510c50, 0)
guest.mem_write(0x485ac2, b'\0')
call(0x12560, stop=0x125b3)  # Stop after the unchanged 16A30 startup call.
rectangles = struct.unpack('<8h', guest.mem_read(0x485a8a, 16))
assert rectangles == (0, 0, 480, 640, 36, 48, 444, 592)
guest.mem_write(GRID, struct.pack('<2h', 1, 1))
guest.mem_write(SPAN, struct.pack('<2h', 1, 1))
guest.mem_write(POSITION, bytes(4))
call(0x2f6b0, [GRID, SPAN, OUTER], [(UC_X86_REG_ESI, POSITION), (UC_X86_REG_EBX, INNER)])
outer = bytes(guest.mem_read(OUTER, 8))
inner = bytes(guest.mem_read(INNER, 8))
assert outer+inner == bytes(guest.mem_read(0x485a8a, 16))
rows = []
for degrees in (55., 70., 90.):
    guest.mem_write(SOURCE, bytes(0x54))
    floats(SOURCE+0x20, (1., 0., 0., 0., 0., 1.))
    # Match the native observer's float conversion; subsequent arithmetic is
    # performed by the unchanged original instructions.
    radians_per_degree = struct.unpack('<f', guest.mem_read(0x54e858, 4))[0]
    radians = struct.unpack('<f', struct.pack('<f', degrees*radians_per_degree))[0]
    floats(SOURCE+0x38, (radians, 1.3333333730697632, 0., 0.))
    call(0x171d90, registers=[(UC_X86_REG_ESI, SOURCE)])
    guest.mem_write(VIEW, bytes(0x74))
    call(0x2f800, [GRID], [(UC_X86_REG_EAX, SOURCE), (UC_X86_REG_EDX, SPAN), (UC_X86_REG_ECX, VIEW)])
    guest.mem_write(VIEW+0x30, outer+inner)
    call(0x2f970, registers=[(UC_X86_REG_EAX, VIEW), (UC_X86_REG_ESI, CLIP)])
    call(0x2fb70, registers=[(UC_X86_REG_ECX, VIEW), (UC_X86_REG_EDX, CLIP), (UC_X86_REG_EAX, SCALARS)])
    angle = struct.unpack('<f', guest.mem_read(VIEW+0x28, 4))[0]
    clip = struct.unpack('<4f', guest.mem_read(CLIP, 16))
    scale_xy = struct.unpack('<2f', guest.mem_read(SCALARS+0x28, 8))
    rows.append({'degrees': degrees, 'render_angle': angle, 'clip': clip, 'scale_xy': scale_xy})

p = ROOT/'analysis/frontend/xemu-capture'
uniforms = (p/'pass-1335-0-uniforms.bin').read_bytes()
captured = [math.sqrt(sum(v*v for v in struct.unpack_from('<4f', uniforms, i*16)[:3]))/16384 for i in range(2)]
error = max(abs(a-b) for a,b in zip(rows[1]['scale_xy'], captured))
assert error < 1e-6, (rows[1]['scale_xy'], captured, error)
fixture = b'H2MPROJ1'+struct.pack('<I', len(rows))
for row in rows:
    fixture += struct.pack('<6f', row['degrees'], row['render_angle'],
                           544/640, 408/480, *row['scale_xy'])
(p/'title-projection-original.bin').write_bytes(fixture)
# Execute the real display/cinematic branches too. These are input-branch
# fixtures; a widescreen frontend framebuffer comparison remains separate.
display_rows = []
CINEMATIC = 0x1005000
GAME_OPTIONS = 0x1006000
for high_definition in (False, True):
    guest.mem_write(0x485ac5, bytes([high_definition, high_definition]))
    call(0x12560, stop=0x125b3)
    display_rectangles = struct.unpack('<8h', guest.mem_read(0x485a8a, 16))
    call(0x2f6b0, [GRID, SPAN, OUTER], [(UC_X86_REG_ESI, POSITION), (UC_X86_REG_EBX, INNER)])
    display_outer = bytes(guest.mem_read(OUTER, 8))
    display_inner = bytes(guest.mem_read(INNER, 8))
    for widescreen in (False, True):
        guest.mem_write(0x485ac2, bytes([widescreen]))
        for cinematic in (False, True):
            u32(0x510c50, CINEMATIC)
            guest.mem_write(CINEMATIC, bytes(8))
            guest.mem_write(CINEMATIC+5, bytes([cinematic]))
            for degrees in (55., 70., 90.):
                guest.mem_write(SOURCE, bytes(0x54))
                floats(SOURCE+0x20, (1., 0., 0., 0., 0., 1.))
                radians = struct.unpack('<f', struct.pack('<f', degrees*radians_per_degree))[0]
                floats(SOURCE+0x38, (radians, 1.3333333730697632, 0., 0.))
                call(0x171d90, registers=[(UC_X86_REG_ESI, SOURCE)])
                guest.mem_write(VIEW, bytes(0x74))
                call(0x2f800, [GRID], [(UC_X86_REG_EAX, SOURCE), (UC_X86_REG_EDX, SPAN), (UC_X86_REG_ECX, VIEW)])
                guest.mem_write(VIEW+0x30, display_outer+display_inner)
                call(0x2f970, registers=[(UC_X86_REG_EAX, VIEW), (UC_X86_REG_ESI, CLIP)])
                call(0x2fb70, registers=[(UC_X86_REG_ECX, VIEW), (UC_X86_REG_EDX, CLIP), (UC_X86_REG_EAX, SCALARS)])
                display_rows.append({'main_menu': False, 'high_definition': high_definition, 'widescreen': widescreen,
                    'cinematic': cinematic, 'degrees': degrees, 'rectangles': display_rectangles,
                    'render_angle': struct.unpack('<f', guest.mem_read(VIEW+0x28, 4))[0],
                    'scale_xy': struct.unpack('<2f', guest.mem_read(SCALARS+0x28, 8))})
# A live original mainmenu capture establishes flag@+1=1, index@+2=0,
# state@+8=3. Run that original suppression condition with every video mode.
guest.mem_write(GAME_OPTIONS, struct.pack('<BBhII', 0, 1, 0, 0, 3))
u32(0x4e6948, GAME_OPTIONS)
for row in list(display_rows):
    guest.mem_write(0x485ac2, bytes([row['widescreen']]))
    floats(VIEW+0x28, [row['render_angle']])
    guest.mem_write(VIEW+0x30, struct.pack('<8h', *row['rectangles']))
    guest.mem_write(CINEMATIC+5, bytes([row['cinematic']]))
    call(0x2f970, registers=[(UC_X86_REG_EAX, VIEW), (UC_X86_REG_ESI, CLIP)])
    call(0x2fb70, registers=[(UC_X86_REG_ECX, VIEW), (UC_X86_REG_EDX, CLIP), (UC_X86_REG_EAX, SCALARS)])
    display_rows.append({**row, 'main_menu': True,
        'scale_xy': struct.unpack('<2f', guest.mem_read(SCALARS+0x28, 8))})
display_fixture = b'H2MDISP2'+struct.pack('<I', len(display_rows))
for row in display_rows:
    display_fixture += struct.pack('<4I4f', row['main_menu'], row['high_definition'], row['widescreen'],
        row['cinematic'], row['degrees'], row['render_angle'], *row['scale_xy'])
(p/'title-projection-display-original.bin').write_bytes(display_fixture)
report = {'executable_sha256': pin, 'scope': 'standard-video single-view frontend projection',
          'unchanged_entries': ['12560', '16A30', '2F6B0', '171D90', '2F800', '2F970', '2FB70'],
          'rectangles': rectangles, 'rows': rows, 'captured_scale_xy': captured,
          'maximum_capture_scale_error': error, 'widescreen_ui_parity_verified': False,
          'complete_native_frame_parity_verified': False,
          'display_branch_instruction_fixtures': display_rows}
(p/'title-original-projection.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
