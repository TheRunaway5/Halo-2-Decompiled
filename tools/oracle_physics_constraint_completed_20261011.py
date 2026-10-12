#!/usr/bin/env python3
"""Completed original322690 arithmetic controls using the real full solver.

Original constructor, preparation, initialization, solver, readback and release
execute. An explicitly declared existing arena schema tests orchestration with
no Jacobian producer. This does not claim source-produced live constraints.
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
    active=False;writes=[];boundary=None;previous=None;stops={};cases=[];estimates=[];pending=None
    constants=[(0x465030,4),(0x465048,4),(0x465044,4),(0x46502c,4),(0x41b760,16),
        (0x41b8f0,36),(0x487638,4),(0x487674,8),(0x48c058,4)]
    # Retain complete actual solver-read banks from the original-only fixture.
    solver_fixture=json.loads((ROOT/'analysis/multiplayer/sandbox-source/physics-solver-20261011/disc-full-v1/fixture.json').read_text())
    ranges=[(p,p+n) for p,n in constants]
    for c in solver_fixture['cases']:
        for p,b in c['banks']:
            if p not in (BASE,0x2000000):ranges.append((p,p+len(b)//2))
    merged=[]
    for p,q in sorted(ranges):
        if merged and p<=merged[-1][1]:merged[-1][1]=max(q,merged[-1][1])
        else:merged.append([p,q])
    retained=[(BASE,SIZE)]+[(p,q-p) for p,q in merged]
    def call(entry,receiver,args):
        w(SP,STOP)
        for i,x in enumerate(args):w(SP+4+i*4,x)
        for register,value in [(R.UC_X86_REG_ESP,SP),(R.UC_X86_REG_ECX,receiver),
            (R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_MXCSR,0x1f80)]:g.reg_write(register,value)
        g.emu_start(entry,STOP,count=1000000)
        assert boundary or g.reg_read(R.UC_X86_REG_EIP)==STOP,(hex(g.reg_read(R.UC_X86_REG_EIP)),hex(g.reg_read(R.UC_X86_REG_EBX)),hex(schema))
    def on_write(_g,_access,p,n,x,_data):
        if not active or 0x2000000<=p<0x2010000:return
        assert any(start<=p and p+n<=start+size for start,size in retained),(hex(p),n)
        writes.append(dict(instruction=g.reg_read(R.UC_X86_REG_EIP),address=p,
            before=bytes(g.mem_read(p,n)).hex(),written=(x&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
    def on_code(_g,p,_size,_data):
        nonlocal boundary,previous,pending
        if active and p==0x1dc782:pending=g.reg_read(R.UC_X86_REG_XMM0)&0xffffffff
        elif active and p==0x1dc786:
            assert pending is not None
            estimates.append([pending,g.reg_read(R.UC_X86_REG_XMM1)&0xffffffff]);pending=None
        if active and p in stops:
            n,receiver=stops[p];esp=g.reg_read(R.UC_X86_REG_ESP)
            boundary=dict(instruction=previous,entry=p,entry_esp=esp,
                ecx=g.reg_read(R.UC_X86_REG_ECX) if receiver else None,
                arguments=[r(esp+4+i*4) for i in range(n)])
            g.emu_stop()
        previous=p
    g.hook_add(UC_HOOK_MEM_WRITE,on_write);g.hook_add(UC_HOOK_CODE,on_code)
    for branch in ('fits','whole','split','tail'):
        for destination in ('solver',):
            for body_count in (1,2):
              for profile_enabled in (False,True):
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
                  w(ALLOCATOR+0x14,POOL+16) # Exercise actual local scratch-release stores.
                  w(CONSTRAINTS,CONSTRAINT);w(CONSTRAINT,BASE+0x3b00);w(BASE+0x3b24,STOP+0x140)
                  w(CONSTRAINT+12,BASE+0x1000);w(CONSTRAINT+0x10,BASE+0x1000+(body_count-1)*0x100)
                  count=1 if destination=='jacobian' else 0
                  if destination=='motion':
                      w(BASE+0x1800,BASE+0x3c00);w(BASE+0x3c0c,STOP+0x160)
                  # The declaration controls an already-existing arena schema;
                  # it neither supplies a solved body nor a producer return.
                  first=ARENA_BYTES+first_offset
                  schema=first+0x100+28
                  jac=first;acc=ARENA_BYTES
                  if branch=='whole':
                      acc=POOL;jac=POOL+first_offset;schema=jac+0x100+28
                  elif branch=='split':
                      schema=POOL+(8*2-extent+0x100+first_offset)+28
                  elif branch=='tail':
                      schema=POOL+28
                  w(schema,1|(24<<8)|(1<<16));w(schema+4,jac);w(schema+8,BASE+0xc000)
                  w(schema+12,acc);w(schema+16,acc+0x80);w(schema+20,8)
                  #322937 overwrites the first dword with an end record. The
                  #solver unconditionally consumes 24 bytes as its first header;
                  #these valid pointers and the following end are explicit
                  #diagnostic controls for that zero-producer input interval.
                  w(schema+24,0x400)
                  f(SOLVER+4,.25);f(SOLVER+8,1);f(SOLVER+16,.5);w(SOLVER+0x1c,2);f(SOLVER+0x20,.5)
                  if profile_enabled:w(0x487674,BASE+0xd000);w(0x487678,BASE+0xd100)
                  stops={STOP+0x100:(1,True),STOP+0x140:(2,True),STOP+0x160:(3,True)}
                  args=[STEP,SOLVER,CONTEXT,GRAVITY,ISLAND,BODIES,body_count,CONSTRAINTS,count]
                  # Also enter through actual31CFE0 for the connected constraint branch.
                  for connected in (False,):
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
                      print(branch,body_count,profile_enabled,hex(schema),flush=True)
                      writes=[];boundary=None;estimates=[];pending=None;active=True;call(entry,receiver,call_args);active=False
                      assert boundary is None
                      cases.append(dict(name=f'{branch}_{destination}_{body_count}_{"profile" if profile_enabled else "direct"}',
                          image='1.1',entry=entry,receiver=receiver,args=call_args,entry_esp=SP,
                          iterations=1,fp=[0x27f,0x1f80],banks=before,
                          after_banks=[[p,bytes(g.mem_read(p,n)).hex()] for p,n in retained],
                          writes=writes,estimates=estimates,boundary=boundary,connect_constrained_islands=True,connect_full_solver=True,
                          declared_input_controls='Original constructor and completed solve; declared existing empty schema header; no producer or solve result supplied'))
                      # Restore complete original inputs before the alternate entry;
                      # preparation must not accumulate force/damping from the first prefix.
                      for p,data in before:g.mem_write(p,bytes.fromhex(data))
    result=dict(schema=1,fixture_kind='constraint_completed',original_sha256=PIN,tool_sha256=sha(__file__),
        base=BASE,size=SIZE,cases=cases,source_instruction_patches=[],supplied_solver_results=[],
        native_expected_data_used=False,live_gameplay_proof=False,scope=__doc__)
    p=a.output/'fixture.json';p.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
    report=dict(cases=len(cases),ordered_persistent_writes=sum(len(c['writes']) for c in cases),
        full_state_bytes=sum(sum(len(data)//2 for _,data in c['after_banks']) for c in cases),
        required_stops=sum(c['boundary'] is not None for c in cases),fixture_sha256=sha(p))
    (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
