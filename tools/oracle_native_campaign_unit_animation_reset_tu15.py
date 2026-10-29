#!/usr/bin/env python3
"""Unchanged116920 two-overlay reset; physical full banks and lazy fault prefixes."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
BASE,SIZE,STOP,STACK=0x2000000,0x12000,0x3000000,0x203f000
OBJECTS,HEADERS,BODY=BASE+0x100,BASE+0x2000,BASE+0x4000
REGIONS=[[BASE,SIZE],[0x4ecbd0,4],[0x46a0d0,4]]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();owned=Path(os.environ['HALO2_TEST_SCRATCH'])/'original-animation-reset';owned.mkdir();image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text');body=_trace(code,0x116920,{0x116920});section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));observer=owned/'observer';subprocess.run(['c++','-std=c++20','-O2',str(Path(__file__).with_name('observe_native_campaign_ai_orders_tu15.cpp')),'-o',str(observer)],check=True,capture_output=True);visited=set();canonical_one=int.from_bytes(image.read(0x46a0d0,4),'little');specs=[]
 for seed in [0,1,7]:
  for offset in [0x400,0,-0x100,0x1000,0x264]:
   for index in [0,2,31]:
    for one in [canonical_one,0x7fa12345,0x80000000]:
     specs.append(dict(name=f'reset-seed-{seed}-offset-{offset}-index-{index}-one-{one:x}',unit=0xea000000+index,initial_bank_seed=seed,canonical_one=one==canonical_one,writes=[[0x4ecbd0,4,OBJECTS],[OBJECTS+0x44,4,HEADERS],[HEADERS+index*12+8,4,BODY],[BODY+0x33e,2,offset&0xffff],[0x46a0d0,4,one]],initial_mxcsr=0x1f80))
 def physical(g,pc,occurrence,spec):
  asm=[f'.section .original,"ax"\n.incbin "{text}"'];sections=[f'.original 0x{section.va:x} : {{ *(.original) }}']
  for i,s in enumerate(s for s in image.sections if s.name!='.text'):
   b=owned/f'source{i}.bin';b.write_bytes(bytes(g.mem_read(s.va,s.vsize)));asm.append(f'.section .source{i},"aw"\n.incbin "{b}"');sections.append(f'.source{i} 0x{s.va:x} : {{ *(.source{i}) }}')
  b=owned/'input.bin';b.write_bytes(bytes(g.mem_read(BASE,0x40000)));asm.append(f'.section .input,"aw"\n.incbin "{b}"');sections.append(f'.input 0x{BASE:x} : {{ *(.input) }}')
  asm.append(f'.code32\n.section .bridge,"ax"\n.globl _start\n_start:\n fninit\n ldmxcsr control\n mov ${STACK},%esp\n mov ${spec["unit"]},%eax\n int3\n jmp *entry\n.balign 4\nentry: .long 0x116920\ncontrol: .long 0x1f80\n');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(asm));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n');subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True);subprocess.run([str(observer),str(owned/'reference'),hex(pc),str(occurrence),hex(BASE),str(SIZE),str(owned/'result.bin')],check=True,capture_output=True);raw=(owned/'result.bin').read_bytes();fp,steps=struct.unpack_from('<2I',raw);return fp,steps,raw[8:]
 def execute(spec,fault_at=None):
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
  for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
  g.mem_map(BASE,0x40000);g.mem_map(STOP,4096);g.mem_write(BASE,bytes((i*73+spec['initial_bank_seed']*29)&255 for i in range(SIZE)))
  for p,n,v in spec['writes']:g.mem_write(p,v.to_bytes(n,'little'))
  g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(R.UC_X86_REG_ESP,STACK);g.reg_write(R.UC_X86_REG_EAX,spec['unit']);g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_MXCSR,spec['initial_mxcsr']);initial=[(s.va,bytes(g.mem_read(s.va,s.vsize))) for s in image.sections if s.name!='.text']+[(BASE,bytes(g.mem_read(BASE,0x40000)))];before=bytes(g.mem_read(BASE,SIZE));events=[];sites=[];occurrences=[];visits={}
  def instruction(_g,pc,_n,_u):visits[pc]=visits.get(pc,0)+1;visited.add(pc)
  def access(_g,kind,p,n,value,_u):
   if not any(at<=p and p+n<=at+size for at,size in REGIONS):return
   raw=bytes(g.mem_read(p,n)).hex();events.append(['r',p,n,raw] if kind==UC_MEM_READ else ['w',p,n,raw,(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()]);pc=g.reg_read(R.UC_X86_REG_EIP);sites.append(pc);occurrences.append(visits[pc])
   if fault_at is not None and len(events)-1==fault_at:g.emu_stop()
  g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,access);g.hook_add(UC_HOOK_MEM_WRITE,access);g.emu_start(0x116920,STOP,count=10000)
  if fault_at is None:assert g.reg_read(R.UC_X86_REG_EIP)==STOP
  expected=bytearray(before)
  for e in events if fault_at is None else events[:-1]:
   if e[0]=='w':assert BASE<=e[1]<BASE+SIZE;expected[e[1]-BASE:e[1]-BASE+e[2]]=bytes.fromhex(e[4])
  for p,data in initial:g.mem_write(p,data)
  fp,steps,bank=physical(g,sites[-1] if fault_at is not None else STOP,occurrences[-1] if fault_at is not None else 1,spec);assert bank==expected,(spec['name'],fault_at,'physical bank differs');return dict(events=events,fault_at=fault_at,mxcsr=fp,physical_steps=steps,physical_after_sha256=hashlib.sha256(bank).hexdigest())
 scenarios=[execute(s) for s in specs];faults=[]
 names=[f'reset-seed-7-offset-{offset}-index-2-one-{one:x}' for offset,one in [(0x400,canonical_one),(0x264,canonical_one),(-0x100,0x7fa12345)]]
 for name in names:
  index=next(i for i,s in enumerate(specs) if s['name']==name)
  for i in range(len(scenarios[index]['events'])):
   result=execute(specs[index],i);assert result['events']==scenarios[index]['events'][:i+1];del result['events'];faults.append(dict(scenario=index,**result))
 fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],required_backend='physical-i386-complete-bank-fp+unicorn-ordered-source-events',specs=specs,scenarios=scenarios,faults=faults,regions=REGIONS,source_body_sha256=hashlib.sha256(b''.join(bytes(code.at(pc).bytes) for pc in sorted(body.code))).hexdigest());encoded=json.dumps(fixture,separators=(',',':'),sort_keys=True)+'\n'
 if a.verify:assert a.fixture.read_text()==encoded,'Original animation reset fixture differs'
 else:assert not a.fixture.exists();a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(encoded)
 report=dict(passed=True,whole_calls=len(specs),canonical_one_calls=sum(s['canonical_one'] for s in specs),bit_transport_stress_calls=sum(not s['canonical_one'] for s in specs),fault_prefixes=len(faults),fixture_bytes=len(encoded.encode()),fixture_sha256=hashlib.sha256(encoded.encode()).hexdigest(),original_sites_executed=len(visited&set(body.code)),original_sites_total=len(body.code),full_unit_action_begin_closed=False);a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
