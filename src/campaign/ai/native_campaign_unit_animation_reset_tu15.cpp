#include "halo2/native_campaign_unit_animation_reset_tu15.hpp"
#include <array>
#include <bit>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8u);return v;}
void write(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t v,unsigned n){std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(i*8u));m.write(p,std::span(b).first(n));}
class Channel {
    SourcePhysicsContactMemory &memory_;std::uint32_t base_;
    void put(unsigned offset,unsigned bytes,std::uint32_t value){write(memory_,base_+offset,value,bytes);}
public:
    Channel(SourcePhysicsContactMemory&m,std::uint32_t p):memory_(m),base_(p){}
    void invalidate_ids(){put(0,4,0xffffffffu);put(4,2,0xffffu);put(6,2,0xffffu);}
    void clear_frame(){put(0x1c,4,0);}
    void set_rate(std::uint32_t one_bits){put(0x18,4,one_bits);}
    void reset_lookup_keys(){put(8,4,0xffffffffu);put(0xc,1,0xffu);put(0xd,1,0xffu);put(0xe,2,0xffffu);}
    void clear_playback_flags(){put(0x10,1,0);put(0x11,1,0);put(0x12,2,0);}
    void clear_event_masks(){put(0x14,2,0);put(0x16,2,0);}
};
}
void native_campaign_unit_animation_reset_tu15(SourcePhysicsContactMemory&m,std::uint32_t unit){
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u);
    const auto one_bits=read(m,0x46a0d0u),body=read(m,rows+(unit&0xffffu)*12u+8u);
    const auto offset=std::bit_cast<std::int16_t>(std::uint16_t(read(m,body+0x33eu,2)));
    const auto extension=body+std::uint32_t(std::int32_t(offset));
    Channel first(m,extension+0x9cu),second(m,extension+0xbcu);
    first.invalidate_ids();first.clear_frame();first.set_rate(one_bits);
    first.reset_lookup_keys();first.clear_playback_flags();first.clear_event_masks();
    second.invalidate_ids();second.clear_frame();second.clear_playback_flags();
    second.set_rate(one_bits);second.clear_event_masks();second.reset_lookup_keys();
}
}
