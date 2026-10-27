#!/usr/bin/env python3
"""Unchanged physical TU1.5 world collision scheduling/listener qualification.

No collision/deferred/listener result is supplied. Complete zero-island paths
execute actual3228C0 and authenticated RET8 callbacks; other owners stop at
actual entry. Original instruction/data operands define the oracle. All large
outputs belong to the enclosing run_test.py scratch.
"""
import argparse,copy,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE,WORLD,GRAVITY,STEP,ACTIVE,ISLAND,DISPATCHER,LISTENERS,LISTENER=0x1000000,0x1000100,0x1000800,0x1000900,0x1000a00,0x1000b00,0x1000c00,0x1001000,0x1001800
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=((BASE,0x4000),(0x48c974,8),(0x45f000,0x1000))
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def metadata(image):
 code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
 for entry in(0x319d50,0x3228c0,0x3229e0,0x8fdb0):
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    access=op.access;assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),'Missing architectural access')
    # Capstone23 marks x87 FST/FSTP destinations as READ; the authenticated
    # opcodes are architectural stores, as in the existing pose oracle.
    if i.mnemonic in ('fst','fstp'):access=CS_AC_WRITE
    base=i.reg_name(op.mem.base);index=i.reg_name(op.mem.index);assert not op.mem.segment
    operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=operands))
  listings[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[157,46,49,1] and image.read(0x8fdb0,3)==bytes.fromhex('c20800')
 assert struct.unpack('<I',image.read(0x45fd04,4))[0]==0x8fdb0
 assert struct.unpack('<I',image.read(0x45fd08,4))[0]==0x2e4d60
 return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[dict(address=p,bytes=n)for p,n in BANKS])
def cases(image):
 out=[];table=dict(address=0x45f000,before=image.read(0x45f000,0x1000).hex())
 def make(name,entry=0x319d50,seconds=0x3d088889,inverse=0x41f00000,iterations=4,interval=0x3e800000,step=STEP,configuration=GRAVITY,changes=(),active=0,operations=0,deletes=0,updates=0,pattern=(),signed_count=None):
  raw=bytearray(0x4000)
  def put(p,v,n=4):raw[p-BASE:p-BASE+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
  put(WORLD+8,ACTIVE);put(WORLD+0xc,active);put(ACTIVE,ISLAND);put(WORLD+0xc4,DISPATCHER);put(WORLD+0xcc,configuration)
  put(WORLD+0x24,operations);put(WORLD+0xf4,deletes);put(WORLD+0x100,updates);put(WORLD+0x17c,iterations);put(WORLD+0x180,interval)
  for i,v in enumerate((0x3f000000,0xbf800000,0xc0800000,0x80000000)):put(GRAVITY+i*4,v)
  for i,v in enumerate((seconds,inverse,0x3f800000,0x80000000)):put(step+i*4,v)
  array_offset,count_offset=(0xa8,0xac)if entry==0x3229e0 else(0x90,0x94)
  put(WORLD+array_offset,LISTENERS);put(WORLD+count_offset,len(pattern)if signed_count is None else signed_count)
  for i,kind in enumerate(pattern):
   receiver=LISTENER+i*0x10 if kind else 0;put(LISTENERS+i*4,receiver)
   if kind:put(receiver,0x45fd00 if kind==1 else 0x45fd04)
  for p,v,n in changes:put(p,v,n)
  boundary=None
  if entry==0x319d50:
   if operations&0x80000000==0 and operations>0:boundary=dict(kind='operations',owner=0x319750,arguments=[WORLD])
   elif active&0x80000000==0 and active>0:boundary=dict(kind='island',owner=0x321e50,arguments=[ISLAND,DISPATCHER,WORLD+0x140])
   elif deletes:boundary=dict(kind='deletes',owner=0x319cd0,arguments=[WORLD])
   elif updates:boundary=dict(kind='updates',owner=0x316e10,arguments=[WORLD])
  if boundary is None:
   for i in range(len(pattern)-1,-1,-1):
    if pattern[i]==2:boundary=dict(kind='island_listener'if entry==0x3229e0 else'world_listener',owner=0x2e4d60,arguments=[LISTENER+i*0x10,0x2e4d60,ISLAND if entry==0x3229e0 else WORLD,step]);break
  n=len(out);cw=0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10);sw=((n%8)<<11)|((n%8)<<8)|((n%2)<<14)|(n*7%64);mx=(0x1f80,0x3f80,0x5f80,0x9fc0)[n%4]
  out.append(dict(name=name,entry=entry,world=WORLD,step=step,island=ISLAND,banks=[dict(address=BASE,before=raw.hex()),dict(address=0x48c974,before=bytes(8).hex()),table],boundary=boundary,before_fp=[cw,sw,mx],stop_pc=boundary['owner']if boundary else STOP))
 for rounding in range(4):
  for precision in(0,2,3):
   for value in(0,0x80000000,0x3d088889,0x3f800000):
    for iteration in(0,0xffffffff):
     make(f'finite_{rounding}_{precision}_{value:x}_{iteration:x}',seconds=value,iterations=iteration if len(out)%2 else 0x7fffffff,interval=(0x3e800000,0x3f800000,0xbf800000,0)[len(out)%4]);out[-1]['before_fp'][0]=0x7f|(precision<<8)|(rounding<<10)
 for iteration in(0,1,0x80000000):make(f'integer_iteration_{iteration:x}',iterations=iteration)
 for field in('seconds','inverse','interval'):
  for bits in(1,0x00800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345,0x80000000):make(f'exception_{field}_{bits:x}',**{field:bits})
 for pair,(a,b)in enumerate(((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321))):
  make(f'paired_nan_scaled_{pair}',seconds=a,interval=b)
  make(f'paired_nan_iteration_{pair}',seconds=a,inverse=b,interval=0x3f800000)
 for step,configuration in((WORLD+0x140,GRAVITY),(WORLD+0x150,GRAVITY),(WORLD+0x170,GRAVITY),(WORLD+0x198,GRAVITY),(STEP,WORLD+0x150),(STEP,STEP),(WORLD+0xcc,GRAVITY)):
  make(f'alias_{step:x}_{configuration:x}',step=step,configuration=configuration,seconds=GRAVITY if step==WORLD+0xcc else 0x3d088889)
 for name,options in(('operations',dict(operations=1)),('island',dict(active=1)),('deletes',dict(deletes=1)),('updates',dict(updates=1)),('negative_operations',dict(operations=0xffffffff)),('negative_island_count',dict(active=0xffffffff)),('negative_deletes',dict(deletes=0xffffffff)),('negative_updates',dict(updates=0xffffffff))):make('actual_prefix_'+name,**options)
 patterns=((),(0,),(1,),(0,1,0,1),(1,0,1,0),(1,1,1),(0,0,0),(0,1,0,1,0))
 for entry in(0x319d50,0x3228c0,0x3229e0):
  for pi,pattern in enumerate(patterns):make(f'listener_{entry:x}_{pi}',entry=entry,pattern=pattern)
  for count in(0xffffffff,0x80000001):make(f'negative_listener_count_{entry:x}_{count:x}',entry=entry,signed_count=count)
  make(f'actual_custom_listener_{entry:x}',entry=entry,pattern=(0,1,0,2))
  make(f'fault_seed_{entry:x}',entry=entry,pattern=(0,1,0,1,0))
 make('fault_seed_island_prefix',active=1)
 return out

def physical(image,m,cases,scratch):
 owned=scratch/'physical-original';owned.mkdir(exist_ok=True);observer=owned/'observer'
 if not observer.exists():subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_motion_pose_tu15.cpp'),'-o',str(observer)],check=True,capture_output=True)
 section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(m,separators=(',',':')))
 for c in cases:
  sections=[f'.original 0x{section.va:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{text}"']
  for n,b in enumerate(c['banks']):
   path=owned/f'bank{n}.bin';path.write_bytes(bytes.fromhex(b['before']));assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{b["address"]:x} : {{ *(.bank{n}) }}')
  args=[c['step']]if c['entry']==0x319d50 else[c['world'],*([c['island']]if c['entry']==0x3229e0 else[]),c['step']]
  stack=bytearray(0x10000);struct.pack_into('<'+'I'*(len(args)+1),stack,SP-STACK,STOP,*args);(owned/'stack.bin').write_bytes(stack);assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
  cw,sw,mxcsr=c['before_fp'];env=bytearray(28);struct.pack_into('<H',env,0,cw);struct.pack_into('<H',env,4,sw);struct.pack_into('<H',env,8,0xffff);(owned/'environment.bin').write_bytes(env)
  assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${WORLD},%ecx
 mov ${SP},%esp
 int3
 jmp *entry
.balign 4
entry: .long {c['entry']}
sse_control: .long {mxcsr}
environment: .incbin "{owned/'environment.bin'}"
''');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
  subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
  r=subprocess.run([str(observer),str(owned/'reference'),str(owned/'metadata.json'),str(owned/'output.json'),hex(c['stop_pc'])],capture_output=True,text=True);assert r.returncode==0,(c['name'],r.stdout,r.stderr)
  observed=json.loads((owned/'output.json').read_text())
  for b,after in zip(c['banks'],observed.pop('banks')):assert b['address']==after['address'];b['after']=after['after']
  c.update(observed,physical_verified=True)
 return cases

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=True)
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);m=metadata(image);rows=physical(image,m,cases(image),scratch);faults=[]
 for seed in[c for c in rows if c['name'].startswith('fault_seed')]:
  seen=set();occurrences={}
  for read in seed['reads']:
   key=(read['address'],read['bytes']);occurrences[key]=occurrences.get(key,0)+1
   if read['instruction']in seen:continue
   seen.add(read['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_fault_'+hex(read['instruction']),stop_pc=read['instruction'],fault=dict(address=read['address'],bytes=read['bytes'],occurrence=occurrences[key]));faults.append(c)
 read_fault_count=len(faults)
 code=_Code(image,'.text');write_faults=[]
 for seed in[c for c in rows if c['name'].startswith('fault_seed')]:
  seen=set();occurrences={}
  for write in seed['writes']:
   key=(write['address'],len(bytes.fromhex(write['written'])));occurrences[key]=occurrences.get(key,0)+1
   instruction=code.at(write['instruction'])
   # A CPU FSTP write trap retains a live source x87 value. This semantic
   # kernel does not claim that machine postimage; qualify integer stores only.
   if instruction.mnemonic in ('fst','fstp') or write['instruction']in seen:continue
   seen.add(write['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_write_fault_'+hex(write['instruction']),stop_pc=write['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='write'));write_faults.append(c)
 faults+=write_faults
 rows+=physical(image,m,faults,scratch)
 f=dict(image_sha256=NEW_PIN,required_backend='unchanged-tu15-world-collision-physical-operands-stores-full-fp-v1',body_sites=m['body_sites'],listing_sha256=m['listing_sha256'],cases=rows);fixture=scratch/'collision.json';fixture.write_text(json.dumps(f,separators=(',',':')))
 project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_world_collision.cpp';listener=ROOT/'src/physics/native_tu15_world_collision_listeners.cpp';text=source.read_text();mutation='m.write(0x319e1bu,w+0x12cu,1,1);';assert text.count(mutation)==1;(project/'mutated.cpp').write_text(text.replace(mutation,'m.write(0x319e1bu,w+0x12cu,0,1);'))
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],"--oracle-only",sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and("ordered collision store"in r.stderr or"retained collision"in r.stderr),(r.returncode,r.stdout,r.stderr)
print("Unchanged physical original rejects cleared collision flag:",r.stderr.strip())
''')
 cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_world_collision_validation LANGUAGES CXX)\ninclude(CTest)\n'
 for name,src in(('native_tu15_world_collision_tests',source),('native_tu15_world_collision_mutation',project/'mutated.cpp')):
  cmake+=f'add_executable({name} "{ROOT}/tests/native_tu15_world_collision_tests.cpp" "{src}" "{listener}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
 cmake+=f'add_test(NAME native_world_collision_original_tu15 COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_tu15_world_collision_tests> "{fixture}")\nadd_test(NAME native_world_collision_mutation_rejected COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_tu15_world_collision_mutation> "{fixture}")\n';(project/'CMakeLists.txt').write_text(cmake)
 results=[]
 for compiler in('g++','clang++'):
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_tu15_world_collision_tests','--target','native_tu15_world_collision_mutation','--filter','^native_world_collision_(original_tu15|mutation_rejected)$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
 files=(source,listener,ROOT/'include/halo2/native_tu15_world_collision.hpp',ROOT/'include/halo2/native_tu15_world_collision_listeners.hpp',ROOT/'tests/native_tu15_world_collision_tests.cpp',ROOT/'tools/observe_native_motion_pose_tu15.cpp',Path(__file__))
 report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_cases=len(rows),complete_original_calls=sum('fault'not in c and c['boundary']is None for c in rows),residual_entry_prefixes=sum('fault'not in c and c['boundary']is not None for c in rows),read_fault_prefixes=read_fault_count,integer_store_fault_prefixes=len(write_faults),paired_nan_cases=sum(c['name'].startswith('paired_nan')for c in rows),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),physical_original_instructions=sum(c['original_instructions']for c in rows),image_sha256=NEW_PIN,body_sites=m['body_sites'],listing_sha256=m['listing_sha256'],required_backend=f['required_backend'],fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},owned_scratch=str(scratch),negative_control='Private mutation clears collision flag instead of setting it; unchanged physical original rejects ordered store',retained_large_artifacts=[],excluded=['unmasked platform traps','island collision321E50 residual execution','deferred operation/delete/update owner execution','custom listener owner execution','profiling-enabled original timestamp comparison','write-fault live x87 machine postimage','full live world composition'])
 temporary_files=[p for p in scratch.rglob('*')if p.is_file()]
 report.update(temporary_file_count_before_owner_cleanup=len(temporary_files),temporary_logical_bytes_before_owner_cleanup=sum(p.stat().st_size for p in temporary_files),temporary_allocated_bytes_before_owner_cleanup=sum(p.stat().st_blocks*512 for p in temporary_files))
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','complete_original_calls','read_fault_prefixes','paired_nan_cases','ordered_original_stores')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
