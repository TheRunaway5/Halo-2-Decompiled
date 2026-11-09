#pragma once
#include "halo2/native_tu15_node_resource_kernel.hpp"
namespace halo2 {
enum class NativeTu15AnimationWaitPhase {save,job,ready,promoted_ready,recheck_ready,yielded_ready,clear,finished};
struct NativeTu15AnimationWait {std::uint32_t tracking{},job{},ready{};bool waited{},absent{};};
// A genuine request's ready byte owns completion. Promotion/yield are actual
// caller services; no iteration budget or synthetic completion is introduced.
template<class Memory,class Owners,class Progress>NativeTu15AnimationWait
native_tu15_wait_animation_resource(const Memory&memory,std::uint32_t tracking,Owners&owners,Progress progress){
    using P=NativeTu15AnimationWaitPhase;NativeTu15AnimationWait data;data.tracking=tracking;progress(P::save,data);
    progress(P::job,data);data.job=native_node_resource_value(memory,data.tracking+8u);data.absent=data.job==0xffffffffu;
    const auto ready=[&](P phase){progress(phase,data);return native_node_resource_value(memory,data.tracking+4u,1);};
    if(!data.absent){
        data.ready=ready(P::ready);
        if(!data.ready){
            data.tracking=owners.promote(data);data.ready=ready(P::promoted_ready);
            if(!data.ready)data.ready=ready(P::recheck_ready);
            while(!data.ready){data.tracking=owners.yield(data);data.ready=ready(P::yielded_ready);}
            data.waited=true;
        }
        progress(P::clear,data);owners.store(data.tracking+8u,0xffffffffu);
    }
    progress(P::finished,data);return data;
}
} // namespace halo2
