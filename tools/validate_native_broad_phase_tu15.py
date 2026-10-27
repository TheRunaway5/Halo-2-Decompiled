#!/usr/bin/env python3
"""Unchanged physical TU1.5 broad-phase orchestration and temporary arrays.

Platform-private SEH links execute in the original with real i386 FS setup;
comparison covers retained physics fields and explicitly owned pair packets.
Unconverted allocator/shape/sweep/consume owners stop at genuine entries, never
receive a supplied result. All generated outputs belong to run_test.py scratch.
"""
import argparse,copy,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE,HEADER,ISLAND,WORLD,ALLOCATOR,DISPATCHER,GRAVITY,BODY_ARRAY,BODY,SHAPE,TRANSFORM,ARENA=0x1000000,0x1000100,0x1000300,0x1000500,0x1000800,0x1000900,0x1000a00,0x1000c00,0x1002000,0x1002800,0x1003000,0x1004000
STACK,SP,STOP,FS=0x2000000,0x200f000,0x3000000,0x4000000
FIRST,SECOND=SP-0x34,SP-0x20
SHAPE_OWNER,SWEEP_OWNER=0x31edf0,0x323420
BANKS=((BASE,0x10000),(0x41f000,0x1000),(0x45f000,0x1000),(0x46a000,0x1000),(0x48c000,0x1000))
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def metadata(image):
 code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
 for entry in(0x321700,0x3177e0,0x317870):
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    if op.mem.segment:
     assert i.reg_name(op.mem.segment)=='fs';continue
    access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access;assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),'Missing architectural access')
    base=i.reg_name(op.mem.base);index=i.reg_name(op.mem.index)
    operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=operands))
  listings[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[278,49,52];assert struct.unpack('<I',image.read(0x45f4cc+0x24,4))[0]==0x2e10a0;assert struct.unpack('<I',image.read(0x45f4cc+0x28,4))[0]==0x2e1110
 assert struct.unpack('<I',image.read(0x41f1e0+0x18,4))[0]==SHAPE_OWNER and struct.unpack('<I',image.read(0x41f1e0+0x14,4))[0]==SWEEP_OWNER
 return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[dict(address=p,bytes=n)for p,n in BANKS]+[dict(address=FIRST,bytes=40)])
def cases(image):
 out=[]
 def make(name,entry=0x321700,reserve=2,bodies=0,remaining=0x4000,boundary=ARENA+0x8000,capacity=0x80000002,gravity=0xc0800000,changes=()):
  banks={p:bytearray(image.read(p,n)if p in(0x41f000,0x45f000,0x46a000)else bytes(n))for p,n in BANKS}
  if entry==0x321700:banks[FIRST]=bytearray(40)
  def put(p,v,n=4):
   bank=next(lo for lo,raw in banks.items()if lo<=p and p+n<=lo+len(raw));banks[bank][p-bank:p-bank+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little')
  put(0x48c938,ALLOCATOR);put(ALLOCATOR,0x45f4cc);put(ALLOCATOR+8,ARENA);put(ALLOCATOR+0xc,remaining);put(ALLOCATOR+0x14,boundary)
  put(ISLAND+0x20,WORLD);put(WORLD+0x238,reserve);put(WORLD+0xcc,GRAVITY);put(GRAVITY+8,gravity);put(ISLAND+0x3c,BODY_ARRAY);put(ISLAND+0x40,bodies);put(DISPATCHER,0x41f1e0)
  for n in range(3):put(BODY_ARRAY+n*4,BODY+n*0x100);put(BODY+n*0x100+0xc,SHAPE);put(BODY+n*0x100+0x14,TRANSFORM);put(SHAPE,0x41f1e0)
  for offset,value in((0,ARENA),(4,0),(8,capacity),(0xc,ARENA),(0x10,reserve)):put(HEADER+offset,value)
  for p,v,n in changes:put(p,v,n)
  if entry==0x3177e0:args=[reserve];receiver=HEADER;stop=0x2e10a0 if (reserve*8+15&0xffffffff)&0xfffffff0>remaining and not remaining&0x80000000 else STOP
  elif entry==0x317870:
   args=[];receiver=HEADER;size=((reserve*8+15)&0xffffffff)&0xfffffff0;stop=0x2e1110 if size and(boundary==0 or boundary==ARENA)else 0x149e00 if not capacity&0x80000000 else STOP
  else:
   args=[DISPATCHER,WORLD+0x140];receiver=ISLAND
   pair_bytes=((reserve*8+15)&0xffffffff)&0xfffffff0
   if pair_bytes>remaining and not pair_bytes&0x80000000:stop=0x2e10a0
   elif pair_bytes*2>remaining and not pair_bytes&0x80000000:stop=0x2e10a0
   elif bodies and not bodies&0x80000000 and pair_bytes*2+((bodies*32+15)&0xfffffff0)>remaining:stop=0x2e10a0
   elif bodies and not bodies&0x80000000 and pair_bytes*2+((bodies*32+15)&0xfffffff0)+((bodies*4+15)&0xfffffff0)>remaining:stop=0x2e10a0
   else:stop=SHAPE_OWNER if bodies and not bodies&0x80000000 else SWEEP_OWNER
  n=len(out);out.append(dict(name=name,entry=entry,receiver=receiver,arguments=args,island=ISLAND,dispatcher=DISPATCHER,configuration=WORLD+0x140,header=HEADER,first=FIRST,second=SECOND,reserve=reserve,banks=[dict(address=p,before=raw.hex())for p,raw in banks.items()],before_fp=[(0x7f,0x27f,0x37f,0x67f,0xa7f)[n%5],((n%8)<<11)|((n%8)<<8)|((n%2)<<14)|(n*7%64),(0x1f80,0x3f80,0x5f80,0x9fc0)[n%4]],boundary=None,stop_pc=stop))
 for reserve in(0,1,2,3,16,0x20000000):
  make(f'construct_{reserve:x}',entry=0x3177e0,reserve=reserve)
  make(f'destroy_{reserve:x}',entry=0x317870,reserve=reserve)
 for capacity in(0,1,4,0x7fffffff):make(f'owned_release_{capacity:x}',entry=0x317870,capacity=capacity)
 for reserve in(1,3):make(f'actual_construct_fallback_{reserve}',entry=0x3177e0,reserve=reserve,remaining=0)
 for boundary in(0,ARENA):make(f'actual_destruct_fallback_{boundary:x}',entry=0x317870,boundary=boundary)
 for reserve in(0,1,3):
  for bodies in(0,1,3,0xffffffff):make(f'broad_prefix_{reserve}_{bodies:x}',reserve=reserve,bodies=bodies)
 for remaining in(0,32):make(f'actual_main_allocate_{remaining}',reserve=0,bodies=1,remaining=remaining)
 for bits in(0,0x80000000,1,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'margin_{bits:x}',gravity=bits)
 for rounding in range(4):
  for precision in(0,2,3):
   for bits in(0xc0800000,0x80000000,1,0x3f800001,0x7f800000,0x7fc12345,0x7f812345):
    make(f'margin_control_{rounding}_{precision}_{bits:x}',gravity=bits);out[-1]['before_fp'][0]=0x7f|(precision<<8)|(rounding<<10)
 make('header_alias_arena_cursor',entry=0x3177e0,reserve=1,changes=((ALLOCATOR+8,HEADER,4),))
 make('fault_seed_construct',entry=0x3177e0,reserve=3);make('fault_seed_destroy',entry=0x317870,reserve=3);make('fault_seed_broad_empty');make('fault_seed_broad_shape',bodies=1)
 return out
def physical(image,m,rows,scratch):
 owned=scratch/'physical-original';owned.mkdir(exist_ok=True);observer=owned/'observer'
 if not observer.exists():
  adapted=(ROOT/'tools/observe_native_constraint_step_tu15.cpp').read_text();source=owned/'observer.cpp';source.write_text(adapted)
  subprocess.run(['c++','-std=c++20','-O2',str(source),'-o',str(observer)],check=True,capture_output=True)
 section=next(s for s in image.sections if s.name=='.text');text=owned/'text.bin';text.write_bytes(image.section_bytes(section));(owned/'metadata.json').write_text(json.dumps(m,separators=(',',':')))
 for c in rows:
  specific=dict(m);specific['regions']=[dict(address=b['address'],bytes=len(bytes.fromhex(b['before'])))for b in c['banks']];(owned/'metadata.json').write_text(json.dumps(specific,separators=(',',':')))
  sections=[f'.original 0x{section.va:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{text}"']
  stack=bytearray(0x10000)
  for n,b in enumerate(c['banks']):
   if STACK<=b['address'] and b['address']+len(bytes.fromhex(b['before']))<=STACK+len(stack):
    off=b['address']-STACK;stack[off:off+len(bytes.fromhex(b['before']))]=bytes.fromhex(b['before']);continue
   path=owned/f'bank{n}.bin';path.write_bytes(bytes.fromhex(b['before']));assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"');sections.append(f'.bank{n} 0x{b["address"]:x} : {{ *(.bank{n}) }}')
  args=c['arguments'];struct.pack_into('<'+'I'*(len(args)+1),stack,SP-STACK,STOP,*args);(owned/'stack.bin').write_bytes(stack);assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
  assembly.append(f'.section .tls,"aw"\n.space 4096');sections.append(f'.tls 0x{FS:x} : {{ *(.tls) }}')
  cw,sw,mxcsr=c['before_fp'];env=bytearray(28);struct.pack_into('<H',env,0,cw);struct.pack_into('<H',env,4,sw);struct.pack_into('<H',env,8,0xffff);(owned/'environment.bin').write_bytes(env)
  assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 mov $123,%eax
 mov $1,%ebx
 mov $fs_descriptor,%ecx
 mov $16,%edx
 int $0x80
 test %eax,%eax
 js setup_failure
 mov $7,%eax
 mov %ax,%fs
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${c['receiver']},%ecx
 mov ${SP},%esp
 int3
 jmp *entry
.balign 4
entry: .long {c['entry']}
sse_control: .long {mxcsr}
fs_descriptor: .long 0,{FS},0xfffff,0x51
setup_failure:
 mov $1,%eax
 mov $125,%ebx
 int $0x80
environment: .incbin "{owned/'environment.bin'}"
''');sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly));(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
  subprocess.run(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],check=True,capture_output=True);subprocess.run(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],check=True,capture_output=True)
  r=subprocess.run([str(observer),str(owned/'reference'),str(owned/'metadata.json'),str(owned/'output.json'),hex(c['stop_pc'])],capture_output=True,text=True);assert r.returncode==0,(c['name'],r.stdout,r.stderr)
  observed=json.loads((owned/'output.json').read_text())
  for b,after in zip(c['banks'],observed.pop('banks')):assert b['address']==after['address'];b['after']=after['after']
  c.update(observed,physical_verified=True)
  if 'fault'not in c and c['stop_pc']!=STOP:
   gp=c['boundary_gp'];args=c['boundary_stack'];target=c['stop_pc']
   if target==0x2e10a0:c['boundary']=dict(kind='allocate',receiver=gp[1],owner=target,arguments=args[1:2])
   elif target==0x2e1110:c['boundary']=dict(kind='release',receiver=gp[1],owner=target,arguments=args[1:3])
   elif target==0x149e00:c['boundary']=dict(kind='owned',receiver=gp[1],owner=target,arguments=args[1:4])
   elif target==SHAPE_OWNER:c['boundary']=dict(kind='shape',receiver=gp[1],owner=target,arguments=args[1:4])
   elif target==SWEEP_OWNER:c['boundary']=dict(kind='sweep',receiver=gp[1],owner=target,arguments=args[1:6])
   else:raise AssertionError(('Unknown real broad-phase boundary',hex(target)))
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=True)
 assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);m=metadata(image);rows=physical(image,m,cases(image),scratch);faults=[];write_faults=[]
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
 fixture=scratch/'broad.json';f=dict(image_sha256=NEW_PIN,required_backend='unchanged-tu15-broad-phase-physical-semantic-packets-fp-v1',body_sites=m['body_sites'],listing_sha256=m['listing_sha256'],cases=rows);fixture.write_text(json.dumps(f,separators=(',',':')))
 project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_broad_phase.cpp';text=source.read_text();mutation='reserve|0x80000000u';assert text.count(mutation)==1;(project/'mutated.cpp').write_text(text.replace(mutation,'reserve|0x40000000u'))
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],"--oracle-only",sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and("broad-phase store"in r.stderr or"retained broad-phase"in r.stderr),(r.returncode,r.stdout,r.stderr)
print("Unchanged physical original rejects lost borrowed pair-array flag:",r.stderr.strip())
''')
 cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_broad_phase_validation LANGUAGES CXX)\ninclude(CTest)\n'
 for name,src in(('native_tu15_broad_phase_tests',source),('native_tu15_broad_phase_mutation',project/'mutated.cpp')):
  cmake+=f'add_executable({name} "{ROOT}/tests/native_tu15_broad_phase_tests.cpp" "{src}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
 cmake+=f'add_test(NAME native_broad_phase_original_tu15 COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_tu15_broad_phase_tests> "{fixture}")\nadd_test(NAME native_broad_phase_mutation_rejected COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_tu15_broad_phase_mutation> "{fixture}")\n';(project/'CMakeLists.txt').write_text(cmake)
 results=[]
 for compiler in('g++','clang++'):
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_tu15_broad_phase_tests','--target','native_tu15_broad_phase_mutation','--filter','^native_broad_phase_(original_tu15|mutation_rejected)$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
 files=(source,ROOT/'include/halo2/native_tu15_broad_phase.hpp',ROOT/'tests/native_tu15_broad_phase_tests.cpp',ROOT/'tools/observe_native_constraint_step_tu15.cpp',Path(__file__))
 report=dict(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_cases=len(rows),complete_original_calls=sum('fault'not in c and c['stop_pc']==STOP for c in rows),actual_owner_prefixes=sum('fault'not in c and c['stop_pc']!=STOP for c in rows),read_fault_prefixes=len(faults),integer_store_fault_prefixes=len(write_faults),paired_nan_cases=sum(c['name'].startswith('paired_nan')for c in rows),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),physical_original_instructions=sum(c['original_instructions']for c in rows),image_sha256=NEW_PIN,body_sites=m['body_sites'],listing_sha256=m['listing_sha256'],required_backend=f['required_backend'],fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},owned_scratch=str(scratch),negative_control='Private mutation loses actual borrowed-array high bit; unchanged original rejects retained array publication',retained_large_artifacts=[],excluded=['platform-private SEH/CPU-frame postimage','platform exception-unwind execution','real geometry admission for raw virtual-boundary fixtures','shape/sweep/growth/consume/custom allocator residual execution','complete321700 after actual remaining phases','profiling-enabled original timestamps','unmasked platform traps','full live world composition'])
 temporary=[p for p in scratch.rglob('*')if p.is_file()];report.update(temporary_file_count_before_owner_cleanup=len(temporary),temporary_logical_bytes_before_owner_cleanup=sum(p.stat().st_size for p in temporary),temporary_allocated_bytes_before_owner_cleanup=sum(p.stat().st_blocks*512 for p in temporary))
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','original_cases','complete_original_calls','actual_owner_prefixes','read_fault_prefixes','integer_store_fault_prefixes')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
