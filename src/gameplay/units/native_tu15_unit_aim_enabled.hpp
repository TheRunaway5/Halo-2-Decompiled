#pragma once
// Boundary adapter for the unit aim enabled, 1139D0
// (native_tu15_unit_aim_enabled_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EDI the unit's handle, AL
// the answer (RET 0); EBX and ESI saved. C9550 (ECX the unit) and 113C70 (EAX
// the unit; 3 pushed, RET 4) are direct; every register each returns comes
// back. The two compiled searches keep the original's compare chains: their
// flags, EAX as their SUBs leave it, and BL set before the first branch.
// Each exit leaves through the original's epilogue. Each access publishes
// its original instruction.
// HALO2_REFERENCE_UNIT_AIM_ENABLED=1 restores the translated body.
// HALO2_VERIFY_UNIT_AIM_ENABLED=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_unit_aim_enabled_systems.hpp"

namespace halo2::native_unit_aim_enabled_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_unit_aim_enabled;
using Access=logic::unit_aim_enabled::Access;
using Exit=logic::unit_aim_enabled::Exit;
using Kept=logic::unit_aim_enabled::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_UNIT_AIM_ENABLED");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_UNIT_AIM_ENABLED");return v;}
enum Branch : unsigned {aiming, jumped, not_aiming, unit_dropped, animation_dropped, unready, answered, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Unit aim enabled"};return v;}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline constexpr std::uint32_t sites[]{0x1139d0u,0x1139d6u,0x1139e3u,0x1139e7u,0x1139f0u,0x1139f9u,0x113a4bu,0x113a52u,0x113ad5u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::animation_state)+1u);
template<class State,class Children> struct Adapter : Port<State> {
    Children &children;      // C9550 and 113C70
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k):Port<State>(s,c,t,{}),children(k){}
    void direct(std::uint32_t pc,std::uint32_t target,Kept &kept) {
        auto child=[&](State &v){children(v,target);};
        this->call(pc,pc+5u,target,child);
        auto &gp=this->gp;
        kept.answer=gp[ebx];kept.animation_address=gp[esi];kept.handle=gp[edi];
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case objects_load:gp[ecx]=v;break;
        case objects_data:gp[edx]=v;break;
        case unit_load:case state_load:case animation_state:gp[eax]=v;break;
        case channels_offset:gp[ecx]=sx16(v);break;                                                        // MOVSX ECX,word
        case animation_offset:gp[esi]=sx16(v);break;                                                       // MOVSX ESI,word
        case jump_index:gp[edx]=v;break;                                                                   // MOVZX EDX,byte
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    std::uint32_t unit_index(std::uint32_t handle) {                                                     // MOV EAX,EDI; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]
        const auto index=this->flags.logic(handle&logic::handle_mask,4);
        this->gp[eax]=3u*index;
        return index;
    }
    void saving(unsigned n){static constexpr std::uint32_t pcs[]{0x1139eeu,0x1139efu};static constexpr unsigned regs[]{ebx,esi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);}                                                     // PUSH EBX / ESI
    std::uint32_t animation_address(std::uint32_t offset,std::uint32_t unit){return this->gp[esi]=this->flags.plus(offset,unit,4);}   // ADD ESI,EAX
    // The first search's compare chain on EAX (CMP, SUB, signed JG/JGE/JLE); MOV BL,1 after its first CMP.
    // Its leaves are the logic's; the chain answers whether it went through the jump table.
    bool state_searched(std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        const auto above=[&](std::uint32_t c){fl.minus(v,c,4);return std::int32_t(v)>std::int32_t(c);};
        const auto sub=[&](std::uint32_t c){v=gp[eax]=fl.minus(v,c,4);return v==0u;};
        const bool high=above(0xc000075u);set_low_byte(gp[ebx],1u);
        if(high) {
            if(above(0xe00067du)) {
                if(above(0x130005bcu)){fl.minus(v,0x140005b3u,4);return false;}
                if(v==0x130005bcu||sub(0x11000074u))return false;
                sub(2u);return false;
            }
            if(v==0xe00067du)return false;
            if(above(0xc0006cdu)) {
                fl.minus(v,0xe000038u,4);
                if(v==0xe000038u)return false;
                if(!above(0xe000606u))return false;
                fl.minus(v,0xe00060au,4);return false;
            }
            if(v==0xc0006cdu)return false;
            fl.minus(v,0xc000077u,4);
            if(v==0xc000077u)return false;
            fl.minus(v,0xc0006b3u,4);return false;
        }
        if(v==0xc000075u)return false;
        if(above(0xa000040u)) {
            if(above(0xb0005b2u)){fl.minus(v,0xc000073u,4);return false;}
            if(v==0xb0005b2u||sub(0xa0005b9u))return false;
            sub(2u);return false;
        }
        if(v==0xa000040u)return false;
        if(above(0x6000007u)) {
            sub(logic::jump_table::first);fl.minus(v,logic::jump_table::last-logic::jump_table::first,4);
            if(v>logic::jump_table::last-logic::jump_table::first)return false;
            this->reached(Branch::jumped);
            return true;
        }
        fl.minus(v,0x6000006u,4);
        if(std::int32_t(v)>=std::int32_t(0x6000006u))return false;
        fl.minus(v,0xffffffffu,4);
        if(v==0xffffffffu)return false;
        fl.minus(v,0x500000au,4);return false;
    }
    std::uint32_t state_answered(bool aiming) {                                                          // (XOR BL,BL)
        if(aiming)this->reached(Branch::aiming);
        else{this->reached(Branch::not_aiming);set_low_byte(this->gp[ebx],this->flags.logic(0u,1));}
        return this->gp[ebx];
    }
    bool dropped_by_unit(Kept &kept) {                                                                   // MOV ECX,EDI; CALL C9550; TEST AL,AL
        auto &gp=this->gp;
        gp[ecx]=kept.handle;direct(0x113accu,0xc9550u,kept);
        this->test8(gp[eax]);
        if(low_byte(gp[eax]))this->reached(Branch::unit_dropped);
        return low_byte(gp[eax])!=0u;
    }
    std::uint32_t dropped(bool by_animation) {                                                           // XOR BL,BL
        if(by_animation)this->reached(Branch::animation_dropped);
        set_low_byte(this->gp[ebx],this->flags.logic(0u,1));return this->gp[ebx];
    }
    // The second search's compare chain on EAX.
    void animation_searched(std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        const auto above=[&](std::uint32_t c){fl.minus(v,c,4);return std::int32_t(v)>std::int32_t(c);};
        const auto sub=[&](std::uint32_t c){v=gp[eax]=fl.minus(v,c,4);return v==0u;};
        if(above(0xc00005bu)) {
            if(above(0xe0000c2u)) {
                fl.minus(v,0xf00003au,4);
                if(v!=0xf00003au)fl.minus(v,0x1000003cu,4);
            } else if(v!=0xe0000c2u && !sub(0xd00003fu) && !sub(3u))sub(0xfffff9u);
        } else if(v!=0xc00005bu) {
            if(above(0xb00005au)) {
                if(!sub(0xc000043u))sub(0x15u);
            } else {
                fl.minus(v,0xb000059u,4);
                if(std::int32_t(v)<std::int32_t(0xb000059u)) {
                    fl.minus(v,0x400004au,4);
                    if(v!=0x400004au){fl.minus(v,0x600008cu,4);if(v!=0x600008cu)fl.minus(v,0xa00003eu,4);}
                }
            }
        }
    }
    bool ready(Kept &kept) {                                                                             // PUSH 3; MOV EAX,EDI; CALL 113C70; TEST AL,AL
        auto &gp=this->gp;
        this->push(0x113b42u,logic::ready_mode);gp[eax]=kept.handle;
        direct(0x113b46u,0x113c70u,kept);
        this->test8(gp[eax]);
        const bool yes=low_byte(gp[eax])!=0u;
        this->reached(yes?Branch::answered:Branch::unready);
        return yes;
    }
    void finish(Exit exit) {                                                                             // POP ESI; (MOV AL,BL); POP EBX; RET
        auto &gp=this->gp;
        const auto pc=exit==Exit::unready?0x113b4fu:0x113b52u;
        gp[esi]=this->pop(pc);
        if(exit==Exit::answered)set_low_byte(gp[eax],gp[ebx]);
        const auto last=exit==Exit::unready?pc+1u:pc+3u;
        gp[ebx]=this->pop(last);
        this->ret(last+1u,0u);this->publish(last+1u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        finish(logic::unit_aim_enabled::enabled(memory,*this,Kept{gp[ebx],gp[esi],gp[edi]}));
    }
};
template<class State,class Reference,class Children> bool bridge(State &s,Reference reference,Children children) {
    return run_owner(s,0x1139d0u,0x113b56u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children>(v,c,t,children);});
}
} // namespace halo2::native_unit_aim_enabled_detail
