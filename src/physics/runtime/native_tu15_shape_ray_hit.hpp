#pragma once
// Boundary adapter for the shape ray hit, 2E9140
// (native_tu15_shape_ray_hit_systems.hpp). Its callers and the virtual
// calls keep the original ABI, which the adapter owns: ECX the shape, the
// input, body and collector on the stack (RET C); an EBP frame, ESP aligned
// down to 16 below 0x2C bytes, ESI saved below it and the frame read from
// the ESP each call left; the shape's cast (vtable +0x20: ECX the shape,
// the hit flag, the input and the output pushed, RET C) and the collector's
// first virtual (ECX the collector, the body and the output pushed, RET 8)
// through the unit's dispatch at their call instructions, every register
// each returns coming back. XMM0 to XMM3 hold what the original's moves and
// arithmetic leave in them. Each access publishes its original instruction.
// HALO2_REFERENCE_SHAPE_RAY_HIT=1 restores the translated body.
// HALO2_VERIFY_SHAPE_RAY_HIT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_shape_ray_hit_systems.hpp"

namespace halo2::native_shape_ray_hit_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_shape_ray_hit;
using Access=logic::shape_ray_hit::Access;
using Kept=logic::shape_ray_hit::Kept;
using Outcome=logic::shape_ray_hit::Outcome;
using Rotated=logic::shape_ray_hit::Rotated;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SHAPE_RAY_HIT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SHAPE_RAY_HIT");return v;}
enum Branch : unsigned {missed, reported, branch_count};
static_assert(unsigned(Outcome::reported)==Branch::reported);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Shape ray hit"};return v;}
inline constexpr std::uint32_t sites[]{0x2e9149u,0x2e914cu,0x2e914fu,0x2e9152u,0x2e915bu,0x2e9164u,0x2e9167u,0x2e916cu,0x2e9171u,0x2e9174u,
    0x2e9177u,0x2e9184u,0u,0x2e91a7u,0x2e91bau,0x2e91bfu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::add_hit_slot)+1u);
inline constexpr std::uint32_t column_sites[]{0x2e9188u,0x2e918cu,0x2e91a4u};
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below the saved ESI
    unsigned columns{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    std::uint32_t argument(unsigned n)const{return this->gp[ebp]+8u+4u*n;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t instruction(Access a,std::uint32_t){return a==Access::column?column_sites[columns++]:sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case shape_vtable:case key:gp[edx]=v;break;
        case collector_argument:gp[esi]=v;break;
        case early_out:case input_argument:case motion:case collector_vtable:gp[eax]=v;break;
        case body_argument:gp[ecx]=v;break;
        default:break;
        }
    }
    void loaded_vector(Access a,std::uint32_t,__m128 v){if(a==Access::normal)this->s.xmm[0]=v;}       // MOVAPS XMM0
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // PUSH EBP; MOV EBP,ESP; AND ESP,-16; SUB ESP,2C
        auto &gp=this->gp;auto &fl=this->flags;
        this->push(0x2e9140u,gp[ebp]);gp[ebp]=gp[esp];
        gp[esp]=fl.minus(fl.logic(gp[esp]&0xfffffff0u,4),logic::local::size,4);
    }
    void saving(){this->push(0x2e914bu,this->gp[esi]);depth=0u;}                                       // PUSH ESI
    void output_pushed(){this->gp[eax]=local(logic::local::output);put(0x2e915au,this->gp[eax]);}     // LEA EAX,[ESP+10]; PUSH EAX
    void input_pushed(std::uint32_t input) {                                                            // PUSH EAX; LEA EAX,[ESP+17]; PUSH EAX
        auto &gp=this->gp;
        put(0x2e915eu,input);gp[eax]=local(logic::local::hit);put(0x2e9163u,gp[eax]);
    }
    std::uint32_t cast(std::uint32_t target,Kept &kept) {                                               // CALL [EDX+20]
        auto child=[&](State &v){dispatch(v,target,0x2e9164u);};
        this->call(0x2e9164u,0x2e9167u,target,child);depth-=12u;
        kept.collector=this->gp[esi];
        return this->gp[eax];
    }
    void byte_compared(std::uint32_t v){this->flags.minus(v&0xffu,0u,1);}                              // CMP byte [EAX],0
    std::uint32_t rotation(std::uint32_t motion_address) {                                              // ADD EAX,20
        return this->gp[eax]=this->flags.plus(motion_address,logic::motion::rotation,4);
    }
    void rotated(const Rotated &r) {                                                                     // the SHUFPS/MULPS/ADDPS registers
        auto &x=this->s.xmm;
        x[0]=r.normal;x[1]=r.x_component;x[2]=r.z_term;x[3]=r.y_term;
        this->reached(Branch::reported);
    }
    void hit_pushed(std::uint32_t body_address) {                                                       // LEA EDX,[ESP+10]; PUSH EDX; PUSH ECX; MOV ECX,ESI
        auto &gp=this->gp;
        gp[edx]=local(logic::local::output);put(0x2e91b0u,gp[edx]);put(0x2e91b4u,body_address);gp[ecx]=gp[esi];
    }
    void reported(std::uint32_t target,Kept &kept) {                                                    // CALL [EAX]
        auto child=[&](State &v){dispatch(v,target,0x2e91bfu);};
        this->call(0x2e91bfu,0x2e91c1u,target,child);depth-=8u;
        kept.collector=this->gp[esi];
    }
    void restoring() {                                                                                   // POP ESI; MOV ESP,EBP; POP EBP
        auto &gp=this->gp;
        gp[esi]=this->pop(0x2e91c1u);gp[esp]=gp[ebp];gp[ebp]=this->pop(0x2e91c4u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::shape_ray_hit::cast(memory,*this,gp[ecx],Kept{gp[esi]});
        this->reached(unsigned(outcome));
        this->ret(0x2e91c5u,12u);this->publish(0x2e91c5u);
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x2e9140u,0x2e91c5u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_shape_ray_hit_detail
