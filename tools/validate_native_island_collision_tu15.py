#!/usr/bin/env python3
"""Physical original321E50 scheduling and closed allocation-monitor owners.

Original149B90/282B70/282B90 run completely;321E50 stops at its real321700
owner. No collision result is supplied. Private ELF/observer/fixture/builds
remain under enclosing run_test.py scratch, with small bounded reports only.
"""
import argparse,copy,difflib,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE,ISLAND,WORLD,DISPATCHER,CONFIGURATION,HEAP_A,HEAP_B,TABLE_A,TABLE_B=0x1000000,0x1000100,0x1000300,0x1000700,0x1000800,0x1001000,0x1001100,0x1002000,0x1002400
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
BANKS=((BASE,0x5000),(0x486000,0x1000),(0x56df00,0x100))
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def metadata(image):
 code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
 for entry in(0x321e50,0x282b70,0x282b90,0x149b90):
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    access=op.access;assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),'Missing architectural access')
    base=i.reg_name(op.mem.base);index=i.reg_name(op.mem.index);assert not op.mem.segment
    operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=operands))
  listings[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[32,5,9,36]
 return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[dict(address=p,bytes=n)for p,n in BANKS])
def cases():
 out=[]
 def make(name,entry=0x149b90,count_a=3,count_b=2,pointer_a=HEAP_A,pointer_b=HEAP_B,values=(1,0xffffffff,0x80000000,0x7fffffff,5,0x13579bdf,0xdeadbeef,0,0x40000000),baseline=0,wrap=False,alias=None):
  banks=[dict(address=p,before=bytearray(n))for p,n in BANKS]
  def put(p,v,n=4):
   bank=next(b for b in banks if b['address']<=p and p+n<=b['address']+len(b['before']));off=p-bank['address'];bank['before'][off:off+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
  put(ISLAND+0x20,WORLD);put(ISLAND+0xae,0x7e,1);put(WORLD+0x23e,1,1);put(0x486070,pointer_a);put(0x486074,pointer_b);put(0x56dfcc,0x69,1);put(0x56dfd4,baseline);put(0x56dfd8,0xaabbccdd)
  for heap,table,count in((HEAP_A,TABLE_A,count_a),(HEAP_B,TABLE_B,count_b)):
   put(heap+0x20,count);put(heap+0x1c,(min(count,9)*8)if count<10 else 0);put(heap+0x18,table)
   if wrap:put(heap+0x18,0xf0000000);put(heap+0x1c,(table+min(count,9)*8-0xf0000000)&0xffffffff)
   for i,v in enumerate(values):put(table+i*8,0xfeed0000+i);put(table+i*8+4,v)
  if alias=='size-subject':
   heap=0x56dfbc;put(0x486070,heap);put(0x486074,0);put(heap+0x20,2);put(heap+0x18,TABLE_A+16-ISLAND);put(heap+0x1c,ISLAND)
  elif alias=='count-baseline':
   heap=0x56dfb4;put(0x486070,heap);put(0x486074,0);put(heap+0x18,TABLE_A);put(heap+0x1c,24);put(heap+0x20,3)
  elif alias=='changed-size':
   heap=ISLAND+0x92;put(0x486070,heap);put(0x486074,0);put(heap+0x18,TABLE_A);put(heap+0x1c,8);put(heap+0x20,1)
  for b in banks:b['before']=b['before'].hex()
  n=len(out);boundary=dict(owner=0x321700,arguments=[ISLAND,DISPATCHER,CONFIGURATION])if entry==0x321e50 else None
  out.append(dict(name=name,entry=entry,island=ISLAND,dispatcher=DISPATCHER,configuration=CONFIGURATION,banks=banks,before_fp=[(0x7f,0x27f,0x37f,0x67f,0xa7f)[n%5],((n%8)<<11)|((n%8)<<8)|((n%2)<<14)|(n*7%64),(0x1f80,0x3f80,0x5f80,0x9fc0)[n%4]],boundary=boundary,stop_pc=0x321700 if boundary else STOP))
 for entry in(0x149b90,0x282b70,0x282b90,0x321e50):
  for a in(0,1,3,9,0xffffffff,0x80000000):
   for b in(0,1,2,0xffffffff):make(f'counts_{entry:x}_{a:x}_{b:x}',entry=entry,count_a=a,count_b=b)
  for pa,pb in((0,0),(0,HEAP_B),(HEAP_A,0),(HEAP_A,HEAP_A)):
   make(f'heap_identity_{entry:x}_{pa:x}_{pb:x}',entry=entry,pointer_a=pa,pointer_b=pb)
  for values in((0,)*9,(0xffffffff,)*9,(0x7fffffff,)*9,(0x80000000,)*9):make(f'wrapping_values_{entry:x}_{values[0]:x}',entry=entry,values=values)
  make(f'wrapped_address_{entry:x}',entry=entry,wrap=True)
  make(f'fault_seed_{entry:x}',entry=entry)
 for entry in(0x282b70,0x321e50):make(f'alias_size_subject_{entry:x}',entry=entry,alias='size-subject')
 make('alias_count_baseline',entry=0x282b90,alias='count-baseline');make('alias_changed_size',entry=0x282b90,alias='changed-size')
 make('zero_difference_preserves_flags',entry=0x282b90,count_a=3,count_b=2,values=(0,)*9,baseline=0)
 make('wrapped_total_matches_baseline',entry=0x282b90,count_a=1,count_b=1,values=(0x80000000,)*9,baseline=0)
 return out

def physical(image,m,rows,scratch):
 owned=scratch/'physical-original';owned.mkdir(exist_ok=True);observer=owned/'observer'
 if not observer.exists():
  base=(ROOT/'tools/observe_native_motion_pose_tu15.cpp').read_text();needle='{"banks",banks}}.dump()';assert base.count(needle)==1
  adapted=base.replace(needle,'{"banks",banks},{"return_gp",gp(regs)}}.dump()');source=owned/'observer.cpp';source.write_text(adapted)
  subprocess.run(['c++','-std=c++20','-O2',str(source),'-o',str(observer)],check=True,capture_output=True)
 section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(m,separators=(',',':')))
 for c in rows:
  sections=[f'.original 0x{section.va:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{text}"']
  for n,b in enumerate(c['banks']):
   path=owned/f'bank{n}.bin';path.write_bytes(bytes.fromhex(b['before']));assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{b["address"]:x} : {{ *(.bank{n}) }}')
  args=[DISPATCHER,CONFIGURATION]if c['entry']==0x321e50 else([]if c['entry']==0x149b90 else[ISLAND]);stack=bytearray(0x10000);struct.pack_into('<'+'I'*(len(args)+1),stack,SP-STACK,STOP,*args);(owned/'stack.bin').write_bytes(stack);assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
  cw,sw,mxcsr=c['before_fp'];env=bytearray(28);struct.pack_into('<H',env,0,cw);struct.pack_into('<H',env,4,sw);struct.pack_into('<H',env,8,0xffff);(owned/'environment.bin').write_bytes(env)
  assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${ISLAND},%ecx
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
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=True)
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);m=metadata(image);rows=physical(image,m,cases(),scratch);faults=[];write_faults=[]
 for seed in[c for c in rows if c['name'].startswith('fault_seed')]:
  seen=set();occurrences={}
  for read in seed['reads']:
   key=(read['address'],read['bytes']);occurrences[key]=occurrences.get(key,0)+1
   if read['instruction']in seen:continue
   seen.add(read['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_read_fault_'+hex(read['instruction']),stop_pc=read['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key]));faults.append(c)
  seen=set();occurrences={}
  for write in seed['writes']:
   key=(write['address'],len(bytes.fromhex(write['written'])));occurrences[key]=occurrences.get(key,0)+1
   if write['instruction']in seen:continue
   seen.add(write['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_write_fault_'+hex(write['instruction']),stop_pc=write['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='write'));write_faults.append(c)
 rows+=physical(image,m,[*faults,*write_faults],scratch)
 fixture=scratch/'island-collision.json';f=dict(image_sha256=NEW_PIN,required_backend='unchanged-tu15-island-collision-physical-operands-stores-return-full-fp-v1',body_sites=m['body_sites'],listing_sha256=m['listing_sha256'],cases=rows);fixture.write_text(json.dumps(f,separators=(',',':')))
 project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_island_collision.cpp';text=source.read_text();mutation='cursor-=8u;';assert text.count(mutation)==1;(project/'mutated.cpp').write_text(text.replace(mutation,'cursor-=4u;'))
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],"--oracle-only",sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and("allocation total differs"in r.stderr or"island collision read order"in r.stderr or"island collision store"in r.stderr),(r.returncode,r.stdout,r.stderr)
print("Unchanged physical original rejects wrong allocation-record stride:",r.stderr.strip())
''')
 cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_island_collision_validation LANGUAGES CXX)\ninclude(CTest)\n'
 for name,src in(('native_tu15_island_collision_tests',source),('native_tu15_island_collision_mutation',project/'mutated.cpp')):
  cmake+=f'add_executable({name} "{ROOT}/tests/native_tu15_island_collision_tests.cpp" "{src}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
 cmake+=f'add_test(NAME native_island_collision_original_tu15 COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_tu15_island_collision_tests> "{fixture}")\nadd_test(NAME native_island_collision_mutation_rejected COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_tu15_island_collision_mutation> "{fixture}")\n';(project/'CMakeLists.txt').write_text(cmake)
 results=[]
 for compiler in('g++','clang++'):
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_tu15_island_collision_tests','--target','native_tu15_island_collision_mutation','--filter','^native_island_collision_(original_tu15|mutation_rejected)$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
 base=(ROOT/'tools/observe_native_motion_pose_tu15.cpp').read_text();adapted=(scratch/'physical-original/observer.cpp').read_text();(a.output/'observer-adaptation.diff').write_text(''.join(difflib.unified_diff(base.splitlines(True),adapted.splitlines(True),fromfile='physical-observer-base',tofile='owned-private-observer')))
 files=(source,ROOT/'include/halo2/native_tu15_island_collision.hpp',ROOT/'tests/native_tu15_island_collision_tests.cpp',ROOT/'tools/observe_native_motion_pose_tu15.cpp',Path(__file__))
 report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_cases=len(rows),complete_original_calls=sum('fault'not in c and c['boundary']is None for c in rows),actual_owner_prefixes=sum('fault'not in c and c['boundary']is not None for c in rows),read_fault_prefixes=len(faults),integer_store_fault_prefixes=len(write_faults),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),physical_original_instructions=sum(c['original_instructions']for c in rows),image_sha256=NEW_PIN,body_sites=m['body_sites'],listing_sha256=m['listing_sha256'],required_backend=f['required_backend'],fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},private_observer_sha256=sha(scratch/'physical-original/observer.cpp'),observer_change='Actual stopped GP register array recorded; unchanged single-step loop/instructions/memory comparisons',owned_scratch=str(scratch),negative_control='Private mutation changes reverse allocation-record stride8 to4; unchanged original rejects actual return/read/store',retained_large_artifacts=[],excluded=['321700/321A70/321D20 residual execution','complete321E50 after actual remaining phases','unmasked platform traps','full live world composition'])
 temporary=[p for p in scratch.rglob('*')if p.is_file()];report.update(temporary_file_count_before_owner_cleanup=len(temporary),temporary_logical_bytes_before_owner_cleanup=sum(p.stat().st_size for p in temporary),temporary_allocated_bytes_before_owner_cleanup=sum(p.stat().st_blocks*512 for p in temporary))
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','complete_original_calls','actual_owner_prefixes','read_fault_prefixes','integer_store_fault_prefixes')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
