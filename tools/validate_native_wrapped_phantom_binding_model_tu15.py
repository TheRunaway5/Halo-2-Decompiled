#!/usr/bin/env python3
"""Qualify closed BV->native callback22->actual enter with native pool owners.

This is a declared component model, not an original-XBE or live-map claim.
Required linked kernels have separate physical authority. Borrowed archives
remain owned by the caller; this validator never copies or changes them.
"""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
HEADER='include/halo2/native_tu15_wrapped_factory_binding.hpp'
HELPER='tests/helpers/native_world_wrapped_phantom_model_tu15.hpp'
INCLUDES='#include "halo2/native_tu15_phantom_callback_agent.hpp"\n#include "halo2/native_tu15_gameplay_phantom_enter.hpp"\n'
CLASS='        if(method.identity==0x308540u)return native_tu15_phantom_shape_class_22();\n'
BRANCH='''            }else if(target.identity==0x2f78c0u){
                NativeTu15PhantomCallbackAgentServices services;services.allocation=allocation;
                services.classify_shape=shape_class;
                services.enter=[&](auto shape,auto method,auto self,auto other,auto context,auto&state){
                    if(method.identity!=0x1f34d0u){
                        ++stats.unqualified_owners;
                        throw std::runtime_error("Native wrapped factory requires qualified enter owner="+std::to_string(method.identity));
                    }
                    native_tu15_enter_gameplay_phantom(memory,shape.identity,self.identity,other.identity,
                        context.identity,state,allocation,observe);
                };
                const NativeTu15PhantomCallbackAgentArguments next{input.first,input.second,input.context,input.user_data};
                result=native_tu15_create_phantom_callback_agent(memory,next,fp,services,observe);
'''
def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def candidate(text):
 if CLASS not in text:
  text=text.replace('#include <limits>',INCLUDES+'#include <limits>');text=text.replace('        ++stats.class_queries;\n','        ++stats.class_queries;\n'+CLASS);needle='            }else if(target.identity==0x2f9990u';at=text.index(needle);text=text[:at]+BRANCH+text[at:]
 return text

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--build-root',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();build=a.build_root.resolve(strict=True);frozen=build.parent/'source';report=dict(passed=False,scope=__doc__,owned_scratch=str(scratch),retained_large_artifacts=[],borrowed_build_root=str(build),production_world_source_claimed=False)
 try:
  archives=[build/('libhalo2_native_'+n+'.a')for n in('pair_consumer','object_velocity','array_storage','shape_bounds')];archivepins={str(p):sha(p)for p in archives};sourcefiles=[HEADER,HELPER,'include/halo2/native_tu15_world_factory_scratch.hpp','src/physics/native_tu15_phantom_callback_agent.cpp','src/physics/native_tu15_gameplay_phantom_enter.cpp','src/physics/native_tu15_gameplay_phantom_membership.cpp','src/physics/native_tu15_contact_agent_factory.cpp','src/physics/native_tu15_array_allocator.cpp'];pins={p:sha(ROOT/p)for p in sourcefiles}
  for p in sourcefiles[3:]:assert sha(frozen/p)==pins[p],('Borrowed native kernel differs from ROOT',p)
  project=scratch/'project';project.mkdir();(project/'candidate/halo2').mkdir(parents=True);(project/'baseline/halo2').mkdir(parents=True);text=(ROOT/HEADER).read_text();private=candidate(text);old=private.replace(INCLUDES,'').replace(CLASS,'').replace(BRANCH,'');assert old!=private,'Baseline must expose the closed callback gap';(project/'baseline/halo2'/Path(HEADER).name).write_text(old);(project/'candidate/halo2'/Path(HEADER).name).write_text(private)
  residual=(ROOT/'tests/native_tu15_world_residuals_tests.cpp').read_text();start=residual.index('namespace {');end=residual.index('constexpr std::uint32_t world=',start);prefix=residual[start:end]+'}\n'
  driver='#include "halo2/native_tu15_wrapped_factory_binding.hpp"\n#include <algorithm>\n#include <iostream>\n#include <map>\n#include <stdexcept>\n#include <vector>\n'+prefix+'using namespace halo2;\n#include "'+str(ROOT/HELPER)+'"\nint main(){try{native_world_wrapped_phantom_model();std::cout<<"PASS4 native pooled callback/enter chains and12 strict owner refusals\\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\\n";return 1;}}\n';(project/'driver.cpp').write_text(driver);(project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "class owner=3179840"in r.stderr,(r.returncode,r.stderr)\nprint("Baseline RED: closed factory cannot classify actual308540 callback shape")\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(wrapped_phantom_model LANGUAGES CXX)\ninclude(CTest)\n'
  for name in('baseline','candidate'):
   cmake+=f'add_executable(model_{name} driver.cpp)\ntarget_include_directories(model_{name} PRIVATE {name} "{ROOT}/include")\ntarget_compile_features(model_{name} PRIVATE cxx_std_20)\ntarget_compile_options(model_{name} PRIVATE -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_options(model_{name} PRIVATE -Wl,--gc-sections)\ntarget_link_libraries(model_{name} PRIVATE -Wl,--start-group '+ ' '.join('"'+str(p)+'"'for p in archives)+f' -Wl,--end-group)\n';cmd=f'"{sys.executable}" "{project}/negative.py" 'if name=='baseline'else'';cmake+=f'add_test(NAME model_{name} COMMAND {cmd}$<TARGET_FILE:model_{name}>)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1','--target','model_baseline','--target','model_candidate','--filter','^model_'];r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-runner.log')).write_text(r.stdout+r.stderr);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,r.stdout[-2000:]+r.stderr[-2000:]
  assert all(sha(Path(p))==h for p,h in archivepins.items());assert all(sha(ROOT/p)==h for p,h in pins.items());report.update(passed=True,archive_sha256=archivepins,source_sha256=pins,compiler_results=results,positive_profiles_per_compiler=4,unknown_owner_profiles_per_compiler=12,real_native_pool_allocation_and_release=True,original_or_live_map_qualification=False,private_candidate_header_sha256=hashlib.sha256(private.encode()).hexdigest())
 except BaseException as e:report['error']=str(e);raise
 finally:(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0
if __name__=='__main__':raise SystemExit(main())
