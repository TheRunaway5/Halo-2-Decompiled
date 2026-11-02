#!/usr/bin/env python3
"""Stage only a typed readonly18A300 BEFORE observer and qualify its interface."""
import argparse,difflib,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--base-live-world',type=Path,default=ROOT/'analysis/physics/native-world-reversed-list-destructor-stage-20261102/staged/src/physics/physics_live_world_tu15.cpp');a=p.parse_args();a.stage=a.stage.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],interface_model_only=True,native_controller_admission_added=False,source_physics_mutated=False)
 try:
  assert sha(a.base_live_world)=='b137a99291e2445b49aad122008a9e7d4012ebc6737171dd2f6fc384a5f70445'
  original=a.base_live_world.read_text();source=original
  include='#include "halo2/native_tu15_physics_frame_observation.hpp"';assert source.count(include)==1
  source=source.replace(include,include+'\n#include "halo2/native_tu15_look_controller_input_observer.hpp"')
  needle='        auto a=frame(args);a.gp[0]=0;run_actual(0x18a300u,a);';assert source.count(needle)==1
  hook='''        auto a=frame(args);a.gp[0]=0;
        // Read-only diagnostic at the actual typed Hostcall; the existing
        // controller implementation and its input/return behavior are unchanged.
        (void)native_tu15_observe_look_controller_input(bank,{a.gp[0],args[0],args[1],
          {a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr},
          a.x87_depth,player,unit});
        run_actual(0x18a300u,a);'''
  source=source.replace(needle,hook)
  world='src/physics/physics_live_world_tu15.cpp';dst=a.stage/'staged'/world;dst.parent.mkdir(parents=True,exist_ok=True);dst.write_text(source)
  (a.stage/'live-world-only.diff').write_text(''.join(difflib.unified_diff(original.splitlines(keepends=True),source.splitlines(keepends=True),fromfile='native01/'+world,tofile='readonlyLook/'+world)))
  model=scratch/'model.cpp';model.write_text(r'''
#include "helpers/native_look_controller_input_capture_tu15.hpp"
#include <map>
#include <thread>
#include <iostream>
using namespace halo2;using J=nlohmann::json;
void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
bool observe_other_translation_unit(const SourcePhysicsContactMemory&,const NativeTu15LookControllerInput&);
struct Memory:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::vector<std::byte>>banks;mutable unsigned reads{};unsigned writes{};
 void read(std::uint32_t p,std::span<std::byte>b)const override{++reads;auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing input bank");--i;check(std::uint64_t(p-i->first)+b.size()<=i->second.size(),"Missing input extent");std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
 void write(std::uint32_t,std::span<const std::byte>)override{++writes;throw std::runtime_error("Readonly observer wrote source memory");}
 void put(std::uint32_t p,std::uint32_t n){auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing setup bank");--i;for(unsigned k=0;k<4;++k)i->second.at(p-i->first+k)=std::byte(n>>(k*8u));}
 template<class F>void visit_committed(F&&f)const{for(const auto&[p,b]:banks)f(p,std::span<const std::byte>(b));}
};
int main(){try{
 Memory m,other;m.banks.emplace(0x4f9b54u,std::vector<std::byte>(4));m.banks.emplace(0x1000000u,std::vector<std::byte>(4096));m.put(0x4f9b54u,0x1000000u);m.put(0x1000014u,0xffffffffu);
 NativeTu15LookControllerInput input{0,0x3e800000u,0xbe000000u,{0x27f,0,0x1fa0},0,0xabcd1234u,0x12345678u};
 check(!observe_other_translation_unit(m,input)&&m.reads==0,"Unbound observer touched memory");unsigned outer{},inner{};
 {NativeTu15LookControllerInputObserverBinding a(m,[&](const auto&owner,const auto&i){check(&owner==&m&&i.control_index==0&&i.yaw_delta_bits==input.yaw_delta_bits,"Typed observer input differs");++outer;});
  check(observe_other_translation_unit(m,input),"Outer binding not visible across TUs");
  {NativeTu15LookControllerInputObserverBinding b(m,[&](const auto&,const auto&){++inner;});check(observe_other_translation_unit(m,input),"Nested binding missing");}
  check(observe_other_translation_unit(m,input)&&outer==2&&inner==1,"Nested binding restoration differs");
  check(!observe_other_translation_unit(other,input),"Observer leaked to another owner");bool worker{};std::thread t([&]{worker=observe_other_translation_unit(m,input);});t.join();check(!worker,"Observer leaked to another executor");
 }
 check(!observe_other_translation_unit(m,input),"Destroyed binding remained active");
 {NativeTu15LookControllerInputObserverBinding a(m,[](const auto&,const auto&){throw std::runtime_error("deliberate diagnostic failure");});bool failed{};try{observe_other_translation_unit(m,input);}catch(...){failed=true;}check(failed,"Diagnostic failure was swallowed");}
 const auto owned=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"));const auto destination=owned/"look-before";
 test::NativeLookControllerInputCaptureTu15 capture(destination);capture.observe(input,m);check(!capture.complete()&&!std::filesystem::exists(destination),"Inactive control was captured");
 m.put(0x1000014u,input.caller_unit);const auto before=m.banks;
 {NativeTu15LookControllerInputObserverBinding binding(m,[&](const auto&,const auto&i){capture.observe(i,m);});check(observe_other_translation_unit(m,input),"Actual typed capture missing");}
 check(capture.complete()&&m.banks==before&&m.writes==0,"Capture mutated source banks");
 std::ifstream file(destination/"capture-native-before.json");const auto data=J::parse(file);
 check(data.at("input").at("control_index")==0&&data.at("input").at("yaw_delta_bits")==input.yaw_delta_bits&&data.at("input").at("pitch_delta_bits")==input.pitch_delta_bits,"Captured formals differ");
 check(data.at("input").at("floating_point").at("x87_status")==0&&data.at("record_matches_caller_unit")==true&&data.at("skipped_none_inputs")==1,"Captured FP/filter provenance differs");
 check(data.at("source_memory_mutated")==false&&data.at("physics_result_supplied")==false&&data.at("native_controller_admission_added")==false&&!data.at("input").contains("gp"),"Capture fabricated execution outputs");
 for(const auto&r:data.at("regions")){std::ifstream in(destination/r.at("before_file").get<std::string>(),std::ios::binary);std::vector<char>b((std::istreambuf_iterator<char>(in)),{});const auto&expected=before.at(r.at("address"));check(b.size()==expected.size()&&std::memcmp(b.data(),expected.data(),b.size())==0,"BEFORE bank bytes differ");}
 capture.observe(input,m);check(m.banks==before,"Repeated capture touched source");
 bool escaped{};try{test::NativeLookControllerInputCaptureTu15 bad(owned.string()+"-escape");}catch(...){escaped=true;}check(escaped,"Capture accepted an unowned path");
 auto deep=input;deep.preserved_x87_depth=1;bool refused{};try{test::NativeLookControllerInputCaptureTu15 fresh(owned/"deep");fresh.observe(deep,m);}catch(...){refused=true;}check(refused&&!std::filesystem::exists(owned/"deep"),"Unprovided x87 values were invented");
 std::cout<<"Readonly look interface: cross-TU owner/executor/nesting/failure/positivefilter/ownedbytes GREEN\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
''')
  (scratch/'other.cpp').write_text('#include "halo2/native_tu15_look_controller_input_observer.hpp"\nbool observe_other_translation_unit(const halo2::SourcePhysicsContactMemory&m,const halo2::NativeTu15LookControllerInput&i){return halo2::native_tu15_observe_look_controller_input(m,i);}\n')
  results=[]
  for compiler in('g++','clang++'):
   executable=scratch/('look-model-'+compiler);command=[compiler,'-std=c++20','-O1','-pthread','-I'+str(a.stage/'staged/include'),'-I'+str(ROOT/'include'),'-I'+str(a.stage/'staged/tests'),str(model),str(scratch/'other.cpp'),'-o',str(executable)]
   r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-build.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:]
   # Each compiler owns a distinct capture under the same disposable owner.
   env=dict(os.environ,HALO2_TEST_SCRATCH=str(scratch/compiler));Path(env['HALO2_TEST_SCRATCH']).mkdir();r=subprocess.run([str(executable)],env=env,capture_output=True,text=True);(a.output/(compiler+'-run.log')).write_text(r.stdout+r.stderr);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,r.stderr
  files=[world,'include/halo2/native_tu15_look_controller_input_observer.hpp','tests/helpers/native_look_controller_input_capture_tu15.hpp'];hashes={name:sha(a.stage/'staged'/name)for name in files}
  report.update(passed=True,results=results,source_hashes=hashes,scope=__doc__)
  pins=dict(qualified_interface=True,diagnostic_only=True,native_admission_added=False,source_physics_mutated=False,root_reviewed=False,peer_reviewed=False,base_live_world=str(a.base_live_world.resolve()),base_hashes={world:sha(a.base_live_world)},staged_hashes=hashes,interface_report=str((a.output/'report.json').relative_to(ROOT)),scope='Onlyper-memory/executor typed readonly18A300before-existing-run_actual hook. Formalindex/dyaw/dpitch/currentFP/depth andcallerownedplayer/unit DATA; noGP/after/outcomes. Onepositivecontrol/depth0/allbankcapture underownedpath. No controllerkernel/admission/control/gates changed. InterfacecrossTU/nested/thread/constcapture tests only; nofullHostbuild/liveclaim.',host_fragment='Optional NativeLookControllerInputCaptureTu15 capture before NativeTu15LookControllerInputObserverBinding; bind world.source_objects->memory(), constownercheck, observe(input,world.source_objects->memory()). Explicit HALO2_TU15_CAPTURE_LOOK_CONTROLLER_INPUT and HALO2_TU15_LOOK_CONTROLLER_INPUT_CAPTURE_DIRECTORY; default outerowned/native-look-controller-before.')
  # Final report hash is written after the cleanup accounting below.
  save(a.stage/'source-pins.json',pins)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
  if report['passed']:
   pins=json.loads((a.stage/'source-pins.json').read_text());pins['interface_report_sha256']=sha(a.output/'report.json');save(a.stage/'source-pins.json',pins)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
