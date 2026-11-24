#pragma once
// Boundary adapter for the unit state change, 1115D0
// (native_tu15_unit_state_change_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX the unit's handle, DL
// the flag and the held byte on the stack (RET 4), EAX the state; ECX
// pushed for 111D10's local, then EBX, EBP, ESI and EDI, read from the ESP
// each child left. 111D10 (EAX the unit, EDX the local, ESI the argument's
// slot) and 1D6430 (EDI the animation, ECX and EDX none; three pushed, RET
// C) are direct; every register each returns comes back. The state search
// keeps the original's compare tree (its flags, and ECX as the SUBs leave
// it). Each exit leaves through the original's epilogue. Each access
// publishes its original instruction.
// HALO2_REFERENCE_UNIT_STATE_CHANGE=1 restores the translated body.
// HALO2_VERIFY_UNIT_STATE_CHANGE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_unit_state_change_systems.hpp"

namespace halo2::native_unit_state_change_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_unit_state_change;
using Access=logic::unit_state_change::Access;
using Exit=logic::unit_state_change::Exit;
using Kept=logic::unit_state_change::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_UNIT_STATE_CHANGE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_UNIT_STATE_CHANGE");return v;}
enum Branch : unsigned {none, direct, kept_state, flagged, held, unchosen, same, restarted, changed, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Unit state change"};return v;}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline constexpr std::uint32_t sites[]{0x1115d4u,0x1115dau,0x1115e8u,0x1115ecu,0x1115f3u,0x11167fu,0x11169cu,0x1116a4u,0x1116a4u,
    0x1116adu,0x1116f5u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::animation_offset)+1u);
template<class State,class Children> struct Adapter : Port<State> {
    Children &children;      // 111D10 and 1D6430
    std::uint32_t depth{};   // bytes pushed beyond ECX, EBX, EBP, ESI and EDI (negative before them)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k):Port<State>(s,c,t,{}),children(k){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void direct(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){children(v,target);};
        this->call(pc,pc+5u,target,child);depth-=popped;
        auto &gp=this->gp;
        kept.unit_address=gp[ebx];kept.block_address=gp[ebp];kept.chosen=gp[esi];kept.next=gp[edi];
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case objects_load:case objects_data:case chosen_load:gp[esi]=v;break;
        case unit_load:gp[ebx]=v;break;
        case block_offset:gp[ebp]=sx16(v);break;                                                           // MOVSX EBP,word
        case state_load:gp[ecx]=v;break;
        case held_load:set_low_byte(gp[ecx],v);break;                                                      // MOV CL
        case animation_offset:gp[edi]=sx16(v);break;                                                       // MOVSX EDI,word
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The frame and the unit.
    void reserving(){this->push(0x1115d0u,this->gp[ecx]);depth=0u-16u;}                                    // PUSH ECX
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / ESI / EDI
        static constexpr std::uint32_t pcs[]{0x1115d1u,0x1115d2u,0x1115d3u,0x111601u};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t unit_index(std::uint32_t handle) {                                                     // MOV ECX,EAX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2]
        const auto index=this->flags.logic(handle&logic::handle_mask,4);
        this->gp[ecx]=3u*index;
        return index;
    }
    std::uint32_t block_address(std::uint32_t offset,std::uint32_t unit){return this->gp[ebp]=this->flags.plus(offset,unit,4);}   // ADD EBP,EBX
    // The compare tree on the state (ECX), DL the flag; EDI the state led to.
    std::uint32_t flagged_state(std::uint32_t base,unsigned step) {                                      // XOR ECX,ECX; TEST DL,DL; SETNE CL; (LEA / ADD)
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.logic(0u,4);this->test8(gp[edx]);
        set_low_byte(gp[ecx],fl.zf?0u:1u);
        if(step==2u)gp[ecx]=2u*gp[ecx]+base;
        else gp[ecx]=fl.plus(gp[ecx],base,4);
        return gp[ecx];
    }
    std::uint32_t searched(std::uint32_t current,const logic::Next &next) {
        auto &gp=this->gp;auto &fl=this->flags;
        namespace id=logic::state;
        const auto led=[&](std::uint32_t value) {                                                       // MOV EDI,ECX; CMP EDI,7000101; CMP EDI,50000CB
            gp[edi]=value;this->compare32(value,id::none);
            if(value!=id::none)this->compare32(value,id::direct);
        };
        this->compare32(current,id::high);                                                               // CMP ECX,6000086
        saving(4u);gp[edi]=id::none;                                                                     // PUSH EDI; MOV EDI,7000101
        if(std::int32_t(current)>std::int32_t(id::high)) {                                               // SUB ECX,700002C / D / 90
            gp[ecx]=fl.minus(current,id::third,4);
            if(gp[ecx]!=0u){gp[ecx]=fl.minus(gp[ecx],id::fourth-id::third,4);
                if(gp[ecx]!=0u)gp[ecx]=fl.minus(gp[ecx],id::fifth-id::fourth,4);}
        } else if(current==id::high) {
            flagged_state(id::high,1u);led(gp[ecx]);                                                     // ADD ECX,6000086
        } else {
            this->compare32(current,id::second);                                                         // CMP ECX,6000084
            if(std::int32_t(current)>std::int32_t(id::second)) {
                this->compare32(current,id::low);                                                        // CMP ECX,6000085
                if(current==id::low){flagged_state(id::low,2u);led(gp[ecx]);}                            // LEA ECX,[ECX+ECX+6000085]
            } else if(current!=id::second) {
                this->compare32(current,id::first);                                                      // CMP ECX,4000089
                if(current!=id::first)this->compare32(current,id::direct);                               // CMP ECX,50000CB
            }
        }
        // The state led to (EDI), as the logic decided.
        gp[edi]=next.state;
        switch(next.lead) {
        case logic::Lead::none:this->reached(Branch::none);break;
        case logic::Lead::direct:this->reached(Branch::direct);break;
        case logic::Lead::change:this->reached(next.state==logic::state::flagged?Branch::flagged:Branch::kept_state);break;
        }
        return gp[edi];
    }
    // Another state chosen.
    bool set(std::uint32_t v){this->test8(v);if(v&0xffu)this->reached(Branch::held);return (v&0xffu)!=0u;}    // TEST CL,CL
    bool chosen(Kept &kept) {                                                                            // LEA EDX,[ESP+10]; LEA ESI,[ESP+18]; CALL 111D10; TEST AL,AL
        auto &gp=this->gp;
        gp[edx]=local(logic::local::chosen_local);gp[esi]=local(logic::local::argument);
        direct(0x111693u,0x111d10u,0u,kept);
        this->test8(gp[eax]);
        if(!low_byte(gp[eax]))this->reached(Branch::unchosen);
        return low_byte(gp[eax])!=0u;
    }
    bool different(std::uint32_t a,std::uint32_t b){this->compare32(a,b);if(a==b)this->reached(Branch::same);return a!=b;}   // CMP ESI,EDI
    std::uint32_t counted_down(std::uint32_t v){return this->flags.increment(v&0xffu,1,true);}                // DEC byte
    bool negative(std::uint32_t){return this->flags.sf;}                                                      // JNS
    void leaving_restarted() {                                                                           // MOV EAX,EDI; POP EDI; POP ESI
        auto &gp=this->gp;
        gp[eax]=gp[edi];gp[edi]=this->pop(0x1116abu);gp[esi]=this->pop(0x1116acu);
        this->reached(Branch::restarted);
    }
    void change_pushed(std::uint32_t next){put(0x1116f0u,0u);put(0x1116f2u,1u);put(0x1116f4u,next);}          // PUSH 0; PUSH 1; PUSH EDI
    void changed(Kept &kept,std::uint32_t) {                                                             // MOV EDX,7000101; ADD EDI,EBX; MOV ECX,EDX; CALL 1D6430; MOV EDI,ESI
        auto &gp=this->gp;
        gp[edx]=logic::state::none;gp[edi]=this->flags.plus(gp[edi],kept.unit_address,4);gp[ecx]=gp[edx];
        direct(0x111705u,0x1d6430u,12u,kept);
        gp[edi]=gp[esi];
        this->reached(Branch::changed);
    }
    void finish(Exit exit) {                                                                             // MOV EAX,EDI; POP EDI; POP ESI; POP EBP; POP EBX; POP ECX; RET 4
        auto &gp=this->gp;
        std::uint32_t pc;
        if(exit==Exit::restarted)pc=0x1116b1u;
        else {
            pc=exit==Exit::direct?0x111635u:0x11170eu;
            gp[eax]=gp[edi];gp[edi]=this->pop(pc);gp[esi]=this->pop(pc+1u);pc+=2u;
        }
        gp[ebp]=this->pop(pc);gp[ebx]=this->pop(pc+1u);gp[ecx]=this->pop(pc+2u);
        this->ret(pc+3u,4u);this->publish(pc+3u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto handle=gp[eax];const bool flag_set=low_byte(gp[edx])!=0u;
        finish(logic::unit_state_change::change(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]},handle,flag_set));
    }
};
template<class State,class Reference,class Children> bool bridge(State &s,Reference reference,Children children) {
    return run_owner(s,0x1115d0u,0x111713u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children>(v,c,t,children);});
}
} // namespace halo2::native_unit_state_change_detail
