#!/usr/bin/env python3
"""Whole unchanged actor/order-script assignment26C600 and real hash owners.

Physical i386 verifies complete retained banks and FP status. Unicorn supplies
ordered source read/store provenance. No code is patched, callee return is
supplied, native code is used as an expectation, or runtime binary is installed.
"""
import argparse,hashlib,json,os,random,struct,subprocess
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

BASE,SIZE,STOP,STACK=0x2000000,0x12000,0x3000000,0x203f000
ACTORS,ROWS,TABLE,NODES=BASE+0x100,BASE+0x2000,BASE+0x5000,BASE+0x8000
REGIONS=[[BASE,SIZE],[0x5020c8,4],[0x56aeb0,4],[0x47d6c0,4]]
ENTRIES=[0x26c600,0x140d50,0x140db0,0x140e00,0x267a30,0x267a40]

def inputs(seed,kind):
 rng=random.Random(seed);writes=[]
 def put(p,v):writes.append([p,v&0xffffffff])
 put(0x5020c8,ACTORS);put(0x56aeb0,TABLE);put(0x47d6c0,0xffffffff)
 put(ACTORS+0x44,ROWS);put(TABLE+0x20,16 if seed%2 else 1024);put(TABLE+0x28,4)
 put(TABLE+0x2c,0x267a30);put(TABLE+0x30,0x267a40)
 keys=[0x80000100+seed*0x101+i*0x100 for i in range(3)]
 actors=[0xe1000000+i for i in range(4)]
 count=16 if seed%2 else 1024
 slot=TABLE+0x3c+(((keys[0]&255)*4)%count)*4
 put(slot,NODES)
 for i,key in enumerate(keys):
  put(NODES+i*16,key);put(NODES+i*16+4,(key&255)*4)
  put(NODES+i*16+8,NODES+(i+1)*16 if i<2 else 0)
  put(NODES+i*16+12,actors[[1,0,2][i]])
 for i in range(3,6):put(NODES+i*16+8,NODES+(i+1)*16 if i<5 else 0)
 put(TABLE+0x38,NODES+3*16)
 for i in range(4):put(ROWS+i*0x888+0x418,keys[[1,0,2,0][i]])
 target=[0xffffffff,keys[0],keys[0]+0x300,keys[1]][kind%4]
 if seed%5==0:put(ROWS+0x418,0xffffffff)
 if seed%7==0:put(TABLE+0x38,0)
 if seed%3==0:put(NODES+12,0xffffffff)
 if seed%4==0:put(NODES+4,rng.getrandbits(32))
 if seed%6==0:put(ROWS+0x418,keys[0]+0x400) # Erase a key not in the retained chain.
 return dict(name=f'association-{seed}-{kind}',actor=actors[0],target=target,writes=writes)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args()
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();owned=scratch/'physical-orders';owned.mkdir()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 code=_Code(image,'.text');bodies={e:_trace(code,e,set(ENTRIES)) for e in ENTRIES}
 observer=owned/'observer';subprocess.run(['c++','-std=c++20','-O2',str(Path(__file__).with_name('observe_native_campaign_ai_orders_tu15.cpp')),'-o',str(observer)],check=True,capture_output=True)
 section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));visited=set()
 def hardware(g,pc,occurrence):
  asm=[f'.section .original,"ax"\n.incbin "{text}"'];sections=[f'.original 0x{section.va:x} : {{ *(.original) }}']
  for i,s in enumerate(s for s in image.sections if s.name!='.text'):
   b=owned/f'source{i}.bin';b.write_bytes(bytes(g.mem_read(s.va,s.vsize)));asm.append(f'.section .source{i},"aw"\n.incbin "{b}"');sections.append(f'.source{i} 0x{s.va:x} : {{ *(.source{i}) }}')
  b=owned/'input.bin';b.write_bytes(bytes(g.mem_read(BASE,0x40000)));asm.append(f'.section .input,"aw"\n.incbin "{b}"');sections.append(f'.input 0x{BASE:x} : {{ *(.input) }}')
  asm.append(f'.code32\n.section .bridge,"ax"\n.globl _start\n_start:\n fninit\n ldmxcsr control\n mov ${STACK},%esp\n int3\n jmp *entry\n.balign 4\nentry: .long 0x26c600\ncontrol: .long 0x1f80\n');sections.append('.bridge 0x70000000 : { *(.bridge) }')
  (owned/'reference.s').write_text('\n'.join(asm));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
  subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
  subprocess.run([str(observer),str(owned/'reference'),hex(pc),str(occurrence),hex(BASE),str(SIZE),str(owned/'result.bin')],check=True,capture_output=True)
  raw=(owned/'result.bin').read_bytes();mxcsr,steps=struct.unpack_from('<2I',raw);return mxcsr,steps,raw[8:]
 def execute(spec,fault_at=None):
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
  for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
  g.mem_map(BASE,0x40000);g.mem_map(STOP,4096)
  def put(p,v):g.mem_write(p,struct.pack('<I',v&0xffffffff))
  for p,v in spec['writes']:put(p,v)
  put(STACK,STOP);put(STACK+4,spec['actor']);put(STACK+8,spec['target']);g.reg_write(R.UC_X86_REG_ESP,STACK);g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
  initial=[(s.va,bytes(g.mem_read(s.va,s.vsize))) for s in image.sections if s.name!='.text']+[(BASE,bytes(g.mem_read(BASE,0x40000)))];initial_bank=bytes(g.mem_read(BASE,SIZE));events=[];sites=[]
  def event(_g,access,p,n,value,_u):
   if not any(at<=p and p+n<=at+size for at,size in REGIONS):return
   before=bytes(g.mem_read(p,n)).hex()
   if access==16:events.append(['r',p,n,before])
   else:events.append(['w',p,n,before,(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()])
   sites.append(g.reg_read(R.UC_X86_REG_EIP))
   if fault_at is not None and len(events)-1==fault_at:g.emu_stop()
  g.hook_add(UC_HOOK_MEM_READ,event);g.hook_add(UC_HOOK_MEM_WRITE,event);g.hook_add(UC_HOOK_CODE,lambda _g,pc,_n,_u:visited.add(pc))
  g.emu_start(0x26c600,STOP,count=10000)
  if fault_at is None:assert g.reg_read(R.UC_X86_REG_EIP)==STOP
  expected=bytearray(initial_bank)
  for e in events if fault_at is None else events[:-1]:
   if e[0]=='w':assert BASE<=e[1]<BASE+SIZE;expected[e[1]-BASE:e[1]-BASE+e[2]]=bytes.fromhex(e[4])
  if fault_at is None:assert bytes(g.mem_read(BASE,SIZE))==expected
  for p,data in initial:g.mem_write(p,data)
  pc=STOP if fault_at is None else sites[-1];occurrence=1 if fault_at is None else sites.count(pc)
  mxcsr,steps,physical=hardware(g,pc,occurrence);assert physical==expected,(spec['name'],fault_at,'physical bank differs')
  return dict(events=events,fault_at=fault_at,mxcsr=mxcsr,physical_steps=steps,physical_after_sha256=hashlib.sha256(physical).hexdigest())
 cases=[]
 for seed in range(12):
  for kind in range(4):
   spec=inputs(seed,kind);cases.append(dict(**spec,**execute(spec)))
 spec=inputs(1,1)
 for i in range(len(execute(spec)['events'])):cases.append(dict(**spec,**execute(spec,i)))
 fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],regions=REGIONS,cases=cases,source_bodies={f'{e:08x}':hashlib.sha256(b''.join(bytes(code.at(pc).bytes) for pc in sorted(b.code))).hexdigest() for e,b in bodies.items()},required_backend='physical-i386-complete-bank-fp+unicorn-ordered-source-events')
 encoded=json.dumps(fixture,separators=(',',':'),sort_keys=True)+'\n'
 if a.verify:assert a.fixture.read_text()==encoded,'Original assignment fixture differs'
 else:assert not a.fixture.exists();a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(encoded)
 report=dict(passed=True,whole_cases=48,fault_prefixes=len(cases)-48,fixture_bytes=len(encoded.encode()),fixture_sha256=hashlib.sha256(encoded.encode()).hexdigest(),original_instruction_sites_executed=len(visited&set().union(*(set(b.code) for b in bodies.values()))),original_instruction_sites_total=sum(len(b.code) for b in bodies.values()),native_runtime_executable_used=False,instruction_patches=[],supplied_dependency_returns=[],scope='Full actor/script-key reassignment dependency26C600 with actual erase/find/insert/hash/equality; not complete ai_set_orders or actor behavior reset')
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
