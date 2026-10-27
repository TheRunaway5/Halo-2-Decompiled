#!/usr/bin/env python3
"""Independent unchanged-i386 qualification of actual2E4550's native kernel.

Reuse the geometry input/physical-observer lifecycle, not its native outputs.
All fixture banks, ELFs, observers and compiler products are privately owned.
"""
import argparse,copy,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_geometry_tu15 as reference
ROOT=reference.ROOT
ENTRY=0x2e4550
def metadata(image):
 code=reference._Code(image,'.text');body=reference.complete_body(code,ENTRY);registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];listing=[]
 for pc in sorted(body.code):
  ins=code.at(pc);listing.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n');operands=[]
  for op in ins.operands:
   if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue
   assert not op.mem.segment
   base,index=ins.reg_name(op.mem.base),ins.reg_name(op.mem.index)
   operands.append(dict(base=registers[base]if base else-1,index=registers[index]if index else-1,
    scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,
    read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
  sites.append(dict(instruction=pc,bytes=ins.bytes.hex(),operands=operands))
 return dict(sites=sites,stop=reference.STOP,body_sites=[len(body.code)],
  listing_sha256={hex(ENTRY):hashlib.sha256(''.join(listing).encode()).hexdigest()})
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=ROOT/'src/physics/native_tu15_inverse_geometry.cpp';header=ROOT/'include/halo2/native_tu15_inverse_geometry.hpp'
 files=[source,header,ROOT/'src/physics/native_tu15_geometry.cpp',ROOT/'include/halo2/native_tu15_geometry.hpp',ROOT/'tests/native_tu15_geometry_tests.cpp',ROOT/'tools/observe_native_geometry_tu15.cpp',ROOT/'tools/validate_native_geometry_tu15.py',Path(__file__)]
 pins={str(f.relative_to(ROOT)):reference.sha(f)for f in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[])
 def save():reference.save(a.output/'report.json',report)
 save()
 try:
  assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);meta=metadata(image);owned=reference.prepare(image,meta,scratch)
  reference.ENTRIES=(ENTRY,);inputs=[c for c in reference.inputs()if c['entry']==ENTRY]
  rows=reference.physical(owned,inputs,'complete');prefix=[]
  for seed in[r for r in rows if r['name'].startswith(('fault_seed','alias_fault_seed'))]:
   for phase,items in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
    for n in range(len(items)):
     case=copy.deepcopy(seed);case.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n),pure_typed=False);prefix.append(case)
  rows+=reference.physical(owned,prefix,'prefix');fixture=scratch/'inverse.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,required_backend=reference.BACKEND,cases=rows),separators=(',',':'))+'\n')
  project=scratch/'project';(project/'src').mkdir(parents=True);(project/'tests').mkdir();shutil.copytree(ROOT/'include',project/'include')
  shutil.copyfile(source,project/'src/native.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_geometry.cpp',project/'src/geometry.cpp')
  test=(ROOT/'tests/native_tu15_geometry_tests.cpp').read_text();test=test.replace('#include "halo2/native_tu15_geometry.hpp"','#include "halo2/native_tu15_inverse_geometry.hpp"')
  test=test.replace('switch(entry){','switch(entry){case 0x2e4550:native_tu15_compose_inverse_affine_geometry(s,fp);break;')
  test=test.replace('NativeTu15GeometryMemory view','NativeTu15InverseGeometryMemory view')
  test=test.replace('Native TU1.5 geometry requires aligned vector storage','Native TU1.5 inverse geometry requires aligned vector storage')
  test=test.replace('(c.at("entry")==0x2e44d0?4u:1u)','((c.at("entry")==0x2e44d0||c.at("entry")==0x2e4550)?4u:1u)')
  (project/'tests/test.cpp').write_text(test);text=source.read_text();variants={}
  needle='_mm_shuffle_ps(xy_high,zz_high,0x88)';assert text.count(needle)==1;variants['transpose']=text.replace(needle,'_mm_shuffle_ps(xy_high,zz_high,0xdd)')
  needle='add(add(x_term,y_term),z_term)';assert text.count(needle)==2;variants['association']=text.replace(needle,'add(x_term,add(y_term,z_term))',1)
  needle='const auto z=value(s.basis_column(2))';assert text.count(needle)==1
  changed=text.replace(needle,'const auto cached_translation=value(s.basis_column(3));'+needle)
  needle='translation=value(s.basis_column(3))';assert changed.count(needle)==2
  # Preserve the declaration of the early cache; replace only the later read.
  changed=changed.replace('next_z=value(s.basis_column(2)),translation=value(s.basis_column(3))','next_z=value(s.basis_column(2)),translation=cached_translation')
  variants['early_translation']=changed
  for name,body in variants.items():(project/'src'/f'{name}.cpp').write_text(body)
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)\nassert r.returncode!=0 and any(s in r.stderr for s in("retained geometry bytes differ","ordered geometry store differs","ordered geometry read count differs","ordered geometry operand read differs")),(r.returncode,r.stdout,r.stderr)\nprint("Original rejects inverse-geometry mutation:",r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeInverseGeometry LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\nadd_library(driver OBJECT tests/test.cpp)\ntarget_include_directories(driver PRIVATE include)\ntarget_compile_features(driver PRIVATE cxx_std_20)\ntarget_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)\n'
  for name in('native',*variants):
   cmake+=f'add_executable(inverse_{name} $<TARGET_OBJECTS:driver> src/{name}.cpp src/geometry.cpp)\ntarget_include_directories(inverse_{name} PRIVATE include)\ntarget_compile_features(inverse_{name} PRIVATE cxx_std_20)\ntarget_compile_options(inverse_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:inverse_{name}> "{fixture}"';cmake+=f'add_test(NAME inverse_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','inverse_native',*sum((['--target','inverse_'+n]for n in variants),[]),'--filter','^inverse_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1']
   result=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=result.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),physical_alignment_faults=sum(c.get('alignment_fault',False)for c in rows),read_fault_prefixes=sum(c.get('fault',{}).get('phase')=='before_read'for c in rows),write_fault_prefixes=sum(c.get('fault',{}).get('phase')=='before_write'for c in rows),post_store_observer_prefixes=sum(c.get('fault',{}).get('phase')=='after_write'for c in rows),ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),physical_original_instructions=sum(c['original_instructions']for c in rows),negative_controls=list(variants),source_snapshot_unchanged=all(reference.sha(ROOT/f)==v for f,v in pins.items()),body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],image_sha256=reference.NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],live_activation_proven=False)
 except BaseException as error:report['error']=str(error);raise
 finally:save()
 print(json.dumps({k:report.get(k)for k in('passed','original_cases','ordered_original_reads','ordered_original_stores')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
