#!/usr/bin/env python3
"""Compare live model building with unchanged original i386 host execution.

The test starts from a fresh native world's retained banks with genuinely
resident resources. Source code, callees and SSE estimates run unpatched on
the host CPU. Native output bytes are only read after original execution.
The descriptor-only diagnostic device is an explicit oracle input for the
unconsumed multisample field; no production GPU device is fabricated.
This proves the selected CPU path, not Xbox hardware or framebuffer parity.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from differential_physics_native_frame_tu15_20261012 import regions
from generate_combat_action_tu15 import CombatCode
import oracle_physics_game_frame_hardware_20261011 as hardware


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    call = json.loads((args.case/'call.json').read_text())
    banks = [(p, bytearray(b)) for p, b in regions(args.case/'before.bin')]

    def store(address, data):
        for p, b in banks:
            if p <= address and address+len(data) <= p+len(b):
                b[address-p:address-p+len(data)] = data
                return
        raise ValueError(f'Declared caller input missing at {address:08x}')

    put = lambda p, v: store(p, struct.pack('<I', v))
    real = lambda v: struct.unpack('<I', struct.pack('<f', v))[0]
    collector = call['collector']
    assert not any(p <= collector < p+len(b) for p, b in banks), 'Expected a fresh view collector'
    banks.append((collector, bytearray(4096)))
    banks.append((0x70002000, bytearray(0x3000)))
    put(collector, call['owner'])
    put(0x411568, 0x70002000)
    put(0x70003a14, 0x70004000)
    put(0x70004004, 0x12345678)
    put(0x4c6870, call['view'])
    put(0x4c683c, 3 if call['players'] in (2, 3) else 2 if call['players'] == 4 else 4)
    store(0x4c6839, bytes([int(call['modifier'])]))
    stack, stop = 0x600f0000, 0x60300000
    store(stack, struct.pack('<7I', stop, call['owner'], real(call['metric']),
                            real(call['lighting_metric']), collector+4, 0, 0))
    frame = dict(id=0, entry=0x4b5c0, return_address=stop, exit_entry=stop,
        initial_banks=[dict(address=p, hex=b.hex()) for p, b in banks], exit={},
        entry_abi=dict(gp=[0, 1, 0, 0, stack, 0, 0, 0], eflags=2, mxcsr=0x1f80,
            x87_control_word=0x27f, x87_status=0, x87_tag=0xffff, x87_values=[],
            xmm=['0'*32]*8, fs_base=0x60200000))
    # The earlier original instruction trace only supplies address metadata,
    # never values. The hardware tracer rejects an unlisted instruction/read.
    instructions = json.loads((args.case/'original-instructions.json').read_text())
    paired = dict(address_map={}, entries={'0004b5c0': 0x4b5c0},
                  instruction_map={f'{int(p):08x}': int(p) for p in instructions})
    # The existing harness is also valid for unchanged D3D CPU metadata helpers.
    hardware._Code = lambda image, section: CombatCode(image)
    original = hardware.execute(dict(image='1.5', cases=[frame]), paired, args.output)
    observed = original['cases'][0]
    result = {b['address']: bytes.fromhex(b['hex']) for b in observed['final_banks']}
    compared, differences = 0, []
    for address, expected in regions(args.case/'after.bin'):
        if 0x60000000 <= address < 0x60200000 or address == 0x60400000:
            continue
        actual = bytearray(result[address])
        # Restore only the oracle's declared device-pointer input. Its output
        # field lives in excluded dead stack storage, never a model bank.
        if address <= 0x411568 < address+len(actual):
            offset = 0x411568-address
            actual[offset:offset+4] = expected[offset:offset+4]
        compared += len(expected)
        if actual != expected:
            offsets = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
            differences.append(dict(address=f'{address:08x}', bytes=len(offsets),
                first=[dict(address=f'{address+i:08x}', original=actual[i], native=expected[i])
                       for i in offsets[:16]]))
    report = dict(passed=not differences, scope=__doc__, call=call,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        original_sha256=original['original_sha256'],
        original_instructions=original['single_stepped_original_instructions'],
        original_hardware_binary_sha256=original['original_hardware_binary_sha256'],
        hardware_tool_sha256=original['hardware_tool_sha256'],
        original_instruction_patches=original['original_instruction_patches'],
        original_execution_native_expected_data_used=original['native_expected_data_used'],
        retained_bytes_compared=compared, differences=differences,
        oracle_backend=original['oracle_backend'], xbox_hardware_proof=False,
        original_frame_parity=False)
    (args.output/'comparison.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
