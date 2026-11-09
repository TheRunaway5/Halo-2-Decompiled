// Independent original execution belongs to the offline prefix concatenated by
// the validator. The typed production helper receives only BEFORE DATA and FP.
#include "halo2/native_tu15_contact_producer_binding.hpp"
#include <xmmintrin.h>
int main(int argc,char**argv){
 J report{{"passed",false},{"profiles",J::array()},{"native_original_executor_calls",0},
  {"captured_gp_used",false},{"original_outcomes_supplied",false}};
 try{
  check(argc==5,"Supply original ELF, metadata, report and mutation kind");
  std::ifstream file(argv[2]);const auto meta=J::parse(file);const auto&input=meta.at("input");
  const Regions regions(meta.at("regions"));const std::string kind=argv[4];
  const bool numeric=kind=="cache_count"||kind=="candidate_minimum";
  try{J controls;guards(controls);report["guard_models"]=controls.at("production_guard_models");report["guard_models_passed"]=true;}
  catch(const std::exception&e){report["guard_models_passed"]=false;report["guard_error"]=e.what();}
  if(kind=="disabled_scope"){
   report["passed"]=!report.at("guard_models_passed").get<bool>();
   report["rejection_kind"]="admission_control";
  }else{
   bool accepted=report.at("guard_models_passed");
   for(unsigned mode:meta.at("modes")){
    Child source;source.start(argv[1]);setup_parent(source,input,mode);const auto before=source.fp();
    const auto expected=original(source,meta,regions);const auto after=source.fp();
    J row{{"mode",mode},{"source_instructions",expected.steps},{"source_reads",expected.reads.size()},
     {"source_stores",expected.stores.size()},{"passes",J::array()}};
    for(bool read_pass:{false,true}){
     if(numeric&&!read_pass)continue;
     Child target;target.start(argv[1]);Memory storage(target,regions,read_pass);
     NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};J events=J::array();
     NativeTu15RetainedStoreObserver observer;
     if(!read_pass)observer=NativeTu15RetainedStoreObserver{[&](const auto&e){
      events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
      __m128 x=_mm_set_ss(1.f),y=_mm_set_ss(3.f);asm volatile("divss %1,%0":"+x"(x):"x"(y):"memory");
      std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));cw^=0xc00u;asm volatile("fldcw %0"::"m"(cw):"memory");
      _mm_setcsr(_mm_getcsr()^0x6000u);const double three=3.;asm volatile("fld1;fdivl %0;fstp %%st(0)"::"m"(three):"memory","st");
     }};
     NativeTu15PairDeletionNativeScope scope;NativeTu15ContactProducerBindingActivity activity;
     std::string error;
     try{native_tu15_produce_bound_contacts(storage,{input.at("agent")},{input.at("first")},
      {input.at("second")},{input.at("configuration")},{input.at("output")},
      {0x74000000u,0x10000u},fp,0,{},scope,&activity,observer);}
     catch(const std::exception&e){error=e.what();}
     std::uint64_t compared{},different{};J differences=J::array();
     for(const auto&bank:meta.at("regions")){
      const auto p=bank.at("address").get<std::uint32_t>(),n=bank.at("size").get<unsigned>();
      const auto wanted=source.read(p,n),actual=target.read(p,n);compared+=n;
      for(unsigned i=0;i<n;++i)if(wanted[i]!=actual[i]){++different;if(differences.size()<16)
       differences.push_back(J{{"address",p+i},{"original",std::to_integer<unsigned>(wanted[i])},{"native",std::to_integer<unsigned>(actual[i])}});}
     }
     const bool fp_equal=fp_words(after)==J::array({fp.control_word,fp.status,fp.mxcsr});
     const auto&actual=read_pass?storage.reads:events;const auto&wanted=read_pass?expected.reads:expected.stores;
     const bool ordered_equal=actual==wanted;
     const bool finished=error.empty()&&activity.attempts==1&&activity.completed==1&&!activity.refusals&&
      !activity.unprovided_reads&&!scope.active()&&!activity.providers.native_only.refused;
     const bool pass=numeric?(finished&&(different||!fp_equal)):
      kind=="wrong_store_pc"?(finished&&!different&&fp_equal&&(read_pass?ordered_equal:!ordered_equal)):
      (finished&&!different&&fp_equal&&ordered_equal);
     J result{{"passed",pass},{"read_pass",read_pass},{"native_completed",finished},
      {"native_error",error},{"compared_bytes",compared},{"different_bytes",different},{"differences",differences},
      {"fp_equal",fp_equal},{"expected_fp",fp_words(after)},{"native_fp",J::array({fp.control_word,fp.status,fp.mxcsr})},
      {"ordered_equal",ordered_equal},{"ordered_difference",difference(wanted,actual)},
      {"observer_events",events.size()},{"unprovided_reads",activity.unprovided_reads}};
     row["passes"].push_back(result);accepted=accepted&&pass;
    }
    report["profiles"].push_back(row);
   }
   report["passed"]=accepted;report["rejection_kind"]=numeric?"retained_DATA_or_FP":kind=="wrong_store_pc"?"observer_attribution_only":"baseline";
  }
 }catch(const std::exception&e){report["error"]=e.what();}
 if(argc==5){std::ofstream output(argv[3]);output<<report.dump(2)<<'\n';}
 return report.at("passed").get<bool>()?0:1;
}
