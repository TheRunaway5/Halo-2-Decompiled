#include "halo2/native_tu15_crc32.hpp"
#include <array>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));
    std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(n*8u);return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);memory.read(address,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
}
void native_tu15_crc32_build_table(SourcePhysicsContactMemory &memory,std::uint32_t table,
    const SourcePhysicsContactRecordObserver &observe) {
    for(std::uint32_t byte=0;byte<256u;++byte) {
        auto remainder=byte;
        for(unsigned bit=0;bit<8u;++bit)remainder=(remainder>>1u)^((remainder&1u)?0xedb88320u:0u);
        publish(memory,observe,0x166b09,table+byte*4u,remainder);
    }
}
void native_tu15_crc32_update(SourcePhysicsContactMemory &memory,const NativeTu15CrcInput &input,
    std::int32_t count,std::uint32_t checksum,const SourcePhysicsContactRecordObserver &observe) {
    if(!read(memory,0x56e225u,1)) {
        native_tu15_crc32_build_table(memory,0x56e258u,observe);
        publish(memory,observe,0x166a9b,0x56e225u,1,1);
    }
    auto remainder=read(memory,checksum);
    if(count>0)for(std::uint32_t n=0;n<std::uint32_t(count);++n) {
        const auto byte=input.byte(n);
        const auto polynomial=read(memory,0x56e258u+((std::uint32_t(byte)^remainder)&255u)*4u);
        remainder=(remainder>>8u)^polynomial;
    }
    publish(memory,observe,0x166acd,checksum,remainder);
}
}
