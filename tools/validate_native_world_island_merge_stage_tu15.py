#!/usr/bin/env python3
"""Own a coherent private World-merge closure, model and genuine09 prefix."""
import argparse,ast,json,os,re,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import BeforeInput,sha,save
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
ARCHIVE=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-phantom-owner-original-v1466-20261101/iteration-09-native-census-readonly-guard-alias-range6-frag-vehicle-camera/required-native-constraint-component-before/actual-native-before.tar.zst'
PIN='0fd26e268228c50ea01c07a5fea7b68a1e06c515eac2ec90562f303c5873caac'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',default='g++');p.add_argument('--source-root',type=Path,default=ROOT);a=p.parse_args();a.source_root=a.source_root.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);a.stage=a.stage.resolve();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],scope='Private coherent native World selection; default ten manager/control profiles and genuine09 source-data merge PREFIX; no whole original World or live invocation claim')
 try:
  source=BeforeInput(ARCHIVE,PIN,scratch/'actual09');report['actual09']=source.receipt
  archive=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-flip-controller-original-v1465-20261101/iteration-02-native-pair-owned-release-input-vehicle-camera/required-native-pair-consumer-before/actual-native-before.tar.zst';assert sha(archive)=='05ce4baa6c8d8a71326a629ce93a25eca8259a5b52edd9f51fc62fd5b3cd90dd'
  reference=scratch/'actual02';reference.mkdir();subprocess.run(['tar','--zstd','-xf',str(archive),'-C',str(reference)],check=True)
  image=Xbe(NEW_DEFAULT);assert sha(NEW_DEFAULT)==NEW_PIN;selected=[(0x41c3d0,64),(0x461518,64),(0x4638b8,24),(0x461960,16),(0x44aac8,4)]
  save(reference/'selected-readonly.json',dict(original_sha256=NEW_PIN,regions=[dict(address=p,hex=image.read(p,n).hex())for p,n in selected]))
  tree=ast.parse((ROOT/'tools/validate_native_world_integration_tu15.py').read_text());nodes=[n.value for n in ast.walk(tree)if isinstance(n,ast.AugAssign)and isinstance(n.target,ast.Name)and n.target.id=='cmake'and isinstance(n.value,ast.JoinedStr)and any(isinstance(v,ast.Constant)and isinstance(v.value,str)and v.value.startswith('add_executable(native_tu15_world_residuals_tests ')for v in n.value.values)];assert len(nodes)==1
  legacy=eval(compile(ast.Expression(nodes[0]),'<world-target>','eval'),dict(ROOT=ROOT,source=ROOT/'src/physics/native_tu15_world_integration.cpp',sys=sys))
  files=[str(Path(f).relative_to(ROOT))for f in re.findall(r'"('+re.escape(str(ROOT))+r'/[^"\n]+\.cpp)"',legacy)]
  cmake=(a.stage/'staged/cmake/PhysicsLiveWorldTu15.cmake').read_text()
  for library in ('halo2_native_object_velocity','halo2_native_pair_consumer'):
   start=cmake.index('add_library('+library+' STATIC');end=cmake.index(')',start);files+=re.findall(r'src/[^\s)]+\.cpp',cmake[start:end])
  for added in re.findall(r'target_sources\(halo2_native_(?:object_velocity|pair_consumer)\s+PRIVATE\s+([^)]*)\)',cmake,re.S):
   files+=re.findall(r'src/[^\s)]+\.cpp',added)
  files+=['src/rendering/world/native_tu15_camera_projection.cpp','src/core/native_tu15_data_array_allocate.cpp','src/core/native_tu15_data_array_delete.cpp','src/core/native_tu15_data_array_iteration.cpp']
  files=list(dict.fromkeys(files));assert len(files)==len(set(files));project=scratch/'project';project.mkdir();shutil.copytree(a.source_root/'include',project/'include')
  pins={}
  for relative in files:
   path=a.stage/'staged'/relative
   if not path.is_file():path=a.source_root/relative
   target=project/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target);pins[relative]=sha(path)
  for path in (a.source_root/'src/physics').rglob('*'):
   if path.suffix in('.hpp','.inc'):target=project/path.relative_to(a.source_root);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target);pins[str(path.relative_to(a.source_root))]=sha(path)
  helper='tests/helpers/native_world_convex_factory_prefix_tu15.hpp';target=project/helper;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(a.source_root/helper,target);pins[helper]=sha(a.source_root/helper)
  for path in (a.stage/'staged').rglob('*'):
   if path.is_file():target=project/path.relative_to(a.stage/'staged');target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
  targets=' '.join('"'+str(project/f)+'"'for f in files)
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(nativeWorldMerge LANGUAGES CXX)\ninclude(CTest)\nfind_package(Threads REQUIRED)\n'
  cmake+='add_executable(native_tu15_world_residuals_tests '+targets+')\ntarget_include_directories(native_tu15_world_residuals_tests PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(native_tu15_world_residuals_tests PRIVATE cxx_std_20)\ntarget_compile_options(native_tu15_world_residuals_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(native_tu15_world_residuals_tests PRIVATE Threads::Threads)\n'
  tests=[('default',''),('factory_prefix','"'+str(reference)+'"'),('merge_prefix','merge-prefix "'+str(source.path)+'" "'+str(a.output/'merge-prefix.json')+'"')]
  for name,args in tests:cmake+='add_test(NAME native_world_'+name+'_tu15 COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- $<TARGET_FILE:native_tu15_world_residuals_tests> '+args+')\n'
  (project/'CMakeLists.txt').write_text(cmake)
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/'cmake'),'--target','native_tu15_world_residuals_tests','--filter','^native_world_(default|factory_prefix|merge_prefix)_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+a.compiler]
  r=subprocess.run(command,check=False);report.update(passed=r.returncode==0,exit_code=r.returncode,command=command,frozen_working_source_hashes=pins,base_source_root=str(a.source_root),compiled_cpp_closure=files,compiled_cpp_count=len(files),deduplicated=True,stage_hashes={str(p.relative_to(a.stage/'staged')):sha(p)for p in(a.stage/'staged').rglob('*')if p.is_file()},parent_physical_proof='analysis/physics/native-island-merge-native-children-actual09-20261101/qualified-aggregate.json',remaining=['Positive duplicates','World merge with genuinely inactive island activation','Whole current World physical comparison','Visible live native merge/wake'])
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
