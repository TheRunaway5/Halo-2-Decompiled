#pragma once
// Boundary adapter for the object allocation, BE2B0
// (native_tu15_object_allocate_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: the size on the stack (RET
// 4), the handle in EAX; ESI saved, and EBX, EBP and EDI once the datum
// exists; 16E6A0 gets EDX the array and answers EAX; 1910A0 ECX the pool and
// the slot, the size and 0 pushed (RET C); 16E8A0 EDX the array and EAX the
// handle (through the unit's dispatch); every register each returns coming
// back. Each access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_ALLOCATE=1 restores the translated body.
// HALO2_VERIFY_OBJECT_ALLOCATE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/objects/native_tu15_object_allocate_systems.hpp"

namespace halo2::native_object_allocate_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_object_allocate;
using Access=logic::object_allocate::Access;
using Kept=logic::object_allocate::Kept;
using Outcome=logic::object_allocate::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_OBJECT_ALLOCATE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_OBJECT_ALLOCATE");return v;}
enum Branch : unsigned {no_datum, allocated, no_memory, branch_count};
static_assert(unsigned(Outcome::no_memory)==Branch::no_memory);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Object allocate"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0xbe2b0u,0xbe2c3u,0xbe2c7u,0xbe2d9u,0xbe2f1u,0xbe2f6u,0xbe2fau,0xbe306u,0xbe30du,0xbe318u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::headers_again)+1u);
template<class State,class New,class Allocate,class Route> struct Adapter : Port<State> {
    New &new_child;
    Allocate &allocate_child;
    Route &route;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,New &n,Allocate &a,Route &r)
        :Port<State>(s,c,t,{}),new_child(n),allocate_child(a),route(r){}
    std::uint32_t argument()const{return this->gp[esp]+depth+4u;}
    std::uint32_t headers_after()const{return this->gp[edx];}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:case headers_again:gp[edx]=v;if(a==headers)put(0xbe2b6u,gp[esi]);break;          // PUSH ESI
        case datums:gp[ecx]=v;break;
        case size:                                                                                   // (MOVSX EBX: allocate()) PUSH EBP; PUSH EDI; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]; LEA EDI,[ECX+EAX*4]
            put(0xbe2ccu,gp[ebp]);put(0xbe2cdu,gp[edi]);
            gp[eax]=3u*fl.logic(gp[eax]&0xffffu,4);gp[edi]=gp[ecx]+4u*gp[eax];break;
        case pool:gp[ecx]=v;break;
        case size_again:set_low_word(gp[edx],v);break;
        case memory_address:gp[edi]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        if(a==Access::word_clear){gp[edi]+=step(4u);--gp[ecx];}
        if(a==Access::byte_clear){gp[edi]+=step(1u);--gp[ecx];}
    }
    std::uint32_t step(std::uint32_t width)const{return (this->s.other_flags&0x400u)?0u-width:width;}
    void refresh(Kept &kept){kept={this->gp[esi],this->gp[ebx],this->gp[ebp],this->gp[edi]};}
    // CALL 16E6A0; MOV ESI,EAX; CMP ESI,-1.
    std::uint32_t new_datum(std::uint32_t) {
        auto &gp=this->gp;
        this->call(0xbe2b7u,0xbe2bcu,0x16e6a0u,new_child);
        gp[esi]=gp[eax];this->flags.minus(gp[esi],logic::none,4);
        return gp[esi];
    }
    void saving(){put(0xbe2c6u,this->gp[ebx]);}
    // EBX the size; PUSH 0; PUSH EBX; LEA EBP,[EDI+8]; PUSH EBP; XOR EAX,EAX; CALL 1910A0; TEST AL,AL.
    std::uint32_t allocate(std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        gp[ebx]=kept.size;put(0xbe2dfu,0u);put(0xbe2e1u,gp[ebx]);gp[ebp]=kept.slot;put(0xbe2e5u,gp[ebp]);gp[eax]=this->flags.logic(0u,4);
        this->call(0xbe2e8u,0xbe2edu,0x1910a0u,allocate_child);depth-=12u;
        this->test8(gp[eax]&0xffu);refresh(kept);
        return gp[eax];
    }
    // MOV ECX,EBX; MOV EDX,ECX; SHR ECX,2; XOR EAX,EAX.
    void words_started(std::uint32_t) {
        auto &gp=this->gp;
        gp[ecx]=gp[ebx];gp[edx]=gp[ecx];gp[ecx]=this->flags.right(gp[ecx],2u,4,false);gp[eax]=this->flags.logic(0u,4);
    }
    void bytes_started(std::uint32_t){this->gp[ecx]=this->flags.logic(this->gp[edx]&3u,4);}           // MOV ECX,EDX; AND ECX,3
    // MOV EAX,ESI; CALL 16E8A0.
    void delete_datum(std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        gp[eax]=gp[esi];this->call_routed(route,0xbe320u,0xbe325u,0x16e8a0u);refresh(kept);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        std::uint32_t answer{};
        const auto outcome=logic::object_allocate::allocate(memory,*this,answer);
        this->reached(unsigned(outcome));
        switch(outcome) {
        case Outcome::allocated:
            gp[edi]=take(0xbe30fu);gp[ebp]=take(0xbe310u);gp[ebx]=take(0xbe311u);gp[eax]=answer;gp[esi]=take(0xbe314u);
            this->ret(0xbe315u,4u);this->publish(0xbe315u);return;
        case Outcome::no_memory:
            gp[edi]=take(0xbe325u);gp[ebp]=take(0xbe326u);gp[eax]=this->flags.logic(answer,4);gp[ebx]=take(0xbe32au);   // OR EAX,-1 (NONE)
            break;
        default:break;
        }
        gp[esi]=take(0xbe32bu);this->ret(0xbe32cu,4u);this->publish(0xbe32cu);
    }
};
template<class State,class Reference,class New,class Allocate,class Route>
bool bridge(State &s,Reference reference,New new_child,Allocate allocate,Route route) {
    return run_owner(s,0xbe2b0u,0xbe32cu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,New,Allocate,Route>(v,c,t,new_child,allocate,route);});
}
} // namespace halo2::native_object_allocate_detail
