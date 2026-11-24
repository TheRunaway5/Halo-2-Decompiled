#pragma once
// Boundary adapter for the indexed tag reference, entered at 1CEE60
// (native_tu15_indexed_tag_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: ECX the reference; ESI and (past the own-tag case)
// EDI saved; the definition in EAX; ECX (the index widened, scaled, the
// entry's tag) and EDX (zero past the own-tag case) as the original leaves
// them; the flags of the last instruction that sets them; RET. Each access
// publishes its original instruction.
// HALO2_REFERENCE_INDEXED_TAG=1 restores the translated body.
// HALO2_VERIFY_INDEXED_TAG=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_indexed_tag_systems.hpp"

namespace halo2::native_indexed_tag_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_indexed_tag;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_INDEXED_TAG");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_INDEXED_TAG");return p && *p;}();
    return value;
}
enum Branch : unsigned {own, past_count, no_tag, entry, branch_count};
static_assert(unsigned(logic::indexed_tag::Outcome::entry)==Branch::entry);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Indexed tag verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct IndexedTag : Port<State> {
    using Access=logic::indexed_tag::Access;
    IndexedTag(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case tag:return 0x1cee60u;
        case index:return 0x1cee62u;
        case tags:return 0x1cee6cu;
        case definition:return 0x1cee79u;
        // MOVSX ECX,CX; PUSH EDI; MOV EDI,[EAX+4C].
        case count:this->gp[ecx]=logic::widen16(this->gp[ecx]);this->push(0x1cee82u,this->gp[edi]);return 0x1cee83u;
        case entries:return 0x1cee8cu;
        case entry_tag:return 0x1cee94u;
        // AND ECX,FFFF; SHL ECX,4 before MOV EDX,[ECX+ESI+8].
        case entry_definition:this->gp[ecx]=this->flags.shift(this->flags.logic(this->gp[ecx]&0xffffu,4),4u,4);return 0x1ceea5u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case tag:gp[eax]=v;break;
        // MOV CX,[ECX+4]; AND EAX,FFFF; PUSH ESI.
        case index:gp[ecx]=(gp[ecx]&0xffff0000u)|(v&0xffffu);gp[eax]=fl.logic(gp[eax]&0xffffu,4);
            this->push(0x1cee6bu,gp[esi]);break;
        // MOV ESI,[4F0414]; SHL EAX,4; CMP CX,-1.
        case tags:gp[esi]=v;gp[eax]=fl.shift(gp[eax],4u,4);fl.minus(gp[ecx]&0xffffu,0xffffu,2);break;
        case definition:gp[eax]=v;break;
        // MOV EDI,[EAX+4C]; XOR EDX,EDX; CMP ECX,EDI.
        case count:gp[edi]=v;gp[edx]=fl.logic(0u,4);fl.minus(gp[ecx],v,4);break;
        // MOV EDI,[EAX+50]; SHL ECX,5; ADD ECX,EDI.
        case entries:gp[edi]=v;gp[ecx]=fl.plus(fl.shift(gp[ecx],5u,4),v,4);break;
        case entry_tag:gp[ecx]=v;fl.minus(v,logic::none,4);break;                                   // CMP ECX,-1
        case entry_definition:gp[edx]=v;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::indexed_tag::Outcome;
        Memory<IndexedTag> memory{*this};
        const auto result=logic::indexed_tag::resolve(memory,gp[ecx]);
        this->reached(unsigned(result.outcome));
        gp[eax]=result.definition;                                                                   // MOV EAX,EDX (or the own definition)
        if(result.outcome!=Outcome::own)gp[edi]=this->pop(0x1ceeabu);                                // POP EDI
        gp[esi]=this->pop(0x1ceeacu);this->ret(0x1ceeadu,0);this->publish(0x1ceeadu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1cee60u,0x1ceeadu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return IndexedTag<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_indexed_tag_detail
