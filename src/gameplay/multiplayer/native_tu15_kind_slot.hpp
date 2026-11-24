#pragma once
// Boundary adapter for the kind slot, 15CAB0
// (native_tu15_kind_slot_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EDI the kinds, EDX the kind, the count on the stack
// (RET 4), the index in EAX; EBP and ESI saved (and EBX while the kinds are
// compared). Each access publishes its original instruction; the compare
// tree leaves BL, ECX and the flags as the original's does.
// HALO2_REFERENCE_KIND_SLOT=1 restores the translated body.
// HALO2_VERIFY_KIND_SLOT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_kind_slot_systems.hpp"

namespace halo2::native_kind_slot_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_kind_slot;
using Access=logic::kind_slot::Access;
using Outcome=logic::kind_slot::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_KIND_SLOT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_KIND_SLOT");return v;}
enum Branch : unsigned {none_listed, matched, none_empty, empty_found, branch_count};
static_assert(unsigned(Outcome::empty_found)==Branch::empty_found);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Kind slot"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x15cab0u,0x15cab6u,0x15cabdu,0x15cac2u,0x15cad7u,0x15cb46u};
template<class State> struct Adapter : Port<State> {
    bool compared{};            // EBX saved for the compare loop
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t argument()const{return this->gp[esp]+8u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        if(a==listed && !compared){this->push(0x15cad6u,gp[ebx]);compared=true;}                       // PUSH EBX
        if(a==listed || a==empty) {
            // The loop's count test, before each entry.
            (void)fl;
        }
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case session:case index:gp[ecx]=v;if(a==index)this->push(0x15cabcu,gp[ebp]);break;              // PUSH EBP
        case count:gp[ebp]=v;this->push(0x15cac1u,gp[esi]);break;                                      // PUSH ESI
        case variant:                                                                                  // OR EAX,-1; TEST ESI,ESI; XOR ESI/ECX; TEST EBP,EBP
            gp[esi]=v;gp[eax]=fl.logic(0xffffffffu,4);this->test32(v);
            if(v)gp[esi]=fl.logic(0u,4);else gp[ecx]=fl.logic(0u,4);
            this->test32(gp[ebp]);break;
        case listed:set_low_word(gp[ecx],v);break;                                                    // MOV CX,[EDI+ESI*2]
        case empty:                                                                                   // CMP word [EDI+ECX*2],0; INC ECX; CMP ECX,EBP
            fl.minus(v&0xffffu,0u,2);
            if(v&0xffffu){gp[ecx]=fl.increment(gp[ecx],4,false);fl.minus(gp[ecx],gp[ebp],4);}
            break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The compare tree, as the original runs it.
    void considered(std::uint32_t word,std::uint32_t kind,bool taken) {
        auto &gp=this->gp;auto &fl=this->flags;
        namespace k=logic::kinds;
        gp[ebx]=logic::widen16(word);fl.minus(gp[ebx],kind,4);set_low_byte(gp[ebx],gp[ebx]==kind?1u:0u);   // MOVSX EBX,CX; CMP EBX,EDX; SETZ BL
        bool kind_paired=false;
        fl.minus(kind,k::other_paired,4);
        if(kind==k::other_paired)kind_paired=true;
        else{fl.minus(kind,k::paired,4);kind_paired=kind==k::paired;}
        if(kind_paired) {
            fl.minus(word,k::other_paired,2);
            if(word==k::other_paired)set_low_byte(gp[ebx],1u);
            else{fl.minus(word,k::paired,2);if(word==k::paired)set_low_byte(gp[ebx],1u);}
        }
        fl.minus(word,k::any,2);
        if(word==k::any)set_low_byte(gp[ebx],fl.logic((gp[ebx]|1u)&0xffu,1));                            // OR BL,1
        else {
            fl.minus(word,k::open,2);
            bool open=word==k::open;
            if(!open){fl.minus(word,k::other_open,2);open=word==k::other_open;}
            if(open) {
                fl.minus(kind,k::paired,4);
                bool zero=kind==k::paired;
                if(!zero){fl.minus(kind,k::other_paired,4);zero=kind==k::other_paired;}
                gp[ecx]=zero?fl.logic(0u,4):1u;                                                         // XOR ECX,ECX or MOV ECX,1
                set_low_byte(gp[ebx],fl.logic((gp[ebx]|gp[ecx])&0xffu,1));                              // OR BL,CL
            }
        }
        this->test8(gp[ebx]&0xffu);                                                                    // TEST BL,BL
        if(!taken){gp[esi]=fl.increment(gp[esi],4,false);fl.minus(gp[esi],gp[ebp],4);}                 // INC ESI; CMP ESI,EBP
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        std::uint32_t answer{};
        const auto outcome=logic::kind_slot::find(memory,*this,gp[edi],gp[edx],answer);
        this->reached(unsigned(outcome));
        switch(outcome) {
        case Outcome::matched:
            gp[ebx]=this->pop(0x15cb38u);gp[eax]=answer;gp[esi]=this->pop(0x15cb3bu);gp[ebp]=this->pop(0x15cb3cu);
            this->ret(0x15cb3du,4u);this->publish(0x15cb3du);return;
        case Outcome::empty_found:
            gp[esi]=this->pop(0x15cb57u);gp[eax]=answer;gp[ebp]=this->pop(0x15cb5au);
            this->ret(0x15cb5bu,4u);this->publish(0x15cb5bu);return;
        case Outcome::none_empty:
            if(std::int32_t(gp[ebp])>0){gp[esi]=this->pop(0x15cb52u);gp[ebp]=this->pop(0x15cb53u);this->ret(0x15cb54u,4u);this->publish(0x15cb54u);return;}
            break;
        case Outcome::none_listed:
            if(compared)gp[ebx]=this->pop(0x15cb32u);
            break;
        }
        gp[esi]=this->pop(0x15cb33u);gp[ebp]=this->pop(0x15cb34u);
        this->ret(0x15cb35u,4u);this->publish(0x15cb35u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x15cab0u,0x15cb5bu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_kind_slot_detail
