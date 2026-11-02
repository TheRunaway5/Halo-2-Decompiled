#include "halo2/native_campaign_ai_awareness_tu15_binding.hpp"
#include "halo2/native_campaign_ai_awareness_tu15.hpp"
#include <atomic>
#include <bit>
#include <limits>

namespace halo2 {
namespace {
std::atomic<std::uint64_t> calls{};
constexpr bool native_extended=std::endian::native==std::endian::little && sizeof(long double)>=10 &&
    std::numeric_limits<long double>::digits==64 && std::numeric_limits<long double>::max_exponent==16384;
std::uint32_t returned(const SourcePhysicsContactMemory &memory,std::uint32_t at) {
    std::array<std::byte,4> bytes{};memory.read(at,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
}
NativeCampaignAiAwarenessReturnRequiredTu15::NativeCampaignAiAwarenessReturnRequiredTu15(
    std::uint32_t target):std::runtime_error("Native AI awareness requires its actual HS return continuation"),target_(target){}
bool native_campaign_ai_awareness_tu15_owns_entry(std::uint32_t entry){return entry==0x27d7d0u || entry==0x27da40u;}
bool native_campaign_ai_awareness_tu15_owns_call(const SourcePhysicsContactRuntimeCall &call) {
    return !call.tail && ((call.target==0x27d7d0u && call.instruction==0x2af0f0u) ||
        (call.target==0x27da40u && call.instruction==0x2af13fu));
}
void native_campaign_ai_awareness_tu15_run(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) {
    if(!native_campaign_ai_awareness_tu15_owns_call(call) || !abi.floating_point || abi.active_mmx ||
        abi.x87_depth>7 || (abi.x87_depth && !native_extended))
        throw std::invalid_argument("Unqualified native AI awareness caller/live FP ownership");
    const auto &control=*abi.floating_point;
    if((control.x87_control_word&0x3fu)!=0x3fu || (abi.x87_status&0x8080u) ||
        (control.mxcsr&0x1f80u)!=0x1f80u || (control.mxcsr&0xffff0000u))
        throw std::invalid_argument("Native AI awareness caller requires its masked live FP policy/status");
    NativeTu15PhysicsFrameFp fp{control.x87_control_word,abi.x87_status,control.mxcsr};
    struct Publication {
        SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
        ~Publication(){abi.floating_point->x87_control_word=fp.control_word;abi.floating_point->mxcsr=fp.mxcsr;abi.x87_status=fp.status;}
    } publication{abi,fp};
    calls.fetch_add(1,std::memory_order_relaxed);
    // The literal callers load both values before their pushes. CALL does
    // not change these registers. Private pushed argument slots are valid,
    // disjoint and immutable under the admitted actual caller profile.
    const bool ai=call.target==0x27d7d0u;
    if(ai)native_campaign_ai_magically_see_tu15(memory,abi.gp[2],abi.gp[1],fp,abi.x87_depth,observe);
    else native_campaign_ai_magically_see_object_tu15(memory,abi.gp[0],abi.gp[1],fp,abi.x87_depth,observe);
    const auto target=returned(memory,abi.gp[4]);
    if(target!=(ai?0x2af0f5u:0x2af144u))throw NativeCampaignAiAwarenessReturnRequiredTu15(target);
    abi.gp[4]+=ai?12u:8u;
}
std::uint64_t native_campaign_ai_awareness_calls_tu15(){return calls.load(std::memory_order_relaxed);}
}
