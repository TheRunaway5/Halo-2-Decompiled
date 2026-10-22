#include "halo2/source_campaign_ai_scalar_defaults_tu15.hpp"
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
struct Copy {std::uint32_t load,store,source,destination;};
constexpr std::array first{Copy{0x3826a0,0x3826a8,0x46abac,0x56e080}};
constexpr std::array second{Copy{0x3826c0,0x3826c8,0x46a1e4,0x56e084},
    Copy{0x3826d0,0x3826d8,0x46abb0,0x56e088}};
constexpr std::array third{Copy{0x3826f0,0x3826f8,0x46a544,0x56e08c}};
std::uint32_t word(std::span<const std::byte,4> bytes) {
    std::uint32_t value{};
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
}
bool source_campaign_ai_scalar_defaults_tu15_owns_entry(std::uint32_t entry) {
    return entry==0x3826a0 || entry==0x3826c0 || entry==0x3826f0;
}
void source_run_campaign_ai_scalar_defaults_tu15(SourcePhysicsContactMemory& memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe) {
    std::span<const Copy> copies;std::uint32_t ret{};
    switch(entry) {
    case 0x3826a0:copies=first;ret=0x3826b0;break;
    case 0x3826c0:copies=second;ret=0x3826e0;break;
    case 0x3826f0:copies=third;ret=0x382700;break;
    default:throw std::invalid_argument("Unowned original AI scalar initializer");
    }
    for(const auto& copy:copies) {
        if(services.instruction)services.instruction(copy.load);
        std::array<std::byte,4> bytes;memory.read(copy.source,bytes);
        // Legacy MOVSS m32 clears the upper96 bits and transports all low
        // bits, including NaN payloads, without floating-point arithmetic.
        abi.xmm[0]={word(bytes),0,0,0};
        if(services.instruction)services.instruction(copy.store);
        SourcePhysicsContactRecordStore event{copy.store,copy.destination,{},{bytes.begin(),bytes.end()}};
        if(observe){event.before.resize(4);memory.read(copy.destination,event.before);}
        memory.write(copy.destination,bytes);
        if(observe)observe(event);
    }
    if(services.instruction)services.instruction(ret);
    // RET consumes its caller's real stack word. A missing caller must fail
    // after preceding publications, preserving the same partial ABI state.
    std::array<std::byte,4> return_address;memory.read(abi.gp[4],return_address);
    abi.gp[4]+=4u;
}
}
