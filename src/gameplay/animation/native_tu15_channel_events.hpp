#pragma once
// Boundary adapter for the channel events, 1CF700
// (native_tu15_channel_events_systems.hpp). Its callers and the callback keep
// the original ABI, which the adapter owns: EBX the callback and six
// arguments on the stack (RET 18), the mask in EAX; a 0x28-byte frame below
// the saved EBP, ESI and EDI; the callback (CALL EBX, through the dispatch;
// stdcall, three pushed) gets the registers each pass leaves (the events'
// ESI index and EDI count, the sounds' EBP index, ESI entry and DI frame,
// the effects' EDI index, EBP count and SI frame), every register it
// returns coming back. Each access publishes its original instruction.
// HALO2_REFERENCE_CHANNEL_EVENTS=1 restores the translated body.
// HALO2_VERIFY_CHANNEL_EVENTS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/animation/native_tu15_channel_events_systems.hpp"

namespace halo2::native_channel_events_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_channel_events;
using Access=logic::channel_events::Access;
using Kept=logic::channel_events::Kept;
using Pass=logic::channel_events::Pass;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_CHANNEL_EVENTS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_CHANNEL_EVENTS");return v;}
enum Branch : unsigned {fired, event_called, sound_called, effect_called, gated_out, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Channel events"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
// Each role's instruction in the events, sounds and effects passes (one instruction for the shared prologue and end).
inline constexpr std::uint32_t sites[][3]{
    {0x1cf703u,0x1cf703u,0x1cf703u},{0x1cf709u,0x1cf709u,0x1cf709u},{0x1cf717u,0x1cf717u,0x1cf717u},{0x1cf71bu,0x1cf71bu,0x1cf71bu},
    {0x1cf723u,0x1cf723u,0x1cf723u},{0x1cf72cu,0x1cf72cu,0x1cf72cu},{0x1cf734u,0x1cf734u,0x1cf734u},{0x1cf73au,0x1cf7e4u,0x1cf90cu},
    {0x1cf746u,0x1cf7f1u,0x1cf919u},{0x1cf74au,0x1cf7f5u,0x1cf91du},{0u,0x1cf7fcu,0u},{0x1cf755u,0x1cf810u,0x1cf930u},
    {0x1cf759u,0x1cf814u,0x1cf934u},{0x1cf75fu,0x1cf817u,0x1cf937u},{0x1cf76au,0x1cf826u,0x1cf946u},{0x1cf76fu,0x1cf82bu,0x1cf94bu},
    {0x1cf777u,0x1cf837u,0x1cf957u},{0x1cf782u,0u,0u},{0x1cf785u,0u,0u},{0x1cf794u,0u,0u},{0u,0x1cf846u,0x1cf966u},
    {0u,0x1cf862u,0x1cf982u},{0u,0x1cf866u,0x1cf986u},{0u,0x1cf874u,0x1cf994u},{0u,0x1cf884u,0x1cf9a0u},{0u,0x1cf88cu,0x1cf9a8u},
    {0u,0x1cf899u,0x1cf9b5u},{0u,0x1cf8c0u,0u},{0x1cf7a9u,0x1cf8ceu,0x1cf9dcu},{0x1cf79cu,0x1cf8e4u,0x1cf9fdu},
    {0x1cf7a4u,0x1cf8ecu,0x1cfa05u},{0x1cf7aeu,0x1cf8ddu,0x1cf9f6u},{0x1cf7b3u,0x1cf8d3u,0x1cf9ecu},{0x1cf7bdu,0x1cf8bcu,0x1cf9e0u},
    {0x1cf7c5u,0x1cf8cau,0x1cfa0au},{0x1cf7b7u,0x1cf8d7u,0x1cf9f0u},{0x1cf7cfu,0x1cf8f3u,0x1cfa14u},{0x1cf7d5u,0x1cf8f9u,0x1cfa1au},
    {0u,0x1cf8ffu,0u},{0x1cfa29u,0x1cfa29u,0x1cfa29u}};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::mask_answer)+1u);
inline constexpr std::uint32_t pushes_at[3][3]{{0x1cf7adu,0x1cf7bbu,0x1cf7bcu},{0x1cf8d2u,0x1cf8dbu,0x1cf8dcu},{0x1cf9ebu,0x1cf9f4u,0x1cf9f5u}};
inline constexpr std::uint32_t calls_at[3]{0x1cf7cdu,0x1cf8f1u,0x1cfa12u};
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes below the return address
    unsigned pass{};            // 0 events, 1 sounds, 2 effects
    unsigned pushed{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)][pass];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case to_load:case to_again:x[2]=lane0(v);break;
        case from_load:case from_again:x[1]=lane0(v);break;
        case options_load:if(pass==0u)set_low_byte(gp[eax],v);else gp[eax]=v;break;
        case pass_animation:if(pass==0u)gp[eax]=v;else gp[ecx]=v;break;
        case pass_count:if(pass==0u)gp[edi]=v;else if(pass==1u)gp[eax]=v;else gp[ebp]=v;break;
        case animation_load:if(pass==0u)gp[ecx]=v;else gp[edx]=v;break;
        case list_load:if(pass==0u)gp[edx]=v;else gp[eax]=v;break;
        case entry_frame:                                                                              // MOV DX / DI / SI,[...+2]; MOVSX ECX
            if(pass==0u){gp[eax]=gp[edx]+4u*gp[esi];set_low_word(gp[edx],v);}
            else if(pass==1u)set_low_word(gp[edi],v);
            else set_low_word(gp[esi],v);
            gp[ecx]=logic::widen16(v);
            break;
        case start_load:fl.scalar_compare(x[0],lane0(v),false);break;                                  // COMISS XMM0,[start]
        case end_load:x[3]=lane0(v);fl.scalar_compare(x[3],x[0],false);break;                         // MOVSS XMM3,[end]; COMISS XMM3,XMM0
        case event_bit:gp[ecx]=logic::widen16(v);break;
        case mask_load:gp[ebp]=v;break;
        case entry_name:set_low_word(gp[eax],v);break;
        case definition_load:gp[edx]=3u*logic::widen16(gp[eax]&0xffffu);gp[eax]=v;break;               // LEA EDX,[EAX+EAX*2]; MOV EAX,[def]
        case references:gp[ecx]=v;gp[eax]=v+4u*gp[edx];break;                                         // LEA EAX,[ECX+EDX*4]
        case reference_tag:gp[edx]=v;break;
        case reference_flags:set_low_word(gp[eax],v);break;
        case options_gate:case options_gate_second:gp[ecx]=v;
            fl.logic(((v>>8)&0xffu)&(a==options_gate?0x20u:0x10u),1);break;                         // TEST CH,20 / TEST CH,10
        case sound_extra:gp[edx]=v;break;
        case frame_load:if(pass==2u)gp[ecx]=v;else gp[edx]=v;break;
        case context_load:if(pass==2u)gp[edx]=v;else gp[eax]=v;break;
        case count_load:gp[eax]=v;break;
        case mask_answer:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saving() {                                                                                    // PUSH EBP; PUSH ESI; XOR ESI,ESI; COMISS XMM1,XMM2
        auto &gp=this->gp;auto &fl=this->flags;
        put(0x1cf70fu,gp[ebp]);put(0x1cf710u,gp[esi]);gp[esi]=fl.logic(0u,4);
        fl.scalar_compare(this->s.xmm[1],this->s.xmm[2],false);
    }
    void saved(){put(0x1cf716u,this->gp[edi]);}
    void ranged(bool){this->flags.scalar_compare(this->s.xmm[1],this->s.xmm[2],false);}              // COMISS XMM1,XMM2
    void passing(Pass which,std::uint32_t options,bool) {                                             // TEST AL,AL; TEST AH,1; TEST AH,2
        pass=unsigned(which);
        if(which==Pass::events)this->test8(options&0xffu);
        else this->flags.logic(((options>>8)&0xffu)&(which==Pass::sounds?1u:2u),1);
    }
    void counted(std::uint32_t count,const Kept &) {                                                   // CMP EDI,ESI; XOR EBP + TEST EAX; XOR EDI + TEST EBP
        auto &gp=this->gp;auto &fl=this->flags;
        if(pass==0u)this->compare32(count,gp[esi]);
        else if(pass==1u){gp[ebp]=fl.logic(0u,4);this->test32(count);}
        else{gp[edi]=fl.logic(0u,4);this->test32(count);}
    }
    void sound_entry(std::uint32_t address,std::uint32_t){this->gp[esi]=address;}                     // LEA ESI,[EAX+EBP*8]
    void effect_entry(std::uint32_t address,std::uint32_t){this->gp[eax]=address;}                    // LEA EAX,[EAX+EDI*4]
    void converted(std::uint32_t at_frame){this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0(at_frame));}   // CVTSI2SS XMM0,ECX
    void entry(bool,bool){}
    void marked(std::uint32_t bit,std::uint32_t mask,const Kept &kept) {                               // MOV EAX,1; SHL EAX,CL; OR EBP,EAX; TEST EBX,EBX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.shift(1u,bit&0xffu,4);gp[ebp]=fl.logic(mask,4);this->test32(kept.callback);
    }
    void named(std::uint32_t name) {                                                                   // CMP AX,FFFF; (MOVSX EAX,AX; CMP EAX,-1)
        auto &gp=this->gp;auto &fl=this->flags;
        fl.minus(name&0xffffu,0xffffu,2);
        if((name&0xffffu)!=0xffffu){gp[eax]=logic::widen16(name);this->compare32(gp[eax],logic::none);}
    }
    void referenced(std::uint32_t reference,std::uint32_t tag,bool tag_read) {                         // TEST EAX,EAX; CMP EDX,-1; (TEST EBX,EBX)
        if(!tag_read){this->test32(reference);return;}
        this->compare32(tag,logic::none);
        if(pass==1u && tag!=logic::none)this->test32(this->gp[ebx]);
    }
    void gate_set(std::uint32_t flags,bool forward_ok) {                                               // TEST AL,10; MOV CL,1; (COMISS; XOR CL,CL)
        auto &gp=this->gp;auto &fl=this->flags;
        set_low_byte(gp[ecx],1u);
        if(flags&logic::gate::forward_only){fl.scalar_compare(this->s.xmm[1],this->s.xmm[2],false);if(!forward_ok)set_low_byte(gp[ecx],fl.logic(0u,1));}
        if(flags&logic::gate::backward_only)fl.scalar_compare(this->s.xmm[2],this->s.xmm[1],false);
    }
    void gated(bool forward_ok){this->test8(forward_ok?1u:0u);if(!forward_ok)this->reached(Branch::gated_out);}   // TEST CL,CL
    bool callback_tested(const Kept &kept){this->test32(kept.callback);return kept.callback!=0u;}    // TEST EBX,EBX
    void flags_word(std::uint32_t flags) {                                                             // MOVZX EAX (EDX),AX; LEA ECX (EAX),[ESP+20]
        auto &gp=this->gp;
        if(pass==1u){gp[eax]=flags&0xffffu;gp[ecx]=local(logic::local::record);}
        else{gp[edx]=flags&0xffffu;gp[eax]=local(logic::local::record);}
    }
    void pushing(std::uint32_t value) {
        auto &gp=this->gp;
        if(pushed==0u && pass==0u)gp[ecx]=value;                                                      // LEA ECX,[ESP+20]
        put(pushes_at[pass][pushed++],value);
    }
    void call_back(Kept &kept) {                                                                       // CALL EBX (the events' XOR EAX,EAX before)
        auto &gp=this->gp;
        if(pass==0u)this->flags.logic(0u,4);
        const auto target=gp[ebx];
        auto child=[&](State &v){dispatch(v,target,calls_at[pass]);};
        this->call(calls_at[pass],calls_at[pass]+2u,target,child);depth-=0xcu;
        pushed=0u;
        kept.callback=gp[ebx];
        if(pass==0u){kept.index=gp[esi];kept.count=gp[edi];}
        else if(pass==1u)kept.index=gp[ebp];
        else{kept.index=gp[edi];kept.count=gp[ebp];}
        this->reached(pass==0u?Branch::event_called:pass==1u?Branch::sound_called:Branch::effect_called);
    }
    void advanced(std::uint32_t index,std::uint32_t count) {                                           // INC ESI (EBP, EDI); CMP
        auto &gp=this->gp;
        const unsigned reg=pass==0u?esi:pass==1u?ebp:edi;
        gp[reg]=this->flags.increment(index-1u,4,false);this->compare32(index,count);
    }
    void restoring(){auto &gp=this->gp;gp[edi]=take(0x1cfa2du);gp[esi]=take(0x1cfa2eu);gp[ebp]=take(0x1cfa2fu);}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x28u,4);depth=0x28u;                                                 // SUB ESP,28
        Memory<Adapter> memory{*this};
        const auto mask=logic::channel_events::fire(memory,*this,Kept{gp[ebx],0u,0u});
        gp[eax]=mask;this->reached(Branch::fired);
        gp[esp]=fl.plus(gp[esp],0x28u,4);                                                              // ADD ESP,28
        this->ret(0x1cfa33u,0x18u);this->publish(0x1cfa33u);
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x1cf700u,0x1cfa33u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_channel_events_detail
