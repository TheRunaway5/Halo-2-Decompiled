#!/usr/bin/env python3
"""Unchanged TU1.5 action marking, required interrupts and local notification."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
BASE,SIZE,STOP,STACK=0x2000000,0x12000,0x3000000,0x203f000
OBJECTS,HEADERS,BODIES,PLAYERS,PLAYER_ROWS,CLOCK,HISTORY=BASE+0x100,BASE+0x2000,BASE+0x3000,BASE+0x200,BASE+0x7000,BASE+0x300,BASE+0x9000
ENTRIES=[0xb9390,0xe8f10,0x8fdb0,0x1efa70,0x1eec30]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();owned=Path(os.environ['HALO2_TEST_SCRATCH'])/'original-unit-action-support';owned.mkdir();image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text');bodies={e:_trace(code,e,set(ENTRIES)) for e in ENTRIES};section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));observer=owned/'observer';subprocess.run(['c++','-std=c++20','-O2',str(Path(__file__).with_name('observe_native_campaign_ai_orders_tu15.cpp')),'-o',str(observer)],check=True,capture_output=True);visited=set()
 interrupt_types=[0x1c,0x1d,0x1f,0x20,0];descriptor_inputs=[]
 for kind in interrupt_types:
  p=0x473d28+kind*4;descriptor=int.from_bytes(image.read(p,4),'little');descriptor_inputs += [[p,4,descriptor],[descriptor+12,4,int.from_bytes(image.read(descriptor+12,4),'little')]]
 regions=[[BASE,SIZE],[STACK+4,8],[0x4ecbd0,4],[0x4f54f4,4],[0x52be64,4],[0x51df54,4]]+[[p,n] for p,n,_v in descriptor_inputs]
 def declaration(name,entry):
  writes=[[p,n,v] for p,n,v in descriptor_inputs];writes += [[0x4ecbd0,4,OBJECTS],[OBJECTS+0x44,4,HEADERS]]
  return dict(name=name,entry=entry,eax=0xe1000000,ecx=0xe1000000,edi=0x1d,writes=writes,initial_mxcsr=0x1f80)
 specs=[]
 for flags in [[0],[2],[0xff],[0,0],[0,2,0x80],[4,0xff,0x40]]:
  s=declaration(f'mark-parents-{flags}',0xb9390)
  for i,flag in enumerate(flags):s['writes'] += [[HEADERS+i*12+2,1,flag],[HEADERS+i*12+8,4,BODIES+i*0x1000],[BODIES+i*0x1000+0x14,4,0xe1000000+i+1 if i+1<len(flags) else 0xffffffff]]
  specs.append(s)
 for flag in [0,2]:
  s=declaration(f'mark-NONE-initial-read-{flag}',0xb9390);s['eax']=0xffffffff;s['writes'] += [[OBJECTS+0x44,4,HEADERS-0xffff*12],[HEADERS+2,1,flag]];specs.append(s)
 for kind in interrupt_types:
  for offset in [0x400,-0x100]:
   for bits in [0,0xffffffff,0x80000000,0x12345678]:
    s=declaration(f'interrupt-{kind:x}-offset-{offset}-bits-{bits:x}',0xe8f10);s['edi']=kind;s['writes'] += [[HEADERS+8,4,BODIES],[BODIES+0x346,2,offset&0xffff],[BODIES+offset+4+(kind>>5)*4,4,bits]];specs.append(s)
 timestamps=[[0,0,0,0],[4,3,2,1],[0xffffffff,0x80000000,0x7fffffff,0],[1,0,0,1]]
 for selected in range(4):timestamps.append([0 if i==selected else 100 for i in range(4)])
 for local in [-1,-2,0,2,32767]:
  for times in timestamps:
   for result in [0,1,0xfe]:
    s=declaration(f'notify-local-{local}-times-{times}-result-{result}',0x1efa70);s['eax']=0xe2000002;s['writes'] += [[0x4f54f4,4,PLAYERS],[PLAYERS+0x44,4,PLAYER_ROWS],[PLAYER_ROWS+2*0x21c+0x28,2,local&0xffff],[0x52be64,4,(HISTORY-local*0x1b0-0x150)&0xffffffff],[0x51df54,4,CLOCK],[CLOCK+8,4,0x80000001],[STACK+4,4,0xabcd001d],[STACK+8,4,0x76543200|result]]
    for i,tick in enumerate(times):s['writes'] += [[HISTORY+i*8,4,tick],[HISTORY+i*8+4,4,0x12345678]]
    specs.append(s)
 for times in timestamps:
  s=declaration(f'record-direct-{times}',0x1eec30);s['eax']=HISTORY;s['writes'] += [[0x51df54,4,CLOCK],[CLOCK+8,4,0xffffffff],[STACK+4,4,0x76543210],[STACK+8,4,0x123456ff]]
  for i,tick in enumerate(times):s['writes'] += [[HISTORY+i*8,4,tick],[HISTORY+i*8+4,4,0xabcdef01]]
  specs.append(s)
 def physical(g,pc,occurrence,spec):
  asm=[f'.section .original,"ax"\n.incbin "{text}"'];sections=[f'.original 0x{section.va:x} : {{ *(.original) }}']
  for i,s in enumerate(s for s in image.sections if s.name!='.text'):
   b=owned/f'source{i}.bin';b.write_bytes(bytes(g.mem_read(s.va,s.vsize)));asm.append(f'.section .source{i},"aw"\n.incbin "{b}"');sections.append(f'.source{i} 0x{s.va:x} : {{ *(.source{i}) }}')
  b=owned/'input.bin';b.write_bytes(bytes(g.mem_read(BASE,0x40000)));asm.append(f'.section .input,"aw"\n.incbin "{b}"');sections.append(f'.input 0x{BASE:x} : {{ *(.input) }}')
  asm.append(f'.code32\n.section .bridge,"ax"\n.globl _start\n_start:\n fninit\n ldmxcsr control\n mov ${STACK},%esp\n mov ${spec["eax"]},%eax\n mov ${spec["ecx"]},%ecx\n mov ${spec["edi"]},%edi\n int3\n jmp *entry\n.balign 4\nentry: .long {spec["entry"]}\ncontrol: .long 0x1f80\n');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(asm));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n');subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True);subprocess.run([str(observer),str(owned/'reference'),hex(pc),str(occurrence),hex(BASE),str(SIZE),str(owned/'result.bin')],check=True,capture_output=True);raw=(owned/'result.bin').read_bytes();fp,steps=struct.unpack_from('<2I',raw);return fp,steps,raw[8:]
 def execute(spec,fault_at=None):
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
  for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
  g.mem_map(BASE,0x40000);g.mem_map(STOP,4096)
  for p,n,v in spec['writes']:g.mem_write(p,v.to_bytes(n,'little'))
  g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(R.UC_X86_REG_ESP,STACK);g.reg_write(R.UC_X86_REG_EAX,spec['eax']);g.reg_write(R.UC_X86_REG_ECX,spec['ecx']);g.reg_write(R.UC_X86_REG_EDI,spec['edi']);g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_MXCSR,spec['initial_mxcsr']);initial=[(s.va,bytes(g.mem_read(s.va,s.vsize))) for s in image.sections if s.name!='.text']+[(BASE,bytes(g.mem_read(BASE,0x40000)))];before=bytes(g.mem_read(BASE,SIZE));events=[];sites=[];occurrences=[];visits={};calls=[];frontier=[]
  def instruction(_g,pc,_n,_u):
   visits[pc]=visits.get(pc,0)+1;visited.add(pc)
   if pc==0xe8f41 and g.reg_read(R.UC_X86_REG_EAX)!=0x8fdb0:frontier.append(dict(callback=g.reg_read(R.UC_X86_REG_EAX),unit=g.reg_read(R.UC_X86_REG_ECX),type=g.reg_read(R.UC_X86_REG_EDI)));g.emu_stop()
   if pc==0x8fdb0:calls.append(dict(callback=pc,unit=g.reg_read(R.UC_X86_REG_ECX),type=g.reg_read(R.UC_X86_REG_EDI),event_index=len(events)))
  def access(_g,kind,p,n,value,_u):
   if not any(at<=p and p+n<=at+size for at,size in regions):return
   raw=bytes(g.mem_read(p,n)).hex();events.append(['r',p,n,raw] if kind==UC_MEM_READ else ['w',p,n,raw,(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()]);pc=g.reg_read(R.UC_X86_REG_EIP);sites.append(pc);occurrences.append(visits[pc])
   if fault_at is not None and len(events)-1==fault_at:g.emu_stop()
  g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,access);g.hook_add(UC_HOOK_MEM_WRITE,access);g.emu_start(spec['entry'],STOP,count=10000)
  if fault_at is None and not frontier:assert g.reg_read(R.UC_X86_REG_EIP)==STOP
  expected=bytearray(before)
  for e in events if fault_at is None else events[:-1]:
   if e[0]=='w':assert BASE<=e[1]<BASE+SIZE;expected[e[1]-BASE:e[1]-BASE+e[2]]=bytes.fromhex(e[4])
  for p,data in initial:g.mem_write(p,data)
  pc=sites[-1] if fault_at is not None else (0xe8f41 if frontier else STOP);occurrence=occurrences[-1] if fault_at is not None else 1;fp,steps,bank=physical(g,pc,occurrence,spec);assert bank==expected,(spec['name'],fault_at,'physical bank differs');return dict(events=events,fault_at=fault_at,mxcsr=fp,calls=calls,frontier=frontier,physical_steps=steps,physical_after_sha256=hashlib.sha256(bank).hexdigest())
 scenarios=[execute(s) for s in specs];faults=[]
 names=['mark-parents-[0, 2, 128]','interrupt-20-offset-1024-bits-ffffffff','notify-local-2-times-[4, 3, 2, 1]-result-254','notify-local--1-times-[0, 0, 0, 0]-result-0']
 for name in names:
  index=next(i for i,s in enumerate(specs) if s['name']==name)
  for i in range(len(scenarios[index]['events'])):
   result=execute(specs[index],i);assert result['events']==scenarios[index]['events'][:i+1];del result['events'];faults.append(dict(scenario=index,**result))
 fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],required_backend='physical-i386-complete-bank-fp+unicorn-ordered-source-events',specs=specs,scenarios=scenarios,faults=faults,regions=regions,source_bodies={f'{e:08x}':hashlib.sha256(b''.join(bytes(code.at(pc).bytes) for pc in sorted(b.code))).hexdigest() for e,b in bodies.items()});encoded=json.dumps(fixture,separators=(',',':'),sort_keys=True)+'\n'
 if a.verify:assert a.fixture.read_text()==encoded,'Original unit action support fixture differs'
 else:assert not a.fixture.exists();a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(encoded)
 report=dict(passed=True,whole_calls=sum(not r['frontier'] for r in scenarios),unsupported_interrupt_prefixes=sum(bool(r['frontier']) for r in scenarios),actual_empty_interrupt_calls=sum(len(r['calls']) for r in scenarios),fault_prefixes=len(faults),fixture_bytes=len(encoded.encode()),fixture_sha256=hashlib.sha256(encoded.encode()).hexdigest(),original_sites_executed=len(visited&set().union(*(set(b.code) for b in bodies.values()))),original_sites_total=sum(len(b.code) for b in bodies.values()),full_e8e10_e8e50_closed=False);a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
