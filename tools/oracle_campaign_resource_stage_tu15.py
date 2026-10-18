#!/usr/bin/env python3
"""Original 191CF0 control-flow oracle; external owners are explicit fixtures.

Only the unchanged 152-byte resource-stage owner is under test. Its selector
callee returns a declared test selector; resource callbacks write canaries and
clobber volatile registers. These are not original resource implementations.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
ENTRY, END, SELECTOR = 0x191cf0, 0x191d88, 0x191ca0
STACK, SP, OPTIONS = 0x4000000, 0x4000800, 0x2000000
CALLBACK, CANARY, MISSING, RETURN = 0x3001000, 0x3100000, 0x300e000, 0x300f000
ALL = tuple(range(8))
# name, previous selector, declared next selector, release rows, acquire rows,
# unavailable owner (selector, release row, or acquire row).
CASES = (
    ('same-positive', 3, 3, ALL, ALL, None),
    ('same-zero', 0, 0, ALL, ALL, None),
    ('same-negative', 0xffffffff, 0xffffffff, ALL, ALL, None),
    ('new-from-zero', 0, 4, ALL, ALL, None),
    ('new-from-negative', 0xffffffff, 2, (0, 7), (0, 3, 7), None),
    ('replace-positive', 4, 2, ALL, ALL, None),
    ('replace-positive-empty', 4, 5, (), (), None),
    ('replace-positive-sparse', 2, 3, (0, 7), (1, 6), None),
    ('duplicate-target', 4, 2, (0, 3, 7), (0, 3, 7), None),
    ('missing-selector', 3, 2, ALL, ALL, ('selector', 0)),
    ('missing-release', 4, 2, ALL, ALL, ('release', 3)),
    ('missing-acquire', 4, 2, ALL, ALL, ('acquire', 3)),
)

def word(v):
    return struct.pack('<I', v)

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def dump(p, value):
    p.write_text(json.dumps(value, indent=2) + '\n')

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--helpers', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--execute-original-after-grant', action='store_true')
    a = p.parse_args()
    if not a.execute_original_after_grant:
        raise RuntimeError('Prepared only: bounded original CPU execution required')
    if not a.xbe.is_file():
        return 77
    assert sha(a.xbe) == PIN
    assert not a.output.exists()
    sys.path.insert(0, str(a.helpers))
    from xbe import Xbe
    from functions import _Code, _trace
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
    import unicorn.x86_const as r
    image = Xbe(a.xbe)
    code = _Code(image, '.text')
    pcs = set(_trace(code, ENTRY, {ENTRY}).code)
    raw = image.read(ENTRY, END - ENTRY)
    assert len(raw) == 152 and len(pcs) == 45 and max(pcs) == 0x191d87
    assert all(raw[pc-ENTRY:pc-ENTRY+code.at(pc).size] == bytes(code.at(pc).bytes) for pc in pcs)
    a.output.mkdir(parents=True)
    report = dict(status='RUNNING_ORIGINAL_RESOURCE_STAGE_ORACLE', suite='campaign-resource-stage',
                  xbe_sha256=PIN, oracle_sha256=sha(__file__), entry=ENTRY,
                  original_body_hex=raw.hex(), static_original_PCs=sorted(pcs),
                  tested_owner_instruction_patches=[],
                  declared_external_owner_scope='Test selector MOV EAX,constant/RET; test release/acquire MOV canary/volatile registers/RET or RET4. Not resource implementations or readiness.',
                  full_startup_proven=False, cases=[])
    dump(a.output/'report.json', report)
    try:
        for name, old, selected, releases, acquires, absent in CASES:
            uc = Uc(UC_ARCH_X86, UC_MODE_32)
            for at, size in ((0x191000, 0x1000), (0x4f2000, 0x1000), (0x4f9000, 0x1000),
                             (0x45f000, 0x1000), (STACK, 0x1000), (OPTIONS, 0x1000),
                             (0x3000000, 0x10000), (CANARY, 0x1000)):
                uc.mem_map(at, size)
            uc.mem_write(ENTRY, raw)
            cursors = bytearray().join(word(0x12340000+i) for i in range(17))
            struct.pack_into('<I', cursors, 0, 2)
            stack = bytearray([0xcd])*0x1000
            struct.pack_into('<I', stack, SP-STACK, RETURN)
            banks = {0x4f2cf0: bytes(cursors), 0x4f9b60: word(old), STACK: bytes(stack),
                     OPTIONS: bytes([0xa7])*0x130, CANARY: bytes([0xee])*16}
            targets, fixtures, stub_pcs = {}, {}, set()
            targets[SELECTOR] = dict(role='selector', row=0, missing=absent == ('selector', 0))
            if not targets[SELECTOR]['missing']:
                selector_code = b'\xb8'+word(selected)+b'\xc3'
                uc.mem_write(SELECTOR, selector_code)
                fixtures[SELECTOR] = dict(entry=SELECTOR, role='selector', selected=selected, code_hex=selector_code.hex())
                stub_pcs.update((SELECTOR, SELECTOR+5))
            for role, active, column, bias in (('release', releases, 0, 0), ('acquire', acquires, 4, 8)):
                for row in range(8):
                    target = 0
                    if row in active:
                        ident = bias+(1 if name == 'duplicate-target' else row)
                        target = CALLBACK+ident*0x40
                        missing = absent == (role, row)
                        if missing:
                            target = MISSING+(bias+row)*0x10
                        targets[target] = dict(role=role, row=row, missing=missing)
                        if not missing:
                            tail = b'\xc3' if role == 'release' else b'\xc2\x04\x00'
                            stub = (b'\xc6\x05'+word(CANARY+ident)+bytes([ident+1])+
                                    b'\xb8'+word(0xa0000000|ident)+b'\xb9'+word(0xc0000000|ident)+
                                    b'\xba'+word(0xd0000000|ident)+tail)
                            uc.mem_write(target, stub)
                            fixtures[target] = dict(entry=target, role=role, id=ident,
                                                    canary_address=CANARY+ident, canary_value=ident+1,
                                                    code_hex=stub.hex())
                            stub_pcs.update(target+n for n in (0, 7, 12, 17, 22))
                    banks[0x45f940+8*row+column] = word(target)
            for at, data in banks.items():
                uc.mem_write(at, data)
            regs = [getattr(r, 'UC_X86_REG_'+n) for n in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
            gp = [OPTIONS, 0x20304050, 0x30405060, 0x40506070, SP, 0x50607080, 0x66778899, 0x708090a0]
            xmm = [[0x3f000000+16*n+k for k in range(4)] for n in range(8)]
            for reg, value in zip(regs, gp):
                uc.reg_write(reg, value)
            for n, words in enumerate(xmm):
                uc.reg_write(getattr(r, 'UC_X86_REG_XMM'+str(n)), sum(v << (32*k) for k, v in enumerate(words)))
            for reg, value in ((r.UC_X86_REG_EFLAGS,0x202), (r.UC_X86_REG_FPCW,0x27f),
                               (r.UC_X86_REG_FPSW,0), (r.UC_X86_REG_FPTAG,0xffff), (r.UC_X86_REG_MXCSR,0x1f80)):
                uc.reg_write(reg, value)
            def fp():
                return {key: uc.reg_read(reg) for key, reg in (('cw',r.UC_X86_REG_FPCW),
                        ('sw',r.UC_X86_REG_FPSW),('tag',r.UC_X86_REG_FPTAG),('mxcsr',r.UC_X86_REG_MXCSR))}
            def snapshot(with_return=False):
                result = dict(gp=[uc.reg_read(v) for v in regs],
                              xmm=[[int(uc.reg_read(getattr(r,'UC_X86_REG_XMM'+str(n)))) >> (32*k) & 0xffffffff
                                    for k in range(4)] for n in range(8)], eflags=uc.reg_read(r.UC_X86_REG_EFLAGS))
                if with_return:
                    result['stack_return'] = struct.unpack('<I', uc.mem_read(uc.reg_read(r.UC_X86_REG_ESP), 4))[0]
                return result
            trace, stores, fixture_stores, calls, stopped, last = [], [], [], [], [], [None]
            def locate(at, size):
                matches = [(base,data) for base,data in banks.items() if base <= at and at+size <= base+len(data)]
                assert len(matches) == 1, ('Access outside declared fixture bank', hex(at), size)
            def hook_code(cpu, pc, size, _):
                last[0] = pc
                trace.append(pc)
                assert len(trace) <= 8192
                if pc in targets:
                    row = dict(target=pc, role=targets[pc]['role'], abi=snapshot(True))
                    calls.append(row)
                    if targets[pc]['missing']:
                        stopped.append(row)
                        cpu.emu_stop()
                        return
                assert pc in pcs or pc in stub_pcs, ('Execution outside tested or declared fixture owner', hex(pc))
            def hook_read(cpu, access, at, size, value, _):
                locate(at, size)
            def hook_write(cpu, access, at, size, value, _):
                locate(at, size)
                row = dict(pc=last[0], address=at, before=bytes(cpu.mem_read(at,size)).hex(),
                           written=int(value).to_bytes(size,'little').hex())
                (stores if last[0] in pcs else fixture_stores).append(row)
            uc.hook_add(UC_HOOK_CODE, hook_code)
            uc.hook_add(UC_HOOK_MEM_READ, hook_read)
            uc.hook_add(UC_HOOK_MEM_WRITE, hook_write)
            fp_before = fp()
            uc.emu_start(ENTRY, RETURN, count=8192)
            final_pc = uc.reg_read(r.UC_X86_REG_EIP)
            assert len(stopped) == int(absent is not None)
            assert final_pc == (RETURN if absent is None else stopped[0]['target'])
            assert fp() == fp_before
            row = dict(name=name, entry=ENTRY, previous_selector=old, declared_selector=selected,
                       before={f'{k:08x}':v.hex() for k,v in sorted(banks.items())},
                       after={f'{k:08x}':bytes(uc.mem_read(k,len(v))).hex() for k,v in sorted(banks.items())},
                       gp=gp, xmm=xmm, eflags=0x202, fp_before=fp_before, fp_after=fp(),
                       declared_callbacks=list(fixtures.values()), calls=calls, stopped=stopped,
                       final=snapshot(bool(stopped)), final_pc=final_pc, stores=stores,
                       fixture_stores=fixture_stores, trace=trace,
                       original_instruction_count=sum(pc in pcs for pc in trace),
                       fixture_instruction_count=sum(pc in stub_pcs for pc in trace))
            dump(a.output/(name+'.json'), row)
            report['cases'].append(dict(name=name, file=name+'.json'))
            dump(a.output/'report.json', report)
        assert len(report['cases']) == len(CASES)
        report['status'] = 'PASS_UNCHANGED_ORIGINAL_RESOURCE_STAGE_WITH_DECLARED_EXTERNAL_FIXTURES'
        dump(a.output/'report.json', report)
    except BaseException as error:
        report.update(status='FAIL_RETAINED', failure=repr(error))
        dump(a.output/'report.json', report)
        raise
    return 0

if __name__ == '__main__':
    sys.exit(main())
