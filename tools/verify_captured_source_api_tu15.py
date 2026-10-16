#!/usr/bin/env python3
"""Compare an actual captured API with same-input original/hardware/native runs.

The isolated caller's declared return sentinel is the sole permitted input
adaptation. Stack outputs must be listed explicitly; the ordinary complete-bank
comparison excludes the source stack. Actual API instruction traces are not
available, so the separate same-input standalone trace remains mandatory.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def regions(path):
    with path.open('rb') as file:
        while header := file.read(8):
            assert len(header) == 8
            address, size = struct.unpack('<II', header)
            data = file.read(size)
            assert len(data) == size
            yield address, data


def read(bank, address, size):
    for base, data in bank.items():
        if base <= address and address + size <= base + len(data):
            return data[address-base:address-base+size]
    raise AssertionError(hex(address))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--actual', type=Path, required=True)
    parser.add_argument('--original', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--observable-word', type=lambda x: int(x, 0), action='append', default=[])
    args = parser.parse_args()
    assert not args.output.exists()
    actual, original, native = args.actual, args.original, args.native
    metadata = json.loads((actual / 'native.json').read_text())
    reference = json.loads((original / 'report.json').read_text())
    hardware = json.loads((original / 'hardware/report.json').read_text())
    assert metadata['completed'] and reference['completed'] and hardware['completed']
    assert reference['original_xbe_sha256'] == '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    assert not reference['instruction_patches'] and not reference['supplied_game_services']
    assert hardware['original_report_sha256'] == sha(original / 'report.json')
    assert reference['entry'] == metadata['entry']
    raw = (actual / 'native-input.bin').read_bytes()
    stack = struct.unpack_from('<I', raw, 16)[0]
    before = dict(regions(actual / 'before.bin'))
    other = dict(regions(original / 'before.bin'))
    assert set(before) == set(other)
    assert read(before, stack, len(bytes.fromhex(reference['caller_before_hex']))) == bytes.fromhex(reference['caller_before_hex'])
    assert read(other, stack, len(bytes.fromhex(reference['caller_after_hex']))) == bytes.fromhex(reference['caller_after_hex'])
    differences = []
    for base, data in before.items():
        expected = bytearray(data)
        if base <= stack and stack + 4 <= base + len(data):
            expected[stack-base:stack-base+4] = bytes.fromhex(reference['caller_after_hex'])[:4]
        if bytes(expected) != other[base]:
            differences.append(hex(base))
    checks = dict(sole_declared_caller_return_input_change=not differences,
                  CPU_input_exact=raw == (original / 'native-input.bin').read_bytes(),
                  FP_input_exact=(actual / 'native-fp-input.bin').read_bytes() == (original / 'native-fp-input.bin').read_bytes())
    stepped = (original / 'hardware-step-reference.json').exists()
    trace = original / ('hardware/hardware-trace.bin' if stepped else 'original-trace.bin')
    stores = original / ('hardware/hardware-stores.bin' if stepped else 'original-stores.bin')
    checks['strict_native_instruction_trace_exact'] = (native / 'native-trace.bin').read_bytes() == trace.read_bytes()
    checks['strict_native_stores_exact'] = (native / 'native-stores.bin').read_bytes() == stores.read_bytes()
    checks['actual_API_stores_exact'] = (actual / 'native-stores.bin').read_bytes() == stores.read_bytes()
    expected_cpu = bytearray((original / 'hardware/hardware-exit.bin').read_bytes())
    assert len(expected_cpu) == 180
    expected_cpu[161] &= 0xfd
    checks['actual_API_full_CPU_FP_except_IF_exact'] = (actual / 'native-exit.bin').read_bytes() == expected_cpu
    checks['strict_native_full_CPU_FP_except_IF_exact'] = (native / 'native-exit.bin').read_bytes() == expected_cpu
    api_after = dict(regions(actual / 'after.bin'))
    native_after = dict(regions(native / 'native-after.bin'))
    hardware_after = dict(regions(original / 'hardware/hardware-after.bin'))
    mismatches, compared = [], 0
    for base, data in hardware_after.items():
        if 0x60000000 <= base < 0x60200000:
            continue
        compared += len(data)
        if api_after.get(base) != data or native_after.get(base) != data:
            mismatches.append(hex(base))
    checks['actual_API_and_strict_complete_nonstack_banks_exact'] = not mismatches
    observable_words = set(args.observable_word)
    if 'material_output_address' in reference['source_context']:
        assert reference['source_context']['material_output_bytes'] == 2
        observable_words.add(reference['source_context']['material_output_address'])
    words = []
    for address in sorted(observable_words):
        expected = read(hardware_after, address, 2)
        passed = read(api_after, address, 2) == read(native_after, address, 2) == expected
        words.append(dict(address=address, value=int.from_bytes(expected, 'little'), passed=passed))
    checks['observable_caller_WORDs_exact'] = all(word['passed'] for word in words)
    report = dict(scope=__doc__, passed=all(checks.values()), checks=checks,
                  original_entry=reference['entry'], original_instructions=reference['original_instructions'],
                  original_nonstack_writes=reference['retained_stores'], retained_nonstack_bytes=compared,
                  observable_words=words, mismatched_input_regions=differences, mismatched_output_regions=mismatches,
                  original_report_sha256=sha(original / 'report.json'),
                  hardware_report_sha256=sha(original / 'hardware/report.json'),
                  actual_API_build_sha256=sha(actual / 'build.json'), native_build_sha256=sha(args.build / 'build.json'),
                  tool_sha256=sha(Path(__file__)), actual_API_instruction_trace_available=False,
                  physical_xbox_verified=False, full_weapon_parity=False)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'original_entry', 'original_instructions', 'original_nonstack_writes', 'retained_nonstack_bytes')}))
    assert report['passed'], checks


if __name__ == '__main__':
    main()
