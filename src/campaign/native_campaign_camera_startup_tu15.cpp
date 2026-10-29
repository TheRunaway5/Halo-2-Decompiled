#include "halo2/native_campaign_camera_startup_tu15.hpp"
#include "halo2/native_tu15_crc32.hpp"
#include <array>
namespace halo2 {
namespace {
class Storage {
    SourcePhysicsContactMemory &memory_;
    const SourcePhysicsContactRecordObserver &observe_;
public:
    Storage(SourcePhysicsContactMemory &m,const SourcePhysicsContactRecordObserver &o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t address)const {
        std::array<std::byte,4> bytes{};memory_.read(address,bytes);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(8u*n);return value;
    }
    void publish(std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4) {
        SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
        if(observe_){event.before.resize(width);memory_.read(address,event.before);}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(8u*n));
        memory_.write(address,event.written);if(observe_)observe_(event);
    }
};
class SizeInput final:public NativeTu15CrcInput {
    std::uint32_t size_;
public:
    explicit SizeInput(std::uint32_t size):size_(size){}
    std::uint8_t byte(std::uint32_t index)const override{return std::uint8_t(size_>>(index*8u));}
};
void account(SourcePhysicsContactMemory &memory,unsigned bytes,const SourcePhysicsContactRecordObserver &observe) {
    SizeInput input(bytes);native_tu15_crc32_update(memory,input,4,0x4f295cu,observe);
}
}
void native_campaign_camera_input_storage_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    Storage data(memory,observe);const auto offset=data.word(0x4f2954u),base=data.word(0x4f2950u);
    const auto input=offset+base;
    data.publish(0x1888b6,0x4f2954u,offset+0x264u);account(memory,0x264u,observe);
    data.publish(0x1888d7,0x4f9b54u,input);data.publish(0x1888dd,input,0,1);
}
void native_campaign_camera_director_storage_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    Storage data(memory,observe);
    for(unsigned n=0;n<0x143u;++n)data.publish(0x157e1f,0x4f5508u+n*4u,0);
    const auto offset=data.word(0x4f2954u),base=data.word(0x4f2950u);const auto control=offset+base;
    data.publish(0x157e36,0x4f2954u,offset+4u);account(memory,4u,observe);
    data.publish(0x157e4d,control,0,1);
    const auto next=data.word(0x4f2954u),fresh_base=data.word(0x4f2950u);
    data.publish(0x157e5b,0x4f5504u,control);
    const auto director=next+fresh_base;
    data.publish(0x157e67,0x4f2954u,next+0x78u);account(memory,0x78u,observe);
    data.publish(0x157e83,0x51df6cu,director);
}
void native_campaign_cinematic_storage_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    Storage data(memory,observe);const auto offset=data.word(0x4f2954u),base=data.word(0x4f2950u);
    const auto state=offset+base;data.publish(0x13e9f4,0x4f2954u,offset+0x24u);account(memory,0x24u,observe);
    constexpr std::array<std::uint32_t,8> sites{0x13ea16,0x13ea18,0x13ea1b,0x13ea1e,0x13ea21,0x13ea24,0x13ea27,0x13ea2a};
    for(unsigned n=0;n<sites.size();++n)data.publish(sites[n],state+n*4u,0);
    data.publish(0x13ea2e,0x51df50u,state);data.publish(0x13ea34,state+0x20u,0);
}
void native_campaign_camera_default_tu15(SourcePhysicsContactMemory &memory,NativeCampaignCameraDefaultTu15 kind,
    const SourcePhysicsContactRecordObserver &observe) {
    Storage data(memory,observe);
    switch(kind) {
    case NativeCampaignCameraDefaultTu15::Angle:{const auto value=data.word(0x46ab88u);data.publish(0x381718,0x55d6e4u,value);break;}
    case NativeCampaignCameraDefaultTu15::Minimum:{const auto value=data.word(0x46a164u);data.publish(0x381738,0x55d6e8u,value);break;}
    case NativeCampaignCameraDefaultTu15::Maximum:{const auto value=data.word(0x46a5f8u);data.publish(0x381758,0x55d6ecu,value);break;}
    }
}
}
