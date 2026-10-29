#include "halo2/native_campaign_ai_perception_group_link_tu15.hpp"
#include "halo2/native_campaign_ai_perception_classify_tu15.hpp"
#include "halo2/native_campaign_ai_perception_group_storage_tu15.hpp"
#include "halo2/native_campaign_ai_perception_group_actor_tu15.hpp"
#include "halo2/native_campaign_ai_perception_group_pressure_tu15.hpp"
#include <array>
#include <atomic>
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;std::atomic<std::uint64_t>entries{};
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(b[k])<<(k*8u);return v;}
void write(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t v,unsigned n=4){std::array<std::byte,4>b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(v>>(k*8u));m.write(p,std::span(b).first(n));}
std::uint32_t create_group(SourcePhysicsContactMemory&m,std::uint32_t object,
        std::uint32_t clump,NativeTu15PhysicsFrameFp&fp){
    const auto generation=read(m,0x4eabccu);write(m,0x4eabccu,generation+1u);write(m,0x4eabc8u,1,1);
    const auto classified=native_campaign_ai_perception_classify_new_group_tu15(m,object,clump,fp);
    write(m,0x4eabc8u,0,1);
    if(!classified)throw std::runtime_error("Native perception group creation requires the unwritten original classifier output at 274484");
    return native_campaign_ai_perception_group_construct_tu15(m,clump,
        NativeCampaignAiPerceptionGroupRequestTu15{std::bit_cast<std::int16_t>(classified->kind),classified->object,classified->priority},fp);
}
}
std::uint32_t native_campaign_ai_perception_group_link_tu15(SourcePhysicsContactMemory&m,
        std::uint32_t object,std::uint32_t actor,std::uint16_t priority,NativeTu15PhysicsFrameFp&fp,unsigned occupied_x87){
    entries.fetch_add(1,std::memory_order_relaxed);
    const auto actors=read(m,0x5020c8u),actor_rows=read(m,actors+0x44u);
    const auto clump=read(m,actor_rows+(actor&0xffffu)*0x888u+0x7cu);
    if(clump==none)return none;
    const auto clumps=read(m,0x50eef8u),clump_rows=read(m,clumps+0x44u),record=clump_rows+(clump&0xffffu)*0x50u;
    auto group=native_campaign_ai_perception_group_find_tu15(m,clump,object);
    if(group==none){
        group=create_group(m,object,record,fp);if(group==none)return none;
        const auto groups=read(m,0x50eef4u),rows=read(m,groups+0x44u);
        write(m,rows+(group&0xffffu)*0xc4u+0xcu,priority,2);
        native_campaign_ai_perception_group_link_actors_tu15(m,record,group,fp,occupied_x87);
    }
    const auto groups=read(m,0x50eef4u),group_rows=read(m,groups+0x44u),properties=read(m,0x50eef0u);
    auto property=read(m,group_rows+(group&0xffffu)*0xc4u+0x18u);
    while(property!=none){
        const auto rows=read(m,properties+0x44u),member=rows+(property&0xffffu)*0x3cu;
        const auto owner=read(m,member+4u),current=property;
        property=read(m,member+0x34u);
        if(owner==actor)return current;
    }
    return none;
}
std::uint64_t native_campaign_ai_perception_group_link_activity_tu15(){return entries.load(std::memory_order_relaxed);}
}
