#pragma once
// Boundary adapter for the list slots, 1AFA90
// (native_tu15_list_slots_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: the slots, the list, the defaults and their count
// pushed (RET 0x10), the slots filled in EAX; ECX pushed (its low word the
// list's count), EBX, EBP, ESI and EDI saved. Its jump table goes through the
// unit's canonical branch mapping, so an unbound entry raises the translated
// body's error. Each access publishes its original instruction.
// HALO2_REFERENCE_LIST_SLOTS=1 restores the translated body.
// HALO2_VERIFY_LIST_SLOTS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "campaign/ai/native_tu15_list_slots_systems.hpp"
#include <stdexcept>

namespace halo2::native_list_slots_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_list_slots;
using Access=logic::list_slots::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_LIST_SLOTS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_LIST_SLOTS");return v;}
enum Branch : unsigned {filled, stopped, same_id, never, default_id, when_spent, always, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"List slots"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x1afa91u,0x1afa99u,0x1afa9fu,0x1afaabu,0x1afab1u,0x1afab5u,0x1afabcu,0x1afac0u,0x1afac7u,0x1afad3u,
    0x1afadbu,0x1afae2u,0x1afb1cu,0x1afb21u,0x1afb2au,0x1afae8u,0x1afb31u,0x1afb3bu,0x1afb3fu,0x1afaefu,0x1afb00u,0x1afb03u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::default_record_id)+1u);
template<class State,class Branches> struct Adapter : Port<State> {
    Branches &branch_of;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Branches &b):Port<State>(s,c,t,{}),branch_of(b){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case first_id:gp[ecx]=v;break;
        case defaults_argument:case entry_slots:case defaults_again:gp[ebp]=v;break;
        case list_argument:case list_again:case entries:case default_slots:gp[edi]=v;break;
        case list_count:set_low_word(gp[edx],v);break;
        case entry_kind:gp[edi]=logic::widen16(v);break;                                                // MOVSX EDI,word [EDX]
        case default_id:set_low_word(gp[edi],v);break;
        case default_record_id:set_low_word(gp[ecx],v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==0u)put(0x1afa90u,gp[ecx]);                                                                // PUSH ECX
        else if(n==1u){put(0x1afa97u,gp[ebx]);put(0x1afa98u,gp[ebp]);}                                  // PUSH EBX; PUSH EBP
        else{put(0x1afa9du,gp[esi]);put(0x1afa9eu,gp[edi]);}                                             // PUSH ESI; PUSH EDI
    }
    void clearing(std::uint32_t list_address) {                                                        // XOR EBX, EAX, ESI; CMP EDI,EBX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ebx]=fl.logic(0u,4);gp[eax]=fl.logic(0u,4);gp[esi]=fl.logic(0u,4);this->compare32(list_address,0u);
    }
    void entry_compared(std::uint32_t entry,std::uint32_t count){this->flags.minus(entry&0xffffu,count&0xffffu,2);}   // CMP BX,[ESP+10]
    void record(std::uint32_t address){this->gp[edx]=address;}                                         // MOVSX EDX,BX; LEA; LEA
    void kind_tested(std::uint32_t kind){this->compare32(kind,logic::last_kind);}                      // CMP EDI,4
    std::uint32_t branch(std::uint32_t target) {                                                       // JMP [1AFB48+EDI*4]
        const auto chosen=branch_of(target,this->s.image);
        namespace target=logic::targets;
        static constexpr std::uint32_t known[]{target::same_id,target::never,target::default_id,target::when_spent,target::always};
        for(unsigned k=0;k<5u;++k)if(known[k]==chosen)this->reached(Branch::same_id+k);
        return chosen;
    }
    [[noreturn]] void unknown_case(){throw std::runtime_error("Required original solver dispatch target unavailable");}
    void ids_compared(std::uint32_t a,std::uint32_t b){this->flags.minus(a&0xffffu,b&0xffffu,2);}    // CMP CX(DI),[EDX+2]
    void default_indexed(std::uint32_t used){this->gp[edi]=5u*logic::widen16(used);}                  // MOVSX EDI,SI; LEA EDI,[EDI+EDI*4]
    void defaults_compared(std::uint32_t used,std::uint32_t count){this->flags.minus(used&0xffffu,count&0xffffu,2);}   // CMP SI,[ESP+24]
    void entry_slot(std::uint32_t slot,std::uint32_t address) {                                        // MOVSX EDI,AX; ADD EDX,4
        auto &gp=this->gp;
        gp[edi]=logic::widen16(slot);gp[edx]=this->flags.plus(address,logic::entry::value,4);
    }
    void entry_used(std::uint32_t entry){this->gp[ebx]=this->flags.increment(entry-1u,4,false);}     // INC EBX
    void default_slot(std::uint32_t slot,std::uint32_t,std::uint32_t chosen) {                         // MOVSX ECX,SI; LEA; MOVSX EDX,AX; LEA ECX
        auto &gp=this->gp;
        gp[ecx]=chosen;gp[edx]=logic::widen16(slot);
    }
    void default_id_taken(std::uint32_t){}
    void default_used(std::uint32_t used){this->gp[esi]=this->flags.increment(used-1u,4,false);}     // INC ESI
    void slot_advanced(std::uint32_t slot) {                                                           // INC EAX; CMP AX,32
        auto &gp=this->gp;
        gp[eax]=this->flags.increment(slot-1u,4,false);this->flags.minus(slot&0xffffu,logic::slots,2);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto filled_slots=logic::list_slots::fill(memory,*this);
        this->reached(filled_slots==logic::slots?Branch::filled:Branch::stopped);
        gp[edi]=take(0x1afb0eu);gp[esi]=take(0x1afb0fu);gp[ebp]=take(0x1afb10u);gp[ebx]=take(0x1afb11u);gp[ecx]=take(0x1afb12u);
        this->ret(0x1afb13u,0x10u);this->publish(0x1afb13u);
    }
};
template<class State,class Reference,class Branches> bool bridge(State &s,Reference reference,Branches branches) {
    return run_owner(s,0x1afa90u,0x1afb44u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Branches>(v,c,t,branches);});
}
} // namespace halo2::native_list_slots_detail
