#pragma once
#include "halo2/native_tu15_activation_payload_filter.hpp"
#include <stdexcept>
#include <string>
namespace halo2 {
struct NativeTu15DiscretePairBindingActivity {
    std::uint64_t attempts{},completed{},refusals{},unbound_filters{},unbound_motion_types{};
    NativeTu15ActivationServiceStats services;
    NativeTu15ActivationPayloadFilterStats payload;
};
// Actual281530 through native281F10/1D7530 and authenticated motion getters.
// Unknown selected owners refuse before execution. The returned value is the
// caller's result-byte identity; its byte is computed from current source DATA.
inline std::uint32_t native_tu15_test_bound_discrete_pair(SourcePhysicsContactMemory&memory,
    std::uint32_t dispatcher,std::uint32_t result_byte,NativeTu15ContactPayload first,
    NativeTu15ContactPayload second,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15DiscretePairBindingActivity*activity=nullptr){
    NativeTu15DiscretePairBindingActivity local;auto&out=activity?*activity:local;++out.attempts;
    NativeTu15ActivationDiscreteServices services;
    services.test_filter=[&](auto,auto owner,auto a,auto b,auto&state){
        if(owner.identity!=0x281f10u){
            ++out.unbound_filters;
            throw std::runtime_error("Native TU1.5 discrete pair requires filter owner="+std::to_string(owner.identity));
        }
        NativeTu15ActivationPayloadFilterServices payload;
        payload.motion_kind=[&](auto,auto owner,auto&)->std::uint32_t{
            ++out.unbound_motion_types;
            throw std::runtime_error("Native TU1.5 discrete pair requires motion owner="+std::to_string(owner.identity));
        };
        return native_tu15_activation_payload_filter(memory,a,b,state,payload,&out.payload);
    };
    try{
        const auto result=native_tu15_activation_discrete_pair(memory,dispatcher,result_byte,
            first,second,fp,services,observe,&out.services);
        ++out.completed;return result;
    }catch(...){++out.refusals;throw;}
}
}
