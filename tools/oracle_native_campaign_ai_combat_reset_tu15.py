#!/usr/bin/env python3
"""Whole unchanged1EC7B0 combat reset and real query retirement, offline only."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
BASE,SIZE,STOP,STACK=0x2000000,0x12000,0x3000000,0x203f000
ACTORS,PROPERTIES,QUERIES=BASE+0x100,BASE+0x200,BASE+0x300
AR,PR,QR=BASE+0x2000,BASE+0x5000,BASE+0x6000
ENTRIES=[0x1ec7b0,0x267220,0x267290,0x266490,0x16e8a0]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();owned=Path(os.environ['HALO2_TEST_SCRATCH'])/'original-ai-combat-reset';owned.mkdir();image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text');bodies={e:_trace(code,e,set(ENTRIES)) for e in ENTRIES};section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));observer=owned/'observer';subprocess.run(['c++','-std=c++20','-O2',str(Path(__file__).with_name('observe_native_campaign_ai_orders_tu15.cpp')),'-o',str(observer)],check=True,capture_output=True);visited=set();regions=[[BASE,SIZE],[0x5020c8,4],[0x50eef0,4],[0x50eeec,4]]+[[0x47d638+i*16,2] for i in range(9)]
 def declaration(name,mode,states,poison=False,cache=0,actor_index=0,query_missing=False,kind=1):
  writes=[]
  def put(p,v,n=4):writes.append([p,n,v&((1<<(8*n))-1)])
  put(0x5020c8,ACTORS);put(ACTORS+0x44,AR);put(0x50eef0,PROPERTIES);put(PROPERTIES+0x44,PR);put(0x50eeec,QUERIES);put(QUERIES+0x44,QR)
  actor=AR+actor_index*0x888;put(actor+0x58,0xe2000000 if states else -1);put(actor+0x86,0x6543,2);put(actor+0x88,0xf1234567)
  for i in range(8):put(actor+0x5c+i*4,0x76543210+i)
  if cache<8:
   put(actor+0x5c+cache*4,0xe2000000)
   if cache==0:put(actor+0x60,0xe2000000)
  put(QUERIES+0x24,0x124);put(QUERIES+0x2a,8 if poison else 0,1);put(QUERIES+0x48,BASE+0x900);put(BASE+0x900,0xffffffff);put(QUERIES+0x34,0x100);put(QUERIES+0x38,len(states));put(QUERIES+0x3c,len(states))
  for i,state in enumerate(states):
   prop=PR+i*0x3c;put(prop+0x2c,0xe2000000+i+1 if i+1<len(states) else -1);put(prop+0x24,state,2);put(prop+0x10,0x7fc12345);put(prop+0x14,-1 if query_missing else 0xe3000000+i);put(prop+0x18,kind,2);put(QR+i*0x124,0x4321+i,2);put(QR+i*0x124+0xd9,0x7e,1)
  for i in range(9):put(0x47d638+i*16,int.from_bytes(image.read(0x47d638+i*16,2),'little'),2)
  return dict(name=name,mode=mode,actor=0xea000000+actor_index,writes=writes,initial_mxcsr=0x1f80)
 def physical(g,pc,occurrence,spec):
  asm=[f'.section .original,"ax"\n.incbin "{text}"'];sections=[f'.original 0x{section.va:x} : {{ *(.original) }}']
  for i,s in enumerate(s for s in image.sections if s.name!='.text'):
   b=owned/f'source{i}.bin';b.write_bytes(bytes(g.mem_read(s.va,s.vsize)));asm.append(f'.section .source{i},"aw"\n.incbin "{b}"');sections.append(f'.source{i} 0x{s.va:x} : {{ *(.source{i}) }}')
  b=owned/'input.bin';b.write_bytes(bytes(g.mem_read(BASE,0x40000)));asm.append(f'.section .input,"aw"\n.incbin "{b}"');sections.append(f'.input 0x{BASE:x} : {{ *(.input) }}')
  asm.append(f'.code32\n.section .bridge,"ax"\n.globl _start\n_start:\n fninit\n ldmxcsr control\n mov ${STACK},%esp\n mov ${spec["actor"]},%eax\n mov ${spec["mode"]&0xffff},%edi\n int3\n jmp *entry\n.balign 4\nentry: .long 0x1ec7b0\ncontrol: .long 0x1f80\n');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(asm));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n');subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True);subprocess.run([str(observer),str(owned/'reference'),hex(pc),str(occurrence),hex(BASE),str(SIZE),str(owned/'result.bin')],check=True,capture_output=True);raw=(owned/'result.bin').read_bytes();fp,steps=struct.unpack_from('<2I',raw);return fp,steps,raw[8:]
 def execute(spec,fault_at=None):
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
  for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
  g.mem_map(BASE,0x40000);g.mem_map(STOP,4096)
  for p,n,v in spec['writes']:g.mem_write(p,v.to_bytes(n,'little'))
  g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(R.UC_X86_REG_ESP,STACK);g.reg_write(R.UC_X86_REG_EAX,spec['actor']);g.reg_write(R.UC_X86_REG_EDI,spec['mode']&0xffff);g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_MXCSR,spec['initial_mxcsr']);initial=[(s.va,bytes(g.mem_read(s.va,s.vsize))) for s in image.sections if s.name!='.text']+[(BASE,bytes(g.mem_read(BASE,0x40000)))];before=bytes(g.mem_read(BASE,SIZE));events=[];sites=[];occurrences=[];visits={};last_pc=[None]
  def instruction(_g,pc,_n,_u):
   previous=last_pc[0];last_pc[0]=pc;visited.add(pc)
   if not(pc in [0x16e8d1,0x16e8d8] and g.reg_read(R.UC_X86_REG_ECX)==0 and previous==pc):visits[pc]=visits.get(pc,0)+1
  def access(_g,kind,p,n,value,_u):
   if not any(at<=p and p+n<=at+size for at,size in regions):return
   raw=bytes(g.mem_read(p,n)).hex();events.append(['r',p,n,raw] if kind==UC_MEM_READ else ['w',p,n,raw,(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()]);pc=g.reg_read(R.UC_X86_REG_EIP);sites.append(pc);occurrences.append(visits[pc])
   if fault_at is not None and len(events)-1==fault_at:g.emu_stop()
  g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,access);g.hook_add(UC_HOOK_MEM_WRITE,access);g.emu_start(0x1ec7b0,STOP,count=100000)
  if fault_at is None:assert g.reg_read(R.UC_X86_REG_EIP)==STOP
  expected=bytearray(before)
  for e in events if fault_at is None else events[:-1]:
   if e[0]=='w':assert BASE<=e[1]<BASE+SIZE;expected[e[1]-BASE:e[1]-BASE+e[2]]=bytes.fromhex(e[4])
  for p,data in initial:g.mem_write(p,data)
  pc=sites[-1] if fault_at is not None else STOP;occurrence=occurrences[-1] if fault_at is not None else 1;fp,steps,bank=physical(g,pc,occurrence,spec);assert bank==expected,(spec['name'],fault_at,'physical bank differs');return dict(events=events,fault_at=fault_at,mxcsr=fp,physical_steps=steps,physical_after_sha256=hashlib.sha256(bank).hexdigest())
 specs=[]
 for mode in [-32768,-1,0,1,2,3,4,32767]:
  for poison in [False,True]:
   for states in [[],[-1,0,1,2],[3,4,2,32767]]:
    specs.append(declaration(f'mode-{mode}-poison-{poison}-states-{states}',mode,states,poison))
 for mode in [0,1,2]:
  for missing in [False,True]:
   for cache in range(9):
    for kind in [0,1,8]:specs.append(declaration(f'cache-kind-mode-{mode}-missing-{missing}-cache-{cache}-kind-{kind}',mode,[3,4],cache=cache,actor_index=2,query_missing=missing,kind=kind))
 for pointer in [0,0xffffff90]:
  s=declaration(f'mark-null-pointer-{pointer:x}',2,[3]);s['writes'].append([QUERIES+0x44,4,pointer]);specs.append(s)
 scenarios=[execute(s) for s in specs];faults=[]
 names=['mode-0-poison-True-states-[3, 4, 2, 32767]','mode-2-poison-False-states-[3, 4, 2, 32767]','cache-kind-mode-0-missing-False-cache-7-kind-1']
 for name in names:
  index=next(i for i,s in enumerate(specs) if s['name']==name)
  for i in range(len(scenarios[index]['events'])):
   result=execute(specs[index],i);assert result['events']==scenarios[index]['events'][:i+1];del result['events'];faults.append(dict(scenario=index,**result))
 fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],required_backend='physical-i386-complete-bank-fp+unicorn-ordered-source-events',specs=specs,scenarios=scenarios,faults=faults,regions=regions,source_bodies={f'{e:08x}':hashlib.sha256(b''.join(bytes(code.at(pc).bytes) for pc in sorted(b.code))).hexdigest() for e,b in bodies.items()});encoded=json.dumps(fixture,separators=(',',':'),sort_keys=True)+'\n'
 if a.verify:assert a.fixture.read_text()==encoded,'Original combat reset fixture differs'
 else:assert not a.fixture.exists();a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(encoded)
 report=dict(passed=True,whole_calls=len(specs),fault_prefixes=len(faults),fixture_bytes=len(encoded.encode()),fixture_sha256=hashlib.sha256(encoded.encode()).hexdigest(),original_sites_executed=len(visited&set().union(*(set(b.code) for b in bodies.values()))),original_sites_total=sum(len(b.code) for b in bodies.values()),full_actor_order_reset=False,full_ai_set_orders=False);a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
