#include "halo2/native_campaign_camera_reset_tu15.hpp"
#include <array>
namespace halo2 {
NativeCampaignCameraResetRequiredOwnerTu15::NativeCampaignCameraResetRequiredOwnerTu15(std::uint32_t record,std::int32_t view):
    std::runtime_error("Required native246190 camera controller initializer unavailable"),record_(record),view_(view){}
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(n*8u);return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
}
void native_campaign_camera_director_reset_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    for(unsigned n=0;n<0x143u;++n)publish(memory,observe,0x157ec2,0x4f5508u+n*4u,0);
    publish(memory,observe,0x157ec4,0x4f550cu,0);publish(memory,observe,0x157eca,0x4f5510u,0,1);
    for(unsigned view=0;view<4u;++view) {
        const auto state=0x4f5564u+view*0x140u;
        if(read(memory,state-0x50u,1))continue;
        publish(memory,observe,0x157eec,state+4u,0,1);publish(memory,observe,0x157eef,state,0);
        publish(memory,observe,0x157ef1,state-0x4cu,0);
        const auto mode=read(memory,0x4f550cu);
        if(mode==2u)throw NativeCampaignCameraResetRequiredOwnerTu15(state-0x40u,std::int32_t(view));
        if(mode<=4u && mode!=3u) {
            const auto one=read(memory,0x46a0d0u);
            publish(memory,observe,0x157f34,state-0x3cu,0);publish(memory,observe,0x157f39,state-0x40u,0);
            publish(memory,observe,0x157f3e,state-0x44u,0x245400u);publish(memory,observe,0x157f41,state+0xbcu,one);
            publish(memory,observe,0x157f49,state+0xb8u,0,1);
        }
        for(unsigned channel=0;channel<4u;++channel) {
            const auto selector=read(memory,0x48c4b8u+channel*0x20u);
            publish(memory,observe,0x157f62,state+0xc0u+channel*12u,selector);
            publish(memory,observe,0x157f65,state+0xc8u+channel*12u,0);
            publish(memory,observe,0x157f69,state+0xc4u+channel*12u,0);
        }
        publish(memory,observe,0x157f77,state-0x50u,1,1);
    }
    const auto director=read(memory,0x51df6cu);
    publish(memory,observe,0x157f9a,director+2u,0,2);publish(memory,observe,0x157f9f,director+0x3cu,0xffffffffu);
}
}
