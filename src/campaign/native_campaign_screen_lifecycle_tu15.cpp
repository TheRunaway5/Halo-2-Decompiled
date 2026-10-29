#include "halo2/native_campaign_screen_lifecycle_tu15.hpp"
#include <array>

namespace halo2 {
namespace {
std::uint32_t integer(const SourcePhysicsContactMemory &memory,
                      std::uint32_t address,unsigned bytes) {
    std::array<std::byte,4> data{};
    memory.read(address,std::span(data).first(bytes));
    std::uint32_t result{};
    for(unsigned n=0;n<bytes;++n)
        result|=std::to_integer<std::uint32_t>(data[n])<<(8u*n);
    return result;
}
void publish(SourcePhysicsContactMemory &memory,
             const SourcePhysicsContactRecordObserver &observe,
             std::uint32_t site,std::uint32_t address,
             std::span<const std::byte> data) {
    SourcePhysicsContactRecordStore event{site,address,{},{}};
    if(observe) {
        event.before.resize(data.size());memory.read(address,event.before);
        event.written.assign(data.begin(),data.end());
    }
    memory.write(address,data);
    if(observe)observe(event);
}
} // namespace

void native_campaign_screen_stop_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto interface=integer(memory,0x515ec0u,4);
    if(!interface)return;
    constexpr std::array<std::byte,4> zero{};
    // Separate stores preserve the original fault/publication order. A bulk
    // memset would change what a completed-store observer can see or modify.
    for(unsigned word=0;word<0xffu;++word)
        publish(memory,observe,0x1fcc2u,interface+word*4u,zero);
    // Read after the reset, including when the bank aliases these globals.
    if(integer(memory,0x4c1eb0u,4) && !integer(memory,0x4c1ebdu,1)) {
        constexpr std::array dirty{std::byte{1}};
        publish(memory,observe,0x1fcd6u,0x4c1ebdu,dirty);
    }
}

void native_campaign_screen_capture_complete_tu15(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRecordObserver &observe) {
    if(const auto interface=integer(memory,0x515ec0u,4)) {
        constexpr std::array clear{std::byte{0}};
        publish(memory,observe,0x1fce9u,interface+0xa0u,clear);
    }
}
} // namespace halo2
