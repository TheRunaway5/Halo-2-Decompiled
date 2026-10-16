#!/usr/bin/env python3
"""Execute untouched TU1.5 query issue, completion and visibility owners.

Authored definitions and quads come from the original request/quad fixtures.
SDK metadata is explicitly declared input: real observed GPU counts plus
bounded pending/ratio cases. No instruction, callee or result is substituted.
These CPU cases do not establish a live game's query scheduling or pixels.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--fixture', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('--gpu-report', type=Path, required=True)
    p.add_argument('--reuse-only', action='store_true')
    a = p.parse_args()
    assert not a.fixture.exists() and not a.report.exists()
    requests = json.loads(Path('tests/fixtures/lens_request_tu15_original.json').read_text())
    quads = json.loads(Path('tests/fixtures/lens_quad_tu15_hardware.json').read_text())
    gpu = json.loads(a.gpu_report.read_text())
    assert gpu['passed'] and gpu['samples'][0]['count'] == 4624
    x = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(x.data).hexdigest() == NEW_PIN
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(0x10000, 0x800000)
    for section in x.sections:
        u.mem_write(section.va, x.section_bytes(section))
    original = bytes(u.mem_read(0x10000, 0x800000))
    u.mem_map(0x2000000, 0x100000)
    u.mem_map(0x2400000, 0x300000)
    u.mem_map(0x2800000, 4096)
    driver, fifo, pages, stack, stop = 0x2002000, 0x2003000, 0x2005000, 0x2600000, 0x2800000
    trace, observed = [], set()
    regs = [R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX,
            R.UC_X86_REG_EBX, R.UC_X86_REG_ESP, R.UC_X86_REG_EBP,
            R.UC_X86_REG_ESI, R.UC_X86_REG_EDI]

    def instruction(cpu, pc, size, _):
        assert bytes(cpu.mem_read(pc, size)) == x.read(pc, size)
        trace.append(pc)

    def access(cpu, kind, address, size, value, _):
        if address < 0x800000:
            observed.update(range(address & ~4095, (address+size+4095) & ~4095, 4096))

    u.hook_add(UC_HOOK_CODE, instruction)
    u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, access)
    banks, ids, rows = [], {}, []

    def reference(address, data):
        key = address, data.hex()
        if key not in ids:
            ids[key] = len(banks)
            banks.append(dict(address=address, hex=key[1]))
        return ids[key]

    def execute(entry, label, arguments=(), esi=None):
        scratch = bytearray([0xcd])*0x8000
        struct.pack_into('<'+'I'*(len(arguments)+1), scratch, 0x4000, stop, *arguments)
        u.mem_write(stack, bytes(scratch))
        gp = [0x72000000+n*0x1000 for n in range(8)]
        gp[4] = stack+0x4000
        if esi is not None:
            gp[6] = esi
        for reg, value in zip(regs, gp):
            u.reg_write(reg, value)
        for n in range(8):
            u.reg_write(R.UC_X86_REG_XMM0+n, 0)
        for reg, value in ((R.UC_X86_REG_EFLAGS, 0x202), (R.UC_X86_REG_FPCW, 0x27f),
                           (R.UC_X86_REG_FPSW, 0), (R.UC_X86_REG_FPTAG, 0xffff), (R.UC_X86_REG_MXCSR, 0x1f80)):
            u.reg_write(reg, value)
        before = bytes(u.mem_read(0x10000, 0x800000))
        high = [(address, bytes(u.mem_read(address, size))) for address, size in high_ranges]
        trace.clear(); observed.clear()
        u.emu_start(entry, stop, count=200000)
        assert u.reg_read(R.UC_X86_REG_EIP) == stop
        low = [(page, before[page-0x10000:page-0x10000+4096]) for page in sorted(observed)]
        initial = low+high
        final = [(address, bytes(u.mem_read(address, len(data)))) for address, data in initial]
        rows.append(dict(entry=entry, variant=label, entry_gp=gp,
            exit_gp=[u.reg_read(reg) for reg in regs], eflags=u.reg_read(R.UC_X86_REG_EFLAGS), eflags_mask=0xcc5,
            exit_xmm=[f'{u.reg_read(R.UC_X86_REG_XMM0+n):032x}' for n in range(8)],
            x87_status=u.reg_read(R.UC_X86_REG_FPSW), mxcsr=u.reg_read(R.UC_X86_REG_MXCSR),
            instructions=trace.copy(), initial_banks=[reference(p,b) for p,b in initial],
            final_banks=[reference(p,b) for p,b in final]))

    accepted = {(c['map'],c['tag']):c for c in requests['cases'] if c['variant']=='accepted'}

    def setup(request):
        u.mem_write(0x10000, original)
        high_ranges.clear()
        for index in request['initial_banks']:
            bank = requests['banks'][index]
            raw = bytes.fromhex(bank['hex']);u.mem_write(bank['address'], raw)
            if bank['address']>=0x800000 and bank['address']!=stack:
                high_ranges.append((bank['address'],len(raw)))
        for index in request['final_banks']:
            bank = requests['banks'][index]
            if bank['address']==0x52d000:
                u.mem_write(bank['address'],bytes.fromhex(bank['hex']))
        u.mem_write(0x411568,struct.pack('<I',driver));u.mem_write(0x411570,bytes(4))
        raw = bytearray(4096);struct.pack_into('<II',raw,0,fifo,fifo+4096);struct.pack_into('<I',raw,0x2c,1)
        for n in range(6):
            struct.pack_into('<I',raw,0x7d4+n*4,pages+n*4096)
        u.mem_write(driver,bytes(raw));u.mem_write(fifo,bytes(4096));u.mem_write(pages,bytes(6*4096))
        u.mem_write(0x531fc0,bytes(192));u.mem_write(0x532080,struct.pack('<I',0))
        u.mem_write(0x532084,bytes(16));u.mem_write(0x532094,bytes(4))
        u.mem_write(0x52dfd8,b'\1\1');u.mem_write(0x4922c0,bytes(8))
        high_ranges.extend(((driver,4096),(fifo,4096),(pages,6*4096),(stack,0x8000)))

    high_ranges = []
    for quad in quads['cases']:
        if a.reuse_only and quad['variant']!='near':
            continue
        for variant in (('reused-ready','reused-pending') if a.reuse_only else (quad['variant'],)):
            setup(accepted[(quad['map'],quad['tag'])])
            for index in quad['initial_banks']:
                bank = quads['banks'][index]
                if bank['address']==0x491000:
                    u.mem_write(bank['address'],bytes.fromhex(bank['hex']))
                if bank['address']==0x52d000:
                    u.mem_write(0x52dfdc,bytes.fromhex(bank['hex'])[0xfdc:0xffc])
            if a.reuse_only:
                pending=variant=='reused-pending'
                u.mem_write(0x531fc0,struct.pack('<I',1))
                u.mem_write(pages,struct.pack('<QII',0 if pending else gpu['samples'][0]['timestamp'],4624,0xffffffff if pending else 0))
            execute(0x20b90,quad['map']+':'+quad['tag']+':'+variant,(0,0x1f,0,0,1,0x2e520))
    for request in (() if a.reuse_only else accepted.values()):
        for variant,samples,alpha,area,pending,view in (
            ('visible',4624,0,4624,False,0),('half',2312,0,4624,False,0),
            ('occluded',0,255,4624,False,0),('saturated',9248,0,4624,False,0),
            ('empty',0,255,0,False,0),('pending',4624,0,4624,True,0),
            ('view3',4624,0,4624,False,3)):
            setup(request)
            key=struct.unpack('<I',u.mem_read(0x52dfdc,4))[0]
            key=(key & ~0xff000006) | (view<<1) | (alpha<<24)
            u.mem_write(0x52dfdc,struct.pack('<I',key));u.mem_write(0x52dfe2,struct.pack('<H',area))
            u.mem_write(0x531fc0,struct.pack('<I',1));u.mem_write(0x532094,struct.pack('<I',view))
            u.mem_write(0x4922c0,struct.pack('<I',2));u.mem_write(0x532080,struct.pack('<I',2))
            u.mem_write(0x532098,x.read(0x46a0c4,4))
            u.mem_write(pages,struct.pack('<QII',0 if pending else gpu['samples'][0]['timestamp'],samples,0xffffffff if pending else 0))
            label=request['map']+':'+request['tag']+':'+variant
            execute(0x20750,label)
            execute(0x20d60,label,(0,),0x52dfdc)
    if not a.reuse_only:
        setup(next(iter(accepted.values())))
        execute(0x3803b0,'original-static-normalization')
    a.fixture.parent.mkdir(parents=True,exist_ok=True)
    a.fixture.write_text(json.dumps(dict(entry=0x20b90,original_sha256=NEW_PIN,banks=banks,cases=rows),separators=(',',':'))+'\n')
    a.report.write_text(json.dumps(dict(passed=True,cases=len(rows),original_sha256=NEW_PIN,
        entries={hex(e):sum(c['entry']==e for c in rows) for e in sorted({c['entry'] for c in rows})},
        instruction_patches=[],replaced_services=[],gpu_input_report_sha256=hashlib.sha256(a.gpu_report.read_bytes()).hexdigest(),
        generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),full_goal_complete=False,boundary=__doc__),indent=2)+'\n')
    print('PASS unchanged original query issue/completion/visibility:',len(rows),'cases')


if __name__=='__main__':
    main()
