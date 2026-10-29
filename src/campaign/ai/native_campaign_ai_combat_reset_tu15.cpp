#include "halo2/native_campaign_ai_combat_reset_tu15.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8u);return v;}
void write(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t v,unsigned n=4){std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(i*8u));m.write(p,std::span(b).first(n));}
std::uint32_t row(std::uint32_t rows,std::uint32_t datum,std::uint32_t stride){return rows+(datum&0xffffu)*stride;}
std::int16_t signed16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}

//266490: retire the actual query and clear only the first matching cached
// actor reference, before applying the engine's perception category policy.
void retire_query(SourcePhysicsContactMemory&m,std::uint32_t actor,std::uint32_t perception){
    const auto queries=read(m,0x50eeecu),properties=read(m,0x50eef0u),rows=read(m,properties+0x44u);
    const auto property=row(rows,perception,0x3cu),query=read(m,property+0x14u);
    native_tu15_data_array_delete(m,queries,query);
    const auto actors=read(m,0x5020c8u);write(m,property+0x14u,none);
    const auto actor_rows=read(m,actors+0x44u),record=row(actor_rows,actor,0x888u);
    for(unsigned i=0;i<8;++i)if(read(m,record+0x5cu+i*4u)==perception){write(m,record+0x5cu+i*4u,none);break;}
    const auto kind=signed16(read(m,property+0x18u,2));
    const auto policy=read(m,0x47d638u+std::uint32_t(std::int32_t(kind))*16u,2);
    if(policy==2u){write(m,property+0x24u,0,2);return;}
    const auto state=signed16(read(m,property+0x24u,2));
    if(state>=3||state==2)write(m,property+0x24u,1,2);
}

void reset_perceptions(SourcePhysicsContactMemory&m,std::uint32_t actor){
    const auto actors=read(m,0x5020c8u),actor_rows=read(m,actors+0x44u),properties=read(m,0x50eef0u);
    auto perception=read(m,row(actor_rows,actor,0x888u)+0x58u);
    while(perception!=none){
        const auto rows=read(m,properties+0x44u),property=row(rows,perception,0x3cu);
        const auto state=signed16(read(m,property+0x24u,2));const auto current=perception;
        perception=read(m,property+0x2cu);
        if(state<3)continue;
        const auto query=read(m,property+0x14u);
        if(query!=none)retire_query(m,actor,current);
        write(m,property+0x24u,0,2);write(m,property+0x10u,0);
    }
}

void mark_queries(SourcePhysicsContactMemory&m,std::uint32_t actor){
    const auto actors=read(m,0x5020c8u),actor_rows=read(m,actors+0x44u);
    auto perception=read(m,row(actor_rows,actor,0x888u)+0x58u);
    const auto properties=read(m,0x50eef0u),queries=read(m,0x50eeecu);
    while(perception!=none){
        const auto rows=read(m,properties+0x44u),property=row(rows,perception,0x3cu);
        const auto state=signed16(read(m,property+0x24u,2));
        perception=read(m,property+0x2cu);
        if(state<3)continue;
        const auto query=read(m,property+0x14u);if(query==none)continue;
        const auto query_rows=read(m,queries+0x44u),record=row(query_rows,query,0x124u);
        if(record==0||record+0x70u==0)continue;
        write(m,record+0xd9u,1,1);
    }
}
}
void native_campaign_ai_combat_reset_tu15(SourcePhysicsContactMemory&m,std::uint32_t actor,std::int16_t mode){
    if(mode>3)return;
    const auto actors=read(m,0x5020c8u),rows=read(m,actors+0x44u),record=row(rows,actor,0x888u);
    if(mode==2)mark_queries(m,actor);
    else if(mode>=0&&mode<=1)reset_perceptions(m,actor);
    write(m,record+0x86u,std::bit_cast<std::uint16_t>(mode),2);write(m,record+0x88u,0);
}
void native_campaign_ai_tracking_retire_tu15(SourcePhysicsContactMemory&m,
        std::uint32_t actor,std::uint32_t perception){retire_query(m,actor,perception);}
}
