#include "halo2/native_campaign_scenario_lookup_tu15.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t at,unsigned n=4){
    std::array<std::byte,4>b{};m.read(at,std::span(b).first(n));std::uint32_t value{};
    for(unsigned k=0;k<n;++k)value|=std::uint32_t(std::to_integer<unsigned>(b[k]))<<(k*8u);
    return value;
}
std::string_view basename(std::string_view path){
    path=path.substr(0,path.find('\0'));
    const auto slash=path.find_last_of('\\');return slash==path.npos?path:path.substr(slash+1);
}
bool same(const SourcePhysicsContactMemory&m,std::uint32_t candidate,std::uint32_t input){
    for(std::uint32_t offset=0;;offset+=2u){
        const auto first=read(m,candidate+offset,1);
        if(first!=read(m,input+offset,1))return false;
        if(!first)return true;
        const auto second=read(m,candidate+offset+1u,1);
        if(second!=read(m,input+offset+1u,1))return false;
        if(!second)return true;
    }
}
}
std::uint32_t native_campaign_string_last_character_tu15(
    const SourcePhysicsContactMemory&m,std::uint32_t text,std::uint8_t character){
    auto cursor=text;
    while(read(m,cursor++,1)){}
    auto remaining=cursor-text;--cursor;
    do{
        const auto value=read(m,cursor--,1);--remaining;
        if(value==character)break;
    }while(remaining);
    ++cursor;
    return read(m,cursor,1)==character?cursor:0u;
}
std::uint32_t native_campaign_scenario_record_tu15(
    const SourcePhysicsContactMemory&m,std::uint32_t path){
    const auto scenario=read(m,0x4ecc20u);auto globals=read(m,0x4ecc1cu);
    auto original_header=0u;
    if(scenario&&globals&&read(m,globals+0x170u))original_header=read(m,globals+0x174u);
    if(!original_header||std::bit_cast<std::int32_t>(read(m,original_header))<=0)return 0;
    std::uint32_t index{},offset{};
    do{
        if(index)globals=read(m,0x4ecc1cu);
        const auto current_scenario=read(m,0x4ecc20u);auto header=0u;
        if(current_scenario&&globals&&read(m,globals+0x170u))header=read(m,globals+0x174u);
        auto row=0u;
        if(header&&std::bit_cast<std::int32_t>(read(m,header))>0)row=read(m,header+4u)+offset;
        const auto input_slash=native_campaign_string_last_character_tu15(m,path,'\\');
        const auto input=input_slash?input_slash+1u:path;
        const auto candidate_slash=native_campaign_string_last_character_tu15(m,row+8u,'\\');
        const auto candidate=candidate_slash?candidate_slash+1u:row+8u;
        if(same(m,candidate,input))return row;
        const auto count=std::bit_cast<std::int32_t>(read(m,original_header));
        ++index;offset+=0x108u;
        if(std::bit_cast<std::int32_t>(index)>=count)return 0;
    }while(true);
}
const NativeCampaignLevelPathTu15*native_campaign_scenario_record_tu15(
    std::span<const NativeCampaignLevelPathTu15>rows,std::string_view path){
    const auto input=basename(path);
    for(const auto&row:rows)if(basename(row.scenario_path)==input)return &row;
    return nullptr;
}
}
