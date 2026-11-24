#pragma once
// Boundary adapter for the object origin, entered at 30C40
// (native_tu15_object_origin_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: ECX the object handle, EAX the point, the
// word pointer at [ESP+4], RET 4; ESI saved (the copies run through it);
// EAX the word, ECX the word pointer and EDX the position's z as the
// original leaves them; the flags of the AND; RET. Each access publishes
// its original instruction.
// HALO2_REFERENCE_OBJECT_ORIGIN=1 restores the translated body.
// HALO2_VERIFY_OBJECT_ORIGIN=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "rendering/shadows/native_tu15_object_origin_systems.hpp"

namespace halo2::native_object_origin_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_origin;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_ORIGIN");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_ORIGIN");return p && *p;}();
    return value;
}
enum Branch : unsigned {copied, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object origin verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Origin : Port<State> {
    using Access=logic::object_origin::Access;
    std::uint32_t k{};
    Origin(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    // [ESP+8] after PUSH ESI.
    std::uint32_t argument()const{return this->gp[esp]+8u;}
    void component(std::uint32_t index){k=index;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        static constexpr std::uint32_t reads[]{0x30c5au,0x30c5eu,0x30c64u},writes[]{0x30c5cu,0x30c61u,0x30c67u};
        using enum Access;
        switch(a) {
        case headers:return 0x30c40u;
        case header_datums:return 0x30c46u;
        case object_address:return 0x30c52u;
        case position_read:return reads[k];
        case position_store:return writes[k];
        case word_read:return 0x30c6au;
        case word_target:return 0x30c6du;
        case word_store:return 0x30c71u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers:gp[edx]=v;break;
        // MOV EDX,[EDX+44]; AND ECX,FFFF; LEA ECX,[ECX+ECX*2].
        case header_datums:gp[edx]=v;gp[ecx]=this->flags.logic(gp[ecx]&0xffffu,4)*3u;break;
        // MOV ECX,[...]; LEA EDX,[ECX+30]; PUSH ESI.
        case object_address:gp[ecx]=v;gp[edx]=v+logic::object::position;this->push(0x30c59u,gp[esi]);break;
        case position_read:gp[k==2u?edx:esi]=v;break;                                            // MOV ESI (EDX for z)
        case word_read:gp[eax]=v;break;
        case word_target:gp[ecx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        Memory<Origin> memory{*this};
        logic::object_origin::copy(memory,*this,gp[ecx],gp[eax]);
        this->reached(Branch::copied);
        gp[esi]=this->pop(0x30c73u);this->ret(0x30c74u,4u);this->publish(0x30c74u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x30c40u,0x30c74u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Origin<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_object_origin_detail
