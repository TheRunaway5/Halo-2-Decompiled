#!/usr/bin/env python3
"""Genuine fixed-1.1 animation-State inverse, independent original packets."""
import argparse,copy,hashlib,json,os,subprocess,sys
from pathlib import Path
import validate_native_affine_inverse_tu15 as oracle
from native_affine_inverse_animation_inputs import prepare
import native_affine_inverse_animation_replacements as binding
ROOT=oracle.ROOT
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,default=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--smoke',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
 if not args.xbe.is_file():return 77
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
 files=['tools/validate_native_affine_inverse_animation_tu15.py','tools/validate_native_affine_inverse_tu15.py','tools/native_affine_inverse_animation_inputs.py','tools/native_affine_inverse_animation_replacements.py',binding.UNIT,'tests/native_tu15_affine_inverse_animation_tests.cpp','tests/native_tu15_affine_inverse_tests.cpp','src/physics/runtime/native_tu15_affine_inverse.hpp','include/halo2/native_tu15_affine_inverse_kernel.hpp']
 pending=[ROOT/name for name in files];seen=set(pending)
 import re
 while pending:
  path=pending.pop()
  for header in re.findall(r'^#include "([^"\n]+)"',path.read_text(),re.M):
   target=next((p for p in(path.parent/header,ROOT/'include'/header,ROOT/'src'/header)if p.is_file()),None)
   if target is not None and target not in seen:seen.add(target);pending.append(target)
 pins={str(path.relative_to(ROOT)):sha(path)for path in sorted(seen)}
 report=dict(passed=False,production_binding=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_before=pins)
 try:
  assert sha(args.xbe)==oracle.MAP_PACK_PIN;image=oracle.Xbe(args.xbe)
  source=(ROOT/binding.UNIT).read_text()
  if args.require_binding and binding.replace_affine_inverse_animation(source)!=source:raise ValueError('Actual animation inverse default missing')
  report['state']=prepare(ROOT,scratch)
  oracle.ENTRY=0x142d00;oracle.ONE=0x465030;oracle.REGIONS=((0x464000,0x2000),(0x61000000,0x4000),(0x62000000,0x1000))
  import oracle_native_campaign_screen_lifecycle_tu15 as common
  common.ENTRIES=(oracle.ENTRY,);original=common.metadata;code=common._Code(image,'.text')
  def metadata(image):
   data=original(image)
   for site in data['sites']:
    if code.at(site['instruction']).mnemonic in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
   return data
  common.metadata=metadata;sites=metadata(image)['sites'];assert len(sites)==82
  io=[site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic=='push'];traces=[dict(entry=pc,gp=True,xmm=True)for pc in io];rows=oracle.corpus(args.smoke)
  observed,authority=oracle.physical_original_dense(image,rows,scratch,entries=(oracle.ENTRY,),regions=oracle.REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
  prefixes=[]
  if not args.smoke:
   for row,actual in zip(rows,observed['cases'],strict=True):
    if not(row['name'].endswith(('variant0','variant4','variant5','variant6','variant7'))and int(row['name'].split('_')[0][6:])<4):continue
    for phase,events in(('before_read',actual['reads']),('before_write',actual['writes']),('after_observer',actual['writes'])):
     for ordinal,event in enumerate(events):
      prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'_{phase}_{ordinal}',phase=phase,native_fault_ordinal=ordinal,fault=dict(phase='after_write'if phase=='after_observer'else phase,ordinal=ordinal));prefixes.append(prefix)
   directory=scratch/'prefix';directory.mkdir();extra,prefix_authority=oracle.physical_original_dense(image,prefixes,directory,entries=(oracle.ENTRY,),regions=oracle.REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True);rows+=prefixes;observed['cases']+=extra['cases'];report['prefix_authority']=prefix_authority
  executed=set()
  for row,actual in zip(rows,observed['cases'],strict=True):
   row['returned']=row['phase']=='normal';assert actual['completed']==row['returned'];executed.update(actual['executed_sites']);row['expected_pc']=actual['calls'][-1]['entry']if row['returned']else actual['boundary']['instruction']
   row.update({key:actual[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','writes','reads','calls')});row['boundary_live_x87']=actual.get('boundary_live_x87',row['live_x87'])
   for packet in row['calls']:packet.pop('arguments')
   after={bank['address']:bank['after']for bank in actual['banks']}
   for bank in row['banks']:bank['after']=after[bank['address']]
  assert len(executed)==82
  fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=oracle.MAP_PACK_PIN,code_image='MapPack11',io_sites=io,cases=rows)));project=scratch/'project';project.mkdir()
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(animation_inverse LANGUAGES CXX)
include(CTest)
add_executable(animation "{ROOT/'tests/native_tu15_affine_inverse_animation_tests.cpp'}")
target_include_directories(animation PRIVATE "{scratch}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(animation PRIVATE cxx_std_20)
target_compile_options(animation PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME animation COMMAND animation "{fixture}" "{scratch/'native.json'}")
add_test(NAME animation_reference COMMAND animation "{fixture}" "{scratch/'reference.json'}")
set_tests_properties(animation_reference PROPERTIES ENVIRONMENT "HALO2_REFERENCE_AFFINE_INVERSE=1")
'''
  if args.sanitizers:cmake+='target_compile_options(animation PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(animation PRIVATE -fsanitize=address,undefined)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in args.compiler or('g++','clang++'):
   command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/Path(compiler).name),'--target','animation','--filter','^animation(_reference)?$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   result=subprocess.run(command,capture_output=True,text=True);assert result.returncode==0,(result.stdout+result.stderr)[-7000:];native=json.loads((scratch/'native.json').read_text())
   expected=sum(8 if row['phase']=='normal'else 4 for row in rows);assert native['comparisons']==expected and native['actual_animation_bound_calls']==expected//2
   reference=json.loads((scratch/'reference.json').read_text());assert reference['reference_calls']==sum(row['phase']=='normal'for row in rows)*4 and reference['reference_callbacks']>0
   results.append(dict(compiler=compiler,native=native,positive_reference=reference))
  report.update(passed=True,hardware_paths=len(rows),normal_cases=sum(row['phase']=='normal'for row in rows),prefix_cases=len(prefixes),source_sites=len(executed),authority=authority,compilers=results,production_binding=args.require_binding)
 except BaseException as error:report['error']=repr(error);raise
 finally:
  report['source_after']={name:sha(ROOT/name)for name in pins}
  if report['source_after']!=pins:report.update(passed=False,source_drift=True)
  (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
