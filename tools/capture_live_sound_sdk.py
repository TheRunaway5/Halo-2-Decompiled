"""Observe one complete genuine original SDK spatial helper without guest writes.

Run in GDB at a supported original helper entry. The original caller, helper
and all callees execute normally. Only hardware entry/return breakpoints are
used; no CPU/kernel/SDK result is supplied. Raw argument backing and outputs
are evidence for this initialized call, not proof of native DSP or audibility.
H2_SOUND_SDK_OWNER selects position, angles, front_rear or kernel.
H2_SOUND_SDK_OUTPUT names a new evidence directory.
"""
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import gdb

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

OWNERS = {
    # Argument index and exact source-accessed memory extent.
    'position': (0x37debf, 5, [(1, 12), (2, 12)], [(3, 12), (4, 4)]),
    'angles': (0x37def9, 8, [(1, 12), (3, 12), (4, 12)], [(6, 4), (7, 4)]),
    'front_rear': (0x37dfe1, 7, [], [(5, 4), (6, 4)]),
    'kernel': (0x37e098, 4, [], [(3, 8)]),
}
owner = os.environ['H2_SOUND_SDK_OWNER']
entry, count, inputs, outputs = OWNERS[owner]
output = Path(os.environ['H2_SOUND_SDK_OUTPUT'])
output.mkdir(parents=True, exist_ok=False)
image_path = ROOT / 'assets/disc/default.xbe'
sha = lambda raw: hashlib.sha256(raw).hexdigest()
image_sha = sha(image_path.read_bytes())
assert image_sha == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image = Xbe(image_path)
inferior = gdb.selected_inferior()


def read(address, size):
    return bytes(inferior.read_memory(address, size))


def register(name):
    return int(gdb.parse_and_eval('$' + name)) & 0xffffffff


registers = {name: register(name) for name in ('eax', 'ebx', 'ecx', 'edx', 'esi',
             'edi', 'ebp', 'esp', 'eip', 'eflags', 'fctrl', 'fstat', 'ftag', 'mxcsr')}
assert registers['eip'] == entry
stack = registers['esp']
return_pc = struct.unpack('<I', read(stack, 4))[0]
arguments = struct.unpack('<' + 'I' * count, read(stack + 4, 4 * count))
listing = ROOT / f'analysis/audio-spatial-20261011/sdk-base-v1/{entry:08x}.asm'
verified = []
for line in listing.read_text().splitlines():
    fields = line.split()
    pc, raw = int(fields[0], 16), bytes.fromhex(fields[1])
    assert image.read(pc, len(raw)) == raw == read(pc, len(raw))
    verified.append(dict(pc=fields[0], bytes=fields[1]))
regions = []
for role, extents in [('input', inputs), ('output', outputs)]:
    for index, size in extents:
        regions.append(dict(role=role, argument=index, address=f'{arguments[index]:08x}',
                            size=size, before_hex=read(arguments[index], size).hex()))
table = read(0x3871c8, 44)
report = dict(boundary=__doc__, owner=owner, original_entry=f'{entry:08x}',
              image_sha256=image_sha, tool_sha256=sha(Path(__file__).read_bytes()),
              registers=registers, original_return_pc=f'{return_pc:08x}',
              argument_words=[f'{value:08x}' for value in arguments], regions=regions,
              selected_sdk_table_hex=table.hex(), unchanged_instructions=verified,
              instruction_patches=[], guest_memory_writes=[], guest_register_writes=[],
              supplied_callee_returns=[], native_or_audible_parity_claimed=False)
(output / 'entry.json').write_text(json.dumps(report, indent=2) + '\n')


def finish_sdk_capture():
    assert register('eip') == return_pc
    assert register('esp') == stack + 4 + 4 * count
    assert read(0x3871c8, 44) == table
    for region in regions:
        region['after_hex'] = read(int(region['address'], 16), region['size']).hex()
        if region['role'] == 'input':
            assert region['after_hex'] == region['before_hex']
    if owner == 'kernel':
        kernels = struct.unpack('<2I', read(arguments[3], 8))
        report['kernel_original32_bytes'] = []
        for address in kernels:
            raw = read(address, 32)
            assert raw == image.read(address, 32)
            report['kernel_original32_bytes'].append(dict(address=f'{address:08x}', bytes_hex=raw.hex()))
    report['actual_return_pc'] = f'{register("eip"):08x}'
    report['actual_return_esp'] = f'{register("esp"):08x}'
    report['actual_return_eax'] = f'{register("eax"):08x}'
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Original {owner} helper returned normally; {len(verified)} unchanged instructions; '
          f'{len(outputs)} captured outputs; no supplied callee result')


gdb.set_convenience_variable('audio_sdk_return', return_pc)
print(f'Original {owner} helper entry; arguments={report["argument_words"]}')
