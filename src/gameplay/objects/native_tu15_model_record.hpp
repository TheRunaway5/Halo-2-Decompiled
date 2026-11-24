#pragma once
// Boundary adapter for the first-person model record, 168D76
// (native_tu15_model_record_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the model tag, ECX the node
// count, EDX the node matrices, five arguments (RET 14); EBP the frame, ESI
// and EDI saved; 16B240 gets EAX the model tag and the transform, the
// destination, the capacity, the matrices and the node map pushed (it pops
// them); 145560 ECX the transform, EDX the source and the destination pushed
// (it pops it), every register each returns coming back. Each access
// publishes its original instruction.
// HALO2_REFERENCE_MODEL_RECORD=1 restores the translated body.
// HALO2_VERIFY_MODEL_RECORD=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/objects/native_tu15_model_record_systems.hpp"

namespace halo2::native_model_record_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_model_record;
using Access=logic::model_record::Access;
using Kept=logic::model_record::Kept;
using Outcome=logic::model_record::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_MODEL_RECORD");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_MODEL_RECORD");return v;}
enum Branch : unsigned {gathered, transformed, empty, copied, branch_count};
static_assert(unsigned(Outcome::copied)==Branch::copied);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Model record"};return v;}
template<class State,class Gather,class Route> struct Adapter : Port<State> {
    Gather &gather_child;
    Route &route;
    bool started{};      // the transforms' registers set up
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Gather &g,Route &r):Port<State>(s,c,t,{}),gather_child(g),route(r){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[ebp]+8u+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case node_map_test:return 0x168d79u;
        case object:this->push(0x168d7du,gp[esi]);return 0x168d7eu;                                 // PUSH ESI
        case record_address:this->push(0x168d81u,gp[edi]);return 0x168d82u;                         // PUSH EDI
        case object_store:return 0x168d85u;
        case mode:return 0x168d88u;
        case model_store:return 0x168d8bu;
        case mode_store:return 0x168d8du;
        case node_map:return 0x168d92u;
        case transform_push:return 0x168d9cu;
        case transform_test:return 0x168daeu;
        case transform:
            // The first: TEST ECX,ECX; LEA ESI,[EDI+C]; MOV EDI,ECX.
            if(!started){this->test32(gp[ecx]);gp[esi]=gp[edi]+logic::record::matrices;gp[edi]=gp[ecx];started=true;}
            return 0x168dbdu;
        case copy_read:case copy_write:return 0x168ddeu;
        }
        return 0u;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case node_map_test:case transform_test:fl.minus(v,0u,4);break;                               // CMP [EBP+..],0
        case object:case mode:gp[esi]=v;break;
        case record_address:gp[edi]=v;break;
        // PUSH [EBP+14]; ADD EDI,C; PUSH EDX; PUSH 40; PUSH EDI.
        case node_map:
            this->push(0x168d92u,v);gp[edi]=fl.plus(gp[edi],logic::record::matrices,4);
            this->push(0x168d98u,gp[edx]);this->push(0x168d99u,logic::capacity);this->push(0x168d9bu,gp[edi]);break;
        case transform_push:this->push(0x168d9cu,v);break;                                          // PUSH [EBP+10]
        case transform:gp[ecx]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        // Each MOVSD step: ESI/EDI by the direction flag; the count down.
        if(a==Access::copy_write){const auto step=copy_step();this->gp[esi]+=step;this->gp[edi]+=step;--this->gp[ecx];}
    }
    void gather(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) {
        this->call(0x168d9fu,0x168da4u,0x16b240u,gather_child);
    }
    // CMP ECX,40 above; PUSH 40; POP ECX.
    std::uint32_t clamp() {
        this->push(0x168dabu,logic::capacity);this->gp[ecx]=this->pop(0x168dadu);
        return this->gp[ecx];
    }
    // PUSH ESI (the destination; EDX the source); CALL 145560.
    void combine(std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        gp[esi]=kept.destination;gp[edx]=kept.source;
        this->push(0x168dc0u,gp[esi]);
        this->call_routed(route,0x168dc1u,0x168dc6u,0x145560u);
        kept={gp[esi],gp[edx],gp[edi]};
    }
    // ADD ESI,34; ADD EDX,34; DEC EDI.
    void next(const Kept &) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esi]=fl.plus(gp[esi],logic::matrix_size,4);gp[edx]=fl.plus(gp[edx],logic::matrix_size,4);
        gp[edi]=fl.increment(gp[edi],4,true);
    }
    // IMUL ECX,ECX,34; MOV EAX,ECX; SHR ECX,2; ADD EDI,C; MOV ESI,EDX.
    void copy_started(std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.product(gp[ecx],logic::matrix_size);gp[eax]=gp[ecx];gp[ecx]=fl.right(gp[ecx],2u,4,false);
        gp[edi]=fl.plus(gp[edi],logic::record::matrices,4);gp[esi]=gp[edx];
    }
    std::uint32_t copy_step()const{return (this->s.other_flags&0x400u)?0xfffffffcu:4u;}
    // MOV ECX,EAX; AND ECX,3; REP MOVSB (no bytes).
    void copy_finished(){this->gp[ecx]=this->flags.logic(this->gp[eax]&3u,4);}
    void run() {
        auto &gp=this->gp;
        this->push(0x168d76u,gp[ebp]);gp[ebp]=gp[esp];                                               // PUSH EBP; MOV EBP,ESP
        Memory<Adapter> memory{*this};
        const auto outcome=logic::model_record::fill(memory,*this,gp[eax],gp[ecx],gp[edx]);
        if(outcome==Outcome::empty)this->test32(gp[ecx]);                                            // TEST ECX,ECX
        this->reached(unsigned(outcome));
        gp[edi]=this->pop(0x168de7u);gp[esi]=this->pop(0x168de8u);gp[ebp]=this->pop(0x168de9u);
        this->ret(0x168deau,0x14u);this->publish(0x168deau);
    }
};
template<class State,class Reference,class Gather,class Route> bool bridge(State &s,Reference reference,Gather gather,Route route) {
    return run_owner(s,0x168d76u,0x168deau,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Gather,Route>(v,c,t,gather,route);});
}
} // namespace halo2::native_model_record_detail
