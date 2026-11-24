#pragma once
// Boundary adapter for the shape copy, C3960
// (native_tu15_shape_copy_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the source, EDX the record; ESI and EDI saved.
// Each access publishes its original instruction.
// HALO2_REFERENCE_SHAPE_COPY=1 restores the translated body.
// HALO2_VERIFY_SHAPE_COPY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "pstencil/native_tu15_shape_copy_systems.hpp"

namespace halo2::native_shape_copy_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_shape_copy;
using Access=logic::shape_copy::Access;
using Outcome=logic::shape_copy::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SHAPE_COPY");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SHAPE_COPY");return v;}
enum Branch : unsigned {simple, shaped, branch_count};
static_assert(unsigned(Outcome::shaped)==Branch::shaped);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Shape copy"};return v;}
template<class State> struct Adapter : Port<State> {
    unsigned words{};           // the shape's dwords moved
    bool simple_path{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case clear:return 0xc396du;
        case kind:return 0xc396fu;
        case kind_store:return 0xc3976u;
        case first:simple_path=true;return 0xc397au;
        case first_store:return 0xc397du;
        case second:return 0xc3980u;
        case second_store:return 0xc3984u;
        case shape_read: {
            static constexpr std::uint32_t at[]{0xc398cu,0xc3993u,0xc3999u,0xc399fu,0xc39a5u};
            if(!words)gp[esi]=this->flags.plus(gp[esi],logic::source::shape,4);                       // ADD ESI,20
            return at[words];
        }
        case shape_write: {
            static constexpr std::uint32_t at[]{0xc3991u,0xc3996u,0xc399cu,0xc39a2u,0xc39a9u};
            return at[words];
        }
        }
        return 0u;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case kind:gp[eax]=logic::widen16(v);fl.logic(v&0xffffu,2);break;                              // MOVSX EAX,word; TEST AX,AX
        case first:gp[ecx]=v;break;
        case second:gp[eax]=v;break;
        case shape_read:
            if(!words)gp[edx]=fl.plus(gp[edx],4u,4);                                                  // ADD EDX,4
            gp[(words&1u)?eax:ecx]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        if(a==Access::clear){gp[edi]+=step();--gp[ecx];}                                               // each STOSD step
        if(a==Access::shape_write)++words;
    }
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0xfffffffcu:4u;}
    // MOV ESI,EAX; XOR EAX,EAX; MOV ECX,1F; MOV EDI,EDX.
    void clear_started() {
        auto &gp=this->gp;
        gp[esi]=gp[eax];gp[eax]=this->flags.logic(0u,4);gp[ecx]=logic::record_words;gp[edi]=gp[edx];
    }
    void leaving(){this->gp[edi]=this->pop(simple_path?0xc3983u:0xc39a8u);}
    void run() {
        auto &gp=this->gp;
        this->push(0xc3960u,gp[esi]);this->push(0xc3961u,gp[edi]);
        Memory<Adapter> memory{*this};
        const auto outcome=logic::shape_copy::copy(memory,*this,gp[eax],gp[edx]);
        this->reached(unsigned(outcome));
        const std::uint32_t exit=outcome==Outcome::simple?0xc3987u:0xc39acu;
        gp[esi]=this->pop(exit);this->ret(exit+1u,0u);this->publish(exit+1u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0xc3960u,0xc39adu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_shape_copy_detail
