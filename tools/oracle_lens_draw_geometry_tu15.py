#!/usr/bin/env python3
"""Execute unchanged original lens payload, orientation and primitive bodies.

The nine authored definitions and camera banks come from original fixtures.
Original2FD70 produced those cameras. All reached code executes unchanged;
the clean bounded SDK emission bank and primitive arguments are declared
inputs, not proof of resource construction or a live final flare draw.
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
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--fixture',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True)
    p.add_argument("--final-callbacks",action="store_true")
    p.add_argument("--setup-owners",action="store_true")
    a=p.parse_args();assert not a.fixture.exists() and not a.report.exists()
    assert not a.setup_owners or a.final_callbacks
    quads=json.loads(Path('tests/fixtures/lens_quad_tu15_hardware.json').read_text())
    x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
    for section in x.sections:u.mem_write(section.va,x.section_bytes(section))
    original=bytes(u.mem_read(0x10000,0x800000))
    u.mem_map(0x2000000,0x100000);u.mem_map(0x2400000,0x300000);u.mem_map(0x2800000,4096)
    if a.final_callbacks:u.mem_map(0x80000000,0x4000000)
    driver,fifo,scratch,stack,stop=0x2002000,0x2003000,0x2005000,0x2600000,0x2800000
    regs=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
        R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
    trace,observed,high=[],set(),[]
    authored_cache=None
    authored_pages={}

    def instruction(cpu,pc,size,_):
        assert bytes(cpu.mem_read(pc,size))==x.read(pc,size)
        trace.append(pc)

    def access(cpu,kind,address,size,value,_):
        if address<0x800000:observed.update(range(address&~4095,(address+size+4095)&~4095,4096))
        elif a.final_callbacks and address>=0x80000000:
            assert authored_cache is not None
            for page in range(address&~4095,(address+size+4095)&~4095,4096):
                if page in authored_pages:continue
                at=authored_cache.pointer(page,authored_cache.magic,4096)
                raw=bytes(authored_cache.data[at:at+4096])
                authored_pages[page]=raw;cpu.mem_write(page,raw)

    u.hook_add(UC_HOOK_CODE,instruction);u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access)
    banks,bank_ids,rows=[],{},[]

    def reference(address,data):
        key=address,data.hex()
        if key not in bank_ids:bank_ids[key]=len(banks);banks.append(dict(address=address,hex=key[1]))
        return bank_ids[key]

    def setup(case):
        u.mem_write(0x10000,original);high.clear();authored_pages.clear()
        for index in case['initial_banks']:
            b=quads['banks'][index]
            if b['address'] in (driver,fifo,stack):continue
            data=bytes.fromhex(b['hex']);u.mem_write(b['address'],data)
            if b['address']>=0x800000:high.append((b['address'],len(data)))
        # Whole original2FD70 supplied the submitting camera and projection.
        u.mem_write(0x4c65c0,bytes(u.mem_read(0x491e38,0x74)))
        u.mem_write(0x4c6634,bytes(u.mem_read(0x491eac,0xc0)))
        u.mem_write(0x411568,struct.pack('<I',driver));u.mem_write(0x411570,bytes(4))
        u.mem_write(driver,struct.pack('<3I',fifo,fifo+4096,0));u.mem_write(fifo,bytes(4096))
        u.mem_write(scratch,bytes([0xcd])*4096)
        # The standalone primitive consumes an unchanged clean texture bank.
        # It does not invent a bitmap admission or replace a reached service.
        u.mem_write(0x52df98,bytes(32))
        high.extend(((driver,12),(fifo,4096),(scratch,4096),(stack,0x8000)))

    def execute(entry,label,args=(),registers=None):
        raw=bytearray([0xcd])*0x8000;struct.pack_into('<'+'I'*(len(args)+1),raw,0x4000,stop,*args)
        u.mem_write(stack,bytes(raw));gp=[0x72000000+n*4096 for n in range(8)];gp[4]=stack+0x4000
        for index,value in (registers or {}).items():gp[index]=value
        for reg,value in zip(regs,gp):u.reg_write(reg,value)
        for n in range(8):u.reg_write(R.UC_X86_REG_XMM0+n,0)
        for reg,value in ((R.UC_X86_REG_EFLAGS,0x202),(R.UC_X86_REG_FPCW,0x27f),
            (R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):
            u.reg_write(reg,value)
        before=bytes(u.mem_read(0x10000,0x800000));upper=[(p,bytes(u.mem_read(p,n)))for p,n in high]
        trace.clear();observed.clear()
        try:u.emu_start(entry,stop,count=200000)
        except Exception:
            print('Failed original case',label,'at',hex(u.reg_read(R.UC_X86_REG_EIP)),
                'gp',[hex(u.reg_read(r))for r in regs], 'last',[hex(pc)for pc in trace[-12:]])
            raise
        assert u.reg_read(R.UC_X86_REG_EIP)==stop
        lower=[(p,before[p-0x10000:p-0x10000+4096])for p in sorted(observed)]
        initial=lower+upper+list(authored_pages.items());final=[(p,bytes(u.mem_read(p,len(data))))for p,data in initial]
        end=struct.unpack('<I',u.mem_read(driver,4))[0];assert fifo<=end<=fifo+4096
        rows.append(dict(entry=entry,variant=label,entry_gp=gp,exit_gp=[u.reg_read(r)for r in regs],
            exit_xmm=[f'{u.reg_read(R.UC_X86_REG_XMM0+n):032x}'for n in range(8)],
            eflags=u.reg_read(R.UC_X86_REG_EFLAGS),eflags_mask=0xcc5,
            x87_status=u.reg_read(R.UC_X86_REG_FPSW),mxcsr=u.reg_read(R.UC_X86_REG_MXCSR),
            instructions=trace.copy(),initial_banks=[reference(p,b)for p,b in initial],
            final_banks=[reference(p,b)for p,b in final],fifo_bytes=end-fifo))

    if a.final_callbacks:
        from audit_rendering import Cache
        inventory=json.loads(Path('analysis/map-world-features/inventory.json').read_text())
        paths={c['map']:c['cache']for c in inventory['maps']}
        for case in quads['cases']:
            if a.setup_owners and case['variant']!='near':continue
            setup(case)
            cache=Cache(paths[case['map']]);authored_cache=cache
            lens=next(d for d,t in cache.tags.items()if t['group']=='lens' and t['name']==case['tag'])
            lens_tag,lens_root=cache.tag(lens,'lens');bitmap=cache.u32(lens_root+0x24)
            _,bitmap_root=cache.tag(bitmap,'bitm')
            images=cache.block(bitmap_root+0x44,0x74)
            root=bytearray(cache.data[bitmap_root:bitmap_root+0x4c])
            struct.pack_into('<I',root,0x48,0x2520100)
            u.mem_write(0x2520000,bytes(0x10000));u.mem_write(0x2520000,bytes(root))
            table=struct.unpack('<I',u.mem_read(0x4f0414,4))[0]
            u.mem_write(table+(bitmap&65535)*16,struct.pack('<4I',0x6269746d,bitmap,0x2520000,0x4c))
            high.append((table+(bitmap&65535)*16,16))
            high.extend(((0x2520000,0x10000),(0x2530000,4096)))
            for image,at in enumerate(images):
                row=0x2520100+image*0x74;header=0x2530000+image*32
                raw=bytearray(cache.data[at:at+0x74]);struct.pack_into('<I',raw,0x40,0x2540000+image*4096)
                u.mem_write(row,bytes(raw));u.mem_write(header,bytes(32))
                if a.setup_owners:
                    execute(0x1301e0,case['map']+':'+case['tag']+':bitmap-header-'+str(image),(0,header),{0:row})
                else:
                    u.mem_write(stack+0x4000,struct.pack('<3I',stop,0,header))
                    u.reg_write(R.UC_X86_REG_ESP,stack+0x4000);u.reg_write(R.UC_X86_REG_EAX,row)
                    u.reg_write(R.UC_X86_REG_FPCW,0x27f);u.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
                    u.emu_start(0x1301e0,stop,count=20000)
                    assert u.reg_read(R.UC_X86_REG_EIP)==stop
                u.mem_write(row+0x50,struct.pack('<I',header));u.mem_write(row+0x70,struct.pack('<I',1))
            # Explicit ready bitmap/visibility and clean SDK controls. These
            # are not assertions about native resource readiness or I/O.
            u.mem_write(0x4f2d58,bytes(4));u.mem_write(driver+12,bytes(4096-12))
            high[:]=[(p,4096 if p==driver else n)for p,n in high]
            for stage in range(4):
                u.mem_write(0x40eeb0+stage*128,struct.pack('<6I',1,1,1,2,2,2)+bytes(104))
                u.mem_write(driver+12+stage*4,struct.pack('<I',0x80000000))
            program=[0]*60
            for index,value in {54:1,53:0x11002,34:0x08081524,45:0xc5,
                    35:0x0c0c0408,46:0xd4,8:0x050f0004,9:0x0c0d1400}.items():program[index]=value
            u.mem_write(0x491788,struct.pack('<60I',*program))
            if a.setup_owners:
                execute(0x1ccd0,case['map']+':'+case['tag']+':pixel-program',(),{2:0x491788})
                cache.close();authored_cache=None;continue
            u.mem_write(stack+0x4000,struct.pack('<I',stop))
            u.reg_write(R.UC_X86_REG_ESP,stack+0x4000);u.reg_write(R.UC_X86_REG_EDX,0x491788)
            u.emu_start(0x1ccd0,stop,count=20000)
            assert u.reg_read(R.UC_X86_REG_EIP)==stop
            u.mem_write(driver,struct.pack('<I',fifo));u.mem_write(fifo,bytes(4096))
            identity=struct.unpack('<I',u.mem_read(0x52dfdc,4))[0]
            identity=(identity&~0xff000006)|0xff000000
            u.mem_write(0x52dfdc,struct.pack('<I',identity))
            u.mem_write(0x52dfdc+6,struct.pack('<H',1))
            u.mem_write(stack+0x4000,struct.pack('<I',stop))
            u.reg_write(R.UC_X86_REG_ESP,stack+0x4000)
            u.emu_start(0x3803b0,stop,count=20000)
            assert u.reg_read(R.UC_X86_REG_EIP)==stop
            position={'near':(1,0,0),'far':(50,0,0),'behind':(-1,0,0),'off-axis':(5,3,1),'split':(5,0,0)}[case['variant']]
            u.mem_write(0x52dfdc+12,struct.pack('<3f',*position))
            execute(0x2e9c0,case['map']+':'+case['tag']+':'+case['variant'],(0x52dfdc,0x52dfe4))
            cache.close();authored_cache=None
        a.fixture.parent.mkdir(parents=True,exist_ok=True)
        a.fixture.write_text(json.dumps(dict(entry=0x1301e0 if a.setup_owners else 0x2e9c0,original_sha256=NEW_PIN,banks=banks,cases=rows),separators=(',',':'))+'\n')
        a.report.write_text(json.dumps(dict(passed=True,cases=len(rows),original_sha256=NEW_PIN,emitting=sum(c['fifo_bytes']>0 for c in rows),replaced_services=[],instruction_patches=[],full_goal_complete=False,resource_inputs='Explicit ready bitmap headers produced by unchanged1301E0; not native resource admission proof'),indent=2)+'\n')
        print('PASS unchanged original final lens callbacks',len(rows),'emitting',sum(c['fifo_bytes']>0 for c in rows));return
    near=[c for c in quads['cases']if c['variant']=='near']
    for case in near:
        for outputs in (False,True):
            setup(case);out=scratch if outputs else 0
            execute(0x2e3d0,case['map']+':'+case['tag']+(':outputs'if outputs else':null-outputs'),
                (out+32 if outputs else 0,out+48 if outputs else 0,out+52 if outputs else 0,out+56 if outputs else 0),
                {0:out+64 if outputs else 0,2:out,6:0x52dfe4,7:out+16 if outputs else 0})
    for case in quads['cases'][:5]:
        for mode in range(6):
            setup(case)
            direction={'near':(.25,.5,.75),'far':(1,0,0),'behind':(0,1,0),
                'off-axis':(0,0,1),'split':(-.5,.75,.125)}[case['variant']]
            position={'near':(1,0,0),'far':(50,0,0),'behind':(-1,0,0),
                'off-axis':(5,3,1),'split':(5,0,0)}[case['variant']]
            u.mem_write(scratch,struct.pack('<3f',*direction))
            u.mem_write(scratch+16,struct.pack('<3f',*position))
            execute(0x2f2b0,case['variant']+':orientation-'+str(mode),(0x3f000000,),{0:scratch,1:scratch+16,2:mode})
    for case in quads['cases'][:5]:
        for name,size,sine,cosine,color in (('plain',.25,0,1,0x80f02040),('rotated',.5,.6,.8,0xffffffff),('zero',0,0,1,0)):
            setup(case)
            position={'near':(1,0,0),'far':(50,0,0),'behind':(-1,0,0),'off-axis':(5,3,1),'split':(5,0,0)}[case['variant']]
            u.mem_write(scratch,struct.pack('<3f',*position))
            f=lambda v:struct.unpack('<I',struct.pack('<f',v))[0]
            execute(0x48090,case['variant']+':primitive-'+name,(scratch,f(size),f(size),f(sine),f(cosine),color))
    for slot in (0,1,17,510):
        for cold in (True,False):
            setup(near[0]);u.mem_write(scratch,struct.pack('<2I',(0x52dfdc+slot*32)>>5,0xffffffff))
            if not cold:
                u.reg_write(R.UC_X86_REG_ESP,stack+0x4000);u.mem_write(stack+0x4000,struct.pack('<I',stop))
                u.reg_write(R.UC_X86_REG_EDX,0x56e258);u.emu_start(0x166ae0,stop,count=20000)
                u.mem_write(0x56e225,b'\1')
            execute(0x166a80,f'slot-{slot}:crc-'+('cold'if cold else'warm'),(scratch+4,),{0:scratch,7:4})
    a.fixture.parent.mkdir(parents=True,exist_ok=True)
    a.fixture.write_text(json.dumps(dict(entry=0x2e3d0,original_sha256=NEW_PIN,banks=banks,cases=rows),separators=(',',':'))+'\n')
    report=dict(passed=True,cases=len(rows),entries={hex(e):sum(c['entry']==e for c in rows)for e in sorted({c['entry']for c in rows})},
        primitive_quads=sum(c['fifo_bytes']>0 for c in rows),original_sha256=NEW_PIN,
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        instruction_patches=[],replaced_services=[],full_goal_complete=False,boundary=__doc__)
    a.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS unchanged original lens draw geometry:',len(rows),'cases')


if __name__=='__main__':main()
