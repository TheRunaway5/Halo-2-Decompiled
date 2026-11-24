#pragma once
// Boundary adapter for the array grow, entered at 2E3250
// (native_tu15_array_grow_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: the array and the element size
// on the stack (cdecl); EBX, EBP, ESI and EDI saved; the allocator's
// functions (through the unit's dispatch) get ECX the allocator and the
// size and tag (allocate) or the data, size and tag (release) pushed (they
// pop them), 2E2E30 the new data, the old data and the bytes pushed (cdecl),
// every register each returns coming back; the flags of the capacity's TEST
// (or the release's). Each access publishes its original instruction.
// HALO2_REFERENCE_ARRAY_GROW=1 restores the translated body.
// HALO2_VERIFY_ARRAY_GROW=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/renderer/native_tu15_array_grow_systems.hpp"

namespace halo2::native_array_grow_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_array_grow;
using Access=logic::array_grow::Access;
using Kept=logic::array_grow::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ARRAY_GROW");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ARRAY_GROW");return v;}
enum Branch : unsigned {kept_storage, released, branch_count};
static_assert(unsigned(logic::array_grow::Outcome::released)==Branch::released);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Array grow"};return v;}
template<class State,class Route,class Copy> struct Adapter : Port<State> {
    Route &route;
    Copy &copy_child;
    std::uint32_t depth{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Route &r,Copy &k):Port<State>(s,c,t,{}),route(r),copy_child(k){}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u*k;}
    void refresh(Kept &kept){kept={this->gp[esi],this->gp[ebx],this->gp[edi],this->gp[ebp]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case array:return 0x2e3253u;
        case count:return 0x2e3257u;
        case size:return 0x2e3267u;
        case allocator:return 0x2e326bu;
        case vtable:return 0x2e3271u;
        case allocate:return 0x2e327bu;
        case old_data:return 0x2e327eu;
        case count_again:return 0x2e3282u;
        case capacity:return 0x2e3290u;
        case allocator_again:return 0x2e329au;
        case vtable_again:return 0x2e32a0u;
        case data_again:return 0x2e32adu;
        case release:put(0x2e32afu,gp[eax]);return 0x2e32b0u;                                             // PUSH EAX (the data)
        case data_store:return 0x2e32b3u;
        case capacity_store:gp[edi]=this->pop(0x2e32b5u);depth-=4u;return 0x2e32b6u;                     // POP EDI
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case array:gp[esi]=v;break;
        // MOV EAX,[ESI+4]; TEST EAX,EAX; PUSH EDI (LEA EBP,[EAX+EAX] or MOV EBP,1: allocating()).
        case count:gp[eax]=v;this->test32(v);put(0x2e325cu,gp[edi]);break;
        case size:gp[ebx]=v;break;
        case allocator:case allocator_again:gp[ecx]=v;break;
        case vtable:gp[eax]=v;break;
        case vtable_again:gp[edx]=v;break;
        case old_data:gp[ecx]=v;break;
        case count_again:gp[eax]=fl.product(v,gp[ebx]);break;                                            // IMUL EAX,EBX
        // MOV EAX,[ESI+8]; ADD ESP,C; TEST EAX,EAX.
        case capacity:gp[eax]=v;gp[esp]=fl.plus(gp[esp],0xcu,4);depth-=0xcu;this->test32(v);break;
        case data_again:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // EBP the capacity; MOV EDX,EBP; IMUL EDX,EBX; PUSH 12; PUSH EDX.
    void allocating(const Kept &kept,std::uint32_t) {
        auto &gp=this->gp;
        gp[ebp]=kept.capacity;gp[edx]=this->flags.product(gp[ebp],gp[ebx]);put(0x2e3278u,logic::tag);put(0x2e327au,gp[edx]);
    }
    std::uint32_t allocate(Kept &kept,std::uint32_t function) {
        this->call_routed(route,0x2e327bu,0x2e327eu,function);depth-=8u;
        refresh(kept);
        this->gp[edi]=this->gp[eax];kept.data=this->gp[eax];                                             // MOV EDI,EAX
        return this->gp[eax];
    }
    // PUSH EAX; PUSH ECX; PUSH EDI; CALL 2E2E30.
    void copy(Kept &kept,std::uint32_t to,std::uint32_t from,std::uint32_t bytes) {
        put(0x2e3288u,bytes);put(0x2e3289u,from);put(0x2e328au,to);
        this->call(0x2e328bu,0x2e3290u,0x2e2e30u,copy_child);
        refresh(kept);
    }
    // AND EAX,7FFFFFFF; IMUL EAX,EBX; PUSH 12; PUSH EAX.
    void releasing(std::uint32_t bytes) {
        auto &fl=this->flags;
        this->gp[eax]=fl.product(fl.logic(this->gp[eax]&~logic::unowned,4),this->gp[ebx]);(void)bytes;
        put(0x2e32aau,logic::tag);put(0x2e32acu,this->gp[eax]);
    }
    void release(Kept &kept,std::uint32_t function,std::uint32_t,std::uint32_t) {
        this->call_routed(route,0x2e32b0u,0x2e32b3u,function);depth-=0xcu;
        refresh(kept);
    }
    void run() {
        auto &gp=this->gp;
        put(0x2e3250u,gp[ebx]);put(0x2e3251u,gp[ebp]);put(0x2e3252u,gp[esi]);
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::array_grow::grow(memory,*this)));
        gp[esi]=this->pop(0x2e32b9u);gp[ebp]=this->pop(0x2e32bau);gp[ebx]=this->pop(0x2e32bbu);
        this->ret(0x2e32bcu,0u);this->publish(0x2e32bcu);
    }
};
template<class State,class Reference,class Route,class Copy> bool bridge(State &s,Reference reference,Route route,Copy copy) {
    return run_owner(s,0x2e3250u,0x2e32bcu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Route,Copy>(v,c,t,route,copy);});
}
} // namespace halo2::native_array_grow_detail
