"""Observe a genuine original HRTF application, including its MCPX writes.

Run in GDB at base3812E0. Hardware execution breakpoints read the original
CPU's pending write address/value; the game executes every write and both
kernel lock owners normally. Device memory is not read or supplied. This is
Xbox-in-xemu driver evidence, not physical DSP or native audible proof.
H2_SOUND_HRTF_OUTPUT must name a new directory.
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

output = Path(os.environ['H2_SOUND_HRTF_OUTPUT'])
output.mkdir(parents=True, exist_ok=False)
image_path = ROOT / 'assets/disc/default.xbe'
sha = lambda raw: hashlib.sha256(raw).hexdigest()
assert sha(image_path.read_bytes()) == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image = Xbe(image_path)
inferior = gdb.selected_inferior()
read = lambda address, size: bytes(inferior.read_memory(address, size))
reg = lambda name: int(gdb.parse_and_eval('$' + name)) & 0xffffffff
word = lambda address: struct.unpack('<I', read(address, 4))[0]
assert reg('eip') == 0x3812e0
stack, voice = reg('esp'), reg('ecx')
return_pc = word(stack)
voice_before = read(voice, 0x74)
data = word(voice + 0x70)
data_before = read(data, 0xbc)
source, parameters = word(data + 0xb4), word(data + 0xb8)
arguments = struct.unpack('<2I', read(stack + 4, 8))
registers = {name: reg(name) for name in ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi',
             'ebp', 'esp', 'eip', 'eflags', 'fctrl', 'fstat', 'ftag', 'mxcsr')}
verified = []
for line in (ROOT / 'analysis/audio-spatial-20261011/sdk-base-v1/003812e0.asm').read_text().splitlines():
    fields = line.split()
    pc, raw = int(fields[0], 16), bytes.fromhex(fields[1])
    assert raw == image.read(pc, len(raw)) == read(pc, len(raw))
    verified.append(dict(pc=fields[0], bytes=fields[1]))
report = dict(boundary=__doc__, image_sha256=sha(image_path.read_bytes()),
              tool_sha256=sha(Path(__file__).read_bytes()), registers=registers,
              voice_address=f'{voice:08x}', voice_before_hex=voice_before.hex(),
              data_address=f'{data:08x}', data_before_hex=data_before.hex(),
              source_address=f'{source:08x}', parameters_address=f'{parameters:08x}',
              argument_words=[f'{v:08x}' for v in arguments],
              source_before_hex=read(source, 0x44).hex() if source else None,
              parameters_before_hex=read(parameters, 0x38).hex() if parameters else None,
              speaker_config_hex=read(0x386b04, 4).hex(),
              selected_sdk_table_hex=read(0x3871c8, 44).hex(),
              unchanged_instructions=verified, observed_device_writes=[],
              instruction_patches=[], guest_memory_writes=[], guest_register_writes=[],
              supplied_callee_returns=[], native_or_physical_DSP_claimed=False)
(output / 'entry.json').write_text(json.dumps(report, indent=2) + '\n')


def reach(pc):
    bp = gdb.Breakpoint(f'*0x{pc:x}', type=gdb.BP_HARDWARE_BREAKPOINT, temporary=True)
    gdb.execute('continue')
    assert reg('eip') == pc, (hex(reg('eip')), hex(pc))
    if bp.is_valid():
        bp.delete()


def pending(pc, address, value):
    assert reg('eip') == pc
    assert reg('ebp') == stack - 4
    report['observed_device_writes'].append(dict(pc=f'{pc:08x}', address=f'{address:08x}', value=f'{value:08x}'))


def finish_hrtf_capture():
    reach(0x38139f)
    pending(0x38139f, 0xfe820160, reg('eax'))
    bp = gdb.Breakpoint('*0x3813ec', type=gdb.BP_HARDWARE_BREAKPOINT)
    for index in range(15):
        gdb.execute('continue')
        assert reg('eip') == 0x3813ec
        assert reg('ebx') == 0xfe820400 + index * 4
        pending(0x3813ec, reg('ebx'), reg('edx'))
        if index == 0:
            kernels = []
            for pointer in (reg('ecx'), reg('edi')):
                raw = read(pointer, 32)
                assert raw == image.read(pointer, 32)
                kernels.append(dict(address=f'{pointer:08x}', bytes_hex=raw.hex()))
            report['selected_original_kernels'] = kernels
    bp.delete()
    for pc, address in [(0x381427, 0xfe82043c), (0x381433, 0xfe8202f8),
                        (0x38143e, 0xfe82031c)]:
        reach(pc)
        pending(pc, address, reg('eax'))
    reach(return_pc)
    assert reg('esp') == stack + 12
    report['voice_after_hex'] = read(voice, 0x74).hex()
    report['data_after_hex'] = read(data, 0xbc).hex()
    report['actual_return_pc'] = f'{return_pc:08x}'
    report['actual_return_esp'] = f'{reg("esp"):08x}'
    report['device_writes_executed_normally'] = True
    assert len(report['observed_device_writes']) == 19
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Original HRTF application returned normally; 19 original MCPX writes observed; no supplied SDK/kernel/device result')
