#include "halo2/source_live_shadow_selection_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_live_model_draws_tu15.hpp"
#include "halo2/source_geometry_lod.hpp"
#include <bit>
#include <cstring>
#include <cmath>

namespace halo2 {
namespace {
void main_queues(PhysicsLiveWorldTu15& world,const SourceBspAdmission& bsp,
    const SourceObjectAdmission& main,bool publish) {
    auto& memory=world.memory();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto put=[&](std::uint32_t p,const auto& value){world.write_source_call_input(p,std::as_bytes(std::span(&value,1)));};
    struct Queue {
        std::uint32_t header;
        std::span<const SourceBspAdmissionRecord> rows;
        std::array<std::uint32_t,4> streams;
    };
    std::array<Queue,3> queues{{{word(0x556e24u),bsp.clusters,{}},
        {word(0x556e2cu),main.objects,{}},{word(0x556e30u),bsp.instances,{}}}};
    // Validate every original allocation and full live object identity before
    // publishing any queue. Retain authored order and all four real columns.
    for(auto& queue:queues) {
        if(!queue.header)throw FormatError("TU1.5 main queue has no original storage owner");
        const auto slots=Bytes(memory.view(queue.header,24)).u16(0);
        if(!slots || slots>32767 || queue.rows.size()>slots)
            throw FormatError("TU1.5 completed main queue exceeds its original allocation");
        for(unsigned n=0;n<queue.streams.size();++n)
            queue.streams[n]=word(queue.header+8u+n*4u);
        if(!queue.rows.empty())for(unsigned n=0;n<queue.streams.size();++n) {
            if(!queue.streams[n])throw FormatError("TU1.5 completed main queue lacks its four original streams");
            (void)memory.view(queue.streams[n],queue.rows.size()*(n==0 || n==3?2u:4u));
        }
    }
    for(const auto& row:main.objects)(void)world.object_body(row.id);
    for(const auto& queue:queues) {
        const auto& stream=queue.streams;
        if(!publish && Bytes(memory.view(queue.header+4u,2)).u16(0)!=queue.rows.size())
            throw FormatError("TU1.5 shadow selection lost its completed main queue count");
        for(unsigned n=0;n<queue.rows.size();++n) {
            const auto& row=queue.rows[n];
            if(publish) {
                put(stream[0]+n*2u,row.flags);put(stream[1]+n*4u,row.id);
                put(stream[2]+n*4u,row.other);put(stream[3]+n*2u,row.association);
            } else if(Bytes(memory.view(stream[0]+n*2u,2)).u16(0)!=row.flags ||
                      word(stream[1]+n*4u)!=row.id || word(stream[2]+n*4u)!=row.other ||
                      Bytes(memory.view(stream[3]+n*2u,2)).u16(0)!=row.association)
                throw FormatError("TU1.5 shadow selection lost its completed main queue columns");
        }
        if(publish)put(queue.header+4u,std::int16_t(queue.rows.size()));
    }
}
}
void source_live_shadow_main_queues_tu15(PhysicsLiveWorldTu15& world,
    const SourceBspAdmission& bsp,const SourceObjectAdmission& main) {
    main_queues(world,bsp,main,true);
}
SourceLiveShadowSelectionTu15 source_live_shadow_selection_tu15(
    PhysicsLiveWorldTu15& world,const SourceBspAdmission& bsp_main,const SourceObjectAdmission& main,
    const SourceLiveModelDrawContextTu15& context,std::int32_t local_user,
    const SourceShadowSelectionObserverTu15& observe) {
    if(context.entry.mode || context.draw.draw.mode || context.view>=4 ||
       local_user< -1 || local_user>=4 || !context.local_players || context.local_players>4)
        throw FormatError("TU1.5 shadow selection requires its completed ordinary main view");
    auto& memory=world.memory();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto put=[&](std::uint32_t p,const auto& value){world.write_source_call_input(p,std::as_bytes(std::span(&value,1)));};
    const auto queue=word(0x556e2cu);
    if(!queue)throw FormatError("TU1.5 shadow main queue has no original storage owner");
    const auto capacity=Bytes(memory.view(queue,16)).u16(0);
    if(!capacity || capacity>32767 || main.objects.size()>capacity)
        throw FormatError("TU1.5 completed shadow main queue exceeds actual source capacity");
    main_queues(world,bsp_main,main,false);
    // The actual2BA caller owners ran before ordinary native collection.
    // Selection must retain their outputs, not repeat their frame scheduling.
    if(std::memcmp(memory.view(0x4c65f0u,sizeof(context.draw.viewport)).data(),
                   context.draw.viewport.data(),sizeof(context.draw.viewport))!=0)
        throw FormatError("TU1.5 shadow selection lost its actual main viewport owner");
    if(word(0x4c66f8u)!=std::bit_cast<std::uint32_t>(local_user))
        throw FormatError("TU1.5 shadow selection lost its completed main local-user owner");
    const auto policy=source_geometry_lod_policy(context.local_players);
    if(std::memcmp(memory.view(0x4c6840u,policy.feature_bytes.size()).data(),
                   policy.feature_bytes.data(),policy.feature_bytes.size())!=0 ||
       word(0x4c6848u)!=std::bit_cast<std::uint32_t>(policy.coverage_multiplier) ||
       word(0x4c684cu)!=std::bit_cast<std::uint32_t>(policy.secondary_scale) ||
       word(0x473644u)!=std::bit_cast<std::uint32_t>(policy.effect_scale))
        throw FormatError("TU1.5 shadow selection lost its complete player policy owner");
    SourceLiveShadowSelectionTu15 result;
    result.view=context.view;result.frame=word(0x4c6854u);result.queue=queue;result.queue_capacity=capacity;
    result.history_address=0x5332c8u+context.view*0x484u;
    static_assert(sizeof(SourceShadowSelectionCache)==0x484);
    std::memcpy(&result.history_before,memory.view(result.history_address,0x484).data(),0x484);
    result.history_after=result.history_before;
    result.fixed_object_shadows_enabled=policy.feature_bytes[0]!=0;
    result.lightmap_enabled=(world.call_actual(0x226b0u)&255u)!=0;
    result.cinematic_enabled=(world.call_actual(0x335b0u)&255u)!=0;
    if(!result.lightmap_enabled || !result.cinematic_enabled)return result;
    auto abi=world.prepare_source_call();
    //335D0's actual640-byte uncleared caller scratch. Retain its existing
    // fourth flag bytes; never initialize a synthetic candidate flag bank.
    const auto return_address=word(abi.gp[4]);
    const auto output=abi.gp[4]-0x280u;
    //335D0 reserves the output before pushing three saved registers and
    // two arguments.333F0 runs below the output, never over its own locals.
    abi.gp[4]=output-24u;
    put(abi.gp[4],return_address);
    put(abi.gp[4]+4u,output);put(abi.gp[4]+8u,context.entry.mode);
    abi.gp[3]=queue;
    if(observe)observe(true,abi,memory);
    world.run_actual(0x333f0u,abi);
    if(observe)observe(false,abi,memory);
    if(abi.gp[0]>32)throw FormatError("TU1.5 shadow selection exceeds its original32-root bank");
    result.selected.resize(abi.gp[0]);
    if(!result.selected.empty())std::memcpy(result.selected.data(),
        memory.view(output,result.selected.size()*sizeof(SourceShadowCandidate)).data(),
        result.selected.size()*sizeof(SourceShadowCandidate));
    std::memcpy(&result.history_after,memory.view(result.history_address,0x484).data(),0x484);
    return result;
}
}
