#!/usr/bin/env python3
"""Compile exact staged getter, both JSON paths and query/Look gate controls."""
import argparse,hashlib,json,os,re,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  stage=a.stage.resolve();pins=json.loads((stage/'source-pins.json').read_text());fields=pins['fields'];assert len(fields)==20
  for n,h in pins['staged_hashes'].items():assert sha(stage/'staged'/n)==h
  include=scratch/'include';shutil.copytree(a.source_root/'include',include)
  for p in(stage/'staged/include').rglob('*'):
   if p.is_file():q=include/p.relative_to(stage/'staged/include');q.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,q)
  live=(stage/'staged/src/physics/physics_live_world_tu15.cpp').read_text();accept=(stage/'staged/tests/physics_multiplayer_acceptance.cpp').read_text();begin=live.index('    const auto&look_pipeline=impl_->native_look_pipeline;');end=live.index('    const auto&look_initialization=',begin);getter=live[begin:end]
  names=['native_camera_query_'+k for k in fields]+['native_look_update_fp_refusals'];json_paths=[[],[]]
  for name in names:
   lines=re.findall(r'^\s*\{"'+re.escape(name)+r'",native_frames\.'+re.escape(name)+r'\},$',accept,re.M);assert len(lines)==2,(name,len(lines))
   for n,line in enumerate(lines):json_paths[n].append(line.strip())
  def gate(name):
   start=accept.index('  if(std::getenv("'+name+'")) {');end=accept.index('\n  }',start)+4;return accept[start:end]
  gates=[gate('HALO2_REQUIRE_NATIVE_CAMERA_QUERY'),gate('HALO2_REQUIRE_NATIVE_LOOK_CONTROLLER')]
  tests=[]
  for n,(name,path)in enumerate(fields.items()):
   if path=='unknown_owners()':continue
   tests.append(' {Impl value;value.native_look_pipeline.scene_queries.'+path+'='+str(101+n)+'u;const auto data=getter(value);check(data.native_camera_query_'+name+'=='+str(101+n)+'u,"Getter '+name+'");const auto a=json0(data),b=json1(data);check(a.at("native_camera_query_'+name+'")==' +str(101+n)+'u&&b==a,"Both JSON '+name+'");++checks;}')
  unknowns=['instanced.unknown_owners','instanced.traversal.unbound_services','query.unbound_services','dynamic.unbound_data_owners']
  for n,path in enumerate(unknowns):tests.append(' {Impl value;value.native_look_pipeline.scene_queries.'+path+'='+str(17+n)+'u;check(getter(value).native_camera_query_unknown_owners=='+str(17+n)+'u,"Derived unknown contributor");++checks;}')
  tests.append(' {Impl value;auto&out=value.native_look_pipeline.scene_queries;out.instanced.unknown_owners=17;out.instanced.traversal.unbound_services=23;out.query.unbound_services=31;out.dynamic.unbound_data_owners=47;check(getter(value).native_camera_query_unknown_owners==118,"Derived unknown sum");++checks;}')
  tests.append(' {Impl value;value.native_look_pipeline.unsupported_fp_domains=43;check(getter(value).native_look_update_fp_refusals==43&&json0(getter(value)).at("native_look_update_fp_refusals")==43&&json1(getter(value))==json0(getter(value)),"FP counter/getter/JSON");++checks;}')
  failures=['native_camera_query_refusals','native_camera_query_resource_refusals','native_camera_query_unknown_owners','native_camera_query_instanced_refusals','native_camera_query_unprovided_reads','native_camera_query_unprovided_writes','native_look_update_query_refusals','native_look_update_fp_refusals']
  for name in failures:tests.append(' {auto n=positive();n.'+name+'=1;check(!query_gate(n),"Query rejects '+name+'");++checks;}')
  tests.extend([' {auto n=positive();n.native_camera_query_calls=0;check(!query_gate(n),"No zero coverage");++checks;}',' {auto n=positive();++n.native_camera_query_attempts;check(!query_gate(n),"Unbalanced attempts");++checks;}',' {auto n=positive();n.native_look_update_fp_refusals=1;check(!look_gate(n),"Old Look gate rejects unsupported FP");++checks;}'])
  driver='''#include "halo2/native_tu15_look_pipeline_binding.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace halo2;using J=nlohmann::json;
void check(bool ok,const char*message){if(!ok)throw std::runtime_error(message);}
struct Impl{NativeTu15LookPipelineActivity native_look_pipeline;};
NativeTu15PhysicsFrameExecutionStats getter(const Impl&value){const auto*impl_=&value;NativeTu15PhysicsFrameExecutionStats result;
'''+getter+'''return result;}
'''
  for n,lines in enumerate(json_paths):driver+='J json'+str(n)+'(const NativeTu15PhysicsFrameExecutionStats&native_frames){return J{'+''.join(lines)+'};}\n'
  for n,body in enumerate(gates):driver+='bool '+('query_gate'if n==0 else'look_gate')+'(const NativeTu15PhysicsFrameExecutionStats&native_frames){bool passed=true;J report;\n'+body+'\nreturn passed;}\n'
  driver+='''NativeTu15PhysicsFrameExecutionStats positive(){NativeTu15PhysicsFrameExecutionStats n;n.native_camera_query_attempts=2;n.native_camera_query_calls=2;n.native_look_update_calls=2;n.native_look_update_attempts=2;n.native_look_update_context_calls=2;n.native_look_update_camera_calls=2;n.native_look_update_camera_completions=2;return n;}
int main(){try{setenv("HALO2_REQUIRE_NATIVE_CAMERA_QUERY","1",1);setenv("HALO2_REQUIRE_NATIVE_LOOK_CONTROLLER","1",1);unsigned checks{};check(query_gate(positive())&&look_gate(positive()),"Positive gates");++checks;
'''+ '\n'.join(tests)+'''std::cout<<J{{"passed",true},{"checks",checks},{"query_fields",20},{"additional_fp_fields",1},{"JSON_paths",2}}.dump()<<'\\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\\n';return 1;}}
''';(scratch/'driver.cpp').write_text(driver);results=[]
  for compiler in('g++','clang++'):
   exe=scratch/('counter-'+compiler);command=[compiler,'-std=c++20','-O0','-I'+str(include),str(scratch/'driver.cpp'),'-o',str(exe)];r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-build.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-5000:];r=subprocess.run([str(exe)],capture_output=True,text=True);(a.output/(compiler+'-run.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr;results.append(dict(compiler=compiler,passed=True,result=json.loads(r.stdout)))
  assert all(sha(stage/'staged'/n)==h for n,h in pins['staged_hashes'].items());report.update(passed=True,results=results,fields=fields,additional_field='native_look_update_fp_refusals',negative_controls=failures+['zero_calls','unbalanced_attempts','oldLook_FP'],derived_unknown_contributors=unknowns,stage_source_hashes=pins['staged_hashes'],validator_sha256=sha(Path(__file__)),exact_source_fragments=True)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
