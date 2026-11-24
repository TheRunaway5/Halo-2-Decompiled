#pragma once
// Boundary adapter for the bit vector union, entered at 140EC0
// (native_tu15_bit_union_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX a, EDX b, ECX the bit count, the out pointer
// on the stack (RET 4); EDI and, over the words, ESI saved; EAX (a less 4
// after the words), ECX (0, or the count less one), EDX (b less a) as the
// original leaves them; the flags of the last instruction that sets them
// (the DEC, with the SUB's carry). Each access publishes its original
// instruction.
// HALO2_REFERENCE_BIT_UNION=1 restores the translated body.
// HALO2_VERIFY_BIT_UNION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_bit_union_systems.hpp"

namespace halo2::native_bit_union_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_bit_union;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_BIT_UNION");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_BIT_UNION");return p && *p;}();
    return value;
}
enum Branch : unsigned {empty, united, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Bit union verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct BitUnion : Port<State> {
    using Access=logic::bit_union::Access;
    BitUnion(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    // [ESP+C] after PUSH EDI; PUSH ESI.
    std::uint32_t out_slot()const{return this->gp[esp]+0xcu;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case out:return 0x140ecdu;case right:return 0x140ee0u;case left:return 0x140ee3u;case store:return 0x140ee5u;}
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV ESI,[ESP+C]; LEA EAX,[EDI+ECX*4]; SUB EDX,EDI; SUB ESI,EDI; INC ECX; LEA ESP,[ESP].
        case out:gp[esi]=v;gp[eax]=gp[edi]+4u*gp[ecx];gp[edx]=fl.minus(gp[edx],gp[edi],4);
            gp[esi]=fl.minus(gp[esi],gp[edi],4);gp[ecx]=fl.increment(gp[ecx],4,false);break;
        case right:gp[edi]=v;break;                                                                // MOV EDI,[EDX+EAX]
        case left:gp[edi]=fl.logic(gp[edi]|v,4);break;                                              // OR EDI,[EAX]
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void walking(){this->push(0x140eccu,this->gp[esi]);}                                           // PUSH ESI
    // SUB EAX,4; DEC ECX.
    void stepped(){auto &gp=this->gp;auto &fl=this->flags;gp[eax]=fl.minus(gp[eax],4u,4);gp[ecx]=fl.increment(gp[ecx],4,true);}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        const auto bits=gp[ecx];
        // ADD ECX,1F; SAR ECX,5; DEC ECX; PUSH EDI; MOV EDI,EAX.
        gp[ecx]=fl.increment(fl.right(fl.plus(bits,0x1fu,4),5u,4,true),4,true);
        this->push(0x140ec7u,gp[edi]);gp[edi]=gp[eax];
        Memory<BitUnion> memory{*this};
        const auto words=logic::bit_union::unite(memory,*this,gp[eax],gp[edx],bits);
        this->reached(words>0?Branch::united:Branch::empty);
        if(words>0)gp[esi]=this->pop(0x140eeeu);
        gp[edi]=this->pop(0x140eefu);this->ret(0x140ef0u,4u);this->publish(0x140ef0u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x140ec0u,0x140ef0u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return BitUnion<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_bit_union_detail
