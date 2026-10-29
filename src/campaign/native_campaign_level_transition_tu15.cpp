#include "halo2/native_campaign_level_transition_tu15.hpp"
#include <array>
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t at){
    std::array<std::byte,4>b{};m.read(at,b);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::uint32_t(std::to_integer<unsigned>(b[n]))<<(n*8u);
    return value;
}
class RetainedPaths {
    const SourcePhysicsContactMemory&m_;std::uint32_t scenario_,globals_;
    std::uint32_t header()const{
        if(scenario_&&globals_&&read(m_,globals_+0x170u))return read(m_,globals_+0x174u);
        return 0;
    }
public:
    explicit RetainedPaths(const SourcePhysicsContactMemory&m):m_(m),
        scenario_(read(m,0x4ecc20u)),globals_(read(m,0x4ecc1cu)){}
    std::int32_t count()const{
        const auto table=header();return table?std::bit_cast<std::int32_t>(read(m_,table)):0;
    }
    std::uint32_t row(std::uint32_t index)const{
        const auto table=header();
        if(table&&std::bit_cast<std::int32_t>(read(m_,table))>0)return read(m_,table+4u)+index*0x108u;
        return 0;
    }
    std::int32_t campaign(std::uint32_t row)const{return std::bit_cast<std::int32_t>(read(m_,row));}
    std::int32_t map(std::uint32_t row)const{return std::bit_cast<std::int32_t>(read(m_,row+4u));}
};
class CatalogPaths {
    std::span<const NativeCampaignLevelPathTu15>rows_;
public:
    explicit CatalogPaths(std::span<const NativeCampaignLevelPathTu15>rows):rows_(rows){
        if(rows.size()>0x7fffffffu)throw std::invalid_argument("Native campaign catalog exceeds signed source count");
    }
    std::int32_t count()const{return std::int32_t(rows_.size());}
    const NativeCampaignLevelPathTu15*row(std::uint32_t index)const{return &rows_[index];}
    std::int32_t campaign(const NativeCampaignLevelPathTu15*row)const{return row->campaign;}
    std::int32_t map(const NativeCampaignLevelPathTu15*row)const{return row->map;}
};
template<class Paths>std::int32_t next(const Paths&paths,std::int32_t campaign,std::int32_t current){
    const auto count=paths.count();std::int32_t selected=-1;
    for(std::int32_t index=0;index<count;++index){
        const auto row=paths.row(std::uint32_t(index));
        if(paths.campaign(row)!=campaign)continue;
        const auto map=paths.map(row);
        if(map>current&&(selected==-1||map<selected))selected=map;
    }
    return selected;
}
template<class Paths>auto find(const Paths&paths,std::int32_t campaign,std::int32_t map){
    using Result=decltype(paths.row(0));const auto count=paths.count();
    for(std::int32_t index=0;index<count;++index){
        const auto row=paths.row(std::uint32_t(index));
        if(paths.map(row)==map&&paths.campaign(row)==campaign)return row;
    }
    return Result{};
}
}
std::int32_t native_campaign_next_level_key_tu15(const SourcePhysicsContactMemory&m,std::int32_t campaign,std::int32_t current){return next(RetainedPaths(m),campaign,current);}
std::uint32_t native_campaign_level_path_record_tu15(const SourcePhysicsContactMemory&m,std::int32_t campaign,std::int32_t map){return find(RetainedPaths(m),campaign,map);}
std::int32_t native_campaign_next_level_key_tu15(std::span<const NativeCampaignLevelPathTu15>rows,std::int32_t campaign,std::int32_t current){return next(CatalogPaths(rows),campaign,current);}
const NativeCampaignLevelPathTu15*native_campaign_level_path_tu15(std::span<const NativeCampaignLevelPathTu15>rows,std::int32_t campaign,std::int32_t map){return find(CatalogPaths(rows),campaign,map);}
}
