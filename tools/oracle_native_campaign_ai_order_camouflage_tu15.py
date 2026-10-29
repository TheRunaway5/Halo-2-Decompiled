#!/usr/bin/env python3
"""Unchanged actor camo-control1EB590/D3340/B7E50 with actual replication state.

Physical i386 proves complete retained banks, boolean result and FP context;
Unicorn records ordered accesses. Original executables are offline inputs only.
"""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
BASE,SIZE,STOP,STACK=0x2000000,0x12000,0x3000000,0x203f000
RESULT=BASE+SIZE-4
ACTORS,ROWS,OBJECTS,UNITS,BODY,SESSION,STATE,ENTITIES=BASE+0x100,BASE+0x2000,BASE+0x200,BASE+0x3000,BASE+0x4000,BASE+0x5000,BASE+0x6000,BASE+0x9000
REGIONS=[[BASE,SIZE],[0x5020c8,4],[0x4ecbd0,4],[0x4dc04c,4],[0x46a130,4],[0x46a0d0,4]]
ENTRIES=[0x1eb590,0xd3340,0xb7e50]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args()
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();owned=scratch/'physical-camo-orders';owned.mkdir();image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 code=_Code(image,'.text');bodies={e:_trace(code,e,set(ENTRIES)) for e in ENTRIES};section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section))
 observer=owned/'observer';subprocess.run(['c++','-std=c++20','-O2',str(Path(__file__).with_name('observe_native_campaign_ai_orders_tu15.cpp')),'-o',str(observer)],check=True,capture_output=True);visited=set()
 def declaration(enabled,mode,unit,entity,dirty,policy=0x1f80):
  writes=[]
  def put(p,v,n=4):writes.append([p,n,v&((1<<(8*n))-1)])
  put(0x5020c8,ACTORS);put(ACTORS+0x44,ROWS);put(ROWS+0x18,0xe2000000 if unit else 0xffffffff)
  put(0x4ecbd0,OBJECTS);put(OBJECTS+0x44,UNITS);put(UNITS+8,BODY);put(BODY+0x134,0x8765433f);put(BODY+0x2b8,0x7fc12345);put(BODY+0xd4,0xe3000002 if entity else 0xffffffff)
  put(0x4dc04c,SESSION);put(SESSION+8,mode);put(SESSION+4,STATE);put(STATE+0x20a4,ENTITIES);put(ENTITIES+0x44+2*8,4 if dirty else 0,1);put(STATE+0x2098+2*36+0x20,0x12345678)
  for p in [0x46a130,0x46a0d0]:put(p,int.from_bytes(image.read(p,4),'little'))
  return dict(name=f'camo-{enabled}-mode-{mode}-unit-{unit}-entity-{entity}-dirty-{dirty}-fp-{policy:x}',enabled=enabled,actor=0xe1000000,writes=writes,initial_mxcsr=policy)
 def physical(g,pc,occurrence):
  asm=[f'.section .original,"ax"\n.incbin "{text}"'];sections=[f'.original 0x{section.va:x} : {{ *(.original) }}']
  for i,s in enumerate(s for s in image.sections if s.name!='.text'):
   b=owned/f'source{i}.bin';b.write_bytes(bytes(g.mem_read(s.va,s.vsize)));asm.append(f'.section .source{i},"aw"\n.incbin "{b}"');sections.append(f'.source{i} 0x{s.va:x} : {{ *(.source{i}) }}')
  b=owned/'input.bin';b.write_bytes(bytes(g.mem_read(BASE,0x40000)));asm.append(f'.section .input,"aw"\n.incbin "{b}"');sections.append(f'.input 0x{BASE:x} : {{ *(.input) }}')
  asm.append(f'.code32\n.section .stop,"ax"\n mov %eax,{RESULT}\n int3\n');sections.append(f'.stop 0x{STOP:x} : {{ *(.stop) }}')
  asm.append(f'.code32\n.section .bridge,"ax"\n.globl _start\n_start:\n fninit\n ldmxcsr control\n mov ${STACK},%esp\n mov $0xe1000000,%eax\n int3\n jmp *entry\n.balign 4\nentry: .long 0x1eb590\ncontrol: .long {g.reg_read(R.UC_X86_REG_MXCSR)}\n');sections.append('.bridge 0x70000000 : { *(.bridge) }')
  (owned/'reference.s').write_text('\n'.join(asm));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n');subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
  subprocess.run([str(observer),str(owned/'reference'),hex(pc),str(occurrence),hex(BASE),str(SIZE),str(owned/'result.bin')],check=True,capture_output=True);raw=(owned/'result.bin').read_bytes();fp,steps=struct.unpack_from('<2I',raw);bank=bytearray(raw[8:]);result=int.from_bytes(bank[-4:],'little')&255;bank[-4:]=bytes(4);return fp,steps,bytes(bank),result
 def execute(spec,fault_at=None):
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
  for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
  g.mem_map(BASE,0x40000);g.mem_map(STOP,4096)
  for p,n,v in spec['writes']:g.mem_write(p,v.to_bytes(n,'little'))
  g.mem_write(STACK,struct.pack('<II',STOP,int(spec['enabled'])));g.reg_write(R.UC_X86_REG_ESP,STACK);g.reg_write(R.UC_X86_REG_EAX,spec['actor']);g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_MXCSR,spec['initial_mxcsr'])
  initial=[(s.va,bytes(g.mem_read(s.va,s.vsize))) for s in image.sections if s.name!='.text']+[(BASE,bytes(g.mem_read(BASE,0x40000)))];before=bytes(g.mem_read(BASE,SIZE));events=[];sites=[];occurrences=[];visits={}
  def instruction(_g,pc,_n,_u):visits[pc]=visits.get(pc,0)+1;visited.add(pc)
  def access(_g,kind,p,n,value,_u):
   if not any(at<=p and p+n<=at+size for at,size in REGIONS):return
   raw=bytes(g.mem_read(p,n)).hex();events.append(['r',p,n,raw] if kind==UC_MEM_READ else ['w',p,n,raw,(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()]);pc=g.reg_read(R.UC_X86_REG_EIP);sites.append(pc);occurrences.append(visits[pc])
   if fault_at is not None and len(events)-1==fault_at:g.emu_stop()
  g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,access);g.hook_add(UC_HOOK_MEM_WRITE,access);g.emu_start(0x1eb590,STOP,count=10000)
  if fault_at is None:assert g.reg_read(R.UC_X86_REG_EIP)==STOP
  result=g.reg_read(R.UC_X86_REG_EAX)&255;expected=bytearray(before)
  for e in events if fault_at is None else events[:-1]:
   if e[0]=='w':expected[e[1]-BASE:e[1]-BASE+e[2]]=bytes.fromhex(e[4])
  for p,data in initial:g.mem_write(p,data)
  g.reg_write(R.UC_X86_REG_MXCSR,spec['initial_mxcsr'])
  fp,steps,bank,observed=physical(g,STOP+5 if fault_at is None else sites[-1],1 if fault_at is None else occurrences[-1])
  assert bank==expected,(spec['name'],fault_at,'physical bank differs')
  if fault_at is None:assert observed==result,(spec['name'],'physical boolean differs')
  return dict(events=events,fault_at=fault_at,mxcsr=fp,result=bool(observed) if fault_at is None else None,physical_steps=steps,physical_after_sha256=hashlib.sha256(bank).hexdigest())
 specs=[declaration(enabled,mode,unit,entity,dirty) for enabled in [False,True] for mode in [0,1,2,3,4,5,0xffffffff] for unit in [False,True] for entity in [False,True] for dirty in [False,True]]
 specs +=[declaration(enabled,4,True,True,True,policy) for enabled in [False,True] for policy in [0x9fc0,0x3fa1,0x7f80]]
 for enabled in [False,True]:
  for flags in [0,0x100,8,16,0xffffffff]:
   s=declaration(enabled,4,True,True,True);s['name']+=f'-flags-{flags:x}';s['writes'].append([BODY+0x134,4,flags]);specs.append(s)
  for entity in [0xe3000402,0x1234fc02]:
   s=declaration(enabled,4,True,True,True);s['name']+=f'-entitybits-{entity:x}';s['writes'].append([BODY+0xd4,4,entity]);specs.append(s)
 cases=[dict(**s,**execute(s)) for s in specs]
 for enabled in [False,True]:
  s=declaration(enabled,4,True,True,True)
  for i in range(len(execute(s)['events'])):cases.append(dict(**s,**execute(s,i)))
 fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],regions=REGIONS,cases=cases,required_backend='physical-i386-complete-bank-result-fp+unicorn-ordered-source-events',source_bodies={f'{e:08x}':hashlib.sha256(b''.join(bytes(code.at(pc).bytes) for pc in sorted(b.code))).hexdigest() for e,b in bodies.items()})
 encoded=json.dumps(fixture,separators=(',',':'),sort_keys=True)+'\n'
 if a.verify:assert a.fixture.read_text()==encoded,'Original camouflage ownership fixture differs'
 else:assert not a.fixture.exists();a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(encoded)
 report=dict(passed=True,whole_cases=len(specs),fault_prefixes=len(cases)-len(specs),fixture_bytes=len(encoded.encode()),fixture_sha256=hashlib.sha256(encoded.encode()).hexdigest(),original_sites_executed=len(visited&set().union(*(set(b.code) for b in bodies.values()))),original_sites_total=sum(len(b.code) for b in bodies.values()),scope='Whole1EB590,D3340,B7E50 actor camouflage and actual dirty-entity publication; not full orders/reset')
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
