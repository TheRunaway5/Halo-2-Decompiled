#!/usr/bin/env python3
"""Original admitted-seed/contrail initialization reference, without stubs.

Execute whole13A8B0,11E550/219F90,1F38D0 and219300 in their registered
relative order. Other new-map rows are not claimed executed. A reversed-order
control demonstrates the lost RNG draws even if the initial seed happens to
match. Native integration compares original noise/scalar outputs after actual
campaign session admission, never installs these results into runtime memory.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ENTRIES = {0x13a8b0, 0x11e550, 0x219f90, 0x1f38d0, 0x219300}
BASE, GAME, OPTIONS, RANDOM, PHYSICS, STACK, STOP = 0x2000000, 0x2001000, 0x2004000, 0x2007000, 0x2008000, 0x200f000, 0x3000000


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    ap.add_argument('--verify', action='store_true')
    args = ap.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    bodies = {entry: _trace(code, entry, ENTRIES) for entry in ENTRIES}
    allowed = set().union(*(set(body.code) for body in bodies.values()))
    covered, cases = set(), []
    for seed in (0x12345678, 0x87654321):
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(BASE, 0x10000)
        g.mem_write(BASE, b'\xa5' * 0x10000)
        g.mem_map(STOP, 4096)

        def put(at, value):
            g.mem_write(at, struct.pack('<I', value))

        put(0x4f3218, GAME)
        put(0x4f3cd8, RANDOM)
        put(0x52be68, PHYSICS)
        options = bytearray(0x1118)
        struct.pack_into('<I', options, 0, 1)
        struct.pack_into('<I', options, 0x10, seed)
        g.mem_write(OPTIONS, bytes(options))
        steps = []

        def instruction(_g, pc, size, _u):
            assert pc in allowed and bytes(g.mem_read(pc, size)) == image.read(pc, size), hex(pc)
            covered.add(pc)
            if not steps or steps[-1] != pc:
                steps.append(pc)

        g.hook_add(UC_HOOK_CODE, instruction)

        def call(entry):
            put(STACK, STOP)
            put(STACK + 4, OPTIONS)
            for n, reg in enumerate((R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX, R.UC_X86_REG_EBX,
                                     R.UC_X86_REG_ESP, R.UC_X86_REG_EBP, R.UC_X86_REG_ESI, R.UC_X86_REG_EDI)):
                g.reg_write(reg, STACK if reg == R.UC_X86_REG_ESP else 0x11223300 + n)
            g.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
            g.reg_write(R.UC_X86_REG_FPCW, 0x27f)
            g.reg_write(R.UC_X86_REG_FPTAG, 0xffff)
            g.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
            g.emu_start(entry, STOP, count=100000)
            assert g.reg_read(R.UC_X86_REG_EIP) == STOP

        call(0x13a8b0)
        assert int.from_bytes(g.mem_read(RANDOM, 4), 'little') == seed
        call(0x11e550)
        after_noise = int.from_bytes(g.mem_read(RANDOM, 4), 'little')
        expected = seed
        for _ in range(24):
            expected = (expected * 0x19660d + 0x3c6ef35f) & 0xffffffff
        assert after_noise == expected and after_noise != seed
        call(0x1f38d0)
        call(0x219300)
        noise = bytes(g.mem_read(0x505e90, 0xd0c))
        scalars = bytes(g.mem_read(0x505e84, 8))
        assert int.from_bytes(g.mem_read(RANDOM, 4), 'little') == after_noise
        # The old source order reseeds after the draw owner. Replaying the
        # actual admission owner loses those draws; no arithmetic is patched.
        call(0x13a8b0)
        wrong_after_reseed = int.from_bytes(g.mem_read(RANDOM, 4), 'little')
        assert wrong_after_reseed == seed and wrong_after_reseed != after_noise
        cases.append(dict(seed=seed, noise_hex=noise.hex(), scalar_hex=scalars.hex(),
                          noise_sha256=hashlib.sha256(noise).hexdigest(), gameplay_rng_after_noise=after_noise,
                          wrong_order_after_reseed=wrong_after_reseed, original_rng_draws=24,
                          original_instruction_steps=len(steps)))
    fixture = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, instruction_patches=[], supplied_dependency_returns=[], cases=cases)
    raw = (json.dumps(fixture, separators=(',', ':')) + '\n').encode()
    if args.verify or args.fixture.exists():
        assert args.fixture.read_bytes() == raw, 'Preserve original session seed/noise reference'
    else:
        args.fixture.parent.mkdir(parents=True, exist_ok=True)
        args.fixture.write_bytes(raw)
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, cases=len(cases),
                  entry_coverage={f'{p:08x}': dict(static=len(b.code), observed=len(covered & set(b.code)),
                                                  unexecuted=sorted(set(b.code) - covered)) for p, b in sorted(bodies.items())},
                  fixture_bytes=len(raw), fixture_sha256=hashlib.sha256(raw).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  wrong_order_loses_source_rng_draws=True, full_map_or_campaign_qualified=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
