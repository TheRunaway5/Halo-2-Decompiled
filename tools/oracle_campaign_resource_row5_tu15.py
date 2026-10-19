"""Execute only unchanged TU1.5 2255F0; no supplied external owners."""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import sys

ENTRY, END, BANK, STACK, SP, RETURN = 0x2255f0, 0x225604, 0x48b8e8, 0x4000000, 0x4000800, 0x300f000
PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
BODY = 'a1fcb8480085c0740ac705fcb8480000000000c3'
VALUES = (0, 1, 0x7fffffff, 0x80000000, 0xffffffff, 0x12345678, 0xaaaaaaaa, 0x55555555)


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def dump(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--helpers', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--execute-original-after-grant', action='store_true')
    args = parser.parse_args()
    assert args.execute_original_after_grant
    if not args.xbe.is_file():
        return 77
    assert sys.dont_write_bytecode and digest(args.xbe) == PIN
    assert sys.pycache_prefix and not Path(sys.pycache_prefix).exists()
    assert not args.output.exists()
    sys.path.insert(0, str(args.helpers))
    from xbe import Xbe
    from functions import _Code, _trace
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
    import unicorn.x86_const as reg
    image = Xbe(args.xbe)
    code = _Code(image, '.text')
    pcs = set(_trace(code, ENTRY, {ENTRY}).code)
    raw = image.read(ENTRY, END - ENTRY)
    assert raw.hex() == BODY and pcs == {0x2255f0, 0x2255f5, 0x2255f7, 0x2255f9, 0x225603}
    assert int.from_bytes(image.read(0x45f940+5*8, 4), 'little') == ENTRY
    args.output.mkdir()
    report = dict(status='RUNNING_ORIGINAL_ROW5_RELEASE', suite='campaign-resource-row5-release',
                  xbe_sha256=PIN, entry=ENTRY, raw_body_hex=raw.hex(), static_original_PCs=sorted(pcs),
                  oracle_sha256=digest(__file__), external_fixture_owners=[],
                  tested_owner_instruction_patches=[], startup_resource_lifecycle_proven=False, cases=[])
    dump(args.output / 'before-report.json', report)
    for index, initial in enumerate(VALUES):
        cpu = Uc(UC_ARCH_X86, UC_MODE_32)
        for base, size in ((ENTRY & ~0xfff, 0x1000), (BANK & ~0xfff, 0x1000),
                           (STACK, 0x1000), (RETURN, 0x1000)):
            cpu.mem_map(base, size)
        cpu.mem_write(ENTRY, raw)
        storage = bytearray((n * 13 + index) & 0xff for n in range(64))
        struct.pack_into('<I', storage, 0x14, initial)
        cpu.mem_write(BANK, bytes(storage))
        stack = bytearray([0xcd]) * 0x1000
        struct.pack_into('<I', stack, SP-STACK, RETURN)
        cpu.mem_write(STACK, bytes(stack))
        registers = [getattr(reg, 'UC_X86_REG_'+name) for name in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
        gp = [0x10203040, 0x20304050, 0x30405060, 0x40506070, SP, 0x50607080, 0x66778899, 0x708090a0]
        xmm = [[0x3f000000+16*n+k for k in range(4)] for n in range(8)]
        for number, words in enumerate(xmm):
            cpu.reg_write(getattr(reg, 'UC_X86_REG_XMM'+str(number)), sum(v << (32*k) for k,v in enumerate(words)))
        for target, value in ((reg.UC_X86_REG_FPCW, 0x27f), (reg.UC_X86_REG_FPSW, 0),
                              (reg.UC_X86_REG_FPTAG, 0xffff), (reg.UC_X86_REG_MXCSR, 0x1f80)):
            cpu.reg_write(target, value)
        current = [None]
        trace, stores = [], []

        def extent(address, size):
            assert any(base <= address and address+size <= base+length
                       for base, length in ((BANK, 64), (STACK, 0x1000))), ('Undeclared memory', hex(address), size)

        def instruction(machine, pc, size, unused):
            assert pc in pcs, ('Unexpected executed owner', hex(pc))
            current[0] = pc
            trace.append(pc)
            assert len(trace) <= 5

        def read(machine, access, address, size, value, unused):
            extent(address, size)

        def write(machine, access, address, size, value, unused):
            extent(address, size)
            stores.append(dict(pc=current[0], address=address,
                               before=bytes(machine.mem_read(address,size)).hex(),
                               written=int(value).to_bytes(size, 'little').hex()))

        cpu.hook_add(UC_HOOK_CODE, instruction)
        cpu.hook_add(UC_HOOK_MEM_READ, read)
        cpu.hook_add(UC_HOOK_MEM_WRITE, write)
        for repetition in range(2):
            # The second invocation keeps the first invocation's resource bank.
            # Only the caller supplies a fresh return stack/argument registers.
            for target, value in zip(registers, gp):
                cpu.reg_write(target, value)
            flags = 0x202 | ((0x891 if index % 2 else 0))
            cpu.reg_write(reg.UC_X86_REG_EFLAGS, flags)
            before = {f'{BANK:08x}': bytes(cpu.mem_read(BANK,64)).hex(),
                      f'{STACK:08x}': bytes(cpu.mem_read(STACK,0x1000)).hex()}
            trace.clear()
            stores.clear()
            cpu.emu_start(ENTRY, RETURN, count=8)
            assert cpu.reg_read(reg.UC_X86_REG_EIP) == RETURN
            assert cpu.reg_read(reg.UC_X86_REG_FPCW) == 0x27f
            assert cpu.reg_read(reg.UC_X86_REG_FPSW) == 0
            assert cpu.reg_read(reg.UC_X86_REG_FPTAG) == 0xffff
            assert cpu.reg_read(reg.UC_X86_REG_MXCSR) == 0x1f80
            assert bytes(cpu.mem_read(BANK+0x14,4)) == bytes(4)
            name = f'word-{initial:08x}-call-{repetition+1}'
            row = dict(name=name, entry=ENTRY, initial_word=initial, repetition=repetition,
                       before=before, after={f'{BANK:08x}':bytes(cpu.mem_read(BANK,64)).hex(),
                                              f'{STACK:08x}':bytes(cpu.mem_read(STACK,0x1000)).hex()},
                       gp=gp, xmm=xmm, eflags=flags,
                       final=dict(gp=[cpu.reg_read(v) for v in registers], eflags=cpu.reg_read(reg.UC_X86_REG_EFLAGS),
                                  xmm=[[int(cpu.reg_read(getattr(reg,'UC_X86_REG_XMM'+str(n)))) >> (32*k) & 0xffffffff
                                        for k in range(4)] for n in range(8)]),
                       final_pc=RETURN, trace=list(trace), stores=list(stores),
                       fp=dict(cw=0x27f, sw=0, tag=0xffff, mxcsr=0x1f80))
            dump(args.output / (name+'.json'), row)
            report['cases'].append(dict(name=name, file=name+'.json'))
    assert len(report['cases']) == 16 and digest(args.xbe) == PIN
    assert not Path(sys.pycache_prefix).exists()
    report['status'] = 'PASS_UNCHANGED_ORIGINAL_ROW5_RELEASE_NO_EXTERNAL_FIXTURES'
    dump(args.output / 'report.json', report)
    print('PASS16 original row5 release calls, no external fixtures')
    return 0


if __name__ == '__main__':
    sys.exit(main())
