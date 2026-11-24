#pragma once
// Boundary adapter for the channel reset, entered at 1D4E50
// (native_tu15_channel_reset_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the channels; ECX NONE (OR ECX,-1) and
// EDX zero (XOR EDX,EDX) on return; XMM0 zero, XMM1 the float 1; the flags
// of the XOR; RET. Each store publishes its original instruction.
// HALO2_REFERENCE_CHANNEL_RESET=1 restores the translated body.
// HALO2_VERIFY_CHANNEL_RESET=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_channel_reset_systems.hpp"
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"

namespace halo2::native_channel_reset_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_channel_reset;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_CHANNEL_RESET");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_CHANNEL_RESET");return p && *p;}();
    return value;
}
enum Branch : unsigned {reset, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Channel reset verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
// The store instructions: each channel's fields at their own sites.
inline constexpr std::uint32_t field_sites[logic::channel_count][logic::field_count]{
    {0x1d4e60u,0x1d4e62u,0x1d4e66u,0x1d4e6au,0x1d4e6fu,0x1d4e72u,0x1d4e75u,0x1d4e79u,0x1d4e7eu,0x1d4e82u,0x1d4e86u,0x1d4e89u,0x1d4e8cu,0x1d4e8fu},
    {0x1d4e93u,0x1d4e96u,0x1d4e9au,0x1d4e9eu,0x1d4ea3u,0x1d4ea6u,0x1d4ea9u,0x1d4eadu,0x1d4eb2u,0x1d4eb6u,0x1d4ebau,0x1d4ebdu,0x1d4ec0u,0x1d4ec3u},
    {0x1d4ec7u,0x1d4ecau,0x1d4eceu,0x1d4ed2u,0x1d4ed7u,0x1d4edau,0x1d4eddu,0x1d4ee1u,0x1d4ee6u,0x1d4eeau,0x1d4eeeu,0x1d4ef1u,0x1d4ef4u,0x1d4ef7u}};
template<class State> struct ChannelReset : Port<State> {
    using Access=logic::channel_reset::Access;
    std::uint32_t channel{},k{};
    ChannelReset(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t){return a==Access::one?0x1d4e53u:field_sites[channel][k];}
    // MOVSS XMM1,[46A0D0]; OR ECX,-1; XOR EDX,EDX.
    void loaded(Access,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        this->s.xmm[1]=native_aim_clusters::sse::scalar(v);gp[ecx]=fl.logic(gp[ecx]|logic::none,4);gp[edx]=fl.logic(0u,4);
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void field(std::uint32_t c,std::uint32_t index){channel=c;k=index;}
    void run() {
        this->s.xmm[0]=_mm_setzero_ps();                                                             // XORPS XMM0,XMM0
        Memory<ChannelReset> memory{*this};
        logic::channel_reset::reset(memory,*this,this->gp[eax]);
        this->reached(Branch::reset);
        this->ret(0x1d4efbu,0);this->publish(0x1d4efbu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1d4e50u,0x1d4efbu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ChannelReset<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_channel_reset_detail
