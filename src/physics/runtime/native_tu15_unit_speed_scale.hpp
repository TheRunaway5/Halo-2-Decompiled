#pragma once
// Boundary adapter for the unit speed scale, entered at DFD20
// (native_tu15_unit_speed_scale_systems.hpp). Its callers and 2647E0 keep
// the original ABI, which the adapter owns: EAX the unit handle; the result
// in XMM0 (XMM1 the limit, XMM2 one); the scale lives in the slot the entry
// PUSH ECX makes, so ECX returns its final bits (POP ECX); ESI saved (the
// unit); 2647E0 gets the slot's address pushed (and pops it), and every
// register it returns comes back; EAX and EDX as the original leaves them;
// the flags of the last instruction that sets them; RET. Each access
// publishes its original instruction.
// HALO2_REFERENCE_UNIT_SPEED_SCALE=1 restores the translated body.
// HALO2_VERIFY_UNIT_SPEED_SCALE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_unit_speed_scale_systems.hpp"

namespace halo2::native_unit_speed_scale_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_unit_speed_scale;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_UNIT_SPEED_SCALE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_UNIT_SPEED_SCALE");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    no_controller, idle_controller, adjusted, forward_capped, forward, backward_capped, backward,
    other_state, inactive, clear, override_ceiling, override_one, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Unit speed scale verify: %llu calls; %llu ordered accesses (%llu stores) "
            "identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Adjust> struct SpeedScale : Port<State> {
    using Access=logic::speed_scale::Access;
    using Local=logic::speed_scale::Local;
    using Kept=logic::speed_scale::Kept;
    Adjust &adjust_child;
    std::uint32_t depth{};bool controlled{},adjusting{};
    SpeedScale(State &s,std::uint32_t *c,Trace<State> *t,Adjust &a):Port<State>(s,c,t,{}),adjust_child(a){}
    std::uint32_t local(Local)const{return this->gp[esp]+depth;}
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case object_table:return 0xdfd21u;
        case object_datums:return 0xdfd27u;
        case tag_table:return 0xdfd2au;
        case object_address:return 0xdfd39u;
        case object_tag:return 0xdfd3du;
        case tag_address:return 0xdfd47u;
        case controller:return 0xdfd4bu;
        case base:return 0xdfd54u;
        case scale_init:return 0xdfd5cu;
        case controller_table:return 0xdfd64u;
        case controller_datums:return 0xdfd6au;
        case controller_check:return 0xdfd7bu;
        case flags:return 0xdfd8fu;
        case speed_globals:return 0xdfd95u;
        case speed_scale:return 0xdfd9bu;
        case scale_read:return 0xdfda0u;
        case one:return 0xdfda6u;
        case forward_fraction:return 0xdfdb8u;
        case backward_compare:return 0xdfdcau;
        case backward_fraction:return 0xdfdd3u;
        case state:return 0xdfde7u;
        case active:return 0xdfdf0u;
        case bits:return 0xdfdfcu;
        case override_fraction:return 0xdfe0eu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case object_table:gp[ecx]=v;break;
        case object_datums:case tag_address:case controller_table:case speed_globals:gp[edx]=v;break;
        // MOV ECX,[4F0414]; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]; PUSH ESI.
        case tag_table:gp[ecx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;save(0xdfd38u,gp[esi]);break;
        case object_address:gp[esi]=v;break;
        // MOV EAX,[ESI]; AND EAX,FFFF; SHL EAX,4.
        case object_tag:gp[eax]=fl.shift(fl.logic(v&0xffffu,4),4u,4);break;
        // MOV EAX,[ESI+12C]; CMP EAX,-1.
        case controller:gp[eax]=v;fl.minus(v,logic::none,4);controlled=v!=logic::none;
            if(!controlled)this->reached(Branch::no_controller);break;
        case base:this->s.xmm[0]=logic::sse::scalar(v);break;
        // MOV EDX,[EDX+44]; MOV ECX,EAX; AND ECX,FFFF; IMUL ECX,ECX,888.
        case controller_datums:gp[edx]=v;gp[ecx]=fl.product(fl.logic(gp[eax]&0xffffu,4),logic::controller_size);break;
        case controller_check:fl.minus(v,logic::none,4);
            if(v==logic::none)this->reached(Branch::idle_controller);break;
        case flags:gp[eax]=v;break;
        case speed_scale:this->s.xmm[0]=logic::sse::scalar(v);break;
        // MOVSS XMM2,[one]; SHR EAX,17; TEST AL,1.
        case one:this->s.xmm[2]=logic::sse::scalar(v);gp[eax]=fl.right(gp[eax],logic::unit::forward_bit,4,false);
            this->test8(gp[eax]&1u);break;
        case state:fl.minus(v&0xffu,logic::unit::override_state,1);break;                         // CMP byte,6
        case active:set_low_byte(gp[eax],v);this->test8(v);break;                                 // MOV AL; TEST AL,AL
        // XOR ECX,ECX; MOV CX,[ESI+348]; SHR ECX,D; TEST CL,1.
        case bits:(void)fl.logic(0u,4);gp[ecx]=fl.right(v&0xffffu,logic::unit::override_bit,4,false);
            this->test8(gp[ecx]&1u);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // LEA ECX,[ESP+4]; PUSH ECX; CALL 2647E0 (which pops it).
    void adjust(Kept &kept) {
        auto &gp=this->gp;
        gp[ecx]=local(Local::scale);save(0xdfd89u,gp[ecx]);
        this->call(0xdfd8au,0xdfd8fu,0x2647e0u,adjust_child);depth-=4u;
        kept.unit=gp[esi];this->reached(Branch::adjusted);
    }
    void compared(std::uint32_t a,std::uint32_t b){this->flags.scalar_compare(logic::sse::scalar(a),logic::sse::scalar(b),false);}
    void limited(std::uint32_t value,std::uint32_t xmm1) {
        this->s.xmm[0]=logic::sse::scalar(value);this->s.xmm[1]=logic::sse::scalar(xmm1);
    }
    void overridden(std::uint32_t value){this->s.xmm[0]=logic::sse::scalar(value);}
    void run() {
        auto &gp=this->gp;
        save(0xdfd20u,gp[ecx]);depth=0u;                         // PUSH ECX: the scale's slot (at ESP)
        Memory<SpeedScale> memory{*this};
        const auto outcome=logic::speed_scale::of(memory,*this,gp[eax]);
        using logic::speed_scale::Limit;using logic::speed_scale::Override;
        static constexpr Branch limits[]{forward_capped,forward,backward_capped,backward};
        static constexpr Branch overrides[]{other_state,inactive,clear,override_ceiling,override_one};
        this->reached(limits[unsigned(outcome.limit)]);this->reached(overrides[unsigned(outcome.override_kind)]);
        gp[esi]=this->pop(0xdfe1eu);gp[ecx]=this->pop(0xdfe1fu);
        this->ret(0xdfe20u,0);this->publish(0xdfe20u);
    }
};
template<class State,class Reference,class Adjust> bool bridge(State &s,Reference reference,Adjust adjust) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xdfd20u,0xdfe20u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return SpeedScale<State,Adjust>(v,c,t,adjust);});
    return true;
}
} // namespace halo2::native_unit_speed_scale_detail
