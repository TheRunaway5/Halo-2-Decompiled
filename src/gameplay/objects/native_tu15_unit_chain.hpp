#pragma once
// Boundary adapter for the unit chain step, entered at D2B00
// (native_tu15_unit_chain_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: ESI the cursor; EBP and EDI saved; the unit in EAX
// (XOR EAX,EAX when none); ECX (the datums, its low byte the type, its low
// word the unit's word) and EDX (the object) as the original leaves them;
// the flags of the last instruction that sets them (the SHL's included);
// RET. Each access publishes its original instruction.
// HALO2_REFERENCE_UNIT_CHAIN=1 restores the translated body.
// HALO2_VERIFY_UNIT_CHAIN=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_unit_chain_systems.hpp"

namespace halo2::native_unit_chain_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_unit_chain;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_UNIT_CHAIN");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_UNIT_CHAIN");return p && *p;}();
    return value;
}
enum Branch : unsigned {empty, passed, ended, found, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Unit chain verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct UnitChain : Port<State> {
    using Access=logic::unit_chain::Access;
    bool walked{};
    UnitChain(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case first_next:return 0xd2b00u;
        case headers:this->push(0xd2b0au,this->gp[ebp]);walked=true;return 0xd2b0bu;               // PUSH EBP
        case next_handle:return 0xd2b12u;
        case header_datums:return 0xd2b20u;
        case object_address:return 0xd2b23u;
        case object_next:return 0xd2b27u;
        case next_store:return 0xd2b2au;
        case object_type:return 0xd2b2du;
        case next_check:return 0xd2b3eu;
        case handle_store:return 0xd2b49u;
        case unit_word:return 0xd2b4cu;
        case word_store:return 0xd2b54u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV ECX,[ESI+C]; XOR EAX,EAX; CMP ECX,-1.
        case first_next:gp[ecx]=v;gp[eax]=fl.logic(0u,4);fl.minus(v,logic::none,4);
            if(v==logic::none)this->reached(Branch::empty);break;
        case headers:gp[ebp]=v;this->push(0xd2b11u,gp[edi]);break;                                 // PUSH EDI
        // MOV EDI,[ESI+C]; MOV ECX,EDI; AND ECX,FFFF; LEA EAX,[ECX+ECX*2].
        case next_handle:gp[edi]=v;gp[ecx]=fl.logic(v&0xffffu,4);gp[eax]=gp[ecx]*3u;break;
        case header_datums:gp[ecx]=v;break;
        case object_address:gp[edx]=v;break;
        case object_next:gp[eax]=v;break;
        // MOV CL,[EDX+AA]; MOV EAX,1; SHL EAX,CL; TEST AL,3.
        case object_type:set_low_byte(gp[ecx],v);gp[eax]=fl.shift(1u,v&0xffu,4);this->test8(gp[eax]&3u);break;
        case next_check:fl.minus(v,logic::none,4);this->reached(v==logic::none?Branch::ended:Branch::passed);break;   // CMP [ESI+C],-1
        // MOV CX,[EDX+1FC]; POP EDI.
        case unit_word:gp[ecx]=(gp[ecx]&0xffff0000u)|(v&0xffffu);gp[edi]=this->pop(0xd2b53u);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        Memory<UnitChain> memory{*this};
        const auto result=logic::unit_chain::next(memory,gp[esi]);
        if(result.found) {
            this->reached(Branch::found);
            gp[eax]=result.unit;gp[ebp]=this->pop(0xd2b5au);                                        // MOV EAX,EDX; POP EBP
        } else if(walked) {
            gp[edi]=this->pop(0xd2b44u);gp[eax]=this->flags.logic(0u,4);gp[ebp]=this->pop(0xd2b47u);
            this->ret(0xd2b48u,0);this->publish(0xd2b48u);return;
        }
        this->ret(0xd2b5bu,0);this->publish(0xd2b5bu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xd2b00u,0xd2b5bu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return UnitChain<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_unit_chain_detail
