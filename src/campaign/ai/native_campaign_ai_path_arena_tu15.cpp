#include "halo2/native_campaign_ai_path_arena_tu15.hpp"
#include "halo2/native_campaign_ai_resource_retirement_tu15.hpp"
#include <array>

namespace halo2 {
NativeCampaignAiPathArenaRequiredOwnerTu15::NativeCampaignAiPathArenaRequiredOwnerTu15(
    std::uint32_t entry,std::uint32_t identity):
    std::runtime_error("Native TU1.5 path arena requires an unconverted allocator lifetime owner"),
    entry_(entry),identity_(identity){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
void retire_allocator(SourcePhysicsContactMemory &memory,std::uint32_t allocator,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto vtable=read(memory,allocator),destructor=read(memory,vtable+0x20u);
    if(destructor!=0x233120u)throw NativeCampaignAiPathArenaRequiredOwnerTu15(destructor,allocator);
    // Both actual1498E0 sites pass delete=false.233120 then publishes only
    // the base vtable; it does not call the scalar-deleting32B479 branch.
    native_campaign_ai_path_allocator_retire_tu15(memory,allocator,observe);
}
}
void native_campaign_ai_path_allocator_retire_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t allocator,const SourcePhysicsContactRecordObserver &observe) {
    publish(memory,observe,0x233128,allocator,0x45f4a0u);
}
void native_campaign_ai_path_arena_restore_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    auto backing=read(memory,0x51df44u),bytes=0x452e8u;
    publish(memory,observe,0x14968e,0x51df48u,1,1);
    if(backing&15u){const auto padding=(0u-backing)&15u;backing+=padding;bytes-=padding;}
    const auto allocator=read(memory,0x48605cu)+0x105410u;
    if(!allocator){publish(memory,observe,0x1496e6,0x486070u,0);return;}
    publish(memory,observe,0x1496b3,allocator+4,1);
    publish(memory,observe,0x1496ba,allocator+8,0);
    publish(memory,observe,0x1496bd,allocator+0xc,0);
    publish(memory,observe,0x1496c0,allocator+0x10,0);
    publish(memory,observe,0x1496c3,allocator+0x14,0xffffffffu);
    bytes&=0x0ffffffcu;
    publish(memory,observe,0x1496d0,allocator+0x20,0);
    publish(memory,observe,0x1496d3,allocator+0x18,backing);
    publish(memory,observe,0x1496d6,allocator+0x1c,bytes);
    publish(memory,observe,0x1496d9,allocator,0x46393cu);
    publish(memory,observe,0x1496df,0x486070u,allocator);
}
namespace {
void retire_arena(SourcePhysicsContactMemory &memory,
    const NativeCampaignAiResourceProtectionTu15 *protection,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto primary=read(memory,0x486070u);
    if(primary) {
        retire_allocator(memory,primary,observe);
        publish(memory,observe,0x1498f3,0x486070u,0);
        publish(memory,observe,0x1498f9,0x51df48u,0,1);
    }
    const auto secondary=read(memory,0x486074u);
    if(secondary) {
        retire_allocator(memory,secondary,observe);
        const auto backing=read(memory,0x486060u);
        publish(memory,observe,0x149916,0x486074u,0);
        if(!protection)throw NativeCampaignAiPathArenaRequiredOwnerTu15(0x130030u,backing);
        native_campaign_ai_resource_backing_retire_tu15(memory,backing,*protection,observe);
        publish(memory,observe,0x149921,0x486060u,0);
    }
}
std::uint32_t acquire_workspace(SourcePhysicsContactMemory &memory,
    const NativeCampaignAiResourceProtectionTu15 *protection,
    const SourcePhysicsContactRecordObserver &observe) {
    if(!read(memory,0x52be58u,1)) {
        const bool had_allocator=read(memory,0x486070u)!=0;
        publish(memory,observe,0x1d2dac,0x52be54u,had_allocator);
        if(had_allocator)retire_arena(memory,protection,observe);
        auto backing=read(memory,0x51df44u);
        publish(memory,observe,0x1d2dbe,0x51df48u,1,1);
        for(unsigned n=0;n<2;++n) {
            const auto descriptor=0x50208cu+n*12u;
            publish(memory,observe,0x1d2dd0,descriptor+4,backing);
            publish(memory,observe,0x1d2dd3,descriptor,0,1);
            publish(memory,observe,0x1d2dd7,descriptor+8,0x22974u);
            backing+=0x22974u;
        }
        publish(memory,observe,0x1d2ded,0x52be58u,1,1);
    }
    for(unsigned n=0;n<2;++n) {
        const auto descriptor=0x50208cu+n*12u;
        if(!read(memory,descriptor,1)) {
            publish(memory,observe,0x1d2e1a,descriptor,1,1);
            return read(memory,descriptor+4);
        }
    }
    return 0;
}
}
void native_campaign_ai_path_arena_retire_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    retire_arena(memory,nullptr,observe);
}
void native_campaign_ai_path_arena_retire_tu15(SourcePhysicsContactMemory &memory,
    const NativeCampaignAiResourceProtectionTu15 &protection,
    const SourcePhysicsContactRecordObserver &observe) {
    retire_arena(memory,&protection,observe);
}
std::uint32_t native_campaign_ai_path_workspace_acquire_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    return acquire_workspace(memory,nullptr,observe);
}
std::uint32_t native_campaign_ai_path_workspace_acquire_tu15(SourcePhysicsContactMemory &memory,
    const NativeCampaignAiResourceProtectionTu15 &protection,
    const SourcePhysicsContactRecordObserver &observe) {
    return acquire_workspace(memory,&protection,observe);
}
void native_campaign_ai_path_workspace_release_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t workspace,const SourcePhysicsContactRecordObserver &observe) {
    unsigned remaining{};
    for(unsigned n=0;n<2;++n) {
        const auto descriptor=0x50208cu+n*12u;
        if(read(memory,descriptor,1)) {
            if(read(memory,descriptor+4)==workspace)publish(memory,observe,0x1d2e41,descriptor,0,1);
            else ++remaining;
        }
    }
    if(!remaining) {
        const auto restore=read(memory,0x52be54u);
        publish(memory,observe,0x1d2e5c,0x52be58u,0,1);
        publish(memory,observe,0x1d2e62,0x51df48u,0,1);
        if(restore)native_campaign_ai_path_arena_restore_tu15(memory,observe);
    }
}
}
