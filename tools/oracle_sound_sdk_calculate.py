#!/usr/bin/env python3
"""Measure original retained SDK calculation and complete numerical callees.

Actual whole Xbox-in-xemu captures must agree at the CPU contract first. The
unchanged original LightHRTF initializer executes on the host32-bit CPU; then
37CA1F..37CC39 runs between the original lock boundaries, including every
reached numerical callee and ordered retained store. Additional cases supply
declared ABI state, not source readiness, kernel returns or extra game events.
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
    parser.add_argument('--capture', type=Path, action='append', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    image_path = ROOT / 'assets/disc/default.xbe'
    assert sha(image_path) == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    image = Xbe(image_path)
    captures = []
    for path in args.capture:
        capture = json.loads((path / 'report.json').read_text())
        assert capture['owner'] == 'calculate' and capture['image_sha256'] == sha(image_path)
        assert not any(capture[key] for key in ('instruction_patches', 'guest_memory_writes', 'guest_register_writes', 'supplied_callee_returns'))
        captures.append((path, capture))
    context = captures[0][1]['registers']
    fp = (context['fctrl'], context['mxcsr'])
    layout = args.output / 'user-layout.c'
    layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc', str(layout), '-o', str(args.output / 'user-layout')], check=True)
    subprocess.run([str(args.output / 'user-layout')], check=True)
    host = OriginalAudioCpu(image, args.output / 'original32.elf', *fp)
    atexit.register(host.close)
    code = _Code(image, 'DSOUND')
    listener, source, frame, stack, stop = 0x1001000, 0x1002000, 0x100d000, 0x100f000, 0x3000000

    def boundary(pc):
        for i in range(4):
            host.trace(6, host.debug_offset + i * 8, pc)

    boundary(stop)
    host.write(stack, struct.pack('<I', stop))
    result = host.run(0x37e126, stack)
    assert result.rsp == stack + 4
    assert host.read(0x3871c8, 44).hex() == captures[0][1]['selected_sdk_table_hex']
    rows, helper_rows, executed = [], [], set()

    def measure(label, actual, before_listener, before_source, context, expected=None):
        host.control_word, host.mxcsr = context
        host.write(listener, before_listener)
        if before_source is not None:
            host.write(source, before_source)
        host.write(frame - 0x80, bytes(0x110))
        host.write(frame + 0x78, struct.pack('<2I', listener, source if before_source is not None else 0))
        boundary(0x37cc39)
        result, stores, visited = host.run_traced(0x37ca1f, frame - 0x6c, code,
            listener, source + 0x7c - listener, stop_addresses=(0x37cc39,), rbp=frame)
        assert result.rip == 0x37cc39 and result.rsp == frame - 0x6c
        after = [host.read(listener, len(before_listener)),
                 host.read(source, len(before_source)) if before_source is not None else None]
        if expected is not None:
            assert after == expected, (label, [v.hex() if v else None for v in after],
                                      [v.hex() if v else None for v in expected])
        executed.update(visited)
        rows.append(dict(label=label, actual_whole_capture=actual,
            listener_before_hex=before_listener.hex(), source_before_hex=before_source.hex() if before_source is not None else None,
            listener_after_hex=after[0].hex(), source_after_hex=after[1].hex() if after[1] is not None else None,
            x87_control_word=context[0], mxcsr=context[1],
            stores=[dict(instruction=f'{pc:08x}', address=f'{address:08x}', bytes=size,
                         before=f'{old:08x}', after=f'{new:08x}') for pc, address, size, old, new in stores]))

    for path, capture in captures:
        regions = {region['argument']: region for region in capture['regions']}
        before_l = bytes.fromhex(regions[0]['before_hex'])
        before_s = bytes.fromhex(regions[1]['before_hex']) if 1 in regions else None
        expected = [bytes.fromhex(regions[0]['after_hex']),
                    bytes.fromhex(regions[1]['after_hex']) if 1 in regions else None]
        context = capture['registers']
        measure(path.name, True, before_l, before_s, (context['fctrl'], context['mxcsr']), expected)
    template = next(capture for _, capture in captures if len(capture['regions']) == 2)
    before_l, before_s = [bytes.fromhex(region['before_hex']) for region in template['regions']]

    def emit(label, l_edits=(), s_edits=(), absent=False, context=fp):
        l, s = bytearray(before_l), bytearray(before_s)
        for offset, value in l_edits: struct.pack_into('<I', l, offset, value)
        for offset, value in s_edits: struct.pack_into('<I', s, offset, value)
        measure(label, False, l, None if absent else s, context)

    for field in (0, 4):
        for bit in range(32):
            emit(f'listener_flag_{field}_{bit}', [(field, 1 << bit)])
            emit(f'source_flag_{field}_{bit}', s_edits=[(field, 1 << bit)])
    for mode in (0, 1, 2, 0xffffffff):
        for changed in (0, 4, 0x400000, 0x10000, 0x08420002, 0xffffffff):
            for valid in (0, 0x40, 0x18000000, 0xf8000040, 0xffffffff):
                emit(f'mode_flags_{mode:08x}_{changed:08x}_{valid:08x}', [(0, 0), (4, valid & 0x7f)],
                     [(0, changed), (4, valid & 0xffff0000), (0x40, mode)])
    values = [bits(v) for v in (0., -0., .001, .5, 1., -1., 2., 45., 90., 180.)]
    values += [1, 0x00800000, 0x7f7fffff, 0x7f800000, 0xff800000, 0x7fc12345, 0x7f812345]
    for offset in (8, 0xc, 0x10, 0x14, 0x18, 0x1c, 0x28, 0x2c, 0x30, 0x5c, 0x60, 0x64, 0x68, 0x6c, 0x70, 0x74, 0x78):
        for value in values:
            emit(f'source_field_{offset:02x}_{value:08x}', s_edits=[(offset, value), (0, 0x07ff0000), (4, 0)])
    for offset in (8, 0xc, 0x10, 0x14, 0x18, 0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x44, 0x48, 0x4c):
        for value in values:
            emit(f'listener_field_{offset:02x}_{value:08x}', [(offset, value), (0, 0x7f), (4, 0)], [(0, 0x07ff0000), (4, 0)])
    probe = random.Random(0x43414c43)
    for i in range(192):
        l, s = [(offset, bits(probe.randrange(-4000, 4001) / 733)) for offset in (8, 0xc, 0x10, 0x14, 0x18, 0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30, 0x34)], [(offset, bits(probe.randrange(-4000, 4001) / 977)) for offset in (8, 0xc, 0x10, 0x14, 0x18, 0x1c, 0x28, 0x2c, 0x30)]
        emit(f'rotated_state_{i}', l+[(0, 0x7f), (4, 0)], s+[(0, 0x07ff0000), (4, 0), (0x40, i % 3)])
    for flags in (0, 4, 0x7f):
        emit(f'absent_source_{flags}', [(0, flags), (4, 0)], absent=True)

    def helper(owner, label, words, inputs, output_index, size, context=fp):
        entries = dict(cross=0x37db87, cone=0x37dbc4, velocity=0x37dc5a)
        host.control_word, host.mxcsr = context
        backing = {}
        for index, raw in inputs.items():
            words[index] = 0x1005000 + index * 0x100
            host.write(words[index], raw)
            backing[str(index)] = raw.hex()
        words[output_index] = 0x1006000
        host.write(words[output_index], bytes(size))
        host.write(stack, struct.pack('<' + 'I' * (len(words)+1), stop, *words))
        boundary(stop)
        result = host.run(entries[owner], stack)
        assert result.rsp == stack + 4 * (len(words)+1)
        helper_rows.append(dict(owner=owner, label=label,
            argument_words=[f'{value:08x}' for value in words], inputs_hex=backing,
            x87_control_word=context[0], mxcsr=context[1], output_hex=host.read(words[output_index], size).hex()))

    vector = lambda *v: struct.pack('<3I', *v)
    for i, a in enumerate(values):
        for j, b in enumerate(values):
            first, second = vector(a, values[(i+3) % len(values)], b), vector(b, a, values[(j+5) % len(values)])
            helper('cross', f'cross_{i}_{j}', [0, 0, 0], {0: first, 1: second}, 2, 12)
            helper('cone', f'cone_{i}_{j}', [0, 0, 0], {0: first, 1: second}, 2, 4)
            for mode in (0, 1, 2):
                helper('velocity', f'velocity_{mode}_{i}_{j}', [mode, 0, 0, 0, 0],
                       {1: first, 2: second, 3: vector(bits(.6), bits(-.2), bits(.8))}, 4, 4)
    for context in ((0x003f, 0x1f80), (0x063f, 0x3f80), (0x0a3f, 0x5f80), (0x0e3f, 0x7f80), (0x033f, 0x9fc0)):
        for value in values:
            emit(f'fp_state_{context[0]:04x}_{context[1]:04x}_{value:08x}', [(0, 0x7f), (4, 0), (0x20, value)],
                 [(0, 0x07ff0000), (4, 0), (8, value), (0x28, value)], context=context)
    instructions = args.output / 'executed-original.asm'
    instructions.write_text(''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n' for pc in sorted(executed)))
    for pc in executed:
        instruction = code.at(pc)
        assert host.read(pc, instruction.size) == image.read(pc, instruction.size) == instruction.bytes
    host.close()
    fixture = args.output / 'fixtures.json'
    fixture.write_text(json.dumps(dict(schema='ORIGINAL_SOUND_SDK_CALCULATE_V1', listener_alias=listener,
        source_alias=source, cone_angle_scale_hex=image.read(0x45dc10, 4).hex(), cases=rows, helpers=helper_rows), separators=(',', ':'))+'\n')
    report = dict(boundary=__doc__, tool_sha256=sha(__file__), host_runner_sha256=sha(ROOT / 'tools/original_audio_cpu.py'),
        image_sha256=sha(image_path), fixture_sha256=sha(fixture), original_whole_initializer_executed=True,
        actual_captures=[dict(path=str(path), sha256=sha(path/'report.json'))for path,_ in captures],
        actual_capture_states_equal=True, actual_calls=len(captures), declared_states=len(rows)-len(captures),
        complete_helper_cases=len(helper_rows), executed_instruction_rows=len(executed),
        ordered_retained_stores=sum(len(row['stores'])for row in rows),
        instruction_patches=[], supplied_callee_returns=[], expected_values_from_native=False,
        native_or_physical_DSP_claimed=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Original SDK calculation: {len(captures)} actual states agree; {len(rows)-len(captures)} declared states; '
          f'{len(helper_rows)} complete helpers; {report["ordered_retained_stores"]} ordered stores; no supplied kernel result')


if __name__ == '__main__':
    main()
