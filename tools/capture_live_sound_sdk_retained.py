"""Observe a complete genuine retained SDK calculation without guest writes.

Run in GDB at base37CA0A or37AE57. Entry and actual-return hardware breakpoints
observe the original caller, kernel lock owners and every numerical callee
executing normally. Raw retained inputs/outputs are evidence for this one call;
no native DSP, event identity or physical Xbox waveform is inferred.
H2_SOUND_RETAINED_OWNER selects calculate or voice_data.
H2_SOUND_RETAINED_OUTPUT must name a new directory.
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
    'calculate': (0x37ca0a, 2, [('retained', 0, 0x50), ('retained', 1, 0x7c)]),
    'voice_data': (0x37ae57, 5, [('input', 0, 0x50), ('input', 1, 0x7c),
                  ('input', 2, 0x34), ('input', 3, 0x28), ('output', 4, 0x38)]),
}
owner = os.environ['H2_SOUND_RETAINED_OWNER']
entry, count, extents = OWNERS[owner]
output = Path(os.environ['H2_SOUND_RETAINED_OUTPUT'])
output.mkdir(parents=True, exist_ok=False)
image_path = ROOT / 'assets/disc/default.xbe'
sha = lambda raw: hashlib.sha256(raw).hexdigest()
assert sha(image_path.read_bytes()) == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image = Xbe(image_path)
inferior = gdb.selected_inferior()
read = lambda address, size: bytes(inferior.read_memory(address, size))
reg = lambda name: int(gdb.parse_and_eval('$' + name)) & 0xffffffff
assert reg('eip') == entry
stack = reg('esp')
return_pc = struct.unpack('<I', read(stack, 4))[0]
arguments = struct.unpack('<' + 'I' * count, read(stack + 4, count * 4))
verified = []
for line in (ROOT / f'analysis/audio-spatial-20261011/sdk-base-v1/{entry:08x}.asm').read_text().splitlines():
    fields = line.split(); pc, raw = int(fields[0], 16), bytes.fromhex(fields[1])
    assert raw == image.read(pc, len(raw)) == read(pc, len(raw))
    verified.append(dict(pc=fields[0], bytes=fields[1]))
regions = [dict(role=role, argument=index, address=f'{arguments[index]:08x}', size=size,
                before_hex=read(arguments[index], size).hex())
           for role, index, size in extents if arguments[index]]
table = read(0x3871c8, 44)
report = dict(boundary=__doc__, owner=owner, original_entry=f'{entry:08x}',
    image_sha256=sha(image_path.read_bytes()), tool_sha256=sha(Path(__file__).read_bytes()),
    registers={name: reg(name) for name in ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi',
               'ebp', 'esp', 'eip', 'eflags', 'fctrl', 'fstat', 'ftag', 'mxcsr')},
    original_return_pc=f'{return_pc:08x}', argument_words=[f'{value:08x}' for value in arguments],
    regions=regions, selected_sdk_table_hex=table.hex(), unchanged_instructions=verified,
    instruction_patches=[], guest_memory_writes=[], guest_register_writes=[],
    supplied_callee_returns=[], native_or_physical_DSP_claimed=False)
(output / 'entry.json').write_text(json.dumps(report, indent=2) + '\n')


def finish_retained_capture():
    assert reg('eip') == return_pc
    assert reg('esp') == stack + 4 + count * 4
    assert read(0x3871c8, 44) == table
    for region in regions:
        region['after_hex'] = read(int(region['address'], 16), region['size']).hex()
        if region['role'] == 'input':
            assert region['before_hex'] == region['after_hex']
    report['actual_return_pc'] = f'{reg("eip"):08x}'
    report['actual_return_esp'] = f'{reg("esp"):08x}'
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Original {owner} returned normally; {len(verified)} unchanged instructions; '
          f'{len(regions)} raw input/output regions; no supplied kernel/callee return')


gdb.set_convenience_variable('sound_retained_return', return_pc)
print(f'Original {owner} entry; arguments={report["argument_words"]}')
