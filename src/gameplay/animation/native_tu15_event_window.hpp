#pragma once
// Boundary adapter for the event window, 1CEC60
// (native_tu15_event_window_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: ESI the channel (RET); ECX pushed as
// the slot that later holds the event count, EBX saved, and EDI and EBP
// while the events are walked; 1E3150 (EDX the graph, one pushed, RET 4)
// direct, every register it returns coming back; the registers and flags of
// the original's unrolled walk (UCOMISS/LAHF/TEST AH,44 then COMISS). Each
// access publishes its original instruction.
// HALO2_REFERENCE_EVENT_WINDOW=1 restores the translated body.
// HALO2_VERIFY_EVENT_WINDOW=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/animation/native_tu15_event_window_systems.hpp"

namespace halo2::native_event_window_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_event_window;
using Access=logic::event_window::Access;
using Kept=logic::event_window::Kept;
using Outcome=logic::event_window::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_EVENT_WINDOW");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_EVENT_WINDOW");return v;}
enum Branch : unsigned {not_playing, marked, grouped_walk, from_start, from_end, inside, branch_count};
static_assert(unsigned(Outcome::marked)==Branch::marked);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Event window"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x1cec64u,0x1cec69u,0x1cec73u,0x1cec75u,0x1cec83u,0x1cec87u,0x1cec91u,0x1cec9cu,0x1ceca5u,
    0x1cecadu,0x1cecc9u,0x1ceccfu,0u,0u,0u,0u,0u,0u,0x1cedf1u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::count_load)+1u);
// Per slot (0..3 in a group, 4 for the rest): the event base, the event's frame, the channel's frame (UCOMISS).
inline constexpr std::uint32_t base_sites[]{0x1cece0u,0x1ced22u,0x1ced5fu,0x1ced9fu,0x1cee00u};
inline constexpr std::uint32_t frame_sites[]{0x1ceceau,0x1ced25u,0x1ced65u,0x1ceda8u,0x1cee07u};
inline constexpr std::uint32_t channel_sites[]{0x1cecf1u,0x1ced2au,0x1ced6au,0x1ceda4u,0x1cee03u};
// The bit and the OR: per slot, for the window from its start and from its end.
inline constexpr std::uint32_t bit_sites[5][2]{{0x1ced15u,0x1ced15u},{0x1ced52u,0x1ced52u},{0x1ced92u,0x1ced92u},{0x1cedc0u,0x1cedd9u},{0x1cee23u,0x1cee3cu}};
inline constexpr std::uint32_t or_sites[5][2]{{0x1ced1eu,0x1ced1eu},{0x1ced5bu,0x1ced5bu},{0x1ced9bu,0x1ced9bu},{0x1cedc9u,0x1cede2u},{0x1cee2cu,0x1cee45u}};
template<class State,class AnimationOf> struct Adapter : Port<State> {
    AnimationOf &animation_child;
    std::uint32_t depth{};      // bytes below the return address
    unsigned current{};         // the event's slot
    bool start_side{};          // the last event's window was taken from its start
    bool walked{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,AnimationOf &a):Port<State>(s,c,t,{}),animation_child(a){}
    std::uint32_t local()const{return this->gp[esp]+depth-4u;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case events:return base_sites[current];
        case event_frame:return frame_sites[current];
        case channel_frame:return channel_sites[current];
        case event_bit:return bit_sites[current][start_side?0:1];
        case upcoming_load:case upcoming_store:return or_sites[current][start_side?0:1];
        default:return sites[unsigned(a)];
        }
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case graph_tag:case animation_word:gp[eax]=v;break;
        case instances:gp[ecx]=v;gp[eax]=fl.shift(fl.logic(gp[eax]&0xffffu,4),4u,4);break;            // AND EAX,FFFF; SHL EAX,4
        case graph_definition:gp[edx]=v;break;
        case rate:x[0]=lane0(v);break;
        case window_frame:x[2]=lane0(v);break;
        case frame_count:gp[ecx]=logic::widen16(v);break;
        case event_count:case count_load:gp[ebp]=v;break;
        case events:
            if(current==0u){gp[ecx]=v;gp[edx]=4u*gp[ebx];}                                             // LEA EDX,[EBX*4]
            else if(current==1u)gp[ecx]=v;
            else if(current==2u){gp[ecx]=v;gp[edx]=fl.plus(gp[edx],0xcu,4);}                          // ADD EDX,C
            else if(current==3u)gp[ecx]=fl.plus(v,gp[edx],4);                                         // ADD ECX,EDX
            else gp[eax]=v;
            break;
        case event_frame:if(current<3u)gp[eax]=logic::widen16(v);else gp[edx]=logic::widen16(v);break;
        case channel_frame:                                                                            // UCOMISS XMM0,[ESI+1C]; LAHF; TEST AH,44
            fl.scalar_compare(x[0],lane0(v),true);
            gp[eax]=(gp[eax]&0xffff00ffu)|(((fl.bits()|2u)&0xffu)<<8);
            fl.logic((gp[eax]>>8)&0x44u,1);
            break;
        case event_bit:set_low_byte(gp[ecx],v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void cleared(const Kept &kept){this->gp[ebx]=this->flags.logic(kept.index,4);}                    // XOR EBX,EBX
    void playing_tested(std::uint32_t index){this->flags.minus(index&0xffffu,0xffffu,2);}             // CMP word [ESI+6],-1
    void saving_edi(){put(0x1cec8au,this->gp[edi]);}
    std::uint32_t animation_of(std::uint32_t graph,std::uint32_t word,Kept &kept) {                   // PUSH EAX; CALL 1E3150
        auto &gp=this->gp;
        put(0x1cec8bu,word);gp[edx]=graph;
        this->call(0x1cec8cu,0x1cec91u,0x1e3150u,animation_child);depth-=4u;
        kept={gp[esi],gp[edi],gp[ebx]};
        return gp[eax];
    }
    // XORPS XMM1; COMISS XMM0,XMM1; MOV EDI,EAX; (CVTSI2SS XMM1; ADDSS); COMISS XMM2,XMM1 twice: XMM0 the start, XMM2 the end.
    void window(bool,std::uint32_t frame_value,std::uint32_t limit,std::uint32_t start,std::uint32_t end) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        gp[edi]=gp[eax];x[1]=lane0(limit);x[0]=lane0(start);x[2]=lane0(end);
        this->flags.scalar_compare(lane0(frame_value),x[1],false);
    }
    void saving_ebp(){put(0x1cecc8u,this->gp[ebp]);walked=true;}
    void counted(std::uint32_t count){this->compare32(count,logic::group);}                           // CMP EBP,4
    void grouped(std::uint32_t bound) {                                                                // ADD EBP,-3
        this->gp[ebp]=this->flags.plus(bound+(logic::group-1u),0u-(logic::group-1u),4);this->reached(Branch::grouped_walk);
    }
    void remaining(std::uint32_t index,std::uint32_t bound){this->compare32(index,bound);}            // CMP EBX,EBP
    void slot(unsigned n){current=n;}
    // CVTSI2SS XMM1,frame; the COMISS of the event against the window's start, then its end.
    void event_tested(std::uint32_t address,std::uint32_t,std::uint32_t at_frame,bool from_start_side,bool inside_window) {
        auto &gp=this->gp;auto &fl=this->flags;auto &x=this->s.xmm;
        start_side=from_start_side;
        x[1]=_mm_move_ss(x[1],lane0(at_frame));
        fl.scalar_compare(x[1],x[0],false);
        const bool first=from_start_side?!logic::sse::not_above(at_frame,logic::sse::bits(x[0])):!logic::sse::below(at_frame,logic::sse::bits(x[0]));
        if(first)fl.scalar_compare(x[2],x[1],false);
        gp[ecx]=address;
        this->reached(from_start_side?Branch::from_start:Branch::from_end);
        if(inside_window)this->reached(Branch::inside);
    }
    void bit_set(std::uint32_t bit,std::uint32_t word) {                                               // MOV EAX (EDX),1; SHL by CL; OR word [ESI+16]
        auto &gp=this->gp;auto &fl=this->flags;
        const unsigned reg=(current>=3u && !start_side)?edx:eax;
        gp[reg]=fl.shift(1u,bit&0xffu,4);fl.logic(word&0xffffu,2);
    }
    void advanced(std::uint32_t index,std::uint32_t bound,bool grouped_step) {                         // ADD EBX,4 or INC EBX; CMP EBX,EBP
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ebx]=grouped_step?fl.plus(index-logic::group,logic::group,4):fl.increment(index-1u,4,false);
        this->compare32(index,bound);
    }
    void restoring(){auto &gp=this->gp;gp[ebp]=take(0x1cee4eu);gp[edi]=take(0x1cee4fu);}
    void run() {
        auto &gp=this->gp;
        put(0x1cec60u,gp[ecx]);put(0x1cec61u,gp[ebx]);
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::event_window::mark(memory,*this,Kept{gp[esi],gp[edi],gp[ebx]})));
        gp[ebx]=take(0x1cee50u);gp[ecx]=take(0x1cee51u);
        this->ret(0x1cee52u,0u);this->publish(0x1cee52u);
    }
};
template<class State,class Reference,class AnimationOf> bool bridge(State &s,Reference reference,AnimationOf animation_of) {
    return run_owner(s,0x1cec60u,0x1cee52u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,AnimationOf>(v,c,t,animation_of);});
}
} // namespace halo2::native_event_window_detail
