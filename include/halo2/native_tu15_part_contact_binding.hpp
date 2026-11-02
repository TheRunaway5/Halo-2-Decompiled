#pragma once
#include "halo2/native_tu15_function_evaluation.hpp"
#include "halo2/native_tu15_part_contact_response.hpp"

namespace halo2 {
struct NativeTu15PartContactBindingActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15PartContactResponseStats response;
    NativeTu15FunctionEvaluationStats function;
};
// Same production service construction for point callbacks and direct proofs.
// The bounded native evaluator owns its source-kind admission and refuses
// unsupported kinds; no original executor or supplied result is available.
inline void native_tu15_update_bound_part_contact_response(
    SourcePhysicsContactMemory& memory,std::uint32_t slot,std::int32_t part,
    std::uint32_t component,NativeTu15PartContactResponseScratch scratch,
    NativeTu15PhysicsFrameFp& fp,unsigned incoming_x87_depth,
    const NativeTu15ArrayGrowthServices& allocation,
    const SourcePhysicsContactRecordObserver& observer={},
    NativeTu15PartContactBindingActivity* activity=nullptr) {
    NativeTu15PartContactBindingActivity local;auto& result=activity?*activity:local;
    ++result.attempts;
    try {
        NativeTu15ShaderFunctionEvaluationServices services;
        services.evaluate=[&](auto reference,auto first,auto second,auto& state,auto depth) {
            return native_tu15_evaluate_authored_function(memory,reference,first,
                second,state,depth,&result.function);
        };
        native_tu15_update_part_contact_response(memory,slot,part,component,
            scratch,fp,incoming_x87_depth,allocation,services,observer,&result.response);
        ++result.completed;
    }catch(...){++result.refusals;throw;}
}
} // namespace halo2
