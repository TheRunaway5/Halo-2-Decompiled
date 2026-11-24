#pragma once
// Boundary adapter for the carrier, entered at D2B60
// (native_tu15_carrier_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the handle, EBX the result (RET); ECX, EBP,
// ESI and EDI saved; EAX, ECX and EDX the last values the original leaves
// them; the flags of the last test. The original keeps a one in a stack
// local (the saved ECX's slot) for its type test; the adapter makes those
// accesses. Each access publishes its original instruction.
// HALO2_REFERENCE_CARRIER=1 restores the translated body.
// HALO2_VERIFY_CARRIER=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/objects/native_tu15_carrier_systems.hpp"

namespace halo2::native_carrier_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_carrier;
using Access=logic::carrier::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_CARRIER");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_CARRIER");return v;}
enum Branch : unsigned {loose, detached, uncarried, climbed, branch_count};
static_assert(unsigned(logic::carrier::Outcome::uncarried)==Branch::uncarried);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Carrier"};return v;}
template<class State> struct Adapter : Port<State> {
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    void climbed(){this->gp[edi]=this->gp[edx];this->reached(Branch::climbed);}                      // MOV EDI,EDX
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case objects:return 0xd2b66u;
        case object_data:return 0xd2b6bu;
        case object:gp[eax]=gp[edi];gp[ecx]=3u*this->flags.logic(gp[eax]&0xffffu,4);gp[eax]&=0xffffu;return 0xd2b78u;   // MOV EAX,EDI; AND; LEA
        case parent:return 0xd2b7cu;
        case flags:return 0xd2b84u;
        case parent_object:gp[eax]=gp[edx];gp[ecx]=3u*this->flags.logic(gp[eax]&0xffffu,4);gp[eax]&=0xffffu;return 0xd2b98u;
        case type:return 0xd2ba5u;
        case first:return 0xd2bb9u;
        case second:return 0xd2bbbu;
        case field:return 0xd2bbeu;
        case field_store:gp[edi]=this->pop(0xd2bc1u);gp[esi]=this->pop(0xd2bc2u);return 0xd2bc3u;        // POP EDI; POP ESI
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case objects:gp[eax]=v;break;
        case object_data:gp[esi]=v;break;
        case object:gp[ebp]=v;break;
        case parent:gp[edx]=v;this->compare32(v,logic::none);break;                                     // CMP EDX,-1
        case flags:gp[eax]=v>>26;this->test8(gp[eax]&1u);break;                                         // SHR EAX,1A; TEST AL,1
        // MOV ECX,1; MOV [ESP+C],ECX.
        case parent_object:gp[eax]=v;gp[ecx]=1u;this->store(0xd2ba1u,gp[esp]+0xcu,4,1u);break;
        // MOV CL,[EAX+AA]; MOV EAX,[ESP+C]; SHL EAX,CL; TEST AL,3.
        case type:{
            set_low_byte(gp[ecx],v);
            const auto one=this->read(0xd2babu,gp[esp]+0xcu,4);
            gp[eax]=one<<(v&31u);(void)fl;this->test8(gp[eax]&logic::carrier_types);
            break;
        }
        case field:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        this->push(0xd2b60u,gp[ecx]);this->push(0xd2b61u,gp[ebp]);this->push(0xd2b62u,gp[esi]);this->push(0xd2b63u,gp[edi]);
        gp[edi]=gp[eax];                                                                             // MOV EDI,EAX
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::carrier::of(memory,*this,gp[edi],gp[ebx])));
        gp[ebp]=this->pop(0xd2bc6u);gp[ecx]=this->pop(0xd2bc7u);
        this->ret(0xd2bc8u,0u);this->publish(0xd2bc8u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0xd2b60u,0xd2bc8u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_carrier_detail
