"""Observe an original gain, pitch or filter application and real device writes.

Invoke at its original entry with a hardware breakpoint. Original code, locks,
FIFO reads and device writes all execute normally. Only initialized active
non-recursive voices are selected here; recursive pitch needs separate proof.
This is Xbox-in-xemu evidence, not physical DSP or native audible validation.
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

kind = os.environ['H2_SOUND_DEVICE_KIND']
specs = {
    'gain': (0x381ce4, 0x380b97, 0x381d0a, [
        (0x381d2c, 0xfe8202f8, 'edx'), (0x381d36, 0xfe8202fc, 1),
        (0x381d40, 0xfe820360, 'edx'), (0x381d4a, 0xfe820364, 'edx'),
        (0x381d54, 0xfe820368, 'edx'), (0x381d5a, 0xfe8202fc, 0)]),
    'pitch': (0x381d7c, 0x380deb, 0x381dc6, [
        (0x381de6, 0xfe8202f8, 'edx'), (0x381def, 0xfe82037c, 'edx')]),
    'filter': (0x381710, None, None, [
        (0x38180d, 0xfe8202f8, 'esi'), (0x381813, 0xfe8202fc, 1),
        (0x38181c, 0xfe820318, 'esi'), (0x381825, 0xfe820374, 'esi'),
        (0x38182e, 0xfe820378, 'esi'), (0x381834, 0xfe8202fc, 0)])}
entry, helper, helper_return, commands = specs[kind]
output = Path(os.environ['H2_SOUND_DEVICE_OUTPUT'])
output.mkdir(parents=True, exist_ok=False)
image_path = ROOT / 'assets/disc/default.xbe'
sha = lambda raw: hashlib.sha256(raw).hexdigest()
assert sha(image_path.read_bytes()) == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image = Xbe(image_path)
inferior = gdb.selected_inferior()
read = lambda address, size: bytes(inferior.read_memory(address, size))
reg = lambda name: int(gdb.parse_and_eval('$' + name)) & 0xffffffff
word = lambda address: struct.unpack('<I', read(address, 4))[0]
assert reg('eip') == entry
voice, stack = reg('ecx'), reg('esp')
data = word(voice + 0x70)
assert read(voice + 0x12, 1)[0] & 1
count = read(voice + 0x64, 1)[0]
assert 1 <= count <= 3, count
if kind == 'pitch':
    assert not word(data + 8) & 0x182000, 'Select an active ordinary pitch call'
return_pc = word(stack)
regions = []


def backing(label, address, size):
    if address:
        regions.append(dict(label=label, address=f'{address:08x}',
                            before_hex=read(address, size).hex()))


backing('voice', voice, 0x74)
backing('data', data, 0xbc)
backing('parameters', word(data + 0xb8), 0x38)
if kind == 'pitch':
    parent = word(data + 0xb0)
    backing('parent_link', parent, 0x24)
    if parent:
        owner = word(parent + 0x20)
        backing('parent_owner', owner, 0x84)
        if owner:
            parent_data = word(owner + 0x80)
            backing('parent_data', parent_data, 0xbc)
            backing('parent_parameters', word(parent_data + 0xb8), 0x38)
argument = word(stack + 4) if kind == 'filter' else None
if argument:
    backing('filter_override', argument, 0x18)
verified = []
for start, directory in [(entry, 'sdk-device-application-base-v1'),
                          (helper, 'sdk-device-dependencies-base-v1')]:
    if start is None:
        continue
    for line in (ROOT / f'analysis/audio-spatial-20261011/{directory}/{start:08x}.asm').read_text().splitlines():
        fields = line.split()
        pc, raw = int(fields[0], 16), bytes.fromhex(fields[1])
        assert raw == image.read(pc, len(raw)) == read(pc, len(raw))
        verified.append(dict(pc=fields[0], bytes=fields[1]))
report = dict(boundary=__doc__, kind=kind, image_sha256=sha(image_path.read_bytes()),
              tool_sha256=sha(Path(__file__).read_bytes()), regions=regions,
              registers={name: reg(name) for name in ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi',
                   'ebp', 'esp', 'eip', 'eflags', 'fctrl', 'fstat', 'ftag', 'mxcsr')},
              filter_override=None if argument is None else f'{argument:08x}',
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


def finish_device_capture():
    if helper:
        reach(helper)
        helper_stack = reg('esp')
        assert word(helper_stack) == helper_return
        helper_output = word(helper_stack + 4)
        reach(helper_return)
        assert reg('esp') == helper_stack + 8
        report['actual_helper_output_hex'] = read(helper_output, 0x24 if kind == 'gain' else 4).hex()
    for index in range(count):
        for pc, address, value in commands:
            reach(pc)
            assert reg('ebp') == stack - 4
            report['observed_device_writes'].append(dict(pc=f'{pc:08x}', address=f'{address:08x}',
                value=f'{reg(value) if isinstance(value, str) else value:08x}', voice_index=index))
    reach(return_pc)
    assert reg('esp') == stack + (8 if kind == 'filter' else 4)
    assert reg('eax') == 0
    for region in regions:
        region['after_hex'] = read(int(region['address'], 16), len(bytes.fromhex(region['before_hex']))).hex()
    report.update(actual_return_pc=f'{return_pc:08x}', actual_return_esp=f'{reg("esp"):08x}',
                  device_writes_executed_normally=True, command_count=len(commands) * count)
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Original {kind} returned normally; {len(commands) * count} original device writes observed')
