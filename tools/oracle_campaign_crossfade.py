#!/usr/bin/env python3
"""Capture unchanged TU1.5 crossfade timers, capture scheduling and PS programs.

Screen command bodies start after HS argument evaluation. Full1FD00 updates,
13B70 capture scheduling,22050 program construction and its CRT power execute.
GPU target availability/allocation and SDK submissions are explicit recorded
boundaries.13AA50's ordinary tick/can-update callees supply controlled completion
counts. This establishes CPU state/order, not Xbox GPU/pixel parity.
"""
import argparse,hashlib,json,random,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args();x=Xbe(a.xbe);pin=hashlib.sha256(x.data).hexdigest()
if pin!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':raise ValueError('Expected original Xbox TU1.5 English executable')
if a.output.exists():raise ValueError('Refusing to overwrite evidence')
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
u.mem_map(0x2100000,0x100000);u.mem_map(0x3000000,4096)
def pack(v):return struct.pack('<'+'I'*len(v),*v)
def write(p,*v):u.mem_write(p,pack(v))
def words(p,n):return list(struct.unpack('<'+'I'*n,u.mem_read(p,4*n)))
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def now(v):u.mem_write(0x4920c0,struct.pack('<d',v))
def ret(pop=0,eax=None):
    sp=u.reg_read(UC_X86_REG_ESP);r=words(sp,1)[0]
    u.reg_write(UC_X86_REG_ESP,sp+4+pop);u.reg_write(UC_X86_REG_EIP,r)
    if eax is not None:u.reg_write(UC_X86_REG_EAX,eax)
events=[];completed=0;allowed=0;mode=''
stubs={0x14f40:0,0x1ccd0:0,0x34a80:44,0x1dc20:8,0x1de00:0,0x35190:8,
       0x12e9a0:0,0x13aac0:0}
def hook(g,pc,size,_):
    global completed
    if bytes(g.mem_read(pc,size))!=x.read(pc,size):raise ValueError('Original instruction changed')
    if pc not in stubs:return
    sp=g.reg_read(UC_X86_REG_ESP);value=None
    if pc==0x14f40:events.append(dict(kind='texture',target=g.reg_read(UC_X86_REG_EDI),stage=g.reg_read(UC_X86_REG_EBX)))
    elif pc==0x1ccd0:events.append(dict(kind='program',words=words(0x491788,60),texture_state=words(0x40eeb0,14)))
    elif pc==0x34a80:events.append(dict(kind='draw',args=words(sp+4,11)))
    elif pc==0x1dc20:
        events.append(dict(kind='allocate',target=g.reg_read(UC_X86_REG_EBX),dimensions=words(sp+4,2)))
        write(0x4c1eb0,0x2105000);value=1
    elif pc==0x1de00:events.append(dict(kind='available',target=g.reg_read(UC_X86_REG_EAX)));value=1
    elif pc==0x35190:events.append(dict(kind='capture',args=words(sp+4,2)))
    elif pc==0x12e9a0:value=int(completed>=allowed)
    elif pc==0x13aac0:completed+=1
    ret(stubs[pc],value)
u.hook_add(UC_HOOK_CODE,hook)
def run(entry,argv=(),stop=0x3000000,eax=0,ebx=0):
    write(0x21fff00,stop,*argv)
    for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):u.reg_write(reg,0)
    u.reg_write(UC_X86_REG_EAX,eax);u.reg_write(UC_X86_REG_EBX,ebx);u.reg_write(UC_X86_REG_ESP,0x21fff00)
    u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.reg_write(UC_X86_REG_FPCW,0x027f)
    u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
    u.emu_start(entry,stop,count=1000000)
    if u.reg_read(UC_X86_REG_EIP)!=stop:raise ValueError('Original crossfade path did not return')
write(0x515ec0,0x2100000);write(0x4920d8,0);write(0x4c686c,1)
def state():
    return dict(active=u.mem_read(0x21000ac,1)[0],initialized=u.mem_read(0x21000ad,1)[0],
        capture=u.mem_read(0x21000a0,1)[0],output=words(0x21000a4,2),
        pending=u.mem_read(0x2100170,1)[0],control=words(0x2100174,3))
rows=[]
def action(op,time=0,value=0):
    now(time);events.clear()
    if op=='start':write(0x2104000,int(value));run(0x2b4f19,stop=0x2b4f4d,eax=0x2104000)
    elif op=='set':write(0x2104000,bits(value));run(0x2b4fe9,stop=0x2b5029,eax=0x2104000)
    elif op=='stop':run(0x1fcb0)
    elif op=='update':run(0x1fd00)
    elif op=='frame':run(0x13b70)
    else:raise ValueError(op)
    rows.append(dict(op=op,time=time,value=bits(value),state=state(),events=events.copy()))
for epoch in (0.,.1,4096.0003,16777216.25):
    for duration in (-1.,0.,1/30,.5,1.,3.):
        action('stop');action('set',epoch,duration);action('update',epoch)
        action('start',epoch,False)
        for delta in (0,1/120,1/30,.25,.5,1,3,5):action('frame',epoch+delta)
        action('set',epoch+6,1);action('update',epoch+6.125)
        action('start',epoch+6.125,True);action('frame',epoch+6.25);action('stop')
# Exact render-target metadata initializer, with explicit physical backing.
u.mem_write(0x4c1378+0x12*0x98,bytes(0x98))
run(0x1df00,[3,640,480,0,1,1,0x2106000],eax=0x12)
target=words(0x4c1378+0x12*0x98,38)
programs=[];rng=random.Random(0x22050)
pairs=[(alpha,strength) for alpha in (-1.,0.,1/65536,.1,.5,1.,2.) for strength in (-1.,0.,.5,1.,2.,10.)]
pairs += [(rng.uniform(0,1.25),rng.uniform(-.25,5)) for _ in range(256)]
for alpha,strength in pairs:
    events.clear();run(0x22050,[bits(alpha),bits(strength)])
    programs.append(dict(alpha=bits(alpha),strength=bits(strength),events=events.copy()))
#13AA50's elapsed correction with caller-owned completion gates.
write(0x51df54,0x2107000);write(0x2107004,bits(1/30))
elapsed=[]
for requested in (0,1,2,5,150):
    for allowed in sorted({0,requested//2,requested}):
        for seconds in (0,1/120,.05,1/30*requested,5.):
            completed=0;write(0x2108000,bits(seconds));run(0x13aa50,eax=requested,ebx=0x2108000)
            elapsed.append(dict(seconds=bits(seconds),requested=requested,completed=completed,result=words(0x2108000,1)[0]))
report=dict(xbe_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),boundary=__doc__,
    stubs=[f'{p:08x}' for p in stubs],lifecycle=rows,programs=programs,elapsed=elapsed,target=target,
    source_render_clock_chain=['00149160','0013aa50','0013a9b0','0002b5b0','00012a30'],framebuffer_parity_verified=False)
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
print(len(rows),'original timer/lifecycle actions;',len(programs),'composite programs;',len(elapsed),'elapsed correction cases')
