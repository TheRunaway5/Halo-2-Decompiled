#pragma once
// Boundary adapter for the round timer, 164EF0
// (native_tu15_round_timer_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: BL the announce flag, EAX the
// time shown (RET); 0x24 bytes of frame with EBP, ESI and EDI saved below
// them, read from the ESP each child left; the display's virtual +0x64 (ECX
// the display, the remaining ticks, EBX and 1 pushed, RET C), B1E80 (EDX the
// event) and 1A8740 (EDI the event) through the unit's dispatch, every
// register each returns coming back. Each access publishes its original
// instruction.
// HALO2_REFERENCE_ROUND_TIMER=1 restores the translated body.
// HALO2_VERIFY_ROUND_TIMER=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_round_timer_systems.hpp"

namespace halo2::native_round_timer_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_round_timer;
using Access=logic::round_timer::Access;
using Kept=logic::round_timer::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ROUND_TIMER");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ROUND_TIMER");return v;}
enum Branch : unsigned {quiet, current, announced, raised, sent, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Round timer"};return v;}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline constexpr std::uint32_t sites[]{0x164ef0u,0x164ef9u,0x164effu,0x164f03u,0x164f0au,0x164f10u,0x164f1du,0x164f29u,0x164f3du,0x164f44u,
    0x164f49u,0x164fbbu,0x164f58u,0u,0x164f79u,0x164f7eu,0x164f83u,0x164f87u,0x164f8bu,0x164f8fu,0x164fafu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::announced_set)+1u);
inline constexpr std::uint32_t handle_sites[]{0x164f65u,0x164f69u,0x164f6du,0x164f71u,0x164f75u};
inline constexpr std::uint32_t pass_sites[]{0x164f23u,0x164f25u,0x164f46u};
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below the saved registers
    unsigned handles{},passes{};
    std::uint32_t exit{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        if(a==Access::handle_store){if(handles==0u)this->gp[eax]=logic::none;return handle_sites[handles++];}   // MOV EAX,-1
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case variant_load:case tick_load:case variant_again:gp[eax]=v;break;
        case session_load:gp[ebp]=v;break;
        case start_load:case display_load:gp[ecx]=v;break;
        case limit_load:gp[esi]=v;break;
        case time_load:gp[edi]=v;break;
        case player_load:case vtable_load:gp[edx]=v;break;
        case rate_load:gp[eax]=sx16(v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // SUB ESP,24; PUSH EBP
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);this->push(0x164ef8u,gp[ebp]);
    }
    void saving(unsigned n) {                                                                            // PUSH ESI / PUSH EDI
        if(n==0u)this->push(0x164f02u,this->gp[esi]);else{this->push(0x164f09u,this->gp[edi]);depth=0u;}
    }
    std::uint32_t elapsed(std::uint32_t tick,std::uint32_t start) {                                     // SUB EAX,ECX; MOV ECX,0; SETS CL
        auto &gp=this->gp;
        gp[eax]=this->flags.minus(tick,start,4);gp[ecx]=this->flags.sf?1u:0u;
        return gp[eax];
    }
    void pass(std::uint32_t v) {                                                                         // PUSH 1; PUSH EBX; PUSH EAX (MOV EDI,EAX)
        const auto pc=pass_sites[passes++];
        this->push(pc,v);depth+=4u;
        if(pc==0x164f46u)this->gp[edi]=v;
    }
    std::uint32_t clamped(std::uint32_t elapsed_ticks) {                                                // DEC ECX; AND ECX,EAX
        auto &gp=this->gp;auto &fl=this->flags;
        return gp[ecx]=fl.logic(fl.increment(gp[ecx],4,true)&elapsed_ticks,4);
    }
    std::uint32_t remaining(std::uint32_t,std::uint32_t limit,std::uint32_t counted) {                 // IMUL EAX,ESI; SUB EAX,ECX; MOV ECX,0; SETS CL; DEC ECX; AND EAX,ECX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.minus(fl.product(gp[eax],limit),counted,4);
        gp[ecx]=fl.sf?1u:0u;gp[ecx]=fl.increment(gp[ecx],4,true);
        return gp[eax]=fl.logic(gp[eax]&gp[ecx],4);
    }
    std::uint32_t shown_by(std::uint32_t target,Kept &kept) {                                           // CALL [EDX+64]; XOR ECX,ECX; CMP BL,CL; MOV ESI,EAX
        auto &gp=this->gp;
        auto child=[&](State &v){dispatch(v,target,0x164f49u);};
        this->call(0x164f49u,0x164f4cu,target,child);depth-=12u;
        refresh(kept);
        gp[ecx]=this->flags.logic(0u,4);this->flags.minus(gp[ebx]&0xffu,0u,1);
        kept.shown=gp[esi]=gp[eax];
        if(!(kept.announce&0xffu))this->reached(Branch::quiet);
        return kept.shown;
    }
    void compared(std::uint32_t a,std::uint32_t b) {                                                    // CMP ESI,EDI
        this->compare32(a,b);
        if(a==b)this->reached(Branch::current);
    }
    void byte_compared(std::uint32_t v) {                                                               // CMP byte [56E224],CL
        this->flags.minus(v&0xffu,0u,1);
        if(v&0xffu)this->reached(Branch::announced);else this->reached(Branch::raised);
    }
    void mode_compared(std::uint32_t mode){this->flags.minus(mode&0xffu,logic::variant::untimed_mode,1);}   // CMP byte [EAX+C],4
    void sent(Kept &kept) {                                                                              // LEA EDX,[ESP+C]; CALL B1E80
        this->gp[edx]=local(logic::local::event);
        auto child=[&](State &v){dispatch(v,0xb1e80u,0x164f9du);};
        this->call(0x164f9du,0x164fa2u,0xb1e80u,child);
        refresh(kept);
        this->reached(Branch::sent);
    }
    void posted(Kept &kept) {                                                                            // LEA EDI,[ESP+C]; CALL 1A8740
        this->gp[edi]=local(logic::local::event);
        auto child=[&](State &v){dispatch(v,0x1a8740u,0x164fa6u);};
        this->call(0x164fa6u,0x164fabu,0x1a8740u,child);
        refresh(kept);
    }
    void restoring(unsigned n) {
        auto &gp=this->gp;
        switch(n) {
        case 0u:gp[edi]=this->pop(0x164fc1u);gp[eax]=gp[esi];gp[esi]=this->pop(0x164fc4u);break;       // POP EDI; MOV EAX,ESI; POP ESI
        case 1u:gp[edi]=this->pop(0x164fabu);gp[eax]=gp[esi];gp[esi]=this->pop(0x164faeu);break;
        case 2u:gp[ebp]=this->pop(0x164fc5u);gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);exit=0x164fc9u;break;   // POP EBP; ADD ESP,24
        default:gp[ebp]=this->pop(0x164fb6u);gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);exit=0x164fbau;break;
        }
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::round_timer::update(memory,*this,Kept{gp[ebx],gp[esi],gp[edi]});
        this->ret(exit,0u);this->publish(exit);
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x164ef0u,0x164fc9u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_round_timer_detail
