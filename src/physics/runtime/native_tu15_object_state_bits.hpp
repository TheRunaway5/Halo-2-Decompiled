#pragma once
// Boundary adapter for the object state bits, entered at 16E290
// (native_tu15_object_state_bits_systems.hpp). Its callers keep the
// original ABI, which the adapter owns: EAX the object handle, the bits in
// EAX; EBX, EDI and ESI saved (the header byte, the type, the flags word);
// ECX and EDX as the original leaves them (the shifted words and bytes, the
// object); the flags of the last instruction that sets them; RET. Each
// access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_STATE_BITS=1 restores the translated body.
// HALO2_VERIFY_OBJECT_STATE_BITS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_state_bits_systems.hpp"

namespace halo2::native_object_state_bits_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_state_bits;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_STATE_BITS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_STATE_BITS");return p && *p;}();
    return value;
}
enum Branch : unsigned {first_bit, other_type, first_set, first_clear, eleventh_set, eleventh_clear, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object state bits verify: %llu calls; %llu ordered accesses (%llu stores) "
            "identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct StateBits : Port<State> {
    using Access=logic::object_state_bits::Access;
    StateBits(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0x16e290u;
        case header_datums:return 0x16e296u;
        case object_address:return 0x16e2a4u;
        case header_flags:this->push(0x16e2a7u,this->gp[ebx]);return 0x16e2a8u;                    // PUSH EBX
        case type:return 0x16e2b1u;
        case object_byte:return 0x16e2bau;
        case object_flags:this->push(0x16e2c5u,this->gp[esi]);return 0x16e2c6u;                    // PUSH ESI
        case details:return 0x16e303u;
        case state:return 0x16e31bu;
        case kind_flags:return 0x16e33cu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        // MOV EDX,[ECX+44]; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]; LEA ECX,[EDX+EAX*4].
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;gp[ecx]=v+gp[eax]*4u;break;
        case object_address:gp[edx]=v;break;
        // MOV BL; XOR EAX,EAX; TEST BL,10; PUSH EDI.
        case header_flags:
            set_low_byte(gp[ebx],v);gp[eax]=fl.logic(0u,4);this->test8(v&logic::header_bit);this->push(0x16e2b0u,gp[edi]);break;
        case type:gp[edi]=std::uint32_t(std::int32_t(std::int8_t(v)));break;                       // MOVSX EDI
        case object_byte:this->test8(v&1u);break;                                                   // TEST byte,1
        case object_flags:gp[esi]=v;break;
        // MOV CL; SHR CL,2; TEST CL,1.
        case details:set_low_byte(gp[ecx],fl.right(v&0xffu,2u,1,false));this->test8(gp[ecx]&1u);break;
        // MOV DL; SHR DL,3; TEST DL,1.
        case state:set_low_byte(gp[edx],fl.right(v&0xffu,3u,1,false));this->test8(gp[edx]&1u);break;
        // MOV ECX; SHR ECX,1; TEST CL,1.
        case kind_flags:gp[ecx]=fl.right(v,1u,4,false);this->test8(gp[ecx]&1u);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void first(std::uint32_t bits){this->gp[eax]=bits;this->reached(Branch::first_bit);}            // MOV EAX,1
    void tested(std::uint32_t shift) {
        auto &gp=this->gp;
        gp[ecx]=this->flags.right(gp[esi],shift,4,false);this->test8(gp[ecx]&1u);
        if(shift==31u)gp[esi]=this->pop(0x16e2f9u);                                               // POP ESI
    }
    void applied(std::uint32_t bits){this->gp[eax]=this->flags.logic(bits,4);}                      // OR / AND EAX
    void type_tested(){this->test32(this->gp[edi]);}                                                // TEST EDI,EDI
    void eleventh_tested(){this->flags.minus(this->gp[edi],logic::eleventh_type,4);}               // CMP EDI,0B
    void run() {
        auto &gp=this->gp;
        Memory<StateBits> memory{*this};
        const auto result=logic::object_state_bits::of(memory,*this,gp[eax]);
        using logic::object_state_bits::Last;
        // The last bit's OR or AND, the pops and the RET of each exit.
        struct Exit {std::uint32_t pop_edi,pop_ebx,ret;bool applied;};
        static constexpr Exit exits[]{{0x16e354u,0x16e355u,0x16e356u,false},{0x16e329u,0x16e32du,0x16e32eu,true},
            {0x16e32fu,0x16e335u,0x16e336u,true},{0x16e349u,0x16e34du,0x16e34eu,true},{0x16e354u,0x16e355u,0x16e356u,true}};
        static constexpr Branch branches[]{Branch::other_type,Branch::first_set,Branch::first_clear,Branch::eleventh_set,
            Branch::eleventh_clear};
        const auto &exit=exits[unsigned(result.last)];
        this->reached(branches[unsigned(result.last)]);
        if(exit.applied)applied(result.bits);
        gp[edi]=this->pop(exit.pop_edi);gp[ebx]=this->pop(exit.pop_ebx);
        this->ret(exit.ret,0);this->publish(exit.ret);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x16e290u,0x16e356u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return StateBits<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_object_state_bits_detail
