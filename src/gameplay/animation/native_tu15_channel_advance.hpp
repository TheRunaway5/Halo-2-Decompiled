#pragma once
// Boundary adapter for the channel advance, 1CE950
// (native_tu15_channel_advance_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: five arguments on the stack
// (RET 14); a 0xC-byte frame below the saved EBX, EBP, ESI and EDI; 1CEE60
// (ECX the channel), 1E3150 (EDX the graph, one pushed, RET 4), 1CF700 (six
// pushed, EBX the owner, RET 18), 1D3660 (ESI the variant's slot, one pushed,
// RET 4), B8780 (ECX the channel) and 1CE810 (EAX the channel, the step
// pushed, RET 4), direct, every register each returns coming back. Each
// access publishes its original instruction.
// HALO2_REFERENCE_CHANNEL_ADVANCE=1 restores the translated body.
// HALO2_VERIFY_CHANNEL_ADVANCE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/animation/native_tu15_channel_advance_systems.hpp"

namespace halo2::native_channel_advance_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_channel_advance;
using Access=logic::channel_advance::Access;
using Kept=logic::channel_advance::Kept;
using Outcome=logic::channel_advance::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_CHANNEL_ADVANCE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_CHANNEL_ADVANCE");return v;}
enum Branch : unsigned {replaced, stepped, looped, ended, changed, negative_step, switched_animation, branch_count};
static_assert(unsigned(Outcome::changed)==Branch::changed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Channel advance"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
// Each role's instruction; the checks and the joins have one per place they happen.
inline constexpr std::uint32_t sites[]{0x1ce955u,0x1ce95bu,0x1ce960u,0x1ce96du,0x1ce972u,0x1ce978u,0x1ce97bu,0x1ce989u,0x1ce997u,
    0x1ce99cu,0x1ce9a4u,0x1ce9a8u,0x1ce9acu,0x1ce9b0u,0x1ce9bau,0x1ce9c2u,0x1ce9ceu,0x1ce9d6u,0x1ce9dcu,0x1ce9dfu,0x1ce9e3u,
    0x1ce9e8u,0x1ce9f0u,0x1ce9f3u,0x1ce9fbu,0u,0u,0u,0u,0u,0x1cea21u,0x1cea27u,0x1cea32u,0x1cea36u,0x1cea3au,0x1cea3fu,0x1cea6eu,
    0x1cea83u,0x1cea8au,0x1cea8du,0x1cea9bu,0x1cea9du,0x1ceaa1u,0x1ceaa6u,0x1ceaaeu,0x1ceab5u,0x1ceac0u,0x1ceac4u,0x1ceacau,
    0x1cead0u,0x1cead4u,0x1ceaddu,0x1ceae5u,0x1ceaebu,0x1ceaf1u,0x1ceaf5u,0x1ceafau,0x1ceb1cu,0x1ceb1cu,0x1ceb22u,0x1ceb22u,
    0x1ceb26u,0x1ceb2eu,0x1ceb36u,0x1ceb4du};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::step_final)+1u);
inline constexpr std::uint32_t animation_checks[]{0x1cea04u,0x1cea51u,0x1ceb07u,0x1ceb3cu};
inline constexpr std::uint32_t saved_checks[]{0x1cea0eu,0x1cea5bu,0x1ceb0du,0x1ceb42u};
inline constexpr std::uint32_t index_checks[]{0x1cea13u,0x1cea60u,0x1ceb12u,0x1ceb47u};
inline constexpr std::uint32_t joins[]{0x1cea1du,0x1cea6au,0x1ceb18u};
// The three 1CF700 calls: their six pushes and the CALL.
inline constexpr std::uint32_t fire_pushes[3][6]{{0x1ce9e7u,0x1ce9efu,0x1ce9f7u,0x1ce9f8u,0x1ce9f9u,0x1ce9fau},
    {0x1cea3eu,0x1cea43u,0x1cea48u,0x1cea49u,0x1cea4au,0x1cea4bu},{0x1ceadcu,0x1ceaf9u,0x1ceafeu,0x1ceaffu,0x1ceb00u,0x1ceb01u}};
inline constexpr std::uint32_t fire_calls[3]{0x1ce9ffu,0x1cea4cu,0x1ceb02u};
template<class State,class Definition,class AnimationOf,class Fire,class NextVariant,class Answer,class Place> struct Adapter : Port<State> {
    Definition &definition_child;
    AnimationOf &animation_child;
    Fire &fire_child;
    NextVariant &next_child;
    Answer &answer_child;
    Place &place_child;
    std::uint32_t depth{};      // bytes below the return address
    unsigned check{},fires{},pushes{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Definition &d,AnimationOf &a,Fire &f,NextVariant &n,Answer &w,Place &p)
        :Port<State>(s,c,t,{}),definition_child(d),animation_child(a),fire_child(f),next_child(n),answer_child(w),place_child(p){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case playing_animation:return animation_checks[check];
        case saved_index:return saved_checks[check];
        case playing_index:return index_checks[check];
        case events_load:case events_store:return joins[check];
        default:return sites[unsigned(a)];
        }
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case channel_argument:gp[ebp]=v;put(0x1ce959u,gp[esi]);put(0x1ce95au,gp[edi]);break;            // PUSH ESI; PUSH EDI
        case started_load:gp[edi]=v;gp[ecx]=gp[ebp];break;                                            // MOV ECX,EBP
        case graph_tag:case next_animation:case old_frame_again:case loop_load:gp[eax]=v;break;
        case instances:gp[ecx]=v;gp[eax]=fl.shift(fl.logic(gp[eax]&0xffffu,4),4u,4);break;            // AND EAX,FFFF; SHL EAX,4
        case graph_definition:gp[edx]=v;gp[eax]=gp[edi];break;                                       // MOV EAX,EDI
        case current_frame:case step_again:case step_wrap:case one:x[0]=lane0(v);break;
        case frame_argument:x[1]=lane0(v);gp[eax]=fl.logic(0u,4);break;                               // XOR EAX,EAX
        case frame_count:gp[ecx]=logic::widen16(v);break;
        case flags_load:set_low_byte(gp[ecx],v);break;
        case context_load:case context_again:case definition_load:case wrapped_load:case definition_wrap:gp[edx]=v;break;
        case step_load:gp[eax]=v;break;
        case options_load:gp[edx]=v&0xffffu;break;
        case old_frame_load:case context_wrap:gp[ecx]=v;break;
        case owner_load:case owner_wrap:gp[ebx]=v;break;
        case saved_index:if(check<3u)set_low_word(gp[ecx],v);else set_low_word(gp[eax],v);break;
        case last_again:fl.scalar_compare(x[0],lane0(v),false);break;                                 // COMISS XMM0,[ESP+24]
        case options_again:case options_wrap:gp[ecx]=v&0xffffu;break;
        case options_word:gp[eax]=v&0xffffu;break;
        case variant_argument:case switched:gp[edi]=v;break;
        case rate_option:fl.logic(v&logic::option::resets_rate,1);break;                               // TEST byte [EBP+12],8
        case loop_frame_again:gp[eax]=logic::widen16(v);break;
        case frame_count_end:gp[edx]=logic::widen16(v);break;
        case step_final:gp[ecx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    std::uint32_t definition(Kept &kept) {                                                             // CALL 1CEE60; MOV EBX,EAX; XOR ESI,ESI
        auto &gp=this->gp;
        this->call(0x1ce964u,0x1ce969u,0x1cee60u,definition_child);
        refresh(kept);
        gp[ebx]=gp[eax];gp[esi]=this->flags.logic(0u,4);
        return gp[eax];
    }
    void index_tested(std::uint32_t index){this->flags.minus(index&0xffffu,0xffffu,2);}                // CMP word [EBP+6],-1
    std::uint32_t animation_of(std::uint32_t graph,Kept &kept) {                                       // PUSH EAX; CALL 1E3150; MOV ESI,EAX
        auto &gp=this->gp;
        put(0x1ce98fu,kept.started);gp[edx]=graph;
        this->call(0x1ce990u,0x1ce995u,0x1e3150u,animation_child);depth-=4u;
        refresh(kept);
        gp[esi]=gp[eax];
        return gp[eax];
    }
    void last_frame(std::uint32_t,std::uint32_t last){this->s.xmm[0]=lane0(last);}                    // CVTSI2SS XMM0,ECX; SUBSS
    void clamped(std::uint32_t,bool negative) {                                                        // XORPS XMM0,XMM0; COMISS XMM0,XMM1
        auto &x=this->s.xmm;
        x[0]=_mm_setzero_ps();this->flags.scalar_compare(x[0],x[1],false);
        if(negative)this->reached(Branch::negative_step);
    }
    void flags_kept(std::uint32_t flags){set_low_byte(this->gp[ecx],this->flags.logic(flags&logic::flag::kept,1));}   // AND CL,F1
    void pushing(std::uint32_t value){put(fire_pushes[fires][pushes++],value);}
    void pushing_fire(std::uint32_t from,std::uint32_t options,std::uint32_t animation,std::uint32_t definition_value) {
        pushing(from);pushing(options);pushing(animation);pushing(definition_value);
    }
    std::uint32_t fire(Kept &kept) {                                                                   // CALL 1CF700 (EBX the owner)
        auto &gp=this->gp;
        gp[ebx]=kept.held;
        this->call(fire_calls[fires],fire_calls[fires]+5u,0x1cf700u,fire_child);depth-=0x18u;
        refresh(kept);
        ++fires;pushes=0u;
        return gp[eax];
    }
    void checkpoint(unsigned n){check=n;}
    void animation_compared(const Kept &kept,std::uint32_t now){this->flags.minus(kept.started&0xffffu,now&0xffffu,2);}   // CMP DI,[EBP+4]
    void index_compared(std::uint32_t saved,std::uint32_t now){this->flags.minus(saved&0xffffu,now&0xffffu,2);}            // CMP CX (AX),[EBP+6]
    void joined(std::uint32_t value){this->flags.logic(value&0xffffu,2);}                               // OR word [EBP+14],AX
    void step_compared(std::uint32_t,bool){}
    void options_tested(std::uint32_t options) {                                                       // MOV DL,AL; SHR DL,1; TEST DL,1; (TEST AL,20)
        auto &gp=this->gp;auto &fl=this->flags;
        set_low_byte(gp[edx],fl.right(options&0xffu,1u,1,false));fl.logic(gp[edx]&1u,1);
        if(options&logic::option::loops)fl.logic(options&logic::option::switches,1);
    }
    void loop_tested(std::uint32_t loop){this->flags.minus(loop&0xffffu,0u,2);}                        // CMP word [ESI+2E],0
    std::uint32_t next_variant(std::uint32_t current,Kept &kept) {                                     // PUSH EAX; LEA ESI,[ESP+2C]; CALL 1D3660
        auto &gp=this->gp;
        put(0x1cea91u,current);gp[esi]=argument(logic::argument::variant);
        this->call(0x1cea96u,0x1cea9bu,0x1d3660u,next_child);depth-=4u;
        refresh(kept);
        this->reached(Branch::switched_animation);
        return gp[eax];
    }
    void rate_tested(bool){}
    std::uint32_t answer(Kept &kept) {                                                                 // MOV ECX,EBP; CALL B8780; MOV ESI,EAX
        auto &gp=this->gp;
        gp[ecx]=kept.channel;
        this->call(0x1ceab9u,0x1ceabeu,0xb8780u,answer_child);
        refresh(kept);
        gp[esi]=gp[eax];
        return gp[eax];
    }
    void wrapped(std::uint32_t,std::uint32_t loop_from,std::uint32_t wrapped_step) {                   // CVTSI2SS XMM1,EAX; SUBSS/ADDSS XMM0
        auto &x=this->s.xmm;
        x[1]=_mm_move_ss(x[1],lane0(loop_from));x[0]=lane0(wrapped_step);
    }
    void marked(std::uint32_t flags){this->flags.logic(flags&0xffu,1);}                                // OR byte [EBP+11]
    void ended(std::uint32_t,std::uint32_t last_end){this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0(last_end));}   // CVTSI2SS XMM0,EDX; SUBSS
    void place(std::uint32_t step,Kept &kept) {                                                        // PUSH ECX; MOV EAX,EBP; CALL 1CE810
        auto &gp=this->gp;
        put(0x1ceb51u,step);gp[eax]=kept.channel;
        this->call(0x1ceb54u,0x1ceb59u,0x1ce810u,place_child);depth-=4u;
        refresh(kept);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0xcu,4);depth=0xcu;                                                   // SUB ESP,C
        put(0x1ce953u,gp[ebx]);put(0x1ce954u,gp[ebp]);
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::channel_advance::advance(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]})));
        gp[edi]=take(0x1ceb59u);gp[esi]=take(0x1ceb5au);gp[ebp]=take(0x1ceb5bu);gp[ebx]=take(0x1ceb5cu);
        gp[esp]=fl.plus(gp[esp],0xcu,4);                                                               // ADD ESP,C
        this->ret(0x1ceb60u,0x14u);this->publish(0x1ceb60u);
    }
};
template<class State,class Reference,class Definition,class AnimationOf,class Fire,class NextVariant,class Answer,class Place>
bool bridge(State &s,Reference reference,Definition definition,AnimationOf animation_of,Fire fire,NextVariant next_variant,Answer answer,Place place) {
    return run_owner(s,0x1ce950u,0x1ceb60u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return Adapter<State,Definition,AnimationOf,Fire,NextVariant,Answer,Place>(v,c,t,definition,animation_of,fire,next_variant,answer,place);});
}
} // namespace halo2::native_channel_advance_detail
