#pragma once
// Boundary adapter for the linked iterator, entered at 1A8F10
// (native_tu15_filtered_iterator_systems.hpp, linked_iterator). Its callers
// and 16EA60 keep the original ABI, which the adapter owns: ESI the
// iterator; EBP, EDI and (once a record is in hand) EBX saved; 16EA60 gets
// ECX the array and EDX the start index and returns EAX, and every register
// it returns comes back; ECX, EDX and EAX (the result in AL) as the
// original leaves them; the flags of the last instruction that sets them;
// RET. Each access publishes its original instruction.
// HALO2_REFERENCE_LINKED_ITERATOR=1 restores the translated body.
// HALO2_VERIFY_LINKED_ITERATOR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_filtered_iterator_systems.hpp"

namespace halo2::native_linked_iterator_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_filtered_iterator;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_LINKED_ITERATOR");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_LINKED_ITERATOR");return p && *p;}();
    return value;
}
enum Branch : unsigned {first_found, first_end, kept_record, passed_over, scan_found, scan_past, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Linked iterator verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Route> struct Iteration : Port<State> {
    using Access=logic::linked_iterator::Access;
    using Kept=logic::linked_iterator::Kept;
    Route &route;bool saved_ebx{};
    Iteration(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    Kept entry()const{return {this->gp[esi],0};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case index_read:return 0x1a8f10u;
        case array_read:return 0x1a8f15u;
        case size:return 0x1a8f25u;
        case data:return 0x1a8f28u;
        case index_store:return 0x1a8f30u;
        case salt:return 0x1a8f33u;
        case handle_store:return 0x1a8f3bu;
        case maximum:return 0x1a8f40u;
        case index_end:return 0x1a8f43u;
        case handle_clear:return 0x1a8f46u;
        case record_store:this->test32(gp[ecx]);return 0x1a8f51u;                                   // TEST ECX,ECX
        case current:if(!saved_ebx){saved_ebx=true;this->push(0x1a8f55u,gp[ebx]);}return 0x1a8f56u;
        case filter:return 0x1a8f58u;
        case index_reload:return 0x1a8f5eu;
        case array_reload:return 0x1a8f62u;
        case bound:return 0x1a8f67u;
        case bits:return 0x1a8f6eu;
        // MOV ECX,EAX; AND ECX,1F; MOV EBP,1; SHL EBP,CL; MOV ECX,EAX; SAR ECX,5.
        case bit_word:
            gp[ecx]=this->flags.logic(gp[eax]&0x1fu,4);gp[ebp]=this->flags.shift(1u,gp[ecx]&0xffu,4);
            gp[ecx]=this->flags.right(gp[eax],5u,4,true);return 0x1a8f82u;
        case next_size:this->compare32(gp[eax],logic::none);return 0x1a8fb5u;                        // CMP EAX,-1
        case next_data:return 0x1a8fb8u;
        case next_index_store:return 0x1a8fc0u;
        case next_salt:return 0x1a8fc3u;
        case next_handle_store:return 0x1a8fcbu;
        case next_maximum:return 0x1a8f8cu;
        case next_index_end:return 0x1a8f8fu;
        case next_handle_clear:return 0x1a8f92u;
        case next_record_store:gp[eax]=gp[ecx];this->test32(gp[eax]);return 0x1a8f9fu;            // MOV EAX,ECX; TEST
        case final_record:if(saved_ebx)gp[ebx]=this->pop(0x1a8fa3u);return 0x1a8fa4u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case index_read:gp[edx]=v;this->push(0x1a8f13u,gp[ebp]);this->push(0x1a8f14u,gp[edi]);break;
        // MOV EDI,[ESI+4]; INC EDX; MOV ECX,EDI.
        case array_read:gp[edi]=v;gp[edx]=this->flags.increment(gp[edx],4,false);gp[ecx]=gp[edi];break;
        case size:case next_size:gp[ecx]=v;break;
        // MOV EBP,[...+44]; IMUL ECX,EAX; ADD ECX,EBP.
        case data:case next_data:gp[ebp]=v;gp[ecx]=this->flags.plus(this->flags.product(gp[ecx],gp[eax]),gp[ebp],4);break;
        case salt:case next_salt:gp[edx]=this->flags.logic(this->flags.shift(logic::widen16(v),16u,4)|gp[eax],4);break;
        case maximum:case next_maximum:gp[eax]=v;if(a==next_maximum)this->reached(Branch::scan_past);break;
        case current:gp[ecx]=v;break;
        // CMP [ECX+2C],-1.
        case filter:this->compare32(v,logic::none);this->reached(v==logic::none?Branch::passed_over:Branch::kept_record);break;
        case index_reload:gp[eax]=this->flags.increment(v,4,false);break;
        case array_reload:gp[edx]=v;break;
        case bound:gp[ebx]=v;this->compare32(gp[eax],v);break;
        case bits:gp[edi]=v;break;
        case bit_word:
            this->test32(v&gp[ebp]);
            if(!(v&gp[ebp])){gp[eax]=this->flags.increment(gp[eax],4,false);this->compare32(gp[eax],gp[ebx]);}
            else this->reached(Branch::scan_found);
            break;
        // MOV ECX,[ESI]; XOR EAX,EAX; TEST ECX,ECX; POP EDI; SETNZ AL; POP EBP.
        case final_record:
            gp[ecx]=v;gp[eax]=this->flags.logic(0,4);this->test32(v);gp[edi]=this->pop(0x1a8faau);
            gp[eax]=v?1u:0u;gp[ebp]=this->pop(0x1a8faeu);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        if(a==Access::handle_clear||a==Access::next_handle_clear)this->gp[ecx]=this->flags.logic(0,4);   // XOR ECX,ECX
    }
    // CALL 16EA60; CMP EAX,-1.
    std::uint32_t first(Kept &kept,std::uint32_t) {
        auto &gp=this->gp;
        this->call_routed(route,0x1a8f1bu,0x1a8f20u,0x16ea60u);
        this->compare32(gp[eax],logic::none);
        kept={gp[esi],gp[edi]};
        this->reached(gp[eax]!=logic::none?Branch::first_found:Branch::first_end);
        return gp[eax];
    }
    void run() {
        Memory<Iteration> memory{*this};
        logic::linked_iterator::next(memory,*this);
        this->ret(0x1a8fafu,0);this->publish(0x1a8fafu);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1a8f10u,0x1a8fafu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Iteration<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_linked_iterator_detail
