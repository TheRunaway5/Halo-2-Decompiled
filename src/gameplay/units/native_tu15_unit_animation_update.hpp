#pragma once
// Boundary adapter for the unit animation update, 1157D0
// (native_tu15_unit_animation_update_systems.hpp). Its callers and the
// children keep the original ABI, which the adapter owns: three arguments on
// the stack (RET C); a 0xC-byte frame below the saved EBP, ESI and EDI (and
// EBX once the children run); 113B70, 1139D0 and CFB40 get the registers as
// they stand (EAX the unit for CFB40); 1E14F0 ECX the channel and the blend
// pushed (RET 4); 116340 EBX the unit and three arguments pushed (RET C);
// CE230 EAX the unit, CX the index and two arguments pushed that 116280
// pops with its own (RET C); every register each returns coming back. Each
// access publishes its original instruction.
// HALO2_REFERENCE_UNIT_ANIMATION_UPDATE=1 restores the translated body.
// HALO2_VERIFY_UNIT_ANIMATION_UPDATE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_unit_animation_update_systems.hpp"

namespace halo2::native_unit_animation_update_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_unit_animation_update;
using Access=logic::unit_animation_update::Access;
using Kept=logic::unit_animation_update::Kept;
using Outcome=logic::unit_animation_update::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_UNIT_ANIMATION_UPDATE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_UNIT_ANIMATION_UPDATE");return v;}
enum Branch : unsigned {left, no_animation, updated, finished, branch_count};
static_assert(unsigned(Outcome::finished)==Branch::finished);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Unit animation update"};return v;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x1157d3u,0x1157d9u,0x1157dfu,0x1157f0u,0x1157f4u,0x1157f6u,0x1157fcu,0x11580cu,0x115810u,
    0x115816u,0x11581au,0x11582bu,0x115835u,0x115843u,0x11584bu,0x115862u,0x115871u,0x115878u,0x115880u,0x115888u,0x115890u,
    0x115896u,0x1158a6u,0x1158acu,0x1158b4u,0x1158bau,0x1158cau,0x1158ceu,0x1158d2u,0x1158dfu,0x1158ecu,0x1158f0u,0x1158f5u,
    0x1158fau,0x1158feu,0x115902u,0x115909u,0x11590du,0x115911u,0x115915u,0x11592au,0x11592au};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::channels_store)+1u);
template<class State,class Steps> struct Adapter : Port<State> {
    Steps &steps;
    std::uint32_t depth{};      // bytes below the return address
    bool saved{};
    unsigned blends{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Steps &st):Port<State>(s,c,t,{}),steps(st){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case definition:gp[ecx]=fl.shift(fl.logic(gp[ecx]&0xffffu,4),4u,4);break;                    // AND ECX,FFFF; SHL ECX,4
        case first_constant:this->test8(gp[ebx]&0xffu);break;                                         // TEST BL,BL
        default:break;
        }
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:case unit_tag:case slot_blend_load:case parent:case state_load:case headers_again:case unit_again:gp[ecx]=v;break;
        case datums:gp[edx]=v;put(0x1157dcu,gp[ebp]);put(0x1157ddu,gp[esi]);put(0x1157deu,gp[edi]);break;
        case unit_argument:gp[edi]=v;gp[eax]=fl.shift(3u*fl.logic(v&0xffffu,4),2u,4);break;           // MOV EAX,EDI; AND; LEA; SHL EAX,2
        case unit_address:gp[esi]=v;break;
        case instances:case definition_flags:case b_load:case b_again:case datums_again:gp[edx]=v;break;
        case channels_offset:gp[ebp]=logic::widen16(v);break;
        case definition:gp[ecx]=v;break;
        case state_offset:                                                                             // ADD EAX,ESI; SHR EDX,B; ADD EBP,ESI; TEST DL,1
            gp[eax]=fl.plus(logic::widen16(v),gp[esi],4);gp[edx]=fl.right(gp[edx],logic::definition::left_bit,4,false);
            gp[ebp]=fl.plus(gp[ebp],gp[esi],4);this->test8(gp[edx]&1u);break;
        case animation:gp[edx]=v;gp[ecx]=fl.logic(0xffffffffu,4);fl.minus(v,gp[ecx],4);break;         // OR ECX,-1; CMP EDX,ECX
        case graph:fl.minus(v,gp[ecx],4);break;
        case mode:fl.minus(v&0xffffu,gp[ecx]&0xffffu,2);break;
        case first_constant:this->s.xmm[3]=lane0_only(v);break;
        case second_constant:this->s.xmm[2]=lane0_only(v);break;
        case blend_load:case a_load:case a_again:case index_load:gp[eax]=v;break;
        case slot_answer:set_low_byte(gp[ebx],v);this->test8(v&0xffu);break;
        case index_byte:set_low_byte(gp[ecx],v);gp[eax]=fl.logic(0xffffffffu,4);fl.minus(v&0xffu,0xffu,1);break;   // OR EAX,-1; CMP CL,AL
        case first_link:case second_link:fl.minus(v,gp[eax],4);break;
        case index_value:set_low_word(gp[ecx],logic::widen8(v));break;                                 // MOVSX CX,byte
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v){if(a==Access::channels_store)this->flags.logic(v&0xffu,1);}   // AND byte [EBP],FE
    void step(std::uint32_t pc,std::uint32_t target) {
        auto child=[&](State &v){steps(v,target);};
        this->call(pc,pc+5u,target,child);
    }
    static std::uint32_t lane0(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
    void refresh(Kept &kept){kept={this->gp[ebx]&0xffu,this->gp[ebp],this->gp[esi],this->gp[edi],lane0(this->s.xmm[3]),lane0(this->s.xmm[2])};}
    void saving(){put(0x115855u,this->gp[ebx]);saved=true;}
    // CALL 113B70; MOV BL,AL.
    std::uint32_t first_blend(Kept &kept){step(0x115856u,0x113b70u);set_low_byte(this->gp[ebx],this->gp[eax]);refresh(kept);return this->gp[eax];}
    std::uint32_t second_blend(Kept &kept){step(0x11585du,0x1139d0u);refresh(kept);return this->gp[eax];}
    // MOV EAX,EDI; CALL CFB40; TEST AL,AL.
    std::uint32_t refuses(Kept &kept) {
        this->gp[eax]=this->gp[edi];step(0x115868u,0xcfb40u);this->test8(this->gp[eax]&0xffu);refresh(kept);
        return this->gp[eax];
    }
    // PUSH the blend; LEA ECX,[EBP+84] (or +80); CALL 1E14F0.
    void blend(std::uint32_t channel,std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        if(!blends){put(0x11589au,gp[eax]);gp[ecx]=channel;step(0x1158a1u,0x1e14f0u);}
        else{put(0x1158beu,gp[ecx]);gp[ecx]=channel;step(0x1158c5u,0x1e14f0u);}
        depth-=4u;++blends;refresh(kept);
    }
    // PUSH EDX; PUSH EAX; PUSH ECX; MOV EBX,EDI; CALL 116340.
    void update(std::uint32_t,std::uint32_t,std::uint32_t,Kept &kept) {
        auto &gp=this->gp;
        put(0x1158d5u,gp[edx]);put(0x1158d6u,gp[eax]);put(0x1158d7u,gp[ecx]);gp[ebx]=gp[edi];
        step(0x1158dau,0x116340u);depth-=12u;refresh(kept);
    }
    void finishing(std::uint32_t,unsigned which){put(which?0x11590cu:0x115908u,which?this->gp[eax]:this->gp[edx]);}
    // MOV EAX,EDI; CALL CE230 (its arguments stay).
    std::uint32_t finish_value(std::uint32_t,Kept &kept){this->gp[eax]=this->gp[edi];step(0x11591fu,0xce230u);refresh(kept);return this->gp[eax];}
    // PUSH EAX; CALL 116280 (it pops CE230's two arguments with its own).
    void finish(std::uint32_t,Kept &kept){put(0x115924u,this->gp[eax]);step(0x115925u,0x116280u);depth-=12u;refresh(kept);}
    void restoring(){if(saved)this->gp[ebx]=take(0x11592eu);}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0xcu,4);depth=0xcu;                                                 // SUB ESP,C
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::unit_animation_update::run(memory,*this)));
        gp[edi]=take(0x11592fu);gp[esi]=take(0x115930u);gp[ebp]=take(0x115931u);
        gp[esp]=fl.plus(gp[esp],0xcu,4);                                                             // ADD ESP,C
        this->ret(0x115935u,0xcu);this->publish(0x115935u);
    }
};
template<class State,class Reference,class Steps> bool bridge(State &s,Reference reference,Steps steps) {
    return run_owner(s,0x1157d0u,0x115935u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Steps>(v,c,t,steps);});
}
} // namespace halo2::native_unit_animation_update_detail
