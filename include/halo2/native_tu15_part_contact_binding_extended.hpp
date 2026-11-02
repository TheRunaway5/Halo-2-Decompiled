#pragma once
#include "halo2/native_tu15_function_evaluation_extended.hpp"
#include "halo2/native_tu15_part_contact_response.hpp"
namespace halo2 {
struct NativeTu15PartContactExtendedBindingActivity {
  std::uint64_t attempts{},completed{},refusals{};
  NativeTu15PartContactResponseStats response;
  NativeTu15FunctionEvaluationExtendedStats function;
};
// The existing PART response and material/shader owners are unchanged. This
// independently qualified builder supplies the extended native evaluator.
inline void native_tu15_update_bound_part_contact_response_extended(
    SourcePhysicsContactMemory&memory,std::uint32_t slot,std::int32_t part,
    std::uint32_t component,NativeTu15PartContactResponseScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned incoming_x87_depth,
    const NativeTu15ArrayGrowthServices&allocation,
    const SourcePhysicsContactRecordObserver&observer={},
    NativeTu15PartContactExtendedBindingActivity*activity=nullptr){
  NativeTu15PartContactExtendedBindingActivity local;auto&result=activity?*activity:local;
  ++result.attempts;
  try{
    NativeTu15ShaderFunctionEvaluationServices services;
    services.evaluate=[&](auto reference,auto first,auto second,auto&state,auto depth){
      return native_tu15_evaluate_authored_function_extended(memory,reference,
        first,second,state,depth,&result.function);
    };
    native_tu15_update_part_contact_response(memory,slot,part,component,scratch,
      fp,incoming_x87_depth,allocation,services,observer,&result.response);
    ++result.completed;
  }catch(...){++result.refusals;throw;}
}
} // namespace halo2
