#!/usr/bin/env python3
"""Measure the original retained voice-data CPU owner and numerical dependencies.

The whole original Xbox-in-xemu capture must agree first. The unchanged
original LightHRTF initializer then owns algorithm selection on the host32-bit
CPU.37AE68..37B129 executes between the actual kernel-lock boundaries, including
every reached numerical callee. Declared state/curve backing supplies numerical
inputs only; no kernel, device or callee return is supplied. This does not prove
native voice playback, physical DSP response or additional gameplay events.
"""
import argparse
import atexit
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from functions import _Code
from original_audio_cpu import OriginalAudioCpu


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    image_path = ROOT / 'assets/disc/default.xbe'
    assert sha(image_path) == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    image = Xbe(image_path)
    capture = json.loads((args.capture / 'report.json').read_text())
    assert capture['owner'] == 'voice_data' and capture['image_sha256'] == sha(image_path)
    assert not any(capture[key] for key in ('instruction_patches', 'guest_memory_writes',
                                          'guest_register_writes', 'supplied_callee_returns'))
    context = capture['registers']
    fp = context['fctrl'], context['mxcsr']
    layout = args.output / 'user-layout.c'
    layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc', str(layout), '-o', str(args.output / 'user-layout')], check=True)
    subprocess.run([str(args.output / 'user-layout')], check=True)
    host = OriginalAudioCpu(image, args.output / 'original32.elf', *fp)
    atexit.register(host.close)
    codes = [_Code(image, name) for name in ('DSOUND', '.text')]

    class Code:
        def at(self, address):
            return next((code.at(address) for code in codes if code.inside(address)), None)

    code = Code()
    aliases = [0x1001000, 0x1002000, 0x1003000, 0x1004000, 0x1005000]
    curve, frame, stack, stop = 0x1006000, 0x100d000, 0x100f000, 0x3000000

    def boundary(pc):
        for i in range(4):
            host.trace(6, host.debug_offset + i * 8, pc)

    boundary(stop)
    host.write(stack, struct.pack('<I', stop))
    result = host.run(0x37e126, stack)
    assert result.rsp == stack + 4
    assert host.read(0x3871c8, 44).hex() == capture['selected_sdk_table_hex']
    regions = sorted(capture['regions'], key=lambda row: row['argument'])
    assert [row['argument'] for row in regions] == list(range(5))
    template = [bytes.fromhex(row['before_hex']) for row in regions]
    expected = [bytes.fromhex(row['after_hex']) for row in regions]
    assert struct.unpack_from('<I', template[1], 0x50)[0] == 0, 'Uncaptured curve backing'
    rows, helpers, executed = [], [], set()

    def measure(label, actual, buffers, absent=(), curve_bytes=b'', context=fp, expected=None):
        host.control_word, host.mxcsr = context
        for address, raw in zip(aliases, buffers):
            host.write(address, raw)
        if curve_bytes:
            host.write(curve, curve_bytes)
        host.write(frame - 0x80, bytes(0x110))
        words = [0 if index in absent else address for index, address in enumerate(aliases)]
        host.write(frame + 8, struct.pack('<5I', *words))
        boundary(0x37b129)
        result, stores, visited = host.run_traced(0x37ae68, frame - 0x44, code,
            aliases[4], len(buffers[4]), stop_addresses=(0x37b129,), rbp=frame)
        assert result.rip == 0x37b129 and result.rsp == frame - 0x44
        after = [host.read(address, len(raw)) for address, raw in zip(aliases, buffers)]
        assert after[:4] == buffers[:4], 'Original voice-data changed an input owner'
        if expected is not None:
            assert after == expected, (label, [raw.hex() for raw in after])
        executed.update(visited)
        rows.append(dict(label=label, actual_whole_capture=actual,
            before_hex=[raw.hex() for raw in buffers], after_hex=[raw.hex() for raw in after],
            absent_arguments=list(absent), curve_hex=curve_bytes.hex(),
            x87_control_word=context[0], mxcsr=context[1],
            stores=[dict(instruction=f'{pc:08x}', address=f'{address:08x}', bytes=size,
                         before=f'{old:08x}', after=f'{new:08x}') for pc, address, size, old, new in stores]))

    measure(args.capture.name, True, template, expected=expected)

    def emit(label, edits=(), absent=(), curve_bytes=b'', context=fp):
        buffers = [bytearray(raw) for raw in template]
        for region, offset, value in edits:
            struct.pack_into('<I', buffers[region], offset, value)
        if curve_bytes:
            struct.pack_into('<II', buffers[1], 0x50, curve, len(curve_bytes)//4)
        measure(label, False, [bytes(raw) for raw in buffers], absent, curve_bytes, context)

    flags = [0, 0x400000, 0x15200010, 0x40140000, 0x34800008, 0x24000000,
             0x82800028, 0x20000000, 0x14000000, 0xffffffff]
    for region in (0, 1, 2, 3):
        for bit in range(32):
            emit(f'flag_{region}_{bit}', [(region, 0, 1 << bit)])
    for mode in (0, 1, 2, 0xffffffff):
        for changed in flags:
            for features in range(8):
                emit(f'mode_flags_{mode:08x}_{changed:08x}_{features}',
                     [(1, 0, changed), (1, 0x40, mode), (1, 0x58, features)])
    values = [bits(value) for value in (0., -0., .001, .5, 1., -1., 2., 45., 90., 180.)]
    values += [1, 0x00800000, 0x7f7fffff, 0x7f800000, 0xff800000, 0x7fc12345, 0x7f812345]
    fields = {0: (0x38, 0x3c, 0x40),
              1: (0x20, 0x24, 0x34, 0x38, 0x3c, 0x44, 0x48, 0x4c, 0x58, 0x68, 0x6c, 0x70, 0x74, 0x78),
              2: (0xc, 0x30), 3: (4, 8, 0xc, 0x14, 0x18, 0x1c, 0x20, 0x24),
              4: tuple(range(0, 0x38, 4))}
    for region, offsets in fields.items():
        for offset in offsets:
            for value in values:
                emit(f'field_{region}_{offset:02x}_{value:08x}',
                     [(1, 0, 0xffffffff), (region, offset, value)])
    for absent in ((2,), (3,), (2, 3)):
        for changed in flags:
            emit(f'absent_{absent}_{changed:08x}', [(1, 0, changed)], absent=absent)
    curves = [struct.pack('<4f', 1., .75, .25, .001), struct.pack('<4f', .1, .3, .8, 1.),
              struct.pack('<4I', 0, 0x7fc12345, 0x3f800000, 0xbf800000)]
    for index, raw in enumerate(curves):
        for value in values:
            for enabled in (0, 4):
                emit(f'curve_{index}_{value:08x}_{enabled}',
                     [(1, 0, 0xffffffff), (1, 0x38, bits(1)), (1, 0x3c, bits(32)),
                      (1, 0x68, value), (1, 0x58, enabled)], curve_bytes=raw)
    probe = random.Random(0x564f4943)
    for index in range(128):
        edits = [(1, 0, 0xffffffff), (1, 0x58, index % 8), (1, 0x40, index % 3)]
        for region, offsets in ((0, (0x38, 0x3c, 0x40)),
                                (1, (0x38, 0x3c, 0x44, 0x48, 0x4c, 0x68, 0x6c, 0x70, 0x74, 0x78))):
            edits.extend((region, offset, bits(probe.randrange(-4000, 4001)/977)) for offset in offsets)
        emit(f'varied_state_{index}', edits)
    for context in ((0x003f, 0x1f80), (0x063f, 0x3f80), (0x0a3f, 0x5f80), (0x0e3f, 0x7f80), (0x033f, 0x9fc0)):
        for value in values:
            emit(f'fp_state_{context[0]:04x}_{context[1]:04x}_{value:08x}',
                 [(1, 0, 0xffffffff), (1, 0x68, value), (1, 0x6c, value),
                  (1, 0x70, value), (1, 0x74, value), (3, 0x14, value)], context=context)

    def helper(owner, label, entry, words, outputs, backing=None, context=fp):
        host.control_word, host.mxcsr = context
        backing = {} if backing is None else backing
        for index, raw in backing.items():
            words[index] = 0x1007000 + index * 0x100
            host.write(words[index], raw)
        for index in outputs:
            words[index] = 0x1008000 + index * 0x100
            host.write(words[index], bytes(4))
        host.write(stack, struct.pack('<'+'I'*(len(words)+1), stop, *words))
        boundary(stop)
        result, _, visited = host.run_traced(entry, stack, code, 0x1008000, 0x1000,
                                            stop_addresses=(stop,))
        assert result.rsp == stack + 4*(len(words)+1)
        executed.update(visited)
        helpers.append(dict(owner=owner, label=label, entry=f'{entry:08x}',
            argument_words=[f'{value:08x}' for value in words], inputs_hex={str(i):raw.hex()for i,raw in backing.items()},
            output_hex={str(i):host.read(words[i],4).hex()for i in outputs},
            return_eax=f'{result.rax&0xffffffff:08x}', x87_control_word=context[0], mxcsr=context[1]))

    for i, a in enumerate(values):
        for j, b in enumerate(values):
            for enabled in (0, 1):
                helper('distance', f'distance_{i}_{j}_{enabled}', 0x37dca5,
                       [bits(1), bits(1), b, 0, 0, a, enabled, 0], [7])
            helper('cone_volume', f'cone_volume_{i}_{j}', 0x37ddb6,
                   [i*23, j*31, (-600)&0xffffffff, a, 0], [4])
            helper('doppler', f'doppler_{i}_{j}', 0x37de46, [a, b, bits(.35), 0], [3])
            helper('rear_widen', f'rear_widen_{i}_{j}', 0x37e112, [a, b, 0, 0], [2, 3])
    for index, raw in enumerate(curves):
        for value in values:
            helper('distance', f'curve_distance_{index}_{value:08x}', 0x37dca5,
                   [bits(1), bits(1), bits(32), 0, len(raw)//4, value, 0, 0], [7], {3: raw})
    for offset in fields[3]:
        for value in values:
            raw = bytearray(template[3]);struct.pack_into('<I', raw, offset, value)
            helper('environment', f'environment_{offset:02x}_{value:08x}', 0x38368b,
                   [bits(8), bits(1), bits(5000), 0, 0, 0, 0, 0], [4, 5, 6, 7], {3: bytes(raw)})
    for value in values:
        helper('gain', f'gain_{value:08x}', 0x37db07, [value], [])
        helper('volume', f'volume_{value:08x}', 0x379d54, [value], [])
    contexts = ((0x003f, 0x1f80), (0x063f, 0x3f80), (0x0a3f, 0x5f80),
                (0x0e3f, 0x7f80), (0x033f, 0x9fc0))
    curve_positions = [bits(value) for value in (1., 1.000001, 1.01, 2., 8.749999, 8.75, 8.750001,
        16.499998, 16.5, 16.500002, 24.249998, 24.25, 24.250002, 31.999998, 32., 32.000004)]
    for index, raw in enumerate(curves):
        for context in contexts:
            for value in curve_positions:
                for enabled in (0, 1):
                    label = f'dense_curve_{index}_{context[0]:04x}_{value:08x}_{enabled}'
                    helper('distance', label, 0x37dca5,
                           [bits(1), bits(1), bits(32), 0, len(raw)//4, value, enabled, 0], [7], {3: raw}, context)
                    emit(label, [(1, 0, 0xffffffff), (1, 0x38, bits(1)), (1, 0x3c, bits(32)),
                         (1, 0x68, value), (1, 0x58, enabled << 2)], curve_bytes=raw, context=context)
    cone_angles = (bits(0), bits(180), 0x7fc12345)
    for inner in (0, 1, 90, 180, 360, 0x7fffffff, 0x80000000, 0xffffffff):
        for outer in (0, 1, 90, 180, 360, 0x7fffffff, 0x80000000, 0xffffffff):
            for angle in cone_angles:
                for level in (-600, -10000):
                    helper('cone_volume', f'unsigned_cone_{inner:08x}_{outer:08x}_{angle:08x}_{level}',
                           0x37ddb6, [inner, outer, level & 0xffffffff, angle, 0], [4])
    velocities = [bits(value) for value in (-342.00003, -342., -341.99997, -100., -1e-8,
                                           0., 1e-8, 100., 341.99997, 342., 342.00003)]
    for context in contexts:
        for velocity in velocities:
            label = f'doppler_boundary_{context[0]:04x}_{velocity:08x}'
            helper('doppler', label, 0x37de46, [bits(1), bits(1), velocity, 0], [3], context=context)
            emit(label, [(1, 0, 0x82800028), (0, 0x38, bits(1)), (0, 0x40, bits(1)),
                 (1, 0x44, bits(1)), (1, 0x4c, bits(1)), (1, 0x6c, velocity)], context=context)
        for level in (-10000, -6000, -600, -1, 0):
            for mixture in (0., .25, 1., 2.):
                for frequency in [bits(value) for value in (0., 1., 5000., 23999., 24000., 48000.)]+[0x7f800000, 0x7fc12345]:
                    label = f'environment_filter_{context[0]:04x}_{level}_{mixture}_{frequency:08x}'
                    environment = bytearray(template[3])
                    env_edits = [(4, -100), (8, -100), (0xc, -200), (0x14, bits(1)),
                                 (0x18, level), (0x1c, bits(mixture)), (0x20, level), (0x24, bits(.25))]
                    for offset, value in env_edits:struct.pack_into('<I', environment, offset, value & 0xffffffff)
                    helper('environment', label, 0x38368b, [bits(8), bits(1), frequency, 0, 0, 0, 0, 0],
                           [4, 5, 6, 7], {3: bytes(environment)}, context)
                    emit(label, [(1, 0, 0x14000000), (1, 0x68, bits(8)), (1, 0x58, 2 if level == -600 else 0),
                         (2, 0xc, bits(1)), (2, 0x30, frequency)]+[(3, offset, value & 0xffffffff)for offset,value in env_edits],
                         context=context)
        for value in values:
            helper('gain', f'fp_gain_{context[0]:04x}_{value:08x}', 0x37db07, [value], [], context=context)
            helper('volume', f'fp_volume_{context[0]:04x}_{value:08x}', 0x379d54, [value], [], context=context)
    listing = args.output / 'executed-original.asm'
    listing.write_text(''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'
                               for pc in sorted(executed)))
    for pc in executed:
        instruction = code.at(pc)
        assert host.read(pc, instruction.size) == image.read(pc, instruction.size) == instruction.bytes
    host.close()
    literals = (0x45dbb8, 0x45dbd8, 0x45dbc0, 0x4171e4, 0x45dde8, 0x385a44,
                0x4171f4, 0x4171f0, 0x4122dc, 0x4172b4, 0x4123e8, 0x3869e4, 0x45e4f0)
    fixture = args.output / 'fixtures.json'
    fixture.write_text(json.dumps(dict(schema='ORIGINAL_SOUND_SDK_VOICE_DATA_V1', aliases=aliases,
        curve_alias=curve, original_literal_hex={f'{address:08x}':image.read(address,4).hex()for address in literals},
        original_double_literal_hex={'0045dcc0': image.read(0x45dcc0,8).hex()}, cases=rows, helpers=helpers),separators=(',',':'))+'\n')
    report = dict(boundary=__doc__, tool_sha256=sha(__file__), host_runner_sha256=sha(ROOT/'tools/original_audio_cpu.py'),
        image_sha256=sha(image_path), capture_sha256=sha(args.capture/'report.json'), fixture_sha256=sha(fixture),
        original_whole_initializer_executed=True, actual_whole_capture_state_equal=True,
        actual_calls=1, declared_states=len(rows)-1, complete_helper_calls=len(helpers),
        executed_instruction_rows=len(executed), ordered_voice_stores=sum(len(row['stores'])for row in rows),
        instruction_patches=[], supplied_callee_returns=[], expected_values_from_native=False,
        native_or_physical_DSP_claimed=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Original voice data: genuine whole-call state agrees; {len(rows)-1} declared states, '
          f'{len(helpers)} complete helpers, {report["ordered_voice_stores"]} ordered voice stores')


if __name__ == '__main__':
    main()
