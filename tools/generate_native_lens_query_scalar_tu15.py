#!/usr/bin/env python3
"""Execute the unchanged three-instruction OG Xbox TU1.5 lens scalar owner.

The declared caller ABI and loader-zero data are inputs. No instruction,
callee, result or scalar initializer is supplied. This is original-emulated
CPU evidence, without a World, device, constructor schedule or pixel claim.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path


PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
ENTRY, SCALAR, OPERAND = 0x3803B0, 0x532098, 0x46A0C4
QUERY, QUERY_BYTES = 0x52DFD8, 0x40C0
ORIGINAL_HEX = 'f30f1005c4a04600f30f110598205300c3'
PCS = (ENTRY, ENTRY + 8, ENTRY + 16)


def pin(path):
    path = Path(path).resolve()
    with path.open('rb') as stream:
        return dict(path=str(path), sha256=hashlib.file_digest(stream, 'sha256').hexdigest(),
                    bytes=path.stat().st_size)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--fixture', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('--repository', type=Path,
                   default=Path(__file__).resolve().parents[1])
    a = p.parse_args()
    if not a.xbe.is_file():
        print('SKIP missing required local TU1.5 XBE', file=sys.stderr)
        return 77
    if a.fixture.exists() or a.report.exists():
        raise ValueError('Preserve existing fixture/report; choose fresh output paths')
    if pin(a.xbe)['sha256'] != PIN:
        raise ValueError('Present executable is not the authenticated original TU1.5 XBE')
    tools = a.repository.resolve() / 'tools/reference/halo2-decompiled/tools'
    sys.path.insert(0, str(tools))
    from xbe import Xbe
    import unicorn
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn import x86_const as R

    image = Xbe(a.xbe)
    assert image.read(ENTRY, 17).hex() == ORIGINAL_HEX
    assert image.read(SCALAR, 4) == bytes(4)
    assert image.read(OPERAND, 4).hex() == '8180803b'
    scalar_section = [s for s in image.sections if s.va <= SCALAR and SCALAR + 4 <= s.va + s.vsize]
    assert len(scalar_section) == 1 and scalar_section[0].name == '.data'
    assert scalar_section[0].flags & 1 and SCALAR >= scalar_section[0].va + scalar_section[0].rsize
    assert QUERY + QUERY_BYTES == SCALAR

    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x800000)
    for section in image.sections:
        cpu.mem_write(section.va, image.section_bytes(section))
    # Initial memory comes only from the actual loader image. No scalar/result
    # write is used to make this owner executable.
    assert bytes(cpu.mem_read(SCALAR, 4)) == bytes(4)
    stack, stop = 0x62000000, 0x63000000
    cpu.mem_map(stack, 4096)
    cpu.mem_map(stop, 4096)
    gp = [0x11, 0x22, 0x33, 0x44, stack + 0x80, 0x55, 0x66, 0x77]
    xmm = [[0x7FA12000 + i * 16 + j for j in range(4)] for i in range(8)]
    regs = [R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX, R.UC_X86_REG_EBX,
            R.UC_X86_REG_ESP, R.UC_X86_REG_EBP, R.UC_X86_REG_ESI, R.UC_X86_REG_EDI]
    for reg, value in zip(regs, gp):
        cpu.reg_write(reg, value)
    for i, words in enumerate(xmm):
        cpu.reg_write(R.UC_X86_REG_XMM0 + i, int.from_bytes(struct.pack('<4I', *words), 'little'))
    cpu.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
    cpu.reg_write(R.UC_X86_REG_FPCW, 0x27F)
    cpu.reg_write(R.UC_X86_REG_MXCSR, 0x1F80)
    cpu.mem_write(gp[4], struct.pack('<I', stop))
    query_before = bytes(cpu.mem_read(QUERY, QUERY_BYTES))
    scalar_before = bytes(cpu.mem_read(SCALAR, 4))
    instructions, stores = [], []
    current_pc = None

    def instruction(g, pc, size, _):
        nonlocal current_pc
        assert pc in PCS and bytes(g.mem_read(pc, size)) == image.read(pc, size)
        current_pc = pc
        instructions.append(pc)

    def write(g, kind, address, size, value, _):
        assert current_pc == ENTRY + 8 and address == SCALAR and size == 4
        stores.append(dict(instruction=current_pc, address=address,
                           before=list(g.mem_read(address, size)),
                           written=list(value.to_bytes(size, 'little'))))

    cpu.hook_add(UC_HOOK_CODE, instruction)
    cpu.hook_add(UC_HOOK_MEM_WRITE, write)
    cpu.emu_start(ENTRY, stop, count=16)
    assert cpu.reg_read(R.UC_X86_REG_EIP) == stop and instructions == list(PCS)
    assert len(stores) == 1 and stores[0]['before'] == [0] * 4
    assert bytes(cpu.mem_read(SCALAR, 4)) == image.read(OPERAND, 4)
    assert bytes(cpu.mem_read(QUERY, QUERY_BYTES)) == query_before
    exit_gp = [cpu.reg_read(reg) for reg in regs]
    exit_xmm = [list(struct.unpack('<4I', cpu.reg_read(R.UC_X86_REG_XMM0 + i).to_bytes(16, 'little')))
                for i in range(8)]
    expected_gp = gp.copy()
    expected_gp[4] += 4
    expected_xmm = [v.copy() for v in xmm]
    expected_xmm[0] = [0x3B808081, 0, 0, 0]
    assert exit_gp == expected_gp and exit_xmm == expected_xmm
    assert cpu.reg_read(R.UC_X86_REG_EFLAGS) == 0x202
    assert cpu.reg_read(R.UC_X86_REG_FPCW) == 0x27F
    assert cpu.reg_read(R.UC_X86_REG_MXCSR) == 0x1F80
    fixture = dict(format='native_lens_query_scalar_tu15_original_v1', original_sha256=PIN,
                   entry=ENTRY, original_instruction_hex=ORIGINAL_HEX, instructions=instructions,
                   entry_gp=gp, entry_xmm=xmm, entry_eflags=0x202, entry_x87_control=0x27F,
                   entry_mxcsr=0x1F80, exit_gp=exit_gp, exit_xmm=exit_xmm,
                   exit_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),
                   exit_x87_control=cpu.reg_read(R.UC_X86_REG_FPCW),
                   exit_mxcsr=cpu.reg_read(R.UC_X86_REG_MXCSR), exit_pc=stop,
                   scalar_before=list(scalar_before), scalar_after=list(cpu.mem_read(SCALAR, 4)),
                   stores=stores, query_bank_address=QUERY, query_bank_bytes=QUERY_BYTES,
                   query_bank_unchanged=True, instruction_patches=[], replaced_services=[])
    a.fixture.parent.mkdir(parents=True, exist_ok=True)
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.fixture.write_text(json.dumps(fixture, indent=2) + '\n')
    report = dict(status='PASS_UNCHANGED_ORIGINAL_EMULATED_CPU_3803B0', original=pin(a.xbe),
                  generator=pin(__file__), xbe_decoder=pin(tools / 'xbe.py'),
                  fixture=pin(a.fixture), instructions=3, stores=1,
                  unicorn_version=unicorn.__version__, boundary=__doc__,
                  native_constructor_schedule_proven=False, World=False, GPU=False,
                  instruction_patches=[], replaced_services=[])
    a.report.write_text(json.dumps(report, indent=2) + '\n')
    print(report['status'])
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, ValueError, AssertionError) as error:
        print('FAIL present original input/control:', error, file=sys.stderr)
        raise SystemExit(1)
