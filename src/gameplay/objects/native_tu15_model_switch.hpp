#pragma once
// Boundary adapter for the model switch, entered at 1D3930
// (native_tu15_model_switch_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the record (RET); EBX and ESI saved (and
// EDI where the first model shows); ECX and EDX the two models' flags as
// written; XMM0 the last float read; the flags of the last comparison.
// Each access publishes its original instruction.
// HALO2_REFERENCE_MODEL_SWITCH=1 restores the translated body.
// HALO2_VERIFY_MODEL_SWITCH=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_model_switch_systems.hpp"

namespace halo2::native_model_switch_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_model_switch;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_MODEL_SWITCH");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_MODEL_SWITCH");return p && *p;}();
    return value;
}
enum Branch : unsigned {kept, first_shown, second_shown, branch_count};
static_assert(unsigned(logic::model_switch::Outcome::second_shown)==Branch::second_shown);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Model switch verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
template<class State> struct ModelSwitch : Port<State> {
    using Access=logic::model_switch::Access;
    bool far{},cleared{};
    ModelSwitch(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case first_flags:return 0x1d3930u;
        case second_flags:return 0x1d3934u;
        case enabled:this->push(0x1d3938u,this->gp[ebx]);return 0x1d3939u;                        // PUSH EBX
        case second_handle:return 0x1d3947u;
        case second_index:return 0x1d394cu;
        case threshold:return 0x1d3952u;
        case distance:return 0x1d395au;
        case distance_again:return 0x1d3963u;
        case near_bound:return 0x1d396bu;
        case first_handle:
            if(cleared){this->push(0x1d398eu,this->gp[edi]);return 0x1d398fu;}                      // PUSH EDI
            far=true;return 0x1d3974u;
        case switch_flags:case switch_store:
            if(!far){cleared=true;return 0x1d398au;}
            return 0x1d3976u;
        case first_index:return 0x1d39a2u;
        case first_store:return 0x1d39a8u;
        case second_handle_again:return 0x1d39acu;
        case second_index_again:return 0x1d39b1u;
        case second_store:return 0x1d39b7u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case first_flags:gp[ecx]=v;break;
        case second_flags:gp[edx]=v;break;
        case enabled:set_low_byte(gp[ebx],v);this->push(0x1d393fu,gp[esi]);gp[esi]=0xffffffffu;break;   // PUSH ESI; OR ESI,-1
        case threshold:x[0]=lane0_only(v);break;
        case distance:this->flags.scalar_compare(x[0],lane0_only(v),false);break;               // COMISS XMM0,[EAX+80]
        case distance_again:x[0]=lane0_only(v);break;
        case near_bound:this->flags.scalar_compare(x[0],lane0_only(v),false);break;             // COMISS XMM0,[46A234]
        case first_handle:if(cleared)gp[edi]=this->pop(0x1d399fu);else gp[ebx]=v;break;            // (MOV EDI; CMP; POP EDI)
        case second_handle_again:this->compare32(v,logic::none);break;                             // CMP [EAX+20],ESI
        case second_index_again:this->flags.minus(v,logic::no_index,2);break;                      // CMP word [EAX+26],SI
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // OR/AND ECX,380; AND/OR EDX,380.
    void decided(std::uint32_t first_value,std::uint32_t second_value){this->gp[ecx]=first_value;this->gp[edx]=second_value;}
    void run() {
        auto &gp=this->gp;
        Memory<ModelSwitch> memory{*this};
        this->reached(unsigned(logic::model_switch::update(memory,*this,gp[eax])));
        gp[esi]=this->pop(0x1d39bbu);gp[ebx]=this->pop(0x1d39bcu);
        this->ret(0x1d39bdu,0u);this->publish(0x1d39bdu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1d3930u,0x1d39bdu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ModelSwitch<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_model_switch_detail
