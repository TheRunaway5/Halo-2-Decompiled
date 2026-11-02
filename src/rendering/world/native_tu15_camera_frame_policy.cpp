#include "halo2/native_tu15_camera_frame_policy.hpp"
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8u*n);return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(8u*n));
    memory.write(address,event.written);if(observe)observe(event);
}
}
void native_tu15_camera_frame_policy(SourcePhysicsContactMemory &memory,std::uint32_t view_count,
    const SourcePhysicsContactRecordObserver &observe) {
    std::uint32_t coverage;bool ordinary;
    if(view_count==2u || view_count==3u) {
        const auto scale=read(memory,0x456918u);
        publish(memory,observe,0x2c4e4u,0x473644u,scale);
        const auto detail=read(memory,0x46a0ccu);
        publish(memory,observe,0x2c4f4u,0x4c684cu,detail);
        coverage=read(memory,0x456910u);
        publish(memory,observe,0x2c504u,0x4c683cu,3);
        ordinary=false;
    } else if(view_count==4u) {
        const auto scale=read(memory,0x46a130u);
        publish(memory,observe,0x2c4b8u,0x473644u,scale);
        publish(memory,observe,0x2c4c0u,0x4c684cu,scale);
        coverage=read(memory,0x46a22cu);
        publish(memory,observe,0x2c4d0u,0x4c683cu,2);
        ordinary=false;
    } else {
        const auto scale=read(memory,0x46a140u);
        publish(memory,observe,0x2c483u,0x473644u,scale);
        coverage=read(memory,0x46a0d0u);
        publish(memory,observe,0x2c495u,0x4c6841u,0,1);
        publish(memory,observe,0x2c49cu,0x4c684cu,coverage);
        publish(memory,observe,0x2c4a4u,0x4c683cu,4);
        ordinary=true;
    }
    if(!ordinary)publish(memory,observe,0x2c510u,0x4c6841u,1,1);
    publish(memory,observe,0x2c517u,0x4c6840u,ordinary,1);
    publish(memory,observe,0x2c51cu,0x4c6842u,ordinary,1);
    publish(memory,observe,0x2c521u,0x4c6844u,ordinary,1);
    publish(memory,observe,0x2c526u,0x4c6843u,ordinary,1);
    publish(memory,observe,0x2c52bu,0x4c6845u,ordinary,1);
    publish(memory,observe,0x2c530u,0x4c6848u,coverage);
}
std::uint32_t native_tu15_camera_target_query(const SourcePhysicsContactMemory &memory) {
    if(read(memory,0x4c1fe0u) && !read(memory,0x4c1fedu,1))return 20u;
    if(read(memory,0x4c1eb0u) && !read(memory,0x4c1ebdu,1))return 18u;
    return 0xffffffffu;
}
}
