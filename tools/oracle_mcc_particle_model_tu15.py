#!/usr/bin/env python3
"""Compare actual MCC model-particle matrices with unchanged TU1.5 instructions.

The executable is an offline arithmetic reference. Only a prepared MCC cache
supplies authored metadata; no tag bytes, helper returns or GPU results change.
Declared completed values/camera are fixture inputs, not a live birth claim.
Run through run_test.py; binary inputs/outputs remain in HALO2_TEST_SCRATCH.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import random
import struct
import subprocess

from mcc_content import MccCache
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR, UC_X86_REG_FPSW, UC_X86_REG_FPTAG


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-binary', type=Path, required=True)
    parser.add_argument('--mcc-map', type=Path, required=True)
    parser.add_argument('--datum', type=lambda s: int(s, 0), default=3980394882)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cases', type=int, default=128)
    parser.add_argument('--require-byte-exact', action='store_true')
    args = parser.parse_args()
    if args.cases < 32 or args.cases > 1024:
        parser.error('--cases must be between32 and1024')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    cache = MccCache(args.mcc_map)
    tag, _ = cache.tag(args.datum, 'PRTM')
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    original = Uc(UC_ARCH_X86, UC_MODE_32)
    original.mem_map(0x10000, 0x800000)
    for section in image.sections:
        original.mem_write(section.va, image.section_bytes(section))
    first = cache.magic & ~4095
    original.mem_map(first, (cache.magic + len(cache.data) - first + 4095) & ~4095)
    original.mem_write(cache.magic, bytes(cache.data))
    # Retained393-instruction correspondence identifies22B9D0 and its exact
    # matrix-upload stop22C088, camera491E44/50 and pushbuffer411568.
    arena, stack, stop = 0x2100000, 0x21fff00, 0x70000000
    original.mem_map(arena, 0x100000)
    original.mem_map(stop, 4096)
    original.mem_write(0x411568, struct.pack('<I', arena + 0x600))
    original.hook_add(UC_HOOK_CODE,
        lambda cpu, pc, size, user: cpu.emu_stop() if pc == 0x22c088 else None)
    rng = random.Random(0x22b9d0)
    inputs = bytearray(struct.pack('<I', args.cases))
    expected = bytearray()
    f32 = lambda v: struct.unpack('<f', struct.pack('<f', v))[0]
    for index in range(args.cases):
        values = [f32(rng.random()) for _ in range(17)]
        if index < 6:
            values[11] = (0., .125, .25, .5, .75, 1.)[index]
        position = [f32(rng.uniform(-25, 25)) for _ in range(3)]
        direction = [f32(rng.uniform(-5, 5)) for _ in range(3)]
        scale = f32((0., .01, .1, 1., 2.)[index] if index < 5 else rng.uniform(.01, 2))
        model_random = index if index < 16 else rng.getrandbits(16)
        angle = index * .17
        forward = [f32(math.cos(angle)), f32(math.sin(angle)), 0.]
        up = [0., 0., 1.]
        original.mem_write(0x491e44, struct.pack('<3f', *forward))
        original.mem_write(0x491e50, struct.pack('<3f', *up))
        seed = bytearray(64)
        struct.pack_into('<H', seed, 0x1a, model_random)
        original.mem_write(arena, bytes(seed))
        original.mem_write(arena + 0x100, struct.pack('<17f4I', *values, 0xffffffff, 0, 0, 0))
        original.mem_write(arena + 0x200, struct.pack('<6f', *position, *direction))
        original.mem_write(arena + 0x300, struct.pack('<4f', 1., 1., 1., 1.))
        original.mem_write(arena + 0x400, struct.pack('<2I', tag['address'], 2))
        original.mem_write(arena + 0x600, struct.pack('<2I', arena + 0x800, arena + 0x1000))
        original.mem_write(arena + 0x800, bytes(256))
        original.mem_write(stack, struct.pack('<5If2I', stop, arena, arena + 0x100,
            arena + 0x200, arena + 0x20c, scale, arena + 0x300, arena + 0x400))
        original.reg_write(UC_X86_REG_ESP, stack)
        original.reg_write(UC_X86_REG_FPCW, 0x027f)
        original.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        original.reg_write(UC_X86_REG_FPSW, 0)
        original.reg_write(UC_X86_REG_FPTAG, 0xffff)
        try:
            original.emu_start(0x22b9d0, stop, count=100000)
        except Exception as error:
            raise RuntimeError(f'Original TU1.5 PRTM fault at{original.reg_read(UC_X86_REG_EIP):08x},case{index}') from error
        assert original.reg_read(UC_X86_REG_EIP) == 0x22c088
        raw = bytes(original.mem_read(arena + 0x800, 60))
        assert struct.unpack('<15I', raw)[0::5] == (0x101ac0, 0x101ad0, 0x101ae0)
        expected += b''.join(raw[at + 4:at + 20] for at in (0, 20, 40))
        span = original.reg_read(UC_X86_REG_EBP)
        expected += struct.pack('<2I', *struct.unpack('<2H', original.mem_read(span + 4, 4)))
        inputs += struct.pack('<17f6ffH6f', *values, *position, *direction, scale,
            model_random, *forward, *up)
    source, result = scratch / 'model-inputs.bin', scratch / 'model-native.bin'
    source.write_bytes(inputs)
    subprocess.run([str(args.native_binary.resolve()), str(args.mcc_map.resolve()),
        str(source), str(result), 'native-mcc', str(args.datum)], check=True)
    native = result.read_bytes()
    assert len(native) == len(expected)
    differences, span_differences, maximum = [], [], 0.
    for index in range(args.cases):
        a = struct.unpack_from('<12f2I', expected, index * 56)
        b = struct.unpack_from('<12f2I', native, index * 56)
        if a[12:] != b[12:]:
            span_differences.append(dict(case=index, original=a[12:], native=b[12:]))
        for component in range(12):
            delta = abs(a[component] - b[component])
            maximum = max(maximum, delta)
            if not math.isfinite(delta) or delta > 1e-6:
                differences.append(dict(case=index, component=component,
                    original=a[component], native=b[component], error=delta))
    report = dict(entry=0x22b9d0, stop=0x22c088, cases=args.cases,
        original_TU15_sha256=NEW_PIN, runtime_cache_source='Prepared MCC only',
        map=str(args.mcc_map.resolve()), map_sha256=hashlib.sha256(args.mcc_map.read_bytes()).hexdigest(),
        authored_particle=tag, tags_translated_or_rewritten=False,
        declared_completed_values_and_camera=True, supplied_helper_or_GPU_results=False,
        matrix_and_span_byte_exact=native == expected, maximum_float_error=maximum,
        mismatches=differences[:20], span_mismatches=span_differences[:20],
        input_sha256=hashlib.sha256(inputs).hexdigest(),
        original_output_sha256=hashlib.sha256(expected).hexdigest(),
        native_output_sha256=hashlib.sha256(native).hexdigest(),
        raw_outputs=scratch.as_posix(), large_artifacts_retained=[], full_weapon_parity=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'authored_particle'}))
    return bool(differences or span_differences or (args.require_byte_exact and native != expected))


if __name__ == '__main__':
    raise SystemExit(main())
