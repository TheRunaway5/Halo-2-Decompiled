#!/usr/bin/env python3
"""Execute unchanged TU1.5 model building on an exported fresh native world.

Only the declared native caller's view policy, output storage and call frame
are supplied. Every executed instruction must match the pinned original XBE.
No model output, query response, resource success or palette is replayed.
The original stack and native temporary scratch have different layouts and
are excluded; every other retained byte is compared without a tolerance.
This is a model-builder check, not a matched rendered-frame check.
"""
import argparse
import ctypes
import hashlib
import json
import struct
from pathlib import Path

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from differential_physics_native_frame_tu15_20261012 import regions, segment


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', type=Path, required=True)
    parser.add_argument('--synchronization', type=Path, required=True,
                        help='native_model_sync_probe.cpp linked with the tested native synchronization body')
    args = parser.parse_args()
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    assert digest(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    call = json.loads((args.case / 'call.json').read_text())
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    mapped = set()

    def mount(address, data):
        pages = [p for p in range(address & ~4095, (address + len(data) + 4095) & ~4095, 4096)
                 if p not in mapped]
        start = 0
        while start < len(pages):
            end = start + 1
            while end < len(pages) and pages[end] == pages[end - 1] + 4096:
                end += 1
            guest.mem_map(pages[start], (end - start) * 4096)
            start = end
        mapped.update(pages)
        guest.mem_write(address, data)

    for address, data in regions(args.case / 'before.bin'):
        mount(address, data)
    # Same zero-initialized caller output as the first native invocation.
    collector = call['collector']
    mount(collector, struct.pack('<I', call['owner']) + bytes(4092))
    mount(0x70000000, bytes(4096))
    guest.mem_write(0x70000008, segment(0, 0x9b) + segment(0, 0x93) + segment(0x60200000, 0x93))
    guest.reg_write(R.UC_X86_REG_GDTR, (0, 0x70000000, 0xfff, 0))
    for register, value in ((R.UC_X86_REG_CS, 8), (R.UC_X86_REG_DS, 16),
                            (R.UC_X86_REG_ES, 16), (R.UC_X86_REG_SS, 16),
                            (R.UC_X86_REG_FS, 24)):
        guest.reg_write(register, value)
    assert guest.reg_read(R.UC_X86_REG_FS_BASE) == 0x60200000
    word = lambda p: int.from_bytes(guest.mem_read(p, 4), 'little')
    put = lambda p, v: guest.mem_write(p, struct.pack('<I', v))
    real = lambda v: struct.unpack('<I', struct.pack('<f', v))[0]
    # Original GetLevelDesc reads its current GPU target solely to write
    # multisample state in a dead1D2D0 stack field. Supply the same explicit
    # non-aliasing diagnostic device used by oracle_cache_bitmap_view_tu15;
    # production uses the independently checked CPU metadata boundary.
    # This pointer is a declared reference-only input, excluded below.
    device_pointer = word(0x411568)
    mount(0x70002000, bytes(0x3000))
    put(0x411568, 0x70002000)
    put(0x70003a14, 0x70004000)
    put(0x70004004, 0x12345678)
    put(0x4c6870, call['view'])
    put(0x4c683c, 3 if call['players'] in (2, 3) else 2 if call['players'] == 4 else 4)
    guest.mem_write(0x4c6839, bytes([int(call['modifier'])]))
    stack, stop = 0x600f0000, 0x60300000
    guest.mem_write(stack, struct.pack('<7I', stop, call['owner'], real(call['metric']),
                    real(call['lighting_metric']), collector + 4, 0, 0))
    for name, value in [('EAX', 0), ('ECX', 1), ('EDX', 0), ('EBX', 0),
                        ('ESP', stack), ('EBP', 0), ('ESI', 0), ('EDI', 0)]:
        guest.reg_write(getattr(R, 'UC_X86_REG_' + name), value)
    for i in range(8):
        guest.reg_write(getattr(R, f'UC_X86_REG_XMM{i}'), 0)
    for register, value in ((R.UC_X86_REG_EFLAGS, 2), (R.UC_X86_REG_FPCW, 0x27f),
                            (R.UC_X86_REG_FPSW, 0), (R.UC_X86_REG_FPTAG, 0xffff),
                            (R.UC_X86_REG_MXCSR, 0x1f80)):
        guest.reg_write(register, value)
    instructions = {}
    executed = 0
    # The fresh native world stops and joins its resource worker before this
    # synchronous test. Recreate only its two unlocked original queue mutexes;
    # a contended acquire, semaphore or another handle rejects the comparison.
    native = ctypes.CDLL(str(args.synchronization.resolve()))
    native.model_sync_create.restype = ctypes.c_void_p
    native.model_sync_destroy.argtypes = [ctypes.c_void_p]
    for name in ('model_sync_acquire', 'model_sync_release'):
        getattr(native, name).argtypes = [ctypes.c_void_p, ctypes.c_uint]
        getattr(native, name).restype = ctypes.c_int
    synchronization = native.model_sync_create()
    assert synchronization
    assert [word(p) for p in (0x4ecc28, 0x4ecc2c)] == [0xf3000001, 0xf3000002]
    platform = []

    def original(uc, pc, size, _):
        nonlocal executed
        if pc in (0x2e0478, 0x2e0342):
            sp = uc.reg_read(R.UC_X86_REG_ESP)
            handle = word(sp+4)
            assert handle in (word(0x4ecc28), word(0x4ecc2c))
            acquire = pc == 0x2e0478
            if acquire:
                assert word(sp+8) == 0xffffffff
            operation = native.model_sync_acquire if acquire else native.model_sync_release
            result = operation(synchronization, handle)
            assert result == 0, ('Native synchronization did not succeed', pc, handle, result)
            platform.append(dict(entry=f'{pc:08x}', handle=f'{handle:08x}', acquire=acquire, actual_result=result))
            uc.reg_write(R.UC_X86_REG_EAX, 0 if acquire else 1)
            uc.reg_write(R.UC_X86_REG_ESP, sp + (12 if acquire else 8))
            uc.reg_write(R.UC_X86_REG_EIP, word(sp))
            return
        actual = bytes(uc.mem_read(pc, size))
        assert actual == image.read(pc, size), f'Original instruction differs at {pc:08x}'
        instructions[pc] = actual.hex()
        executed += 1

    guest.hook_add(UC_HOOK_CODE, original)
    differences = []
    compared = 0
    error = None
    try:
        guest.emu_start(0x4b5c0, stop, count=10000000)
        assert guest.reg_read(R.UC_X86_REG_EIP) == stop, 'Original model builder did not return'
        put(0x411568, device_pointer)
        for address, expected in regions(args.case / 'after.bin'):
            # These are the diagnostic/native caller's explicitly owned work
            # areas; none supplies any retained original output under test.
            if 0x60000000 <= address < 0x60200000 or address == 0x60400000:
                continue
            actual = bytes(guest.mem_read(address, len(expected)))
            compared += len(expected)
            if actual != expected:
                offsets = [i for i, (x, y) in enumerate(zip(actual, expected)) if x != y]
                differences.append(dict(address=f'{address:08x}', bytes=len(offsets),
                    first=[dict(address=f'{address+i:08x}', original=actual[i], native=expected[i])
                           for i in offsets[:16]]))
    except (UcError, AssertionError, ValueError) as failure:
        error = str(failure)
    report = dict(original_sha256=NEW_PIN, original_entry='0004b5c0',
        tool_sha256=digest(Path(__file__)), before_sha256=digest(args.case/'before.bin'),
        after_sha256=digest(args.case/'after.bin'), call=call,
        original_instructions=executed, unique_original_instructions=len(instructions),
        original_pc=f'{guest.reg_read(R.UC_X86_REG_EIP):08x}',
        original_stack=[f'{word(guest.reg_read(R.UC_X86_REG_ESP)+i*4):08x}' for i in range(8)],
        retained_bytes_compared=compared, differences=differences, error=error,
        model_count=word(collector+0x164) >> 16,
        synchronization_sha256=digest(args.synchronization), platform=platform,
        synchronization_policy='Reconstructed unlocked native queue mutexes; no contended scheduling proof',
        declared_reference_only_device='Non-aliasing target for unconsumed CPU bitmap multisample field',
        model_output_replayed=False, reference_frame_parity=False)
    (args.case/'original-comparison.json').write_text(json.dumps(report, indent=2)+'\n')
    (args.case/'original-instructions.json').write_text(json.dumps(instructions, sort_keys=True)+'\n')
    print(json.dumps(report))
    native.model_sync_destroy(synchronization)
    return 1 if error or differences else 0


if __name__ == '__main__':
    raise SystemExit(main())
