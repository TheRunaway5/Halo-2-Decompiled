#!/usr/bin/env python3
"""Unchanged TU1.5 clump teardown, true data-array free and behavior frontier.

Physical i386 proves complete banks and FP; Unicorn records ordered accesses.
Nonzero behavior invalidation callbacks stop before the real indirect CALL.
"""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
BASE,SIZE,STOP,STACK=0x2000000,0x20000,0x3000000,0x203f000
CLUMPS,GROUPS,ACTORS,PROPERTIES,QUERIES,ATTACHMENTS,OBJECTS=[BASE+0x100*i for i in range(1,8)]
CR,GR,AR,PR,QR,TR,OR,BODY=[BASE+x for x in [0x1000,0x2000,0x4000,0x8000,0xa000,0xc000,0xd000,0xe000]]
GLOBALS=[0x50eef8,0x50eef4,0x5020c8,0x50eef0,0x50eeec,0x510fa8,0x4ecbd0]
ENTRIES=[0x272ec0,0x274800,0x2731e0,0x16e8a0,0x2748e0,0x274970,0x1ea400,0x299aa0]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');p.add_argument('--complete-callbacks',action='store_true');a=p.parse_args();owned=Path(os.environ['HALO2_TEST_SCRATCH'])/'original-clump-retirement';owned.mkdir();image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text');entries=ENTRIES+([0x1bda70,0x1c8e90,0x1c75b0,0x1c9750] if a.complete_callbacks else []);bodies={e:_trace(code,e,set(entries)) for e in entries}
 section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));observer=owned/'observer';subprocess.run(['c++','-std=c++20','-O2',str(Path(__file__).with_name('observe_native_campaign_ai_orders_tu15.cpp')),'-o',str(observer)],check=True,capture_output=True);visited=set()
 # Real kind1 descriptor has no+28 callback; kind27 owns the observed
 # nonzero invalidation dependency. Descriptor inputs stay original bytes.
 kinds=[1,4,27]+([21,56,124] if a.complete_callbacks else []);descriptor_inputs=[]
 for kind in kinds:
  p=0x47b478+kind*4;value=int.from_bytes(image.read(p,4),'little');descriptor_inputs += [[p,4,value],[value+0x28,4,int.from_bytes(image.read(value+0x28,4),'little')]]
 regions=[[BASE,SIZE]]+[[p,4] for p in GLOBALS+([0x50eefc] if a.complete_callbacks else [])]+[[p,n] for p,n,_v in descriptor_inputs]
 def declaration(name,groups,actors,perceptions=False,poison=False,tail=False,callback=False,attachments=0):
  writes=[]
  def put(p,v,n=4):writes.append([p,n,v&((1<<(8*n))-1)])
  for p,v in zip(GLOBALS,[CLUMPS,GROUPS,ACTORS,PROPERTIES,QUERIES,ATTACHMENTS,OBJECTS]):put(p,v)
  for p,n,v in descriptor_inputs:put(p,v,n)
  for i,(pool,rows,stride,count) in enumerate([(CLUMPS,CR,0x50,3),(GROUPS,GR,0xc4,max(1,groups)),(ACTORS,AR,0x888,max(1,actors)),(PROPERTIES,PR,0x3c,6),(QUERIES,QR,0x124,6)]):
   put(pool+0x44,rows);put(pool+0x24,stride);put(pool+0x2a,8 if poison else 0,1);put(pool+0x48,BASE+0x900+i*4);put(BASE+0x900+i*4,0xffffffff);put(pool+0x34,0x200);put(pool+0x38,count if tail else 0x100);put(pool+0x3c,count)
   for j in range(count):put(rows+j*stride,0x1234+j,2)
  put(CR+0x14,0xe2000000 if groups else 0xffffffff);put(CR+0x18,0xe3000000 if actors else 0xffffffff);put(CR+0x10,actors,2)
  # A second live record and a hole qualify extent contraction beyond index0.
  put(CR+0x50,0,2);put(CR+0xa0,0,2)
  for i in range(max(groups,1)):
   put(GR+i*0xc4+0x14,0xe2000000+i+1 if i+1<groups else 0xffffffff);put(GR+i*0xc4+0x18,0xe4000000+i*2 if perceptions else 0xffffffff)
  total=groups*2 if groups and perceptions else (2 if perceptions else 0)
  for i in range(actors):
   actor=AR+i*0x888;put(actor+0x80,0xe3000000+i+1 if i+1<actors else 0xffffffff);put(actor+0x7c,0xe1000000);put(actor+0x58,0xe4000000+i if total and i<total else 0xffffffff);put(actor+0x190,0 if callback else -1,2);put(actor+0x90,27 if callback else 1,2)
   ref=0xe4000000+i
   for off in [0x338,0x32c,0x344,0x368,0x724,0x7e0,0x3b4,0x4b8,0x68c,0x6a4,0x6b4]:put(actor+off,ref)
   for off in [0x722,0x688,0x6a0,0x6b0]:put(actor+off,1,2)
   put(actor+0x4ac,7,2);put(actor+0x358,0x1234,2);put(actor+0x4e4,0x4321);put(actor+7,1 if attachments else 0,1);put(actor+0x1c,0xe6000000 if attachments else -1)
  for i in range(total):
   prop=PR+i*0x3c;actor=i%actors;previous=i-actors;next_=i+actors
   put(prop+4,0xe3000000+actor);put(prop+8,0xe2000000+i//2);put(prop+0x14,0xe5000000+i if i%2==0 else -1);put(prop+0x2c,0xe4000000+next_ if next_<total else -1);put(prop+0x30,0xe4000000+previous if previous>=0 else -1);put(prop+0x34,0xe4000000+i+1 if i%2==0 else -1);put(prop+0x38,0xe4000000+i-1 if i%2 else -1)
  if attachments:
   put(ATTACHMENTS+0x44,TR);put(TR+0x18,0xe7000000);put(OBJECTS+0x44,OR)
   for i in range(attachments):
    body=BODY+i*0x800;put(OR+i*12+8,body);put(body+0x134,0);put(body+0x13a,0x200,2);put(body+0x210,0xe4000000);put(body+0x250,0xe4000001);put(body+0x20c,0xe7000000+i+1 if i+1<attachments else -1)
  return dict(name=name,clump=0xe1000000,writes=writes,initial_mxcsr=0x1f80)
 def physical(g,pc,occurrence):
  asm=[f'.section .original,"ax"\n.incbin "{text}"'];sections=[f'.original 0x{section.va:x} : {{ *(.original) }}']
  for i,s in enumerate(s for s in image.sections if s.name!='.text'):
   b=owned/f'source{i}.bin';b.write_bytes(bytes(g.mem_read(s.va,s.vsize)));asm.append(f'.section .source{i},"aw"\n.incbin "{b}"');sections.append(f'.source{i} 0x{s.va:x} : {{ *(.source{i}) }}')
  b=owned/'input.bin';b.write_bytes(bytes(g.mem_read(BASE,0x40000)));asm.append(f'.section .input,"aw"\n.incbin "{b}"');sections.append(f'.input 0x{BASE:x} : {{ *(.input) }}')
  asm.append(f'.code32\n.section .bridge,"ax"\n.globl _start\n_start:\n fninit\n ldmxcsr control\n mov ${STACK},%esp\n int3\n jmp *entry\n.balign 4\nentry: .long 0x272ec0\ncontrol: .long 0x1f80\n');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(asm));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n');subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True);subprocess.run([str(observer),str(owned/'reference'),hex(pc),str(occurrence),hex(BASE),str(SIZE),str(owned/'result.bin')],check=True,capture_output=True);raw=(owned/'result.bin').read_bytes();fp,steps=struct.unpack_from('<2I',raw);return fp,steps,raw[8:]
 def execute(spec,fault_at=None):
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
  for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
  g.mem_map(BASE,0x40000);g.mem_map(STOP,4096)
  for p,n,v in spec['writes']:g.mem_write(p,v.to_bytes(n,'little'))
  g.mem_write(STACK,struct.pack('<II',STOP,spec['clump']));g.reg_write(R.UC_X86_REG_ESP,STACK);g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_MXCSR,spec['initial_mxcsr']);initial=[(s.va,bytes(g.mem_read(s.va,s.vsize))) for s in image.sections if s.name!='.text']+[(BASE,bytes(g.mem_read(BASE,0x40000)))];before=bytes(g.mem_read(BASE,SIZE));events=[];sites=[];occurrences=[];visits={};boundary=[];calls=[];last_pc=[None]
  def instruction(_g,pc,_n,_u):
   # Unicorn emits a final same-PC REP hook with ECX=0 after the last
   # repeated store. Physical single-step advances EIP on that last store.
   # An initial zero-count REP still counts as one real instruction visit.
   previous=last_pc[0];last_pc[0]=pc;visited.add(pc)
   if not (pc in [0x16e8d1,0x16e8d8] and g.reg_read(R.UC_X86_REG_ECX)==0 and previous==pc):visits[pc]=visits.get(pc,0)+1
   if pc==0x1ea562:
    sp=g.reg_read(R.UC_X86_REG_ESP);actor,record,reference=struct.unpack('<III',bytes(g.mem_read(sp,12)));request=dict(callback=g.reg_read(R.UC_X86_REG_ECX),actor=actor,behavior_record=record,reference=reference)
    if a.complete_callbacks:calls.append(request)
    else:boundary.append(request);g.emu_stop()
  def access(_g,kind,p,n,value,_u):
   if not any(at<=p and p+n<=at+size for at,size in regions):return
   raw=bytes(g.mem_read(p,n)).hex();events.append(['r',p,n,raw] if kind==UC_MEM_READ else ['w',p,n,raw,(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()]);pc=g.reg_read(R.UC_X86_REG_EIP);sites.append(pc);occurrences.append(visits[pc])
   if fault_at is not None and len(events)-1==fault_at:g.emu_stop()
  g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,access);g.hook_add(UC_HOOK_MEM_WRITE,access);g.emu_start(0x272ec0,STOP,count=200000)
  if fault_at is None and not boundary:assert g.reg_read(R.UC_X86_REG_EIP)==STOP,(spec['name'],hex(g.reg_read(R.UC_X86_REG_EIP)))
  expected=bytearray(before)
  for e in events if fault_at is None else events[:-1]:
   if e[0]=='w':assert BASE<=e[1]<BASE+SIZE;expected[e[1]-BASE:e[1]-BASE+e[2]]=bytes.fromhex(e[4])
  for p,data in initial:g.mem_write(p,data)
  pc=sites[-1] if fault_at is not None else (0x1ea562 if boundary else STOP);occurrence=occurrences[-1] if fault_at is not None else (visits[pc] if boundary else 1);fp,steps,bank=physical(g,pc,occurrence);assert bank==expected,(spec['name'],fault_at,'physical bank differs',hex(pc),occurrence,events[-1] if events else None,[(hex(BASE+i),a,b) for i,(a,b) in enumerate(zip(bank,expected)) if a!=b][:12]);result=dict(events=events,fault_at=fault_at,mxcsr=fp,behavior_boundary=boundary,physical_steps=steps,physical_after_sha256=hashlib.sha256(bank).hexdigest())
  if a.complete_callbacks:result['behavior_calls']=calls
  return result
 specs=[]
 for poison in [False,True]:
  for tail in [False,True]:
   for groups,actors,perceptions in [(0,0,False),(1,0,False),(2,0,False),(0,2,False),(2,2,False),(1,2,True),(2,2,True),(0,2,True)]:specs.append(declaration(f'clump-groups-{groups}-actors-{actors}-perceptions-{perceptions}-poison-{poison}-tail-{tail}',groups,actors,perceptions,poison,tail))
 for count in [1,2]:specs.append(declaration(f'attachments-{count}',2,2,True,attachments=count))
 for count in [0,1,3,4]:
  s=declaration(f'actual-null-behavior-slots-{count}',2,2,True)
  for actor in [AR,AR+0x888]:
   s['writes'].append([actor+0x190,2,count])
   for slot in range(min(count+1,4)):s['writes'].append([actor+0x90+slot*0x40,2,1])
  specs.append(s)
 for perceptions in [False,True]:
  s=declaration(f'nonzero-clump-index-perceptions-{perceptions}',2,2,perceptions,poison=True,tail=True);s['clump']=0xeabc0002
  s['writes'] += [[CR+0xa0,2,0x1357],[CR+0xa0+0x14,4,0xe2000000],[CR+0xa0+0x18,4,0xe3000000],[CR+0xa0+0x10,2,2],[AR+0x7c,4,s['clump']],[AR+0x888+0x7c,4,s['clump']]]
  specs.append(s)
 for poison in [False,True]:specs.append(declaration(f'actual-behavior-frontier-poison-{poison}',1,2,True,poison=poison,callback=True,attachments=2))
 if a.complete_callbacks:
  specs=[]
  for poison in [False,True]:
   for plan in [False,True]:
    for match in [False,True]:
     s=declaration(f'complete-callbacks-poison-{poison}-plan-{plan}-match-{match}',2,2,True,poison,True,attachments=2)
     s['writes'] += [[0x50eefc,4,BASE+0x800],[BASE+0x844,4,BASE+0x11000]]
     for ordinal,actor in enumerate([AR,AR+0x888]):
      ref=0xe4000000+ordinal if match else 0x76543210
      s['writes'] += [[actor+0x190,2,3],[actor+0x90,2,27],[actor+0xd0,2,21],[actor+0x110,2,56],[actor+0x150,2,124],[actor+0xb4,4,ref],[actor+0xa0,4,0xe8000000+ordinal if plan else 0xffffffff],[actor+0xdc,4,ref],[actor+0x120,4,ref],[actor+0x16c,4,ref],[BASE+0x11000+ordinal*0xbc+0x88,4,ref]]
     specs.append(s)
 scenarios=[execute(s) for s in specs];faults=[]
 fault_names=['complete-callbacks-poison-False-plan-True-match-True','complete-callbacks-poison-True-plan-False-match-False'] if a.complete_callbacks else ['clump-groups-0-actors-0-perceptions-False-poison-True-tail-True','attachments-2','actual-behavior-frontier-poison-False']
 for name in fault_names:
  index=next(i for i,s in enumerate(specs) if s['name']==name);s=specs[index]
  for i in range(len(scenarios[index]['events'])):
   result=execute(s,i);assert result['events']==scenarios[index]['events'][:i+1];del result['events']
   if a.complete_callbacks:
    calls=result.pop('behavior_calls');assert calls==scenarios[index]['behavior_calls'][:len(calls)];result['behavior_call_count']=len(calls)
   faults.append(dict(scenario=index,**result))
 fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],regions=regions,specs=specs,scenarios=scenarios,faults=faults,required_backend='physical-i386-complete-bank-fp+unicorn-ordered-source-events',source_bodies={f'{e:08x}':hashlib.sha256(b''.join(bytes(code.at(pc).bytes) for pc in sorted(b.code))).hexdigest() for e,b in bodies.items()})
 if a.complete_callbacks:fixture['complete_callbacks']=True
 encoded=json.dumps(fixture,separators=(',',':'),sort_keys=True)+'\n'
 if a.verify:assert a.fixture.read_text()==encoded,'Original clump retirement fixture differs'
 else:assert not a.fixture.exists();a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(encoded)
 report=dict(passed=True,whole_calls=sum(not c['behavior_boundary'] for c in scenarios),whole_calls_without_behavior_callbacks=0 if a.complete_callbacks else sum(not c['behavior_boundary'] for c in scenarios),actual_behavior_callback_calls=sum(len(c.get('behavior_calls',[])) for c in scenarios),actual_behavior_callback_prefixes=sum(bool(c['behavior_boundary']) for c in scenarios),fault_prefixes=len(faults),fixture_bytes=len(encoded.encode()),fixture_sha256=hashlib.sha256(encoded.encode()).hexdigest(),original_sites_executed=len(visited&set().union(*(set(b.code) for b in bodies.values()))),original_sites_total=sum(len(b.code) for b in bodies.values()),full_nonempty_behavior_closure=a.complete_callbacks,remaining_behavior_callbacks=[] if a.complete_callbacks else 'real descriptor+28 callback at1EA562; no success return supplied');a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))

if __name__=='__main__':main()
