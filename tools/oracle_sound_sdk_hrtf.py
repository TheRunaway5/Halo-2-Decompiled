#!/usr/bin/env python3
"""Measure original HRTF CPU boundaries without supplying device/kernel returns.

The unchanged original algorithm initializer executes first. Declared retained
ABI backing then runs3812FA..38138F (after the kernel lock, before FIFO access)
and each original packing block up to its pending device write. The actual
capture's command words must agree. Additional cases are explicit numerical
controls, not initialized Xbox calls or complete platform/DSP execution.
"""
import argparse
import atexit
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
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
    capture = json.loads((args.capture / 'report.json').read_text())
    image_path = ROOT / 'assets/disc/default.xbe'
    assert sha(image_path) == capture['image_sha256'] == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    assert capture['device_writes_executed_normally']
    assert not any(capture[key] for key in ('instruction_patches', 'guest_memory_writes', 'guest_register_writes', 'supplied_callee_returns'))
    layout = args.output / 'user-layout.c'
    layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc', str(layout), '-o', str(args.output / 'user-layout')], check=True)
    subprocess.run([str(args.output / 'user-layout')], check=True)
    image = Xbe(image_path)
    fp = (capture['registers']['fctrl'], capture['registers']['mxcsr'])
    host = OriginalAudioCpu(image, args.output / 'original32.elf', *fp)
    atexit.register(host.close)
    voice, data, source, parameters, frame = 0x1001000, 0x1002000, 0x1003000, 0x1004000, 0x100e000

    def run(entry, boundary, **registers):
        for index in range(4):
            host.trace(6, host.debug_offset + index * 8, boundary)
        host.prepare(entry, frame - 0x28, registers)
        host.trace(7)
        result = host.wait_stop()
        assert result.rip == boundary, (hex(result.rip), hex(boundary))
        return result

    host.write(frame, struct.pack('<I', 0x3000000))
    result = run(0x37e126, 0x3000000, rsp=frame)
    assert result.rsp == frame + 4
    assert host.read(0x3871c8, 44).hex() == capture['selected_sdk_table_hex']
    rows = []
    initial_voice = bytes.fromhex(capture['voice_before_hex'])
    initial_data = bytes.fromhex(capture['data_before_hex'])
    initial_source = bytes.fromhex(capture['source_before_hex'])
    initial_parameters = bytes.fromhex(capture['parameters_before_hex'])

    def measure(label, actual, angle=None, elevation=None, mode=None, bank=None, handle=None,
                configuration=None, source_override=False, parameters_override=False,
                absent_source=False, absent_parameters=False):
        v, d, s, p = (bytearray(raw) for raw in (initial_voice, initial_data, initial_source, initial_parameters))
        struct.pack_into('<I', v, 0x70, data)
        struct.pack_into('<II', d, 0xb4, 0 if absent_source else source, 0 if absent_parameters else parameters)
        if angle is not None: struct.pack_into('<I', p, 0x28, angle)
        if elevation is not None: struct.pack_into('<I', p, 0x2c, elevation)
        if mode is not None: struct.pack_into('<I', s, 0x40, mode)
        if bank is not None: v[0x66] = bank
        if handle is not None: struct.pack_into('<H', v, 0xc, handle)
        config = bytes.fromhex(capture['speaker_config_hex']) if configuration is None else struct.pack('<I', configuration)
        for address, raw in ((voice, v), (data, d), (source, s), (parameters, p)):
            host.write(address, raw)
        # This explicit mutable SDK setting word is data, not an opcode or
        # source-ready flag. The whole source listing is rechecked below.
        assert os.pwrite(host.memory_fd, config, 0x386b04) == 4
        host.write(frame - 0x20, bytes(0x60))
        host.write(frame - 8, struct.pack('<I', voice))
        overrides = (source if source_override else 0, parameters if parameters_override else 0)
        host.write(frame + 8, struct.pack('<2I', *overrides))
        prefix = run(0x3812fa, 0x38138f, rbp=frame, rsi=voice)
        selected = struct.unpack('<2I', host.read(frame - 0x14, 8))
        entry = prefix.rax & 0xffff
        writes = [(0xfe820160, entry)]
        # Original3813A4..3813EC loads the selected table identities and stores
        # its own retained loop scratch before reaching the first write.
        first = run(0x3813a4, 0x3813ec, rbp=frame, rax=entry)
        assert first.rbx == 0xfe820400
        writes.append((first.rbx & 0xffffffff, first.rdx & 0xffffffff))
        left, right = selected
        for index in range(1, 15):
            host.write(frame + 8, struct.pack('<I', 0xfe820400 + index * 4))
            chunk = run(0x3813bf, 0x3813ec, rbp=frame, rcx=left, rdi=right,
                        rax=left + 1 + index * 2, rsi=index)
            writes.append((chunk.rbx & 0xffffffff, chunk.rdx & 0xffffffff))
        tail = run(0x3813f0, 0x381427, rbp=frame, rcx=left, rdi=right, rsi=15)
        writes.append((0xfe82043c, tail.rax & 0xffffffff))
        handle_result = run(0x38142c, 0x381433, rbp=frame)
        writes.append((0xfe8202f8, handle_result.rax & 0xffffffff))
        commit = run(0x381438, 0x38143e, rbp=frame)
        writes.append((0xfe82031c, commit.rax & 0xffffffff))
        if actual:
            expected = [(int(w['address'], 16), int(w['value'], 16)) for w in capture['observed_device_writes']]
            assert writes == expected
            after = bytearray.fromhex(capture['voice_after_hex'])
            struct.pack_into('<I', after, 0x70, data)
            assert host.read(voice, len(after)) == after
        rows.append(dict(label=label, actual_command_capture=actual,
            voice_before_hex=v.hex(), voice_after_hex=host.read(voice, len(v)).hex(),
            data_hex=d.hex(), source_hex=s.hex(), parameters_hex=p.hex(),
            source_override=f'{overrides[0]:08x}', parameters_override=f'{overrides[1]:08x}',
            speaker_config_hex=config.hex(), x87_control_word=fp[0], mxcsr=fp[1],
            kernel_identities=[f'{value:08x}' for value in selected],
            writes=[dict(address=f'{address:08x}', value=f'{value:08x}') for address, value in writes]))

    measure('actual_hrtf_commands', True)
    angles = [bits(i / 4) for i in range(-720, 721, 7)] + [0, 0x80000000, bits(-180.), bits(180.)]
    for configuration in (0, 2, 0x10000, 0x10002, 0x20000):
        for angle in angles:
            measure(f'angle_{configuration:08x}_{angle:08x}', False, angle=angle, configuration=configuration)
    for bank in (0, 1, 2, 255):
        for handle in (0, 1, 32768, 65535):
            measure(f'bank_handle_{bank}_{handle}', False, bank=bank, handle=handle)
    for mode in (0, 1, 2, 0xffffffff):
        for source_override in (False, True):
            for parameters_override in (False, True):
                for absent_source in (False, True):
                    for absent_parameters in (False, True):
                        measure(f'source_mode_{mode}_{source_override}_{parameters_override}_{absent_source}_{absent_parameters}',
                                False, angle=bits(-120.), mode=mode, source_override=source_override,
                                parameters_override=parameters_override, absent_source=absent_source,
                                absent_parameters=absent_parameters)
    for angle in (0x7f812345, 0x7fc12345, 0xffc12345, 0x7f800000, 0xff800000):
        measure(f'null_nonfinite_{angle:08x}', False, angle=angle, mode=2)
    for line in (ROOT / 'analysis/audio-spatial-20261011/sdk-base-v1/003812e0.asm').read_text().splitlines():
        fields = line.split(); pc, raw = int(fields[0], 16), bytes.fromhex(fields[1])
        assert image.read(pc, len(raw)) == host.read(pc, len(raw)) == raw
    host.close()
    fixture = args.output / 'fixtures.json'
    fixture.write_text(json.dumps(dict(schema='ORIGINAL_SOUND_SDK_HRTF_CPU_V1',
        aliases=dict(voice=voice, data=data, source=source, parameters=parameters), cases=rows), separators=(',', ':')) + '\n')
    report = dict(boundary=__doc__, tool_sha256=sha(__file__), original_image_sha256=sha(image_path),
        original_capture_sha256=sha(args.capture / 'report.json'), host_runner_sha256=sha(ROOT / 'tools/original_audio_cpu.py'),
        fixture_sha256=sha(fixture), actual_command_case_matches=True, declared_cases=len(rows)-1,
        whole_original_initializer_executed=True, declared_source_data_writes=['00386b04'],
        boundaries=['003812fa..0038138f', '003813a4..003813ec', '003813bf..003813ec',
                    '003813f0..00381427', '0038142c..00381433', '00381438..0038143e'],
        instruction_patches=[], supplied_callee_returns=[], native_or_physical_DSP_claimed=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Original HRTF CPU: actual19 command words agree; {len(rows)-1} declared original CPU cases; no supplied kernel/device return')


if __name__ == '__main__':
    main()
