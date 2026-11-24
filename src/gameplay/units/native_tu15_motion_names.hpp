#pragma once
// Boundary adapter for the motion names, E53E0
// (native_tu15_motion_names_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: EAX the biped, EBX and EDI the
// outputs, the motion, the flag and flag* on the stack (RET C); EBP saved;
// 1D3BD0 gets EAX the block and ESI the name (ESI saved around it), every
// register it returns coming back. Its jump table goes through the unit's
// canonical branch mapping, so an unbound entry raises the translated body's
// error. Each access publishes its original instruction.
// HALO2_REFERENCE_MOTION_NAMES=1 restores the translated body.
// HALO2_VERIFY_MOTION_NAMES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_motion_names_systems.hpp"

namespace halo2::native_motion_names_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_motion_names;
using Access=logic::motion_names::Access;
using Kept=logic::motion_names::Kept;
using Outcome=logic::motion_names::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_MOTION_NAMES");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_MOTION_NAMES");return v;}
enum Branch : unsigned {left_still, not_idle, named, unnamed, flagged, crouch, climb, branch_count};
static_assert(unsigned(Outcome::flagged)==Branch::flagged);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Motion names"};return v;}
inline constexpr std::uint32_t sites[]{0xe53e0u,0xe53e6u,0xe53f1u,0xe53f5u,0xe53feu,0xe5408u,0xe5419u,0xe5421u,0xe543au,0xe5440u,
    0xe5450u,0u,0xe54b1u,0xe54b5u,0xe54b9u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::flag_store)+1u);
template<class State,class HasMode,class Branches> struct Adapter : Port<State> {
    HasMode &has_mode_child;
    Branches &branch;
    std::uint32_t depth{};      // bytes below the return address
    std::uint32_t chosen{};     // the table entry's target: the action store's instruction
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,HasMode &h,Branches &b):Port<State>(s,c,t,{}),has_mode_child(h),branch(b){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t){return a==Access::action_store?chosen:sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:case object_address:gp[ecx]=v;break;
        case datums:gp[edx]=v;gp[eax]=3u*fl.logic(gp[eax]&0xffffu,4);break;                          // AND EAX,FFFF; LEA EAX,[EAX+EAX*2]
        case still:fl.minus(v&0xffffu,logic::still_value,2);break;                                     // CMP word [ECX+34C],1
        case motion_argument:gp[ebp]=v;break;
        case physics:gp[eax]=v&0xffu;break;                                                            // MOVZX EAX,byte [ECX+3DC]
        case animation:gp[eax]=logic::widen16(v);break;
        case action:fl.minus(v,logic::names::idle,4);break;                                            // CMP [EDI],400000C
        case table_entry:(void)branch(v,this->s.image);chosen=v;break;                                 // JMP [E54C0+EAX*4]
        case flag_out_argument:gp[eax]=v;break;
        case flag_argument:set_low_byte(gp[ecx],v);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        if(a==Access::crouch_store)this->reached(Branch::crouch);
        if(a==Access::climb_store)this->reached(Branch::climb);
        if(a==Access::action_store)this->reached(Branch::named);   // a named motion always goes on to its flag
    }
    void saving(){this->push(0xe53fdu,this->gp[ebp]);depth=4u;}
    void restoring(){this->gp[ebp]=this->pop(0xe54bbu);depth=0u;}
    void physics_tested(std::uint32_t mode) {                                                          // SUB EAX,2; (SUB EAX,3)
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.minus(mode,logic::climbing,4);
        if(mode!=logic::climbing)gp[eax]=fl.minus(gp[eax],logic::crouched-logic::climbing,4);
    }
    // PUSH ESI; ADD EAX,ECX; MOV ESI,name; CALL 1D3BD0; TEST AL,AL; POP ESI.
    std::uint32_t has_mode(std::uint32_t block,std::uint32_t name,Kept &kept) {
        auto &gp=this->gp;
        this->push(0xe5428u,gp[esi]);
        this->flags.plus(gp[eax],gp[ecx],4);gp[eax]=block;gp[esi]=name;
        this->call(0xe5430u,0xe5435u,0x1d3bd0u,has_mode_child);
        kept={gp[ebx],gp[edi],gp[ebp]};
        const auto answer=gp[eax];
        this->test8(answer&0xffu);
        gp[esi]=this->pop(0xe5437u);
        return answer;
    }
    void indexed(std::uint32_t index){this->gp[eax]=index;this->compare32(index,logic::motions-1u);}  // LEA EAX,[EBP-1]; CMP EAX,A
    [[noreturn]] void unknown_case(){throw std::runtime_error("Required original solver dispatch target unavailable");}
    void motion_tested(std::uint32_t motion){this->test32(motion);}                                    // TEST EBP,EBP
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::motion_names::choose(memory,*this,gp[eax],Kept{gp[ebx],gp[edi],gp[ebp]})));
        this->ret(0xe54bcu,0xcu);this->publish(0xe54bcu);
    }
};
template<class State,class Reference,class HasMode,class Branches> bool bridge(State &s,Reference reference,HasMode has_mode,Branches branch) {
    return run_owner(s,0xe53e0u,0xe54bcu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,HasMode,Branches>(v,c,t,has_mode,branch);});
}
} // namespace halo2::native_motion_names_detail
