#include "halo2/source_prt_contact.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
SourcePrtContactPoint source_prt_contact_short_point(const std::array<std::int16_t,5>& raw) {
    SourcePrtContactPoint out;out.cluster=float(raw[0]);
    constexpr float midpoint=std::bit_cast<float>(0x37800080u);
    for(unsigned i=0;i<4;++i)out.weights[i]=(float(raw[i+1])*2.f+1.f)*midpoint;
    return out;
}
std::optional<SourcePrtContactSample> source_prt_contact_sample(
    std::span<const SourcePrtContactCacheRecord> records,std::int32_t active_count,
    std::span<const std::byte> arena,std::uint32_t owner,std::uint32_t section,
    std::int32_t palette,const std::array<SourcePrtContactPoint,3>& points,
    float fraction0,float fraction1) {
    if(active_count<=0)return {};
    if(active_count>32767 || std::size_t(active_count)>records.size())
        throw FormatError("current source PRT cache prefix is incomplete");
    const SourcePrtContactCacheRecord* found=nullptr;
    for(std::int32_t i=0;i<active_count;++i)if(records[i].words[0]==owner){found=&records[i];break;}
    if(!found)return {};
    if(section>=16)throw FormatError("source PRT contact section escapes record");
    const auto& row=found->words;
    if(std::int32_t(row[2])<0)throw FormatError("source PRT contact arena handle is unavailable");
    const auto count=(row[19+section/4]>>((section&3)*8))&255u;
    if(palette>std::int32_t(count)-1)palette=0;
    // Source address arithmetic wraps at32 bits before dereferencing.
    const std::uint32_t base=(row[2]&0x0fffffffu)+row[3+section]*4u+std::uint32_t(palette)*640u;
    const Bytes bytes(arena);
    const auto load=[&](std::uint32_t offset){return std::bit_cast<float>(bytes.u32(offset));};
    std::array<std::array<float,3>,3> corners{};
    for(unsigned i=0;i<3;++i) {
        const auto cluster=points[i].cluster;
        if(!std::isfinite(cluster) || double(cluster)<-2147483648. || double(cluster)>=2147483648.)
            throw FormatError("source PRT contact cluster has no supported arena address");
        const auto address=base+std::uint32_t(std::int32_t(cluster))*64u;
        const auto& w=points[i].weights;
        for(unsigned c=0;c<3;++c) {
            const auto at=address+(4u+c*4u)*4u;
            float value=load(at+8)*w[2]+load(at)*w[0];
            value=value+load(at+12)*w[3];value=value+load(at+4)*w[1];
            corners[i][c]=value+load(address+c*4u);
        }
    }
    std::array<float,3> color{};
    for(unsigned c=0;c<3;++c) {
        const float value=(corners[1][c]-corners[0][c])*fraction0+
            (corners[2][c]-corners[0][c])*fraction1+corners[0][c];
        color[c]=value>1.f?1.f:value<0.f?0.f:value;
    }
    constexpr float incident_scale=std::bit_cast<float>(0x3f4ccccdu);
    return SourcePrtContactSample{{std::bit_cast<float>(row[23])*incident_scale,
        std::bit_cast<float>(row[24])*incident_scale,std::bit_cast<float>(row[25])*incident_scale},
        {color[0],color[1],color[2]}};
}
}
