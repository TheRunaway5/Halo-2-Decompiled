#include "halo2/native_campaign_ai_clump_callbacks_tu15.hpp"
#include <array>
#include <sstream>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &m,std::uint32_t p) {
    std::array<std::byte,4>b{};m.read(p,b);std::uint32_t v{};
    for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8u);
    return v;
}
void clear(SourcePhysicsContactMemory &m,std::uint32_t p) {
    constexpr std::array<std::byte,4>none{std::byte{0xff},std::byte{0xff},std::byte{0xff},std::byte{0xff}};m.write(p,none);
}
}
void native_campaign_ai_clump_reference_invalidation_tu15(
        SourcePhysicsContactMemory &m,const NativeCampaignAiReferenceInvalidationTu15 &r) {
    unsigned offset;
    switch(r.callback) {
    case 0x1c8e90u:offset=0xcu;break;
    case 0x1c75b0u:offset=0x10u;break;
    case 0x1c9750u:offset=0x1cu;break;
    case 0x1bda70u: {
        if(read(m,r.behavior_record+0x24u)==r.reference)clear(m,r.behavior_record+0x24u);
        const auto plan=read(m,r.behavior_record+0x10u);
        if(plan==0xffffffffu)return;
        const auto pool=read(m,0x50eefcu),rows=read(m,pool+0x44u),target=rows+(plan&0xffffu)*0xbcu+0x88u;
        if(read(m,target)==r.reference)clear(m,target);
        return;
    }
    default: {
        std::ostringstream why;why<<"Native AI clump retirement missing behavior invalidation at 1ea562 -> "<<std::hex<<r.callback;
        throw std::runtime_error(why.str());
    }
    }
    if(read(m,r.behavior_record+offset)==r.reference)clear(m,r.behavior_record+offset);
}
}
