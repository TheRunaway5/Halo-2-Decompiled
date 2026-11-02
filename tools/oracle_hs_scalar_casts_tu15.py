"""Capture the unchanged TU1.5 scalar-cast constructor and its complete table.

No instruction, dependency or result is replaced. Run through run_test.py.
Only the small required reference and bounded report may leave owned scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

TABLE, EXTENT, STACK, STOP = 0x502248, 0x3c10, 0x2101000, 0x3000000
GP = (R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX, R.UC_X86_REG_EBX,
      R.UC_X86_REG_ESP, R.UC_X86_REG_EBP, R.UC_X86_REG_ESI, R.UC_X86_REG_EDI)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert args.output.resolve().is_relative_to(scratch)
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    cold = image.read(TABLE, EXTENT)
    assert len(cold) == 62 * 62 * 4 and not any(cold)
    code = _Code(image, '.text')
    body = complete_body(code, 0x212470)
    allowed = {pc: bytes(code.at(pc).bytes) for pc in body.code}
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    for address, size in ((0x10000, 0x600000), (0x2100000, 0x2000), (STOP, 4096)):
        cpu.mem_map(address, size)
    for section in image.sections:
        cpu.mem_write(section.va, image.section_bytes(section))
    trace, stores, rows = [], [], []

    def instruction(vm, pc, size, _):
        assert pc in allowed and bytes(vm.mem_read(pc, size)) == allowed[pc], hex(pc)
        trace.append(pc)

    def store(vm, _kind, address, size, value, _):
        assert TABLE <= address and address + size <= TABLE + EXTENT or STACK - 4 <= address < STACK
        stores.append([vm.reg_read(R.UC_X86_REG_EIP), address, size, value & ((1 << (size * 8)) - 1)])

    cpu.hook_add(UC_HOOK_CODE, instruction)
    cpu.hook_add(UC_HOOK_MEM_WRITE, store)
    for seed in range(4):
        cpu.mem_write(TABLE, cold)
        before_stack = bytearray((seed + n * 13) & 255 for n in range(256))
        before_stack[128:132] = STOP.to_bytes(4, 'little')
        cpu.mem_write(STACK - 128, bytes(before_stack))
        initial = [(0x11223344 + n * 0x10203 + seed) & 0xffffffff for n in range(8)]
        initial[4] = STACK
        for reg, value in zip(GP, initial):
            cpu.reg_write(reg, value)
        flags = (0x247 if seed & 1 else 0x202) | (0x400 if seed & 2 else 0)
        control = 0x27f if seed & 1 else 0x37f
        cpu.reg_write(R.UC_X86_REG_EFLAGS, flags)
        cpu.reg_write(R.UC_X86_REG_FPCW, control)
        cpu.reg_write(R.UC_X86_REG_FPSW, 0)
        cpu.reg_write(R.UC_X86_REG_FPTAG, 0xffff)
        cpu.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        xmm = [int.from_bytes(bytes((seed + n + j) & 255 for j in range(16)), 'little') for n in range(8)]
        for n, value in enumerate(xmm):
            cpu.reg_write(R.UC_X86_REG_XMM0 + n, value)
        trace.clear()
        stores.clear()
        cpu.emu_start(0x212470, STOP, count=1000)
        assert cpu.reg_read(R.UC_X86_REG_EIP) == STOP
        assert cpu.reg_read(R.UC_X86_REG_ESP) == STACK + 4
        assert set(trace) == set(body.code)
        assert cpu.reg_read(R.UC_X86_REG_EFLAGS) == flags
        assert cpu.reg_read(R.UC_X86_REG_FPCW) == control
        assert cpu.reg_read(R.UC_X86_REG_FPSW) == 0
        assert cpu.reg_read(R.UC_X86_REG_MXCSR) == 0x1f80
        assert [cpu.reg_read(R.UC_X86_REG_XMM0 + n) for n in range(8)] == xmm
        rows.append(dict(initial_gp=initial, flags=flags, control=control,
            gp=[cpu.reg_read(r) for r in GP], stack_before=bytes(before_stack).hex(),
            stack_after=bytes(cpu.mem_read(STACK - 128, 256)).hex(),
            table_after=bytes(cpu.mem_read(TABLE, EXTENT)).hex(),
            stores=stores.copy(), trace=trace.copy()))
    reference = dict(schema='H2_TU15_HS_SCALAR_CASTS_V1', xbe_sha256=NEW_PIN,
        generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        table_address=TABLE, table_bytes=EXTENT, stack=STACK, stop=STOP,
        cold_sha256=hashlib.sha256(cold).hexdigest(),
        instructions=[[pc, allowed[pc].hex()] for pc in sorted(allowed)],
        instruction_patches=[], supplied_results=[], cases=rows)
    args.output.write_text(json.dumps(reference, separators=(',', ':')) + '\n')
    print(json.dumps(dict(cases=len(rows), sites=len(body.code), bytes=args.output.stat().st_size,
        sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())))


if __name__ == '__main__':
    main()
