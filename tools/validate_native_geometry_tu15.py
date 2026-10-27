#!/usr/bin/env python3
"""Physical unchanged TU1.5 semantic geometry qualification.

One private original ELF/observer serves all declared cases; only input memory
and entry registers are installed before each actual original call. Nothing
supplies a geometry result, patches instructions or resumes a stopped prefix.
"""
import argparse,copy,hashlib,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE,MATRIX,SOURCE,DEST=0x1000000,0x1000100,0x1000200,0x1000300
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
ENTRIES=(0x2e44d0,0x2e47b0,0x2e4690)
BACKEND='unchanged-tu15-geometry-physical-operands-fp-v1'
def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def save(p,j):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def metadata(image):
 code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];hashes={};counts=[]
 for entry in ENTRIES:
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment;access=op.access;assert access&(CS_AC_READ|CS_AC_WRITE)
    base=i.reg_name(op.mem.base);index=i.reg_name(op.mem.index);ops.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,bytes=i.bytes.hex(),operands=ops))
  hashes[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[40,35,26]
 return dict(sites=sites,stop=STOP,body_sites=counts,listing_sha256=hashes)
def inputs():
 cases=[]
 def make(entry,name,seed=0,matrix=MATRIX,source=SOURCE,destination=DEST,changes=(),rounding=None,denorm=None,alignment=False,pure=False):
  n=len(cases);rng=random.Random(seed);raw=bytearray().join(struct.pack('<f',rng.uniform(-8,8))for _ in range(0x1000//4))
  for p,bits in changes:struct.pack_into('<I',raw,p-BASE,bits)
  r=n%4 if rounding is None else rounding;d=(0,0x40,0x8000,0x8040)[n%4]if denorm is None else denorm;depth=(0,1,3,8)[n%4];sw=((n%8)<<11)|((n*0x100)&0x4700)|(n*13%64)
  live=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+(k%3)|(0x8000 if k%2 else 0)).hex()for k in range(depth)]
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[1]=destination;gp[4]=SP+(n%4)*4
  cases.append(dict(id=n,name=name,entry=entry,matrix=matrix,source=source,destination=destination,gp=gp,banks=[dict(address=BASE,before=raw.hex())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),sw,0x1f80|(r<<13)|d|(n*7%64)],live_x87=live,alignment_fault=alignment,pure_typed=pure))
 for e in ENTRIES:
  for r in range(4):
   for d in(0,0x40,0x8000,0x8040):make(e,f'finite_{e:x}_{r}_{d:x}',seed=len(cases),rounding=r,denorm=d,pure=r==0 and d==0)
 for e in ENTRIES:
  fields=(MATRIX,MATRIX+16,MATRIX+32,MATRIX+48,MATRIX+12,MATRIX+28,MATRIX+44,MATRIX+60,SOURCE,SOURCE+4,SOURCE+8,SOURCE+12)
  for p in fields:
   for bits in(0x80000000,1,0x00800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(e,f'exception_{e:x}_{p:x}_{bits:x}',seed=len(cases),changes=((p,bits),))
  for pair,(first,second)in enumerate(((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321))):
   for lane in range(4):
    pairs=((MATRIX+lane*4,SOURCE),(MATRIX+16+lane*4,SOURCE+4),(MATRIX+32+lane*4,SOURCE+8),(MATRIX+lane*4,MATRIX+16+lane*4),(MATRIX+lane*4,MATRIX+32+lane*4),(MATRIX+48+lane*4,MATRIX+lane*4))
    for mode,(a,b)in enumerate(pairs):make(e,f'paired_nan_{e:x}_{pair}_{lane}_{mode}',seed=len(cases),changes=((a,first),(b,second)))
  for p in(MATRIX,MATRIX+16,MATRIX+32,MATRIX+48,SOURCE-16,SOURCE,SOURCE+16,SOURCE+32):make(e,f'alias_destination_{e:x}_{p:x}',seed=410,destination=p)
  for matrix,source in((MATRIX,MATRIX),(MATRIX,MATRIX+16),(SOURCE+16,SOURCE)):
   make(e,f'alias_sources_{e:x}_{matrix:x}_{source:x}',seed=411,matrix=matrix,source=source)
  make(e,f'alias_all_{e:x}',seed=412,matrix=MATRIX,source=MATRIX,destination=MATRIX)
  for field in('matrix','source','destination'):
   kwargs={field:{'matrix':MATRIX,'source':SOURCE,'destination':DEST}[field]+4};make(e,f'actual_alignment_fault_{e:x}_{field}',seed=413,alignment=True,**kwargs)
  make(e,f'fault_seed_{e:x}',seed=414,rounding=2,denorm=0)
  make(e,f'alias_fault_seed_{e:x}',seed=415,destination=MATRIX,rounding=1,denorm=0x8040)
 # Source parenthesization of translation has a visible rounding boundary.
 bits=lambda f:struct.unpack('<I',struct.pack('<f',f))[0]
 changes=[(MATRIX+n*16+k*4,bits(0.))for n in range(4)for k in range(4)]+[(SOURCE+k*4,bits(1. if k in(0,2)else 0.))for k in range(4)]
 changes +=[(MATRIX,bits(33554432.)),(MATRIX+32,bits(-33554432.)),(MATRIX+48,bits(1.))]
 make(0x2e4690,'translation_association_boundary',changes=changes,rounding=0,denorm=0)
 return cases
def prepare(image,m,scratch):
 p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(m,separators=(',',':')))
 (p/'reference.s').write_text(f'''.code32
.section .original,"ax"
.incbin "{p/'text.bin'}"
.section .bank,"aw"
.balign 16
.space 4096
.section .stack,"aw"
.space 65536
.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
 fninit
 int3
 jmp _start
''');(p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS {{ .original 0x{section.va:x}:{{*(.original)}} .bank 0x{BASE:x}:{{*(.bank)}} .stack 0x{STACK:x}:{{*(.stack)}} .stop 0x{STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}} }}\n')
 for command in[['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_geometry_tu15.cpp'),'-o',str(p/'observer')]]:
  run=subprocess.run(command,capture_output=True,text=True);assert run.returncode==0,run.stderr[-4000:]
 return p

def physical(p,rows,suffix):
 data=p/(suffix+'-inputs.json');data.write_text(json.dumps(rows,separators=(',',':')));result=p/(suffix+'-observed.json');command=[str(p/'observer'),str(p/'reference'),str(p/'metadata.json'),str(data),str(result)];r=subprocess.run(command,text=True,capture_output=True,timeout=180);assert r.returncode==0,r.stdout+r.stderr;print(r.stdout.strip(),flush=True);observed=json.loads(result.read_text());assert not observed['instruction_patches']and observed['supplied_original_outcomes']is False;byid={x['id']:x for x in observed['cases']}
 for row in rows:
  actual=byid[row['id']]
  for old,new in zip(row['banks'],actual.pop('banks')):assert old['address']==new['address'];old['after']=new['after']
  row.update(actual,physical_verified=True)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[]);save(a.output/'report.json',report)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);m=metadata(image);owned=prepare(image,m,scratch);rows=physical(owned,inputs(),'complete');prefix=[]
  for seed in[r for r in rows if r['name'].startswith(('fault_seed','alias_fault_seed'))]:
   for phase,items in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
    for n in range(len(items)):
     case=copy.deepcopy(seed);case.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n),pure_typed=False);prefix.append(case)
  rows+=physical(owned,prefix,'prefix');fixture=scratch/'geometry.json';fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=m['body_sites'],listing_sha256=m['listing_sha256'],cases=rows),separators=(',',':'))+'\n')
  project=scratch/'project';(project/'include/halo2').mkdir(parents=True);(project/'src').mkdir();(project/'tests').mkdir();source=ROOT/'src/physics/native_tu15_geometry.cpp';header=ROOT/'include/halo2/native_tu15_geometry.hpp';test=ROOT/'tests/native_tu15_geometry_tests.cpp';shutil.copytree(ROOT/'include',project/'include',dirs_exist_ok=True);shutil.copyfile(source,project/'src/native.cpp');shutil.copyfile(test,project/'tests/test.cpp');text=source.read_text();variants={}
  needle='combined=add(combined,z_term)';assert text.count(needle)==1;variants['omit_z']=text.replace(needle,'combined=combined')
  needle='_mm_shuffle_ps(xy_high,z_high,0x88)';assert text.count(needle)==1;variants['wrong_transpose']=text.replace(needle,'_mm_shuffle_ps(xy_high,z_high,0xdd)')
  needle='const auto translated_z=add(z_term,translation);';assert text.count(needle)==1;wrong=text.replace(needle,'const auto translated_z=z_term;');needle='words(add(add(x_term,y_term),translated_z))';assert wrong.count(needle)==1;variants['reassociate_translation']=wrong.replace(needle,'words(add(add(add(x_term,y_term),translated_z),translation))')
  snapshot_pins={str(p.relative_to(project)):sha(p)for p in [project/'src/native.cpp',project/'tests/test.cpp',project/'include/halo2/native_tu15_geometry.hpp',project/'include/halo2/native_tu15_physics_frame.hpp',project/'include/halo2/physics_contact_record.hpp']}
  for name,variant in variants.items():(project/'src'/f'{name}.cpp').write_text(variant)
  (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and any(s in r.stderr for s in('retained geometry bytes differ','ordered geometry store differs')),(r.returncode,r.stdout,r.stderr)
print('Physical original rejects private arithmetic mutation:',r.stderr.strip())
''')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeGeometryTu15 LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT tests/test.cpp)
target_include_directories(driver PRIVATE include)
target_compile_features(driver PRIVATE cxx_std_20)
target_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
'''
  for name in('native',*variants):
   cmake+=f'add_executable(geometry_{name} $<TARGET_OBJECTS:driver> src/{name}.cpp)\ntarget_include_directories(geometry_{name} PRIVATE include)\ntarget_compile_features(geometry_{name} PRIVATE cxx_std_20)\ntarget_compile_options(geometry_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:geometry_{name}> "{fixture}"';cmake+=f'add_test(NAME geometry_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','geometry_native',*sum((['--target','geometry_'+name]for name in variants),[]),'--filter','^geometry_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  files=[source,header,test,ROOT/'include/halo2/native_tu15_physics_frame.hpp',ROOT/'include/halo2/physics_contact_record.hpp',ROOT/'tools/observe_native_geometry_tu15.cpp',Path(__file__)];report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),physical_alignment_faults=sum(c.get('alignment_fault',False)for c in rows),read_fault_prefixes=sum(c.get('fault',{}).get('phase')=='before_read'for c in rows),write_rejection_prefixes=sum(c.get('fault',{}).get('phase')=='before_write'for c in rows),completed_observer_rejection_prefixes=sum(c.get('fault',{}).get('phase')=='after_write'for c in rows),paired_nan_cases=sum(c['name'].startswith('paired_nan')for c in rows),alias_cases=sum('alias'in c['name']for c in rows),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),physical_original_instructions=sum(c['original_instructions']for c in rows),image_sha256=NEW_PIN,body_sites=m['body_sites'],listing_sha256=m['listing_sha256'],required_backend=BACKEND,fixture_sha256=sha(fixture),fixture_bytes=fixture.stat().st_size,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in files},compiled_dependency_snapshot_sha256=snapshot_pins,compiled_snapshot_unchanged=all(sha(project/name)==pin for name,pin in snapshot_pins.items()),negative_controls=list(variants),supplied_original_outcomes=False,instruction_patches=[],live_activation_proven=False,remaining=['Actual caller liveness/typed seam qualification before live replacement','Unmasked SIMD trap platform owner','Original Xbox CPU bit identity beyond this physical host','Full physics/live multiplayer/native-only closure'])
 except BaseException as error:
  report['error']=str(error);raise
 finally:
  report.update(temporary_allocated_bytes_before_owner_cleanup=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file()));save(a.output/'report.json',report)
 print(json.dumps({k:report.get(k)for k in('passed','original_cases','ordered_original_reads','ordered_original_stores')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
