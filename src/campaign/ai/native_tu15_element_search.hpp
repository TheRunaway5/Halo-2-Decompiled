#pragma once
// Boundary adapter for the element search, 1E5360
// (native_tu15_element_search_systems.hpp). Its callers and the child keep
// the original ABI, which the adapter owns: EDX the definition, the output's
// address and the value on the stack (RET 8); a 0xC-byte frame below the
// saved EBX and EBP (ESI and EDI saved past it while searching); 1E5C80 gets
// the packed answer pushed (RET 4), every register it returns coming back.
// Each access publishes its original instruction.
// HALO2_REFERENCE_ELEMENT_SEARCH=1 restores the translated body.
// HALO2_VERIFY_ELEMENT_SEARCH=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "campaign/ai/native_tu15_element_search_systems.hpp"

namespace halo2::native_element_search_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_element_search;
using Access=logic::element_search::Access;
using Kept=logic::element_search::Kept;
using Outcome=logic::element_search::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ELEMENT_SEARCH");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ELEMENT_SEARCH");return v;}
enum Branch : unsigned {no_definition, found_own, found_referenced, exhausted, unresolved, branch_count};
static_assert(unsigned(Outcome::unresolved)==Branch::unresolved);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Element search"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
template<class State,class Report> struct Adapter : Port<State> {
    Report &report_child;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Report &r):Port<State>(s,c,t,{}),report_child(r){}
    std::uint32_t answer()const{return this->gp[esp]+depth-0xcu;}
    std::uint32_t index()const{return this->gp[esp]+depth-8u;}
    std::uint32_t offset()const{return this->gp[esp]+depth-4u;}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case reference_init:return 0x1e536au;
        case element_init:return 0x1e536fu;
        case index_init:gp[ebx]=gp[edx];return 0x1e5376u;                                              // MOV EBX,EDX
        case empty_packed:return 0x1e5448u;
        case empty_output:return 0x1e544cu;
        case empty_answer:gp[ebp]=take(0x1e5450u);return 0x1e5451u;
        case offset_init:put(0x1e5380u,gp[esi]);return 0x1e5381u;                                     // PUSH ESI
        case element_count:return 0x1e539au;
        case elements:return 0x1e53acu;
        case value:return 0x1e53b1u;
        case element_value:return 0x1e53b5u;
        case element_reload:return 0x1e53c1u;
        case reference_word:fl.minus(gp[esi]&0xffffu,logic::none_word,2);return 0x1e53ccu;            // CMP SI,-1
        case reference_store:return 0x1e53d4u;
        case element_store:return 0x1e53d9u;
        case index_load:return 0x1e53e0u;
        case offset_load:return 0x1e53e4u;
        case reference_count:return 0x1e53e8u;
        case index_store:return 0x1e53f1u;
        case offset_store:return 0x1e53f5u;
        case reference_array:return 0x1e53fbu;
        case reference_tag:gp[edi]=gp[esi];gp[eax]=fl.plus(gp[eax],gp[edi],4);return 0x1e5402u;       // MOV EDI,ESI; ADD EAX,EDI
        case instances:return 0x1e540cu;
        case tag_data:gp[eax]=fl.shift(fl.logic(gp[eax]&0xffffu,4),4u,4);return 0x1e541au;           // AND EAX,FFFF; SHL EAX,4
        case found_packed:fl.minus(gp[ebp]&0xffffu,logic::none_word,2);return 0x1e542eu;              // CMP BP,-1
        case found_output:return 0x1e5438u;
        case found_answer:gp[edi]=take(0x1e543cu);return 0x1e543du;
        case missing_packed:return 0x1e545au;
        case missing_output:return 0x1e545eu;
        case missing_answer:gp[edi]=take(0x1e5462u);gp[esi]=take(0x1e5463u);gp[ebp]=take(0x1e5464u);return 0x1e5465u;
        }
        return 0u;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case empty_packed:gp[ecx]=v;break;
        case empty_output:case found_output:case missing_output:gp[eax]=v;break;
        case element_count:gp[edi]=v;gp[esi]=fl.logic(0u,4);this->test32(v);break;                   // XOR ESI,ESI; TEST EDI,EDI
        case elements:gp[eax]=fl.plus(v,gp[ecx],4);break;                                            // ADD EAX,ECX
        case value:gp[ebp]=v;break;
        case element_value:fl.minus(v,gp[ebp],4);break;                                              // CMP [EAX],EBP
        case element_reload:set_low_word(gp[ebp],v);break;
        case reference_word:set_low_word(gp[eax],v);set_low_word(gp[ebp],gp[esi]);break;              // MOV BP,SI
        case index_load:gp[eax]=v;break;
        case offset_load:gp[esi]=v;break;
        case reference_count:                                                                         // INC EAX; ADD ESI,20; CMP EAX,ECX
            gp[ecx]=v;gp[eax]=fl.increment(gp[eax],4,false);gp[esi]=fl.plus(gp[esi],logic::reference::size,4);fl.minus(gp[eax],gp[ecx],4);break;
        case reference_array:gp[eax]=v;break;
        case reference_tag:gp[eax]=v;gp[ecx]=fl.logic(0u,4);fl.minus(v,logic::none,4);break;          // XOR ECX,ECX; CMP EAX,-1
        case instances:case tag_data:gp[ecx]=v;break;
        case found_packed:gp[esi]=v;break;
        case missing_packed:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t){if(a==Access::offset_init)put(0x1e5389u,this->gp[edi]);}   // PUSH EDI
    // XOR ECX,ECX first; then INC ESI; ADD ECX,6C; XOR EAX,EAX; CMP ECX,-6C.
    void element(std::uint32_t index_value,std::uint32_t offset_value) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esi]=index_value;gp[ecx]=offset_value;gp[eax]=fl.logic(0u,4);fl.minus(offset_value,logic::null_offset,4);
    }
    // MOV EBX,ECX; TEST EBX,EBX.
    void next_block(std::uint32_t){this->gp[ebx]=this->gp[ecx];this->test32(this->gp[ebx]);}
    // PUSH ESI; CALL 1E5C80.
    void report(std::uint32_t,Kept &kept) {
        put(0x1e5432u,this->gp[esi]);
        this->call(0x1e5433u,0x1e5438u,0x1e5c80u,report_child);depth-=4u;
        kept.packed=this->gp[esi];
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0xcu,4);depth=0xcu;                                                 // SUB ESP,C
        put(0x1e5363u,gp[ebx]);put(0x1e5364u,gp[ebp]);
        gp[ebp]=fl.logic(gp[ebp]|0xffffffffu,4);this->test32(gp[edx]);                               // OR EBP,-1; TEST EDX,EDX
        Memory<Adapter> memory{*this};
        const auto outcome=logic::element_search::find(memory,*this,gp[edx]);
        this->reached(unsigned(outcome));
        std::uint32_t exit;
        switch(outcome) {
        case Outcome::no_definition:gp[ebx]=take(0x1e5453u);exit=0x1e5454u;break;
        case Outcome::found_own:case Outcome::found_referenced:
            gp[esi]=take(0x1e543fu);gp[ebp]=take(0x1e5440u);gp[ebx]=take(0x1e5441u);exit=0x1e5442u;break;
        default:gp[ebx]=take(0x1e5467u);exit=0x1e5468u;break;
        }
        gp[esp]=fl.plus(gp[esp],0xcu,4);                                                             // ADD ESP,C
        this->ret(exit+3u,8u);this->publish(exit+3u);
    }
};
template<class State,class Reference,class Report> bool bridge(State &s,Reference reference,Report report) {
    return run_owner(s,0x1e5360u,0x1e546bu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Report>(v,c,t,report);});
}
} // namespace halo2::native_element_search_detail
