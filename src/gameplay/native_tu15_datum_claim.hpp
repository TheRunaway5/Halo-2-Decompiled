#pragma once
// Boundary adapter for the datum claim, entered at 16E7F0
// (native_tu15_datum_claim_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: EDX the array, EDI the index (RET);
// ESI (and EBX around the bit) saved; 16E860 (direct in one unit, through
// the dispatch in the other: the bridge's `set_up`) gets the registers as
// the original leaves them, every register it returns coming back; EAX the
// handle (NONE by OR EAX,-1), ECX the new count or the data; the flags of
// the handle's OR. Each access publishes its original instruction.
// HALO2_REFERENCE_DATUM_CLAIM=1 restores the translated body.
// HALO2_VERIFY_DATUM_CLAIM=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/native_tu15_datum_claim_systems.hpp"

namespace halo2::native_datum_claim_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_datum_claim;
using Access=logic::datum_claim::Access;
using Kept=logic::datum_claim::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_DATUM_CLAIM");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_DATUM_CLAIM");return v;}
enum Branch : unsigned {out_of_range, taken, claimed, raised, branch_count};
static_assert(unsigned(logic::datum_claim::Outcome::claimed)==Branch::claimed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Datum claim"};return v;}
template<class State,class SetUp> struct Adapter : Port<State> {
    SetUp &set_up_child;
    bool saved{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,SetUp &u):Port<State>(s,c,t,{}),set_up_child(u){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case limit:return 0x16e7f4u;
        case data:return 0x16e7f9u;
        case datum_size:this->push(0x16e7fcu,gp[esi]);saved=true;return 0x16e7fdu;                       // PUSH ESI
        case salt:return 0x16e805u;
        case bitmap:return 0x16e80bu;
        // MOV EAX,EDI; SAR EAX,5; LEA EAX,[ECX+EAX*4]; PUSH EBX; MOV ECX,EDI; AND ECX,1F; MOV EBX,1; SHL EBX,CL.
        case bits:case bits_store:
            if(a==bits) {
                gp[eax]=gp[ecx]+4u*fl.right(gp[edi],5u,4,true);this->push(0x16e816u,gp[ebx]);
                gp[ecx]=fl.logic(gp[edi]&31u,4);gp[ebx]=fl.shift(1u,gp[ecx]&31u,4);
            }
            return 0x16e823u;
        case count:return 0x16e825u;
        case high:return 0x16e828u;
        case count_store:return 0x16e82eu;
        case high_store:gp[eax]=gp[edi]+1u;return 0x16e837u;                                              // LEA EAX,[EDI+1]
        case salt_again:return 0x16e83fu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case limit:this->compare32(gp[edi],v);break;                                                     // CMP EDI,[EDX+20]
        case data:gp[ecx]=v;break;
        // MOV ESI,[EDX+24]; IMUL ESI,EDI; ADD ESI,ECX.
        case datum_size:gp[esi]=fl.plus(fl.product(v,gp[edi]),gp[ecx],4);break;
        case salt:fl.minus(v,0u,2);break;                                                                // CMP word [ESI],0
        case bitmap:gp[ecx]=v;break;
        case count:gp[ecx]=v;break;
        // MOV EAX,[EDX+38]; INC ECX; CMP EDI,EAX.
        case high:gp[eax]=v;gp[ecx]=fl.increment(gp[ecx],4,false);this->compare32(gp[edi],v);break;
        // MOVSX EAX,word [ESI]; SHL EAX,10; OR EAX,EDI.
        case salt_again:fl.shift(native_object_update::widen16(v&0xffffu),16u,4);break;                 // MOVSX EAX,word [ESI]; SHL EAX,10 (OR EAX,EDI below)
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        if(a==Access::bits_store)this->flags.logic(v,4);                                                 // OR [EAX],EBX
        if(a==Access::count_store)this->gp[ebx]=this->pop(0x16e831u);                                    // POP EBX
        if(a==Access::high_store)this->reached(Branch::raised);
    }
    void set_up(Kept &kept) {
        this->call(0x16e83au,0x16e83fu,0x16e860u,set_up_child);
        kept={this->gp[esi],this->gp[edi]};
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        using Outcome=logic::datum_claim::Outcome;
        this->test32(gp[edi]);                                                                           // TEST EDI,EDI
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        const auto handle=logic::datum_claim::claim(memory,*this,gp[edx],gp[edi],outcome);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::out_of_range){gp[eax]=fl.logic(0xffffffffu,4);this->ret(0x16e851u,0u);this->publish(0x16e851u);return;}
        if(outcome==Outcome::taken){gp[eax]=fl.logic(0xffffffffu,4);gp[esi]=this->pop(0x16e84cu);this->ret(0x16e84du,0u);this->publish(0x16e84du);return;}
        gp[eax]=fl.logic(handle,4);                                                                     // the answer
        gp[esi]=this->pop(0x16e847u);this->ret(0x16e848u,0u);this->publish(0x16e848u);
    }
};
template<class State,class Reference,class SetUp> bool bridge(State &s,Reference reference,SetUp set_up) {
    return run_owner(s,0x16e7f0u,0x16e851u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,SetUp>(v,c,t,set_up);});
}
} // namespace halo2::native_datum_claim_detail
