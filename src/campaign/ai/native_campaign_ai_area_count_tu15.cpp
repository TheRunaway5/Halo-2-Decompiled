#include "halo2/native_campaign_ai_area_count_tu15.hpp"
#include "native_campaign_authored_area_iterator_tu15.hpp"
#include <array>
#include <atomic>
#include <stdexcept>

namespace halo2 { namespace {
std::atomic<std::uint64_t> calls{};
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));
    std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
}
bool native_campaign_ai_has_multiple_areas_tu15(const SourcePhysicsContactMemory &memory,std::uint32_t actor) {
    const auto pool=read(memory,0x5020c8u),row=read(memory,pool+0x44u)+(actor&0xffffu)*0x888u;
    const bool mode=read(memory,row+0x26cu)!=0xffffffffu;
    const auto squad=std::uint16_t(read(memory,row+0x30u,2));
    detail::CampaignAuthoredAreaIteratorTu15 areas(memory,squad,mode);
    return areas.next()!=0xffffu && areas.next()!=0xffffu;
}
bool native_campaign_ai_area_count_tu15_owns_call(const SourcePhysicsContactRuntimeCall &call) {
    return call.target==0x1bbb50u && call.instruction==0x1b0195u && !call.tail;
}
void native_campaign_ai_area_count_tu15_run(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi) {
    if(!native_campaign_ai_area_count_tu15_owns_call(call))
        throw std::runtime_error("Native authored area-count callback requires its actual1B0195 caller");
    calls.fetch_add(1,std::memory_order_relaxed);
    const auto sp=abi.gp[4];
    const auto actor=read(memory,sp+4u);
    const auto result=native_campaign_ai_has_multiple_areas_tu15(memory,actor);
    // The completed result precedes the actual RET read. A rejected return
    // must not roll the algorithm back or advance the caller's retained SP.
    abi.gp[0]=(abi.gp[0]&0xffff0000u)|std::uint32_t(result);
    if(read(memory,sp)!=0x1b0197u)
        throw std::runtime_error("Native authored area-count callback has a different actual return word");
    abi.gp[4]=sp+8u;
}
std::uint64_t native_campaign_ai_area_count_calls_tu15(){return calls.load(std::memory_order_relaxed);}
}
