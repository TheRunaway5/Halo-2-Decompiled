#!/usr/bin/env python3
"""Execute untouched Xbox solver-motion owners against declared arithmetic inputs.

The original constructor creates every pose. Preparation creates real source
accumulators. Readback controls explicitly replace their velocity fields to
exercise average/final velocity arithmetic; these are not computed constraint
solutions, live contacts, or a claim that the iterative solver is ported.
RSQRTSS observations are emulator-only differential inputs, never production.
"""
import argparse, hashlib, json, random, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

BASE, SIZE, SP, STOP = 0x1000000, 0x5000, 0x200f000, 0x3000000
MOTION, STEP, GRAVITY = BASE+0x1000, BASE+0x3000, BASE+0x3020
POSITION, QUATERNION, PACKET = BASE+0x3100, BASE+0x3120, BASE+0x3200
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--image',choices=('1.1','1.5'),required=True)
    ap.add_argument('--correspondence',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    correspondence=json.loads(a.correspondence.read_text())
    assert correspondence['disc_1_1_sha256']==PIN and correspondence['later_owned_xbe_sha256']==NEW_PIN
    assert correspondence['complete_bodies']==len(correspondence['entries'])==22
    assert correspondence['extension_tool_sha256']==sha(ROOT/'tools/research_physics_solver_motion_20261011.py')
    latest=a.image=='1.5'
    mapping={int(k,16):v for k,v in correspondence['entries'].items()}
    sites={v:int(k,16) for k,v in correspondence['instruction_map'].items()}
    operands={int(k,16):v['later'] for k,v in correspondence['operand_map'].items()}
    selected=lambda p:mapping[p] if latest else p
    operand=lambda p:operands[p] if latest else p
    path=NEW_DEFAULT if latest else ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path)==(NEW_PIN if latest else PIN)
    image=Xbe(path);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(BASE,0x10000);g.mem_map(0x2000000,0x10000);g.mem_map(STOP,0x1000)
    w=lambda p,x:g.mem_write(p,(int(x)&0xffffffff).to_bytes(4,'little'))
    f=lambda p,x:g.mem_write(p,struct.pack('<f',x))
    vec=lambda p,x:g.mem_write(p,struct.pack('<4f',*x))
    active=False;writes=[];estimates=[];pending=None;cases=[];fp=(0x27f,0x1f80)
    def call(entry,args):
        w(SP,STOP)
        for i,x in enumerate(args):w(SP+4+i*4,x)
        for register,value in [(R.UC_X86_REG_ESP,SP),(R.UC_X86_REG_ECX,MOTION),
            (R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,fp[0]),(R.UC_X86_REG_MXCSR,fp[1])]:g.reg_write(register,value)
        g.emu_start(selected(entry),STOP,count=100000)
        assert g.reg_read(R.UC_X86_REG_EIP)==STOP,hex(g.reg_read(R.UC_X86_REG_EIP))
        return g.reg_read(R.UC_X86_REG_EAX)
    def on_write(_g,_access,p,n,x,_data):
        if active and BASE<=p and p+n<=BASE+SIZE:
            actual=g.reg_read(R.UC_X86_REG_EIP)
            canonical=sites[actual] if latest else actual
            writes.append(dict(instruction=canonical,address=p,before=bytes(g.mem_read(p,n)).hex(),
                written=(x&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
    def on_code(_g,p,_size,_data):
        nonlocal pending
        if not active:return
        canonical=sites.get(p,p) if latest else p
        if canonical==0x1dc782:pending=g.reg_read(R.UC_X86_REG_XMM0)&0xffffffff
        elif canonical==0x1dc786:
            assert pending is not None
            estimates.append([pending,g.reg_read(R.UC_X86_REG_XMM1)&0xffffffff]);pending=None
    g.hook_add(UC_HOOK_MEM_WRITE,on_write);g.hook_add(UC_HOOK_CODE,on_code)
    def setup(seed):
        nonlocal fp
        rnd=random.Random(seed);g.mem_write(BASE,rnd.randbytes(SIZE));g.mem_write(0x2000000,bytes(0x10000))
        fp=(0x27f,0x1f80)
        vec(POSITION,(1.5,-2,4,0))
        vec(QUATERNION,((0,0,0,1),(0,0,.6,.8),(.5,.5,.5,.5),(0,1,0,0))[seed%4])
        call(0x321320,[POSITION,QUATERNION])
        for off in (0x40,0x50,0xf0,0x100):vec(MOTION+off,tuple(rnd.uniform(-3,3) for _ in range(3))+(0,))
        vec(MOTION+0x20,((0,0,0,0),(.25,-.5,.125,0))[seed%2])
        f(MOTION+0x2c,(0,.25,1,2)[seed%4]);vec(MOTION+0xe0,(.5,.75,.875,.25))
        f(MOTION+12,(0,.5,1,3)[seed%4])
        f(STEP,(0,1/30,1/144,.5)[seed%4]);vec(GRAVITY,(0,0,-4.1712594,0))
        f(MOTION+0x14,(0,.1,100,-1)[seed%4]);f(MOTION+0x18,(.2,100,0,-1)[seed%4])
        w(MOTION+0x10c,(0,0x3f800000,0xbf800000,0x7fc01234,0x80000000)[seed%5])
        # Declared masked CPU rounding controls, not gameplay tuning values.
        mode=(seed//8)%4;fp=(0x27f+(mode<<10),0x1f80+(mode<<13))
        return rnd
    def execute(name,entry,args,iterations,controls):
        nonlocal active,writes,estimates,pending
        before=bytes(g.mem_read(BASE,SIZE));writes=[];estimates=[];pending=None;active=True
        returned=[]
        for _ in range(iterations):returned.append(call(entry,args))
        active=False;assert pending is None
        constants=[[operand(p),bytes(g.mem_read(operand(p),n)).hex()]
            for p,n in ((0x465030,4),(0x465048,4),(0x465044,4),(0x46502c,4),(0x41b760,16))]
        cases.append(dict(name=name,image=a.image,entry=entry,receiver=MOTION,args=args,entry_esp=SP,
            iterations=iterations,fp=fp,before=before.hex(),after=bytes(g.mem_read(BASE,SIZE)).hex(),
            writes=writes,estimates=estimates,constants=constants,boundary=None,returns=returned,
            declared_input_controls=controls))
    for seed in range(40):
        setup(seed)
        execute(f'prepare_{seed}',0x3218a0,[STEP,GRAVITY,PACKET],1,
            'Original constructor pose; declared force/damping/torque/step/mass inputs; untouched accumulator holes')
    for seed in range(40):
        rnd=setup(seed);call(0x3218a0,[STEP,GRAVITY,PACKET])
        # Explicit component arithmetic controls; no precomputed native result.
        for off in (0x10,0x20,0x40,0x50):
            vec(PACKET+off,tuple(rnd.uniform(-2,2) for _ in range(3))+(7 if seed%2 else 0,))
        scale=struct.unpack('<I',struct.pack('<f',(0,.25,1,2)[seed%4]))[0]
        execute(f'readback_{seed}',0x321a60,[STEP,scale,PACKET],4,
            'Original preparation accumulator; declared average/final velocity fields; no constraint solution supplied')
    result=dict(schema=1,fixture_kind='solver_motion',original_sha256=sha(path),tool_sha256=sha(__file__),
        correspondence_sha256=sha(a.correspondence),base=BASE,size=SIZE,cases=cases,
        source_instruction_patches=[],native_expected_data_used=False,scope=__doc__,
        full_iterative_solver_executed=False,live_gameplay_proof=False,
        retained_write_scope='All ordered writes and full stored bytes outside ABI stack; no persistent write excluded')
    p=a.output/'fixture.json';p.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
    report=dict(cases=len(cases),ordered_persistent_writes=sum(len(c['writes']) for c in cases),
        full_state_bytes=SIZE*len(cases),reciprocal_estimates=sum(len(c['estimates']) for c in cases),
        fixture_sha256=sha(p))
    (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
