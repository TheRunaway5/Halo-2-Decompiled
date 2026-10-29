#include "halo2/native_campaign_ai_perception_group_initialize_tu15.hpp"
#include "halo2/native_campaign_ai_perception_group_storage_tu15.hpp"
#include "halo2/native_tu15_data_array_iteration.hpp"
#include <array>
#include <bit>
#include <sstream>
#include <stdexcept>
namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t at,unsigned width=4){std::array<std::byte,4>b{};m.read(at,std::span(b).first(width));std::uint32_t v{};for(unsigned n=0;n<width;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8u);return v;}
void write(SourcePhysicsContactMemory &m,std::uint32_t at,std::uint32_t v,unsigned width=4){std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(v>>(n*8u));m.write(at,std::span(b).first(width));}
std::int16_t signed16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
}
bool native_campaign_ai_perception_team_relation_tu15(const SourcePhysicsContactMemory &m,std::int16_t first,std::int16_t second){
    if(first==-1||second==-1)return true;
    const auto game=read(m,0x4f3218u),mode=read(m,game+8u);
    if(mode==1u){
        if(first<0||first>=16||second<0||second>=16)return true;
        const auto ai=read(m,0x5020c4u);
        return !native_tu15_bit_array_test(m,ai+0xc4u,std::int32_t(first)*16+second);
    }
    if(mode!=2u)return first!=second;
    // Actual160A60: the optional current engine object supplies virtual+6C.
    const auto engines=read(m,0x4f63b8u),index=read(m,engines+0xc14u);
    const auto object=read(m,0x56dfa0u+index*4u);
    if(!object)return false;
    const auto table=read(m,object),callback=read(m,table+0x6cu);
    std::ostringstream why;why<<"Native perception team relation missing multiplayer callback at 160a7b -> "<<std::hex<<callback;
    throw std::runtime_error(why.str());
}
bool native_campaign_ai_perception_team_secondary_relation_tu15(const SourcePhysicsContactMemory &m,std::int16_t first,std::int16_t second){
    if(first==-1||second==-1||first==second)return false;
    const auto game=read(m,0x4f3218u);
    if(read(m,game+8u)!=1u||first<0||first>=16||second<0||second>=16)return false;
    const auto ai=read(m,0x5020c4u);
    const auto index=std::int32_t(first)*16+second;
    if(!native_tu15_bit_array_test(m,ai+0xa4u,index))return false;
    return !native_tu15_bit_array_test(m,ai+0xc4u,index);
}
void native_campaign_ai_perception_group_initialize_tu15(SourcePhysicsContactMemory &m,
        std::uint32_t record,std::uint32_t object,std::uint32_t clump,std::int16_t kind,std::uint16_t priority){
    write(m,record+2u,std::uint16_t(kind),2);write(m,record+8u,object);write(m,record+0x14u,none);write(m,record+0xcu,priority,2);
    const auto policy=read(m,0x47d634u+std::uint32_t(std::int32_t(kind))*16u,2);
    const auto objects=read(m,0x4ecbd0u);
    write(m,record+0x10u,none);write(m,record+0x18u,none);write(m,record+4u,policy==0u,2);
    const auto rows=read(m,objects+0x44u),body=read(m,rows+(object&0xffffu)*12u+8u);
    write(m,body+0xb2u,read(m,body+0xb2u,1)|2u,1);
    const auto mask=1u<<(read(m,body+0xaau,1)&31u);
    bool clear_attachment=true;
    if(mask&3u){
        write(m,record+0x1cu,read(m,body+0x12cu));
        const auto team=signed16(read(m,body+0x138u,2));write(m,record+0x20u,std::uint16_t(team),2);
        const auto clump_team=signed16(read(m,clump+0x12u,2));
        const bool relation=native_campaign_ai_perception_team_relation_tu15(m,clump_team,team);
        write(m,record+0x23u,relation,1);write(m,record+0x24u,!relation,1);
        write(m,record+0x25u,read(m,body+0x13cu)!=none,1);
    }else{
        const auto attachment=(mask&0x1000u)?read(m,body+0x134u):1u;
        if((mask&0x1000u)&&attachment==0u){
            const auto extension=body+std::uint32_t(std::int32_t(signed16(read(m,body+0x13au,2))));
            write(m,record+0x1cu,read(m,extension+4u));
            const auto team=signed16(read(m,body+0x12eu,2));write(m,record+0x20u,std::uint16_t(team),2);
            const auto clump_team=signed16(read(m,clump+0x12u,2));
            const bool relation=native_campaign_ai_perception_team_relation_tu15(m,clump_team,team);
            write(m,record+0x23u,relation,1);write(m,record+0x24u,!relation,1);write(m,record+0x25u,0,1);
            write(m,record+0x22u,read(m,extension+8u)!=none,1);clear_attachment=false;
        }else if(!(mask&0x1000u)||attachment==1u){
            write(m,record+0x1cu,none);write(m,record+0x20u,none,2);
            for(const auto offset:{0x23u,0x24u,0x25u})write(m,record+offset,0,1);
        }else clear_attachment=false;
    }
    if(clear_attachment)write(m,record+0x22u,0,1);
    native_campaign_ai_perception_point_reset_tu15(m,record+0x58u);
    write(m,record+0x54u,none,2);write(m,record+0x56u,none,2);write(m,record+0x38u,none);
    write(m,record+0x2cu,0);write(m,record+0x28u,0);
    for(const auto offset:{0x36u,0x53u,0x52u,0x3cu,0x30u,0x32u,0x33u,0x34u,0x35u})write(m,record+offset,0,1);
}
}
