#!/usr/bin/env python3
"""Compare a fresh native TU1.5 frame with unchanged original host CPU execution.

The input is a native bank export, not Xbox boot evidence. The original runs
under the existing test-only i386 ptrace bridge. No BIOS, instruction patch,
callee substitution, or native expected output is supplied to that execution.
Stack scratch banks are excluded from the memory comparison. Defined integer
flags are compared; x87 control/status, MXCSR, GP and XMM are compared exactly.
This does not establish populated multiplayer-world or Xbox hardware parity.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from differential_physics_native_frame_tu15_20261012 import regions, stores
from oracle_physics_game_frame_hardware_20261011 import execute
from oracle_smg_firing_1_1 import ROOT


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        while b := f.read(1 << 20):
            h.update(b)
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--case', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--correspondence', type=Path, default=ROOT/'analysis/multiplayer/sandbox-source/physics-game-frame-runtime-20261011/correspondence-v2/complete-report.json')
    a = ap.parse_args()
    pins = {name: sha(a.case/name) for name in ('before.bin', 'after.bin', 'native-stores.bin', 'native-exit.bin')}
    pair_pin, tool_pin = sha(a.correspondence), sha(__file__)
    initial = [dict(address=p, hex=b.hex()) for p, b in regions(a.case/'before.bin')]
    frame = dict(id=0, entry=0x1c7660, return_address=0x60300000, exit_entry=0x60300000,
                 initial_banks=initial, exit={}, entry_abi=dict(
                     gp=[0, 0, 0, 0, 0x600f0000, 0, 0, 0], eflags=2, mxcsr=0x1f80,
                     x87_control_word=0x27f, x87_status=0, x87_tag=0xffff,
                     x87_values=[], xmm=['0'*32]*8, fs_base=0x60200000))
    fixture = dict(image='1.5', correspondence_sha256=pair_pin, cases=[frame])
    original = execute(fixture, json.loads(a.correspondence.read_text()), a.output)
    observed = original['cases'][0]
    expected_stores = list(stores(a.case/'native-stores.bin'))
    actual_stores = [(s['instruction'], s['address'], bytes.fromhex(s['before']), bytes.fromhex(s['written']))
                     for s in observed['writes'] if not 0x60000000 <= s['address'] < 0x60200000]
    problems = []
    if actual_stores != expected_stores:
        problems.append('Ordered non-stack source stores differ')
    expected_banks = list(regions(a.case/'after.bin'))
    compared = 0
    for source, (p, expected) in zip(observed['final_banks'], expected_banks, strict=True):
        assert source['address'] == p
        if 0x60000000 <= p < 0x60200000:
            continue
        actual = bytes.fromhex(source['hex'])
        if actual != expected:
            problems.append(f'Final retained bank differs at {p:08x}')
        compared += len(expected)
    raw = (a.case/'native-exit.bin').read_bytes()
    assert len(raw) == 180
    gp = list(struct.unpack_from('<8I', raw))
    xmm = [f'{int.from_bytes(raw[32+n*16:48+n*16], "little"):032x}' for n in range(8)]
    flags, fs, depth, status, cw, mxcsr = struct.unpack_from('<IIIHHI', raw, 160)
    exit_abi = observed['exit']
    fields = dict(gp=gp, xmm=xmm, x87_control_word=cw, x87_status=status, mxcsr=mxcsr)
    for name, expected in fields.items():
        if exit_abi[name] != expected:
            problems.append(f'Exit ABI differs: {name}')
    if len(exit_abi['x87_values']) != depth or depth != 0:
        problems.append('Exit x87 live values need an explicit native export')
    defined_flags = 0xcd5
    if (flags ^ exit_abi['eflags']) & defined_flags:
        problems.append('Defined exit integer flags differ')
    assert fs == 0x60200000
    assert pins == {name: sha(a.case/name) for name in pins}, 'Native evidence changed during comparison'
    assert (pair_pin, tool_pin) == (sha(a.correspondence), sha(__file__)), 'Comparison inputs changed'
    report = dict(
        passed=not problems, errors=problems, scope=__doc__, bios_input_used=False,
        tool_sha256=tool_pin, correspondence_sha256=pair_pin, native_evidence=pins,
        original_sha256=original['original_sha256'], original_entry='001cc8d0',
        oracle_backend=original['oracle_backend'],
        hardware_tool_sha256=original['hardware_tool_sha256'],
        trace_observer_sha256=original['trace_observer_sha256'],
        bridge_input_tool_sha256=original['bridge_input_tool_sha256'],
        original_hardware_binary_sha256=original['original_hardware_binary_sha256'],
        original_instructions=original['single_stepped_original_instructions'],
        original_instruction_patches=original['original_instruction_patches'],
        original_execution_native_expected_data_used=original['native_expected_data_used'],
        ordered_nonstack_stores=len(actual_stores), native_nonstack_stores=len(expected_stores),
        final_retained_bytes_compared=compared, defined_integer_flag_mask=defined_flags,
        original_exit=exit_abi, native_exit=dict(fields, eflags=flags, fs_base=fs, x87_depth=depth),
        original_read_policy=original['unretained_data_read_policy'],
        original_input_sha256=sha(a.output/'input.bin'),
        original_output_sha256=sha(a.output/'output.bin'), original_trace_sha256=sha(a.output/'trace.json'),
        whole_boot_proof=False, live_multiplayer_physics_parity=False, xbox_hardware_proof=False)
    (a.output/'comparison.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k: report[k] for k in ('passed', 'errors', 'original_instructions', 'ordered_nonstack_stores', 'final_retained_bytes_compared')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
