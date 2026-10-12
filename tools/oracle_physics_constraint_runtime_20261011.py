#!/usr/bin/env python3
"""Execute untouched original constraint producers and retain their complete outputs.

Inputs are explicitly declared raw component records, not captured gameplay
contacts. Accumulators are prepared by the original constructor/preparation
owners. RSQRTSS observations belong to Unicorn's CPU, not a verified Xbox CPU.
No executable instruction or producer output is patched.
"""
import argparse, hashlib, json, random, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

BASE, SIZE, STACK, SP, STOP = 0x1000000, 0x10000, 0x2000000, 0x200f000, 0x3000000
OBJ, POINTS, RECORDS, CONTEXT, CURSORS, SCHEMAS, JAC, ACC, OUT = (BASE+x for x in
    (0x1000,0x2000,0x2400,0x2800,0x2900,0x3000,0x4000,0x6000,0x7000))
MOTION, POS, QUAT, STEP, GRAVITY, BODY, PROPS, VALUE, PAYLOAD = (BASE+x for x in
    (0x8000,0x8200,0x8220,0x8240,0x8260,0x9000,0x9100,0x9200,0x9300))
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--image',choices=('1.1','1.5'),required=True)
    ap.add_argument('--correspondence',type=Path,required=True)
    ap.add_argument('--quick',action='store_true')
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    paired=json.loads(a.correspondence.read_text());assert paired['complete_bodies']==36
    assert paired['disc_1_1_sha256']==PIN and paired['later_owned_xbe_sha256']==NEW_PIN
    assert paired['extension_tool_sha256']==sha(ROOT/'tools/research_physics_constraint_runtime_20261011.py')
    latest=a.image=='1.5';mapping={int(k,16):v for k,v in paired['address_map'].items()}
    mapping.update({int(k,16):v for k,v in paired['instruction_map'].items()})
    mapping.update({int(k,16):v for k,v in paired['entries'].items()})
    sites={v:k for k,v in mapping.items()};select=lambda p:mapping[p] if latest else p
    path=NEW_DEFAULT if latest else ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path)==(NEW_PIN if latest else PIN);image=Xbe(path);cases=[]
    specs=[]
    for entry in (0x325bb0,0x322380,0x31bd00,0x31bbe0):
        for count in ([1,2] if a.quick else [0,1,2,4]):
            for variant in range(2 if a.quick else 8):
                specs.append((entry,count,variant,variant%4))
    specs += [(e,2,v,v%4) for e in (0x324d00,0x325b70,0x326820,0x31b810,0x31b880,0x31b9d0,0x1d2d70) for v in range(4)]
    for index,(entry,count,variant,mode) in enumerate(specs):
        name=f'{entry:08x}_count_{count}_variant_{variant}_round_{mode}'
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(BASE,SIZE);g.mem_map(STACK,0x10000);g.mem_map(STOP,0x1000)
        rnd=random.Random(17000+index);g.mem_write(BASE,rnd.randbytes(SIZE));g.mem_write(STACK,rnd.randbytes(0x10000))
        w=lambda p,x:g.mem_write(p,(int(x)&0xffffffff).to_bytes(4,'little'))
        f=lambda p,x:g.mem_write(p,struct.pack('<f',x))
        vec=lambda p,x:g.mem_write(p,struct.pack('<4f',*x))
        fp=(0x27f+(mode<<10),0x1f80+(mode<<13));abi=[0x12345678,0x13579bdf,0x2468ace0,0x23456789]
        def setup_call(owner,args,receiver=0):
            w(SP,STOP)
            for i,x in enumerate(args):w(SP+4+i*4,x)
            for register,value in [(R.UC_X86_REG_ESP,SP),(R.UC_X86_REG_EAX,0),(R.UC_X86_REG_ECX,receiver),
                (R.UC_X86_REG_EDX,0),(R.UC_X86_REG_EBX,abi[0]),(R.UC_X86_REG_EBP,abi[1]),
                (R.UC_X86_REG_ESI,abi[2]),(R.UC_X86_REG_EDI,abi[3]),(R.UC_X86_REG_EFLAGS,2),
                (R.UC_X86_REG_FPCW,fp[0]),(R.UC_X86_REG_MXCSR,fp[1])]:g.reg_write(register,value)
            for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),0)
            g.reg_write(R.UC_X86_REG_FPTAG,0xffff);g.reg_write(R.UC_X86_REG_FPSW,0)
            return select(owner)
        def call(owner,args,receiver=0):
            g.emu_start(setup_call(owner,args,receiver),STOP,count=1000000)
            assert g.reg_read(R.UC_X86_REG_EIP)==STOP,hex(g.reg_read(R.UC_X86_REG_EIP))
        g.mem_write(ACC,bytes(0x200));w(ACC,2)
        for i in range(2):
            vec(POS,(1.5+i,-2+i,4,0));vec(QUAT,(0,0,0,1));call(0x321320,[POS,QUAT],MOTION)
            for off in (0x40,0x50,0xf0,0x100):vec(MOTION+off,tuple(rnd.uniform(-2,2) for _ in range(3))+(0,))
            vec(MOTION+0x20,(.1,-.2,.3,0));f(MOTION+0x2c,.5);vec(MOTION+0xe0,(.5,.75,.875,.25))
            f(STEP,1/30);vec(GRAVITY,(0,0,-4.1712594,0))
            call(0x3218a0,[STEP,GRAVITY,ACC+0x80*(i+1)],MOTION)
            # Independent raw flag controls exercise each original transform branch.
            g.mem_write(ACC+0x80*(i+1)+0xc,bytes([(variant>>i)&1]))
        w(ACC+0x180,3)
        for p,n in ((OBJ,0x100),(POINTS,0x200),(RECORDS,0x200),(CONTEXT,0x40),(CURSORS,0x20),
                    (SCHEMAS,0x1000),(JAC,0x1000),(OUT,0x800),(BODY,0x100),(PROPS,0x100),(VALUE,0x40),(PAYLOAD,0x80)):
            g.mem_write(p,bytes(n))
        w(OBJ+0xc,BODY);w(OBJ+0x34,POINTS);w(OBJ+0x38,count);w(OBJ+0x90,RECORDS)
        f(OBJ+0x40,.75);f(OBJ+0x44,.625)
        w(OBJ+0x50,4 if variant&4 else 0)
        for off,x in ((8,.1),(0x10,-.15),(0x18,.125),(0x1c,.5)):f(OBJ+0x50+off,x)
        vec(OBJ+0x70,((1,0,0,0),(0,1,0,0),(0,0,1,0),(1,1,0,0))[variant%4])
        w(CONTEXT,ACC+0x80);w(CONTEXT+4,ACC+0x100)
        for off,x in ((0xc,.5),(0x10,.2),(0x18,.25),(0x1c,.25)):f(CONTEXT+off,x)
        w(CURSORS,JAC);w(CURSORS+4,SCHEMAS)
        for i in range(count):
            vec(POINTS+i*0x20,(1+i*.25,-.5+i*.125,2-i*.25,-.125))
            normals=((0,0,1,0),(.6,0,.8,0),(0,1,0,0),(0,0,-1,0))
            vec(POINTS+i*0x20+0x10,normals[(i+variant)%4] if variant>=4 else (0,0,1,0))
            record=RECORDS+i*0x1c
            f(record,0 if variant==0 else .5);w(record+4,0 if variant%2==0 else 1)
            f(record+8,.25);f(record+0x10,.125);f(record+0x14,-.0625)
        w(BODY+0x30,PROPS);w(BODY+0x34,variant%4)
        for i in range(4):w(PROPS+i*8,0x2003 if i==variant%3 else 0x1000+i);w(PROPS+i*8+4,VALUE)
        vec(VALUE,(.125,-.25,.5,0))
        for i in range(8):f(PAYLOAD+i*4,.125*(i+1))
        f(PAYLOAD+0x20,.75);f(PAYLOAD+0x24,.625);w(PAYLOAD+0x28,ACC+0x80);w(PAYLOAD+0x2c,ACC+0x100)
        if entry==0x31b9d0:
            w(PAYLOAD+0x10,JAC)
            for i in range(24):f(JAC+i*4,rnd.uniform(-1,1))
        args={0x325bb0:[POINTS,count,RECORDS,OBJ+0x50,CONTEXT,CURSORS],
            0x322380:[CONTEXT,CURSORS],0x31bd00:[CONTEXT,CURSORS],0x31bbe0:[CONTEXT,CURSORS],
            0x324d00:[CONTEXT,CURSORS,OUT,0x1c],0x325b70:[CONTEXT,CURSORS],
            0x326820:[select(0x31b810),0x30,CONTEXT,CURSORS],0x31b810:[CONTEXT,PAYLOAD],
            0x31b880:[CONTEXT,PAYLOAD],0x31b9d0:[CONTEXT,PAYLOAD],0x1d2d70:[BODY]}[entry]
        actual_entry=setup_call(entry,args,OBJ)
        before_image=bytes(g.mem_read(0,0x810000));before_base=bytes(g.mem_read(BASE,SIZE));before_stack=bytes(g.mem_read(STACK,0x10000))
        writes=[];trace=[];ranges=[];estimates=[];pending=None;last_site=0
        def extra(p,n):
            if not(BASE<=p and p+n<=BASE+SIZE) and not(STACK<=p and p+n<=STACK+0x10000):
                assert 0<=p and p+n<=len(before_image),(hex(p),n);ranges.append((p,p+n))
        def on_read(_g,_access,p,n,_x,_data):extra(p,n)
        def on_write(_g,_access,p,n,x,_data):
            extra(p,n);actual=g.reg_read(R.UC_X86_REG_EIP);canonical=sites[actual] if latest else actual
            writes.append(dict(instruction=canonical,address=p,before=bytes(g.mem_read(p,n)).hex(),written=(x&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
        def on_code(_g,p,_size,_data):
            nonlocal pending,last_site
            if pending:
                register,input_bits,site=pending
                estimates.append(dict(instruction=site,input=input_bits,output=g.reg_read(register)&0xffffffff));pending=None
            canonical=sites.get(p,p) if latest else p;trace.append(canonical);last_site=canonical
            if canonical in (0x32601a,0x3261f7):
                pending=(R.UC_X86_REG_XMM1 if canonical==0x32601a else R.UC_X86_REG_XMM4,
                    g.reg_read(R.UC_X86_REG_XMM0)&0xffffffff,canonical)
        g.hook_add(UC_HOOK_MEM_WRITE,on_write);g.hook_add(UC_HOOK_MEM_READ,on_read);g.hook_add(UC_HOOK_CODE,on_code)
        try:g.emu_start(actual_entry,STOP,count=1000000)
        except Exception as e:raise RuntimeError(f'{name}: {e}; last={last_site:08x}') from e
        assert g.reg_read(R.UC_X86_REG_EIP)==STOP,(name,hex(g.reg_read(R.UC_X86_REG_EIP)))
        merged=[]
        for start,end in sorted(ranges):
            if merged and start<=merged[-1][1]:merged[-1][1]=max(end,merged[-1][1])
            else:merged.append([start,end])
        banks=[[BASE,before_base.hex()],[STACK,before_stack.hex()]]+[[p,before_image[p:q].hex()] for p,q in merged]
        cases.append(dict(name=name,image=a.image,entry=entry,receiver=OBJ,args=args,entry_esp=SP,abi=abi,fp=fp,
            banks=banks,after_banks=[[p,bytes(g.mem_read(p,len(bytes.fromhex(b)))).hex()] for p,b in banks],
            writes=writes,trace=trace,estimates=estimates,boundary=None,returned=g.reg_read(R.UC_X86_REG_EAX),
            declared_input_controls=dict(point_count=count,raw_variant=variant,rounding_mode=mode,
                prepared_by_original=[0x321320,0x3218a0])))
        print(name,len(trace),len(writes),len(estimates),flush=True)
    fixture=a.output/'fixture.json';fixture.write_text(json.dumps(dict(schema=1,fixture_kind='constraint_producers',
        original_sha256=sha(path),tool_sha256=sha(__file__),correspondence_sha256=sha(a.correspondence),cases=cases,
        source_instruction_patches=[],native_expected_data_used=False,live_gameplay_proof=False,scope=__doc__,
        retained_write_scope='Every ordered store including ABI stack and all read or written banks'),sort_keys=True,separators=(',',':'))+'\n')
    report=dict(cases=len(cases),instructions=sum(len(c['trace']) for c in cases),ordered_writes=sum(len(c['writes']) for c in cases),
        full_state_bytes=sum(len(bytes.fromhex(b)) for c in cases for _,b in c['banks']),
        emulator_cpu_estimates=sum(len(c['estimates']) for c in cases),fixture_sha256=sha(fixture))
    (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
