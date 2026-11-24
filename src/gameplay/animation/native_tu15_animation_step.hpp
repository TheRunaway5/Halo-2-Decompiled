#pragma once
// Boundary adapter for the animation step, 1D39C0
// (native_tu15_animation_step_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: ESI the state, the step, last
// and three more pushed (RET 0x14), the answer in AL; ECX pushed (the done
// and report flags in its slot), EBX, EBP and EDI saved; 1CEB70 (EAX the
// channel; the state, step and last pushed, RET 0xC), 1E3150 (EDX the
// definition; the sub pushed, RET 4), 1D38A0 (EAX; the state, 0.267 and two
// more pushed, RET 0x10), 1D3930 (EAX the state) and 1CE950 (the channel,
// the scaled frames, the state, step and last pushed, RET 0x14) direct,
// every register each returns coming back. Each access publishes its
// original instruction.
// HALO2_REFERENCE_ANIMATION_STEP=1 restores the translated body.
// HALO2_VERIFY_ANIMATION_STEP=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/animation/native_tu15_animation_step_systems.hpp"

namespace halo2::native_animation_step_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_animation_step;
using Access=logic::animation_step::Access;
using Kept=logic::animation_step::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ANIMATION_STEP");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ANIMATION_STEP");return v;}
enum Branch : unsigned {stepped, counted_down, reported, cleared, blended, clamped, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Animation step"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x1d39c6u,0x1d39d5u,0x1d39d9u,0x1d39dfu,0x1d39e8u,0x1d39eeu,0x1d39f2u,0x1d3a02u,0x1d3a02u,0x1d3a06u,
    0x1d3a12u,0x1d3a1bu,0x1d3a25u,0x1d3a2fu,0x1d3a36u,0x1d3a3cu,0x1d3a42u,0x1d3a4du,0x1d3a59u,0x1d3a63u,0x1d3a69u,0x1d3a6du,0x1d3a72u,
    0x1d3a85u,0u,0x1d3ac5u,0x1d3ac9u,0x1d3acfu,0x1d3ad5u,0x1d3ad5u,0x1d3ae5u,0x1d3af2u,0x1d3b00u,0x1d3b0cu,0x1d3b12u,0x1d3b14u,0x1d3b19u,
    0x1d3b25u,0x1d3b32u,0x1d3b39u,0x1d3b41u,0x1d3b54u,0x1d3b5cu,0x1d3b69u,0x1d3b71u,0x1d3b77u,0x1d3b79u,0x1d3b87u,0x1d3b8bu,0x1d3b94u,
    0x1d3b98u,0x1d3b9cu,0x1d3ba8u,0x1d3baeu,0x1d3bb9u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::done_final)+1u);
inline constexpr std::uint32_t blend_stores[14]{0x1d3a8fu,0x1d3a92u,0x1d3a96u,0x1d3a9au,0x1d3a9fu,0x1d3aa3u,0x1d3aa7u,0x1d3aabu,0x1d3ab0u,
    0x1d3ab4u,0x1d3ab8u,0x1d3abbu,0x1d3abeu,0x1d3ac1u};
template<class State,class CountDown,class Record,class Report,class Switch,class Advance> struct Adapter : Port<State> {
    CountDown &count_down_child;
    Record &record_child;
    Report &report_child;
    Switch &switch_child;
    Advance &advance_child;
    std::uint32_t depth{};      // bytes below the return address
    unsigned blend_count{},tag_register{eax};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,CountDown &d,Record &r,Report &p,Switch &w,Advance &a)
        :Port<State>(s,c,t,{}),count_down_child(d),record_child(r),report_child(p),switch_child(w),advance_child(a){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t top()const{return this->gp[esp];}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return a==Access::blend_store?blend_stores[blend_count++]:sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case owner:case step_first:case blend_sub:case report_b:case first_sub:case second_instances:case last_again:gp[ecx]=v;break;
        case step_last:case blend_again:case report_c:case report_a:case second_handle:case first_instances:case second_tag:
        case second_sub:gp[eax]=v;break;
        case blend_definition:gp[eax]=v;break;
        case instances:case first_tag:case first_definition:case second_definition:case step_again:gp[edx]=v;break;
        case done_check:case done_final:case blending:set_low_byte(gp[eax],v);break;
        case record_flags:set_low_byte(gp[edx],v);break;
        case first_frames:set_low_word(gp[eax],v);break;
        case second_frames:gp[eax]=logic::widen16(v);break;
        default:break;
        }
        if(a==blend_again || a==second_tag)tag_register=eax;
        if(a==first_tag)tag_register=edx;
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saving() {                                                                                      // PUSH ECX, EBX, EBP, EDI; XOR EBP,EBP
        auto &gp=this->gp;
        put(0x1d39c0u,gp[ecx]);put(0x1d39c1u,gp[ebx]);put(0x1d39c2u,gp[ebp]);put(0x1d39c3u,gp[edi]);
        gp[ebp]=this->flags.logic(0u,4);
    }
    void starting(const Kept &kept){auto &gp=this->gp;gp[edi]=kept.channel;gp[ebx]=this->flags.logic(kept.none,4);}   // MOV EDI,ESI; OR EBX,-1
    void compared(std::uint32_t a,std::uint32_t b){this->compare32(a,b);}
    void compared16(std::uint32_t a,std::uint32_t b){this->flags.minus(a&0xffffu,b&0xffffu,2);}
    void tested8(std::uint32_t v){this->test8(v);}
    void tested16(std::uint32_t v){this->flags.logic(v&0xffffu,2);}
    void compared_float(std::uint32_t a,std::uint32_t b){this->flags.scalar_compare(lane0(a),lane0(b),false);}   // COMISS
    void owner_found(std::uint32_t owner_value,const Kept &kept) {                                    // XOR AL,AL; CMP ECX,EBX
        set_low_byte(this->gp[eax],this->flags.logic(0u,1));this->compare32(owner_value,kept.none);
    }
    void count_down(std::uint32_t last,std::uint32_t step,Kept &kept) {                                 // PUSH EAX, ECX, ESI; MOV EAX,EDI; CALL 1CEB70
        auto &gp=this->gp;
        put(0x1d39f6u,last);put(0x1d39f7u,step);put(0x1d39f8u,kept.state);gp[eax]=kept.channel;
        this->call(0x1d39fbu,0x1d3a00u,0x1ceb70u,count_down_child);depth-=12u;
        refresh(kept);
        this->reached(Branch::counted_down);
    }
    std::uint32_t done_anded(std::uint32_t done,std::uint32_t al) {                                     // (MOV AL,1); AND byte [ESP+E],AL
        set_low_byte(this->gp[eax],al);
        return this->flags.logic(done&al&0xffu,1);
    }
    void walked(const Kept &kept,std::uint32_t done) {                                                  // INC EBP; ADD EDI,20; TEST AL,AL
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ebp]=fl.increment(kept.index-1u,4,false);gp[edi]=fl.plus(kept.channel-logic::channel_size,logic::channel_size,4);
        this->test8(done);
    }
    std::uint32_t tag_index(std::uint32_t tag) {                                                        // AND reg,FFFF; SHL reg,4
        auto &fl=this->flags;
        return this->gp[tag_register]=fl.shift(fl.logic(tag&0xffffu,4),4u,4);
    }
    std::uint32_t record_of(unsigned n,std::uint32_t definition,std::uint32_t sub,Kept &kept) {       // PUSH sub; (MOV EDX,EAX); CALL 1E3150
        auto &gp=this->gp;
        static constexpr std::uint32_t pushes[3]{0x1d3a51u,0x1d3b29u,0x1d3b8eu},calls[3]{0x1d3a54u,0x1d3b2au,0x1d3b8fu},
            returns[3]{0x1d3a59u,0x1d3b2fu,0x1d3b94u};
        put(pushes[n],sub);gp[edx]=definition;
        this->call(calls[n],returns[n],0x1e3150u,record_child);depth-=4u;
        refresh(kept);
        return gp[eax];
    }
    std::uint32_t silent(std::uint32_t flags_byte) {                                                    // SHR DL,1; NOT DL; AND DL,1
        auto &gp=this->gp;auto &fl=this->flags;
        const auto shifted=fl.right(flags_byte&0xffu,logic::record::silent_bit,1,false);
        set_low_byte(gp[edx],fl.logic((~shifted)&1u,1));
        return gp[edx]&0xffu;
    }
    void pushing_report(std::uint32_t c){put(0x1d3a71u,c);}                                              // PUSH EAX
    void report(std::uint32_t a,std::uint32_t b,Kept &kept) {                                           // PUSH ECX, 0.267, ESI; CALL 1D38A0
        auto &gp=this->gp;
        gp[eax]=a;put(0x1d3a76u,b);put(0x1d3a77u,logic::report_weight);put(0x1d3a7cu,kept.state);
        this->call(0x1d3a7du,0x1d3a82u,0x1d38a0u,report_child);depth-=16u;
        refresh(kept);
        this->reached(Branch::reported);
    }
    void zeroed(unsigned n){this->s.xmm[n]=_mm_setzero_ps();}                                            // XORPS
    void loaded_xmm(unsigned n,std::uint32_t v){this->s.xmm[n]=lane0(v);}                                // MOVSS from memory
    void clear_eax(){this->gp[eax]=this->flags.logic(0u,4);}                                             // XOR EAX,EAX
    std::uint32_t anded(std::uint32_t v,std::uint32_t mask){this->reached(Branch::cleared);return this->flags.logic(v&mask&0xffu,1);}
    void switch_model(Kept &kept) {                                                                      // MOV EAX,ESI; CALL 1D3930
        this->gp[eax]=kept.state;
        this->call(0x1d3ae0u,0x1d3ae5u,0x1d3930u,switch_child);
        refresh(kept);
    }
    void second_channel(const Kept &kept){this->gp[edi]=kept.channel;}                                  // LEA EDI,[ESI+20]
    std::uint32_t xmm1()const{return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(this->s.xmm[1])));}
    void converted(unsigned n,std::uint32_t v) {                                                        // (MOVSX EDX,AX); CVTSI2SS
        if(n==2u)this->gp[edx]=logic::widen16(this->gp[eax]);
        this->s.xmm[n]=_mm_move_ss(this->s.xmm[n],lane0(v));
    }
    void divided(std::uint32_t v){this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0(v));this->reached(Branch::clamped);}   // DIVSS XMM0,XMM2
    void multiplied(std::uint32_t v){this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0(v));}               // MULSS XMM0,[ESP+34]
    void pushing_advance(std::uint32_t last,std::uint32_t step,const Kept &kept) {                      // PUSH ECX, EDX, ESI, ECX
        put(0x1d3ba0u,last);put(0x1d3ba1u,step);put(0x1d3ba2u,kept.state);put(0x1d3ba3u,last);
    }
    void advance(Kept &kept) {                                                                           // PUSH EDI; CALL 1CE950
        put(0x1d3bb3u,kept.channel);
        this->call(0x1d3bb4u,0x1d3bb9u,0x1ce950u,advance_child);depth-=20u;
        refresh(kept);
        this->reached(Branch::blended);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto done=logic::animation_step::advance(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]});
        this->reached(Branch::stepped);
        set_low_byte(gp[eax],done);
        gp[edi]=take(0x1d3bbdu);gp[ebp]=take(0x1d3bbeu);gp[ebx]=take(0x1d3bbfu);gp[ecx]=take(0x1d3bc0u);
        this->ret(0x1d3bc1u,0x14u);this->publish(0x1d3bc1u);
    }
};
template<class State,class Reference,class CountDown,class Record,class Report,class Switch,class Advance>
bool bridge(State &s,Reference reference,CountDown count_down,Record record,Report report,Switch switch_model,Advance advance) {
    return run_owner(s,0x1d39c0u,0x1d3bc1u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return Adapter<State,CountDown,Record,Report,Switch,Advance>(v,c,t,count_down,record,report,switch_model,advance);});
}
} // namespace halo2::native_animation_step_detail
