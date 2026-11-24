#pragma once
// Boundary adapter for the attachment root, entered at bcf70
// (native_tu15_object_leaves_systems.hpp). Its callers keep the
// original ABI, which the adapter owns: EAX the object handle, and the root
// (or NONE) in EAX on return; ESI saved; ECX (the last object) and EDX (its
// flags shifted) as the original leaves them; the flags of the last
// instruction that sets them; RET.
// HALO2_REFERENCE_OBJECT_LEAVES=1 restores the translated body.
// HALO2_VERIFY_OBJECT_LEAVES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_object_leaves_systems.hpp"

namespace halo2::native_attachment_root_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_leaves;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_LEAVES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_LEAVES");return p && *p;}();
    return value;
}
enum Branch : unsigned {none_given, settled, top, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object leaves (attachment root) verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Leaf : Port<State> {
    Leaf(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    using Access=logic::attachment_root::Access;
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0xbcf75u;
        case header_datums:return 0xbcf7cu;
        // MOV ECX,EAX; AND ECX,FFFF; LEA EDX,[ECX+ECX*2] before MOV ECX,[ESI+EDX*4+8].
        case object_address:this->gp[ecx]=this->flags.logic(this->gp[eax]&0xffffu,4);this->gp[edx]=this->gp[ecx]*3u;return 0xbcf8bu;
        case flags:return 0xbcf8fu;
        case parent:return 0xbcf9au;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;this->push(0xbcf7bu,gp[esi]);break;                                 // PUSH ESI
        case header_datums:gp[esi]=v;break;
        case object_address:gp[ecx]=v;break;
        // MOV EDX,[ECX+4]; SHR EDX,1A; TEST DL,1.
        case flags:gp[edx]=fl.right(v,logic::attachment_root::attached_bit,4,false);this->test8(gp[edx]&1u);break;
        case parent:gp[eax]=v;fl.minus(v,logic::none,4);break;                                      // MOV EAX,[ECX+14]; CMP EAX,-1
        }
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::attachment_root::Outcome;
        this->flags.minus(gp[eax],logic::none,4);                                                    // CMP EAX,-1
        Memory<Leaf> memory{*this};
        const auto result=logic::attachment_root::find(memory,gp[eax]);
        this->reached(unsigned(result.outcome));
        gp[eax]=result.handle;
        if(result.outcome!=Outcome::none_given)gp[esi]=this->pop(0xbcfa2u);
        this->ret(0xbcfa3u,0);this->publish(0xbcfa3u);
    }
    template<class A> void stored(A,std::uint32_t,std::uint32_t){}
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xbcf70u,0xbcfa3u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Leaf<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_attachment_root_detail
