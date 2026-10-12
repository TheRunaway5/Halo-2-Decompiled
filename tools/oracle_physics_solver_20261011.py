#!/usr/bin/env python3
"""Run untouched full Xbox solver CFGs on explicitly declared component inputs.

Schema selectors and finite row records are diagnostic arithmetic inputs, not
claims about game-produced Jacobians or evidence of playable contact parity.
No original instructions, initialization flags or returned results are patched.
"""
import argparse, hashlib, json, random, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

BASE, SIZE, STACK, SP, STOP = 0x1000000, 0x10000, 0x2000000, 0x200f000, 0x3000000
CONFIG, SCHEMAS, JAC, ACC, IMP, OUT = (BASE+x for x in (0x1000,0x2000,0x3000,0x4000,0x6000,0x7000))
MOTION, POS, QUAT, STEP, GRAVITY = (BASE+x for x in (0x8000,0x8200,0x8220,0x8240,0x8260))
LENGTHS={1:24,2:4,3:8,4:16,5:16,6:8,7:20,8:24,9:8,10:16,11:12,12:4,13:4,14:12,15:12,16:4}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--image',choices=('1.1','1.5'),required=True)
    ap.add_argument('--correspondence',type=Path,required=True)
    ap.add_argument('--quick',action='store_true')
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    paired=json.loads(a.correspondence.read_text());assert paired['complete_bodies']==25
    assert paired['disc_1_1_sha256']==PIN and paired['later_owned_xbe_sha256']==NEW_PIN
    assert paired['extension_tool_sha256']==sha(ROOT/'tools/research_physics_solver_20261011.py')
    latest=a.image=='1.5';mapping={int(k,16):v for k,v in paired['address_map'].items()}
    mapping.update({int(k,16):v for k,v in paired['instruction_map'].items()})
    sites={v:k for k,v in mapping.items()};select=lambda p:mapping[p] if latest else p
    path=NEW_DEFAULT if latest else ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path)==(NEW_PIN if latest else PIN);image=Xbe(path);cases=[]
    specs=[('cold_empty',[],1,2,0,False),('warm_empty',[],0,2,0,True)]
    for schema in range(1,17):
        for mode in range(1 if a.quick else 4):
            # Warm selectors are independently declared diagnostic controls.
            for warm in ([2] if a.quick else [2,3,4,5,6]):
                specs.append((f'schema_{schema}_round_{mode}_warm_{warm}',[schema],1+mode%2,warm,mode,True))
    if not a.quick:
        specs += [(f'chain_round_{mode}',[15,12,13,16,4,5,6,9,10,11,2,3,7,8],4,2,mode,True) for mode in range(4)]
        for mode in range(4):
            for schema in (2,3,4,5,6,7,8,9,10,11,12,13):
                specs.append((f'repeated_{schema}_round_{mode}',[schema,schema,schema],4,5 if schema in (2,3) else 2,mode,True))
            for seed in range(12):
                specs.append((f'mixed_pair_round_{mode}_seed_{seed}',[2,3,3,2,3,2],8,5,mode,True))
    for case_index,(name,rows,iterations,warm,mode,preinit) in enumerate(specs):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(BASE,SIZE);g.mem_map(STACK,0x10000);g.mem_map(STOP,0x1000)
        rnd=random.Random(1000+case_index);g.mem_write(BASE,rnd.randbytes(SIZE));g.mem_write(STACK,rnd.randbytes(0x10000))
        w=lambda p,x:g.mem_write(p,(int(x)&0xffffffff).to_bytes(4,'little'))
        f=lambda p,x:g.mem_write(p,struct.pack('<f',x))
        vec=lambda p,x:g.mem_write(p,struct.pack('<4f',*x))
        fp=(0x27f+(mode<<10),0x1f80+(mode<<13));abi=[0x12345678,0x13579bdf,0x2468ace0,0x23456789]
        def setup_call(entry,args,receiver=0):
            w(SP,STOP)
            for i,x in enumerate(args):w(SP+4+i*4,x)
            for register,value in [(R.UC_X86_REG_ESP,SP),(R.UC_X86_REG_EAX,0),(R.UC_X86_REG_ECX,receiver),
                (R.UC_X86_REG_EDX,0),(R.UC_X86_REG_EBX,abi[0]),(R.UC_X86_REG_EBP,abi[1]),
                (R.UC_X86_REG_ESI,abi[2]),(R.UC_X86_REG_EDI,abi[3]),(R.UC_X86_REG_EFLAGS,2),
                (R.UC_X86_REG_FPCW,fp[0]),(R.UC_X86_REG_MXCSR,fp[1])]:g.reg_write(register,value)
            for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),0)
            # All source preparation owners leave a balanced x87 stack.
            g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
            g.reg_write(R.UC_X86_REG_FPSW,0)
            return select(entry)
        def call(entry,args,receiver=0):
            g.emu_start(setup_call(entry,args,receiver),STOP,count=1000000)
            assert g.reg_read(R.UC_X86_REG_EIP)==STOP,hex(g.reg_read(R.UC_X86_REG_EIP))
        vec(POS,(1.5,-2,4,0));vec(QUAT,(0,0,0,1));call(0x321320,[POS,QUAT],MOTION)
        for off in (0x40,0x50,0xf0,0x100):vec(MOTION+off,tuple(rnd.uniform(-2,2) for _ in range(3))+(0,))
        vec(MOTION+0x20,(.1,-.2,.3,0));f(MOTION+0x2c,.5);vec(MOTION+0xe0,(.5,.75,.875,.25))
        f(STEP,1/30);vec(GRAVITY,(0,0,-4.1712594,0))
        g.mem_write(ACC,bytes(0x200));w(ACC,2)
        call(0x3218a0,[STEP,GRAVITY,ACC+0x80],MOTION)
        call(0x3218a0,[STEP,GRAVITY,ACC+0x100],MOTION)
        w(ACC+0x180,3)
        if preinit:call(0x322d60,[])
        # Real original initialization must have succeeded, never force its flag.
        if preinit:assert bytes(g.mem_read(select(0x488180),1))==b'\x01'
        g.mem_write(CONFIG,bytes(0x40));f(CONFIG+4,.25);f(CONFIG+8,1);f(CONFIG+16,.5)
        w(CONFIG+0x1c,iterations);f(CONFIG+0x20,1/max(1,iterations))
        g.mem_write(SCHEMAS,bytes(0x1000));g.mem_write(JAC,bytes(0x1000));g.mem_write(IMP,bytes(0x800));g.mem_write(OUT,bytes(0x800))
        for p in range(JAC,JAC+0x1000,4):f(p,rnd.uniform(-.75,.75))
        for p in range(IMP,IMP+0x800,4):f(p,rnd.uniform(-.1,.1))
        def header(p):
            w(p,1|(24<<8)|(1<<16));w(p+4,JAC);w(p+8,OUT);w(p+12,ACC+0x80);w(p+16,ACC+0x100);w(p+20,8)
        header(SCHEMAS);p=SCHEMAS+24
        for schema in rows:
            n=LENGTHS[schema]
            for q in range(p,p+n,4):f(q,rnd.uniform(-.5,.5))
            w(p,schema|(n<<8)|(warm<<16))
            if schema==1:header(p)
            if schema in (7,8):w(p+4,OUT+0x400);w(p+16,8)
            if schema==14:w(p+4,STOP+0x100)
            if schema==15:f(p+4,.375);f(p+8,.75)
            p+=n
        w(p,0x400)
        entry=setup_call(0x322ed0,[CONFIG,SCHEMAS,ACC,IMP])
        before_image=bytes(g.mem_read(0,0x810000));before_base=bytes(g.mem_read(BASE,SIZE));before_stack=bytes(g.mem_read(STACK,0x10000))
        writes=[];trace=[];ranges=[];boundary=None;last_site=0
        def extra(p,n):
            if not (BASE<=p and p+n<=BASE+SIZE) and not(STACK<=p and p+n<=STACK+0x10000):
                assert 0<=p and p+n<=len(before_image),(hex(p),n)
                ranges.append((p,p+n))
        def on_read(_g,_access,p,n,_x,_data):extra(p,n)
        def on_write(_g,_access,p,n,x,_data):
            extra(p,n);actual=g.reg_read(R.UC_X86_REG_EIP);canonical=sites[actual] if latest else actual
            writes.append(dict(instruction=canonical,address=p,before=bytes(g.mem_read(p,n)).hex(),written=(x&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
        def on_code(_g,p,_size,_data):
            nonlocal boundary,last_site
            canonical=sites.get(p,p) if latest else p
            if p==STOP+0x100 or canonical in (0x378aa0,0x2dfd80):
                esp=g.reg_read(R.UC_X86_REG_ESP);count=2 if p==STOP+0x100 else 0 if canonical==0x378aa0 else 1
                boundary=dict(instruction=last_site,entry=p,entry_esp=esp,ecx=None,
                    arguments=[int.from_bytes(g.mem_read(esp+4+i*4,4),'little') for i in range(count)])
                g.emu_stop();return
            trace.append(canonical);last_site=canonical
        g.hook_add(UC_HOOK_MEM_WRITE,on_write);g.hook_add(UC_HOOK_MEM_READ,on_read);g.hook_add(UC_HOOK_CODE,on_code)
        try:g.emu_start(entry,STOP,count=1000000)
        except Exception as e:raise RuntimeError(f'{name}: {e}; last={last_site:08x}') from e
        assert boundary or g.reg_read(R.UC_X86_REG_EIP)==STOP,(name,hex(g.reg_read(R.UC_X86_REG_EIP)))
        merged=[]
        for start,end in sorted(ranges):
            if merged and start<=merged[-1][1]:merged[-1][1]=max(end,merged[-1][1])
            else:merged.append([start,end])
        banks=[[BASE,before_base.hex()],[STACK,before_stack.hex()]]+[[p,before_image[p:q].hex()] for p,q in merged]
        cases.append(dict(name=name,image=a.image,args=[CONFIG,SCHEMAS,ACC,IMP],entry_esp=SP,abi=abi,fp=fp,
            banks=banks,after_banks=[[p,bytes(g.mem_read(p,len(bytes.fromhex(b)))).hex()] for p,b in banks],
            writes=writes,trace=trace,boundary=boundary,returned=None if boundary else g.reg_read(R.UC_X86_REG_EAX),
            declared_input_controls=dict(schema_selectors=rows,warm_selector=warm,iterations=iterations,preinitialized_by_original=preinit)))
        print(name,len(trace),len(writes),flush=True)
    result=dict(schema=1,fixture_kind='full_solver',original_sha256=sha(path),tool_sha256=sha(__file__),
        correspondence_sha256=sha(a.correspondence),cases=cases,source_instruction_patches=[],
        native_expected_data_used=False,live_gameplay_proof=False,scope=__doc__,
        retained_write_scope='Every ordered store including ABI stack and all read or written banks')
    fixture=a.output/'fixture.json';fixture.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
    report=dict(cases=len(cases),instructions=sum(len(c['trace']) for c in cases),
        ordered_writes=sum(len(c['writes']) for c in cases),full_state_bytes=sum(len(bytes.fromhex(b)) for c in cases for _,b in c['banks']),
        required_stops=sum(c['boundary'] is not None for c in cases),fixture_sha256=sha(fixture))
    (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
