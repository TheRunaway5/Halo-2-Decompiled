#!/usr/bin/env python3
"""Original322690 with real virtual Jacobian producers, solver and readback.

Original constructors, preparation, actual producer CFGs, initialization, solver,
known schema callbacks, readback and release execute. Raw component contacts are
explicit diagnostic inputs; no Jacobian/schema or solved result is supplied.
This is not evidence of a live game world or Xbox CPU estimate parity.
"""
import argparse, hashlib, json, random, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ
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
    correspondence=ROOT/'analysis/multiplayer/sandbox-source/physics-constraint-runtime-20261011/correspondence-verified/complete-report.json'
    assert sha(correspondence)=='27291554f1e41b5b58aa4948274a28848943261aff95c5b57998373173d9283b'
    translated_entries=(0x322ed0,0x322d60,0x322ad0,0x322ab0,0x322ac0,0x31bbe0,0x31bd00,
        0x322380,0x325bb0,0x324d00,0x325b70,0x326820,0x31b810,0x31b880,0x31b9d0,0x1d2d70)
    translated_sites=set()
    for owner in translated_entries:
        listing=correspondence.parent/'instructions'/f'{owner:08x}-disc.asm'
        translated_sites.update(int(line.split()[0],16) for line in listing.read_text().splitlines())
    image=Xbe(path);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(BASE,SIZE);g.mem_map(0x2000000,0x10000);g.mem_map(STOP,0x1000)
    w=lambda p,x,n=4:g.mem_write(p,(int(x)&((1<<(8*n))-1)).to_bytes(n,'little'))
    f=lambda p,x:g.mem_write(p,struct.pack('<f',x))
    vec=lambda p,x:g.mem_write(p,struct.pack('<4f',*x))
    r=lambda p:int.from_bytes(g.mem_read(p,4),'little')
    active=False;writes=[];boundary=None;previous=None;stops={};cases=[];estimates=[];pending=None;ranges=[];trace=[]
    def call(entry,receiver,args):
        w(SP,STOP)
        for i,x in enumerate(args):w(SP+4+i*4,x)
        for register,value in [(R.UC_X86_REG_ESP,SP),(R.UC_X86_REG_ECX,receiver),
            (R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_MXCSR,0x1f80)]:g.reg_write(register,value)
        g.emu_start(entry,STOP,count=1000000)
        assert boundary or g.reg_read(R.UC_X86_REG_EIP)==STOP,(hex(g.reg_read(R.UC_X86_REG_EIP)),hex(g.reg_read(R.UC_X86_REG_EBX)),hex(ISLAND))
    def on_write(_g,_access,p,n,x,_data):
        if not active or 0x2000000<=p<0x2010000:return
        extra(p,n)
        writes.append(dict(instruction=g.reg_read(R.UC_X86_REG_EIP),address=p,
            before=bytes(g.mem_read(p,n)).hex(),written=(x&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
    def extra(p,n):
        if not(BASE<=p and p+n<=BASE+SIZE) and not(0x2000000<=p and p+n<=0x2010000):
            assert 0<=p and p+n<=0x810000,(hex(p),n)
            ranges.append((p,p+n))
    def on_read(_g,_access,p,n,_x,_data):
        if active:extra(p,n)
    def on_code(_g,p,_size,_data):
        nonlocal boundary,previous,pending
        if active:
            if pending:
                register,input_word=pending
                estimates.append([input_word,g.reg_read(register)&0xffffffff]);pending=None
            trace.append(p)
            if p in (0x1dc782,0x32601a,0x3261f7):
                pending=(R.UC_X86_REG_XMM4 if p==0x3261f7 else R.UC_X86_REG_XMM1,g.reg_read(R.UC_X86_REG_XMM0)&0xffffffff)
        if active and p in stops:
            n,receiver=stops[p];esp=g.reg_read(R.UC_X86_REG_ESP)
            boundary=dict(instruction=previous,entry=p,entry_esp=esp,
                ecx=g.reg_read(R.UC_X86_REG_ECX) if receiver else None,
                arguments=[r(esp+4+i*4) for i in range(n)])
            g.emu_stop()
        previous=p
    g.hook_add(UC_HOOK_MEM_READ,on_read);g.hook_add(UC_HOOK_MEM_WRITE,on_write);g.hook_add(UC_HOOK_CODE,on_code)
    for branch in ('fits','whole','split','tail'):
        for producer,vtable in ((0x322380,0x41ba08),(0x31bd00,0x41b34c),(0x31bbe0,0x41b320)):
            for point_count in (1,2,4):
              for raw_variant in range(4):
                  body_count=2;profile_enabled=raw_variant%2==1
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
                  w(ISLAND+8,0x200);w(ISLAND+12,0x1000);w(ISLAND+0x10,0x1000)
                  w(ISLAND+0x14,body_count*0x80);w(ISLAND+0x18,40)
                  w(ARENA+8,ARENA_BYTES)
                  first_offset=body_count*0x80+0x90
                  extent={'fits':0x4000,'whole':first_offset-16,'split':first_offset+0x80,'tail':first_offset+0x1100}[branch]
                  w(ARENA+12,extent)
                  w(ALLOCATOR,BASE+0x3a00);w(BASE+0x3a24,STOP+0x100);w(BASE+0x3a28,STOP+0x120)
                  w(ALLOCATOR+8,POOL);w(ALLOCATOR+12,0x6000)
                  w(ALLOCATOR+0x14,POOL+16) # Exercise actual local scratch-release stores.
                  w(CONSTRAINTS,CONSTRAINT);g.mem_write(CONSTRAINT,bytes(0x100));w(CONSTRAINT,vtable)
                  w(CONSTRAINT+12,BASE+0x1000);w(CONSTRAINT+0x10,BASE+0x1100)
                  points,records,props,value=BASE+0x2200,BASE+0x2400,BASE+0x2600,BASE+0x2700
                  g.mem_write(points,bytes(0x100));g.mem_write(records,bytes(0x100))
                  w(CONSTRAINT+0x34,points);w(CONSTRAINT+0x38,point_count);w(CONSTRAINT+0x90,records)
                  f(CONSTRAINT+0x40,.75);f(CONSTRAINT+0x44,.625)
                  w(CONSTRAINT+0x50,4 if raw_variant&2 else 0)
                  for off,x in ((8,.1),(0x10,-.15),(0x18,.125),(0x1c,.5)):f(CONSTRAINT+0x50+off,x)
                  vec(CONSTRAINT+0x70,(1,0,0,0))
                  for off,x in ((0xc,.5),(0x10,.2),(0x18,.25),(0x1c,.25)):f(CONTEXT+off,x)
                  for i in range(point_count):
                      vec(points+i*0x20,(1+i*.25,-.5+i*.125,2-i*.25,-.125))
                      vec(points+i*0x20+0x10,(0,0,1,0) if raw_variant<2 else ((.6,0,.8,0) if i%2 else (0,0,1,0)))
                      record=records+i*0x1c
                      f(record,0 if raw_variant==0 else .5);w(record+4,raw_variant%2)
                      f(record+8,.25);f(record+0x10,.125);f(record+0x14,-.0625)
                  w(BASE+0x1000+0x30,props);w(BASE+0x1000+0x34,1 if raw_variant==3 else 0)
                  w(props,0x2003);w(props+4,value);vec(value,(.125,-.25,.5,0))
                  g.mem_write(SOLVER,bytes(0x40))
                  f(SOLVER+4,.25);f(SOLVER+8,1);f(SOLVER+16,.5);w(SOLVER+0x1c,2);f(SOLVER+0x20,.5)
                  if profile_enabled:w(0x487674,BASE+0xd000);w(0x487678,BASE+0xd100)
                  stops={STOP+0x100:(1,True),STOP+0x140:(2,True),STOP+0x160:(3,True)}
                  args=[STEP,SOLVER,CONTEXT,GRAVITY,ISLAND,BODIES,body_count,CONSTRAINTS,1]
                  before_image=bytes(g.mem_read(0,0x810000));before_base=bytes(g.mem_read(BASE,SIZE))
                  ranges=[];writes=[];boundary=None;estimates=[];pending=None;trace=[];active=True
                  call(0x322690,0,args);active=False
                  assert boundary is None
                  merged=[]
                  for p,q in sorted(ranges):
                      if merged and p<=merged[-1][1]:merged[-1][1]=max(q,merged[-1][1])
                      else:merged.append([p,q])
                  before=[[BASE,before_base.hex()]]+[[p,before_image[p:q].hex()] for p,q in merged]
                  cases.append(dict(name=f'{branch}_{producer:08x}_{point_count}_{raw_variant}',
                      image='1.1',entry=0x322690,receiver=0,args=args,entry_esp=SP,
                      iterations=1,fp=[0x27f,0x1f80],banks=before,
                      after_banks=[[p,bytes(g.mem_read(p,len(data)//2)).hex()] for p,data in before],
                      writes=writes,trace=trace,constraint_runtime_trace=[p for p in trace if p in translated_sites],
                      estimates=estimates,boundary=boundary,
                      connect_constrained_islands=True,connect_full_solver=True,connect_jacobians=True,
                      declared_input_controls=dict(producer=producer,point_count=point_count,raw_variant=raw_variant,
                          existing_schema_supplied=False,solver_result_supplied=False)))
                  print(cases[-1]['name'],len(trace),len(writes),len(estimates),flush=True)
    result=dict(schema=1,fixture_kind='constraint_produced',original_sha256=PIN,tool_sha256=sha(__file__),
        base=BASE,size=SIZE,cases=cases,source_instruction_patches=[],supplied_solver_results=[],
        native_expected_data_used=False,live_gameplay_proof=False,scope=__doc__,
        correspondence_sha256=sha(correspondence),translated_entries=translated_entries)
    p=a.output/'fixture.json';p.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
    report=dict(cases=len(cases),ordered_persistent_writes=sum(len(c['writes']) for c in cases),
        full_state_bytes=sum(sum(len(data)//2 for _,data in c['after_banks']) for c in cases),
        required_stops=sum(c['boundary'] is not None for c in cases),fixture_sha256=sha(p))
    (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
