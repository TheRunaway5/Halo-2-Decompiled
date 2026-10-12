#!/usr/bin/env python3
"""Untouched322690 constrained-island prefixes with declared arena/body controls.

Actual original constructors and motion preparation execute. Required solver,
Jacobian producer, allocator and alternate-motion owners stop at their entry;
none receives an injected return or completed state. Not live-world proof.
"""
import argparse, hashlib, json, random, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN

BASE,SIZE,SP,STOP=0x1000000,0x10000,0x200f000,0x3000000
ISLAND,WORLD=BASE+0x400,BASE+0x800
STEP,SOLVER,CONTEXT,GRAVITY=BASE+0x3000,BASE+0x3040,BASE+0x3080,BASE+0x30a0
POSITION,QUATERNION,ARENA,ALLOCATOR=BASE+0x3100,BASE+0x3120,BASE+0x3800,BASE+0x3840
BODIES,CONSTRAINTS,CONSTRAINT=BASE+0x3900,BASE+0x3940,BASE+0x3980
ARENA_BYTES,POOL=BASE+0x4000,BASE+0x8000
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(path)==PIN
    image=Xbe(path);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(BASE,SIZE);g.mem_map(0x2000000,0x10000);g.mem_map(STOP,0x1000)
    w=lambda p,x,n=4:g.mem_write(p,(int(x)&((1<<(8*n))-1)).to_bytes(n,'little'))
    f=lambda p,x:g.mem_write(p,struct.pack('<f',x))
    vec=lambda p,x:g.mem_write(p,struct.pack('<4f',*x))
    r=lambda p:int.from_bytes(g.mem_read(p,4),'little')
    active=False;writes=[];boundary=None;previous=None;stops={};cases=[]
    constants=[(0x465030,4),(0x465048,4),(0x465044,4),(0x46502c,4),(0x41b760,16),
        (0x41b8f0,36),(0x487638,4),(0x487674,8),(0x48c058,4)]
    retained=[(BASE,SIZE),*constants]
    def call(entry,receiver,args):
        w(SP,STOP)
        for i,x in enumerate(args):w(SP+4+i*4,x)
        for register,value in [(R.UC_X86_REG_ESP,SP),(R.UC_X86_REG_ECX,receiver),
            (R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_MXCSR,0x1f80)]:g.reg_write(register,value)
        g.emu_start(entry,STOP,count=100000)
        assert boundary or g.reg_read(R.UC_X86_REG_EIP)==STOP,hex(g.reg_read(R.UC_X86_REG_EIP))
    def on_write(_g,_access,p,n,x,_data):
        if not active or 0x2000000<=p<0x2010000:return
        assert any(start<=p and p+n<=start+size for start,size in retained),(hex(p),n)
        writes.append(dict(instruction=g.reg_read(R.UC_X86_REG_EIP),address=p,
            before=bytes(g.mem_read(p,n)).hex(),written=(x&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
    def on_code(_g,p,_size,_data):
        nonlocal boundary,previous
        if active and p in stops:
            n,receiver=stops[p];esp=g.reg_read(R.UC_X86_REG_ESP)
            boundary=dict(instruction=previous,entry=p,entry_esp=esp,
                ecx=g.reg_read(R.UC_X86_REG_ECX) if receiver else None,
                arguments=[r(esp+4+i*4) for i in range(n)])
            g.emu_stop()
        previous=p
    g.hook_add(UC_HOOK_MEM_WRITE,on_write);g.hook_add(UC_HOOK_CODE,on_code)
    for branch in ('fits','whole','split','tail'):
        for destination in ('solver','jacobian','allocator','motion'):
            for body_count in (1,2):
                if destination=='allocator' and branch=='fits':continue
                seed=len(cases);rnd=random.Random(seed);g.mem_write(BASE,rnd.randbytes(SIZE));g.mem_write(0x2000000,bytes(0x10000))
                w(0x487674,0);w(0x487678,0);w(0x48c058,ARENA);w(0x487638,ALLOCATOR)
                vec(POSITION,(1.5,-2,4,0));vec(QUATERNION,(0,0,.6,.8))
                for i in range(body_count):
                    body=BASE+0x1000+i*0x100;motion=BASE+0x1800+i*0x200
                    call(0x321320,motion,[POSITION,QUATERNION]);w(BODIES+4*i,body);w(body+0x3c,motion)
                    for off in (0x40,0x50,0xf0,0x100):vec(motion+off,tuple(rnd.uniform(-2,2) for _ in range(3))+(0,))
                    f(motion+0x10c,1);f(motion+0x14,.1);f(motion+0x18,.2)
                    f(motion+0x2c,.25);vec(motion+0xe0,(.5,.75,.875,.25))
                f(STEP,1/30);f(STEP+4,30);vec(GRAVITY,(0,0,-4.1712594,0))
                w(ISLAND+8,8);w(ISLAND+12,0x100);w(ISLAND+0x10,0x80)
                w(ISLAND+0x14,body_count*0x80);w(ISLAND+0x18,5)
                w(ARENA+8,ARENA_BYTES)
                first_offset=body_count*0x80+0x90
                extent={'fits':0x1000,'whole':first_offset-16,'split':first_offset+0x80,'tail':first_offset+0x110}[branch]
                w(ARENA+12,extent)
                w(ALLOCATOR,BASE+0x3a00);w(BASE+0x3a24,STOP+0x100);w(BASE+0x3a28,STOP+0x120)
                w(ALLOCATOR+8,POOL);w(ALLOCATOR+12,0 if destination=='allocator' else 0x4000)
                w(ALLOCATOR+0x14,POOL)
                w(CONSTRAINTS,CONSTRAINT);w(CONSTRAINT,BASE+0x3b00);w(BASE+0x3b24,STOP+0x140)
                w(CONSTRAINT+12,BASE+0x1000);w(CONSTRAINT+0x10,BASE+0x1000+(body_count-1)*0x100)
                count=1 if destination=='jacobian' else 0
                if destination=='motion':
                    w(BASE+0x1800,BASE+0x3c00);w(BASE+0x3c0c,STOP+0x160)
                stops={0x322ed0:(4,False),STOP+0x100:(1,True),STOP+0x140:(2,True),STOP+0x160:(3,True)}
                args=[STEP,SOLVER,CONTEXT,GRAVITY,ISLAND,BODIES,body_count,CONSTRAINTS,count]
                # Also enter through actual31CFE0 for the connected constraint branch.
                for connected in (False,True):
                    if connected:
                        w(ISLAND+0x20,WORLD);w(ISLAND+0x4c,0);w(ISLAND+0x50,0)
                        w(ISLAND+0x3c,BODIES);w(ISLAND+0x40,body_count);w(ISLAND+0x58,CONSTRAINTS);w(ISLAND+0x5c,1)
                        #31CFE0 derives its input fields from the caller's world-step record.
                        # For solver destination this deliberately reaches the real Jacobian
                        # producer, rather than forging an empty successful constraint.
                        world_step=BASE+0x2ff0
                        call_args=[world_step];entry=0x31cfe0;receiver=ISLAND
                        vec(WORLD+0xe0,(0,0,-4.1712594,0))
                    else:call_args=args;entry=0x322690;receiver=0
                    before=[[p,bytes(g.mem_read(p,n)).hex()] for p,n in retained]
                    writes=[];boundary=None;active=True;call(entry,receiver,call_args);active=False
                    assert boundary is not None
                    cases.append(dict(name=f'{branch}_{destination}_{body_count}_{"connected" if connected else "direct"}',
                        image='1.1',entry=entry,receiver=receiver,args=call_args,entry_esp=SP,
                        iterations=1,fp=[0x27f,0x1f80],banks=before,
                        after_banks=[[p,bytes(g.mem_read(p,n)).hex()] for p,n in retained],
                        writes=writes,estimates=[],boundary=boundary,connect_constrained_islands=True,
                        declared_input_controls='Original constructor; raw arithmetic arena/body/constraint controls; no callee returns supplied'))
                    # Restore complete original inputs before the alternate entry;
                    # preparation must not accumulate force/damping from the first prefix.
                    for p,data in before:g.mem_write(p,bytes.fromhex(data))
    result=dict(schema=1,fixture_kind='constraint_step',original_sha256=PIN,tool_sha256=sha(__file__),
        base=BASE,size=SIZE,cases=cases,source_instruction_patches=[],supplied_solver_results=[],
        native_expected_data_used=False,live_gameplay_proof=False,scope=__doc__)
    p=a.output/'fixture.json';p.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
    report=dict(cases=len(cases),ordered_persistent_writes=sum(len(c['writes']) for c in cases),
        full_state_bytes=sum(sum(len(data)//2 for _,data in c['after_banks']) for c in cases),
        required_stops=len(cases),fixture_sha256=sha(p))
    (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
