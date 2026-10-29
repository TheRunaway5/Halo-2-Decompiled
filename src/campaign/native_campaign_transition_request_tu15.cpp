#include "halo2/native_campaign_transition_request_tu15.hpp"
#include <bit>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t value{};
    for(unsigned k=0;k<n;++k)value|=std::uint32_t(std::to_integer<unsigned>(b[k]))<<(k*8u);
    return value;
}
void write(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t value,unsigned n=4){
    std::array<std::byte,4>b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(value>>(k*8u));m.write(p,std::span(b).first(n));
}
struct RetainedOptions {
    const SourcePhysicsContactMemory&m;std::uint32_t address;
    bool present()const{return address!=0;}
    std::uint32_t read_at(std::uint32_t offset,unsigned bytes=4)const{return read(m,address+offset,bytes);}
};
struct PrivateOptions {
    const NativeCampaignTransitionOptionsTu15&value;
    bool present()const{return true;}
    std::uint32_t read_at(std::uint32_t offset,unsigned bytes=4)const{
        std::uint32_t result{};for(unsigned n=0;n<bytes;++n)result|=std::uint32_t(std::to_integer<unsigned>(value.bytes[offset+n]))<<(n*8u);
        return result;
    }
};
template<class Options>void publish(SourcePhysicsContactMemory&m,const Options&options,
    const NativeCampaignTransitionClockTu15&clock){
    const auto present=options.present();
    if(present){
        // The original REP MOVSD is a forward stream, including overlaps.
        // Reading the whole input first or using memmove changes its effects.
        for(std::uint32_t offset=0;offset<0x1118u;offset+=4u)
            write(m,0x4f9c78u+offset,options.read_at(offset));
    }
    write(m,0x4f9c6eu,!present,1);
    write(m,0x4f9c6du,1,1);
    const auto cell=read(m,0x41b774u);
    if(!clock)throw std::invalid_argument("Native transition request has no actual monotonic clock producer");
    clock(cell);
    const auto tick=read(m,cell);
    write(m,0x4f9c70u,tick);
    if(!read(m,0x535f00u,1))return;
    const auto state=read(m,0x535f04u);
    if(state!=3u&&state!=8u)return;
    if(present){
        if(options.read_at(5u,1))return;
        if(options.read_at(0u)==3u){
            const auto count=std::bit_cast<std::int32_t>(options.read_at(0x264u));
            if(count>=2&&count<=6)return;
        }
    }
    throw NativeCampaignTransitionRequestRequiredOwnerTu15();
}
}
void native_campaign_transition_request_tu15(SourcePhysicsContactMemory&m,
    std::uint32_t options,const NativeCampaignTransitionClockTu15&clock){publish(m,RetainedOptions{m,options},clock);}
void native_campaign_transition_request_value_tu15(SourcePhysicsContactMemory&m,
    const NativeCampaignTransitionOptionsTu15&options,const NativeCampaignTransitionClockTu15&clock){publish(m,PrivateOptions{options},clock);}
}
