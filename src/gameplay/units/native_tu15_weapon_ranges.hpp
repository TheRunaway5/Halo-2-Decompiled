#pragma once
// Boundary adapter for the weapon ranges, 1ADC90
// (native_tu15_weapon_ranges_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: ECX the unit, ESI the record and
// a word on the stack, AL the answer (RET 4); ECX, EBP, EDI and EBX saved as
// the paths save them (ECX's slot holding the widest range); 106160 (EAX the
// weapon; XMM0 its scale) and 1062A0 (EAX the weapon) direct and 106230
// through the unit's dispatch, every register each returns coming back.
// XMM0 to XMM3 hold what the original's scalar moves and arithmetic leave in
// them. Each access publishes its original instruction.
// HALO2_REFERENCE_WEAPON_RANGES=1 restores the translated body.
// HALO2_VERIFY_WEAPON_RANGES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_weapon_ranges_systems.hpp"

namespace halo2::native_weapon_ranges_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_weapon_ranges;
using Access=logic::weapon_ranges::Access;
using Kept=logic::weapon_ranges::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_WEAPON_RANGES");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_WEAPON_RANGES");return v;}
enum Branch : unsigned {armed, melee, ranged, secondary, kept_widest, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Weapon ranges"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t low(__m128 v){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
inline constexpr std::uint32_t sites[]{0x1adca7u,0x1adcadu,0x1adcb3u,0x1adcb7u,0x1adccdu,0x1adcddu,0x1adcebu,0x1adcefu,0x1adcf1u,0x1add00u,
    0x1add04u,0x1add0eu,0x1add27u,0x1add2fu,0x1add3fu,0x1add43u,0x1add4fu,0x1add54u,0x1add60u,0x1add65u,0x1add71u,0x1add76u,0x1add7eu,
    0x1add89u,0x1add95u,0x1add9au,0x1adda5u,0x1addacu,0x1addbau,0x1addc4u,0x1addc7u,0x1addceu,0x1adddeu,0x1addebu,0x1addeeu,0x1addf5u,
    0x1ade00u,0x1ade03u,0x1ade0au,0x1ade0du,0x1ade13u,0x1ade16u,0x1ade1au,0x1ade2fu,0x1ade3fu,0x1ade4du,0x1ade51u,0x1ade53u,0x1ade62u,
    0x1ade66u,0x1ade6fu,0x1ade75u,0x1ade7du,0x1ade82u,0x1ade85u,0x1ade92u,0x1ade95u,0x1adea9u,0x1adeaeu,0x1adebau,0x1adec7u,0x1adeccu,
    0x1aded4u,0x1adedfu,0x1adee7u,0u,0x1adf07u,0x1adf0fu,0x1adf2au,0x1adf44u,0x1adf49u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::none_second_store)+1u);
template<class State,class Scale,class Melee,class Dispatch> struct Adapter : Port<State> {
    Scale &scale_child;Melee &melee_child;Dispatch &dispatch;
    unsigned slot_register{edx},larger{};unsigned final_exit{};
    std::uint32_t exit{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Scale &a,Melee &m,Dispatch &d):Port<State>(s,c,t,{}),scale_child(a),melee_child(m),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+offset;}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        if(a==Access::final_widest_store){static constexpr std::uint32_t stores[]{0x1adefbu,0x1adf1eu,0x1adf38u};return stores[final_exit];}
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case headers_load:case definition_load:case secondary_definition:gp[edi]=v;break;
        case datums_load:case unit_load:case weapon_object:case instances_load:case word_load:case secondary_object:
        case secondary_instances:case word_again:case single_range:gp[ecx]=v;break;
        case primary_load:set_low_word(gp[edx],sx8(v));slot_register=edx;break;                         // MOVSX DX,byte
        case weapon_load:gp[ebx]=v;break;
        case datums_again:case tag_load:case flags_load:case headers_again:case datums_fourth:case secondary_tag:
        case secondary_range0:case double_range:gp[edx]=v;break;
        case range0:case range1:case range2:case range3:x[2]=lane0(v);break;                             // MOVSS XMM2
        case one_load:case secondary_one:case secondary_range2:case secondary_range3:case widest_kept_load:x[1]=lane0(v);break;
        case widest_load:case widest_fallback:case secondary_widest:case secondary_widest_fallback:case fourth_now:case final_widest:
        case final_widest_fallback:x[0]=lane0(v);break;                                                  // MOVSS XMM0
        case third_now:x[3]=lane0(v);break;
        case melee_range:case datums_third:case secondary_weapon:case secondary_range1:gp[eax]=v;break;
        case ranged_kind:set_low_word(gp[edi],v);break;                                                  // MOV DI,word
        case unit_again:gp[ebp]=v;break;
        case secondary_load:set_low_word(gp[eax],sx8(v));slot_register=eax;break;                       // MOVSX AX,byte
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing(){auto &gp=this->gp;this->push(0x1adc90u,gp[ecx]);set_low_byte(gp[eax],this->flags.logic(0u,1));}   // PUSH ECX; XOR AL,AL
    void compared(std::uint32_t a,std::uint32_t b){this->compare32(a,b);}
    void saving_unit(std::uint32_t handle) {                                                             // PUSH EBP; AND ECX,FFFF; LEA EBP,[ECX+ECX*2]; PUSH EDI
        auto &gp=this->gp;
        this->push(0x1adc9cu,gp[ebp]);gp[ecx]=this->flags.logic(handle&logic::handle_mask,4);gp[ebp]=3u*gp[ecx];
        this->push(0x1adca6u,gp[edi]);
    }
    std::uint32_t unit_scaled(){return this->gp[ebp]=this->flags.shift(this->gp[ebp],2u,4);}           // SHL EBP,2
    bool slot_valid(std::uint32_t slot_value,unsigned){this->flags.minus(sx8(slot_value)&0xffffu,0xffffu,2);return sx8(slot_value)!=logic::none;}   // CMP DX/AX,FFFF
    void saving_weapon(){this->push(0x1adcccu,this->gp[ebx]);}                                         // PUSH EBX
    std::uint32_t slot(std::uint32_t slot_value) {                                                       // MOVSX EDX,DX / ECX,AX
        const unsigned target=slot_register==edx?edx:ecx;
        return this->gp[target]=sx8(slot_value);
    }
    std::uint32_t indexed(std::uint32_t handle) {                                                       // MOV ECX,EBX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2]
        auto &gp=this->gp;
        gp[ecx]=this->flags.logic(handle&logic::handle_mask,4);const auto index=gp[ecx];gp[ecx]=3u*index;
        return index;
    }
    std::uint32_t indexed_secondary(std::uint32_t handle){return indexed(handle);}                      // MOV ECX,EAX; AND; LEA
    std::uint32_t tag_offset(std::uint32_t tag) {                                                       // AND EDX,FFFF; SHL EDX,4
        auto &fl=this->flags;
        static_assert((1u<<4)==logic::tag_size);
        return this->gp[edx]=fl.shift(fl.logic(tag&logic::handle_mask,4),4u,4);
    }
    std::uint32_t tag_offset_secondary(std::uint32_t tag){return tag_offset(tag);}
    bool word_compared(std::uint32_t word){this->flags.minus(word&0xffffu,logic::any,2);return (word&0xffffu)==logic::any;}   // CMP CX,FFFF
    bool unusable(std::uint32_t flags_value) {                                                           // SHR EDX,5; TEST DL,1
        auto &gp=this->gp;
        gp[edx]=this->flags.right(flags_value,logic::weapon::unusable_bit,4,false);this->flags.logic(gp[edx]&1u,1);
        return gp[edx]&1u;
    }
    std::uint32_t scaled(Kept &kept) {                                                                   // MOV EAX,EBX; CALL 106160
        this->gp[eax]=kept.weapon_handle;
        this->reached(Branch::armed);
        this->call(0x1add22u,0x1add27u,0x106160u,scale_child);
        refresh(kept);
        return low(this->s.xmm[0]);
    }
    void ranged_value(std::uint32_t value,std::uint32_t inverse) {                                      // DIVSS XMM1,XMM0; MULSS XMM2,...
        this->s.xmm[1]=_mm_move_ss(this->s.xmm[1],lane0(inverse));this->s.xmm[2]=_mm_move_ss(this->s.xmm[2],lane0(value));
    }
    bool widened(std::uint32_t widest,std::uint32_t first_range) {                                      // COMISS XMM0,[EDI+208]
        this->flags.scalar_compare(lane0(widest),lane0(first_range),false);
        return !(this->flags.cf||this->flags.zf);
    }
    void widest_value(std::uint32_t widest){this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0(widest));}  // MULSS XMM0,XMM1
    void kind_cleared(const Kept &kept){this->s.xmm[0]=_mm_setzero_ps();this->gp[eax]=kept.weapon_handle;}   // XORPS XMM0; MOV EAX,EBX
    bool melee(Kept &kept) {                                                                             // CALL 1062A0; TEST AL,AL
        auto &gp=this->gp;
        this->call(0x1addb1u,0x1addb6u,0x1062a0u,melee_child);
        refresh(kept);
        this->test8(gp[eax]);
        if(gp[eax]&0xffu)this->reached(Branch::melee);
        return gp[eax]&0xffu;
    }
    bool ranged(Kept &kept) {                                                                            // MOV EAX,EBX; CALL 106230; TEST AL,AL
        auto &gp=this->gp;
        gp[eax]=kept.weapon_handle;
        auto child=[&](State &v){dispatch(v,0x106230u,0x1addd5u);};
        this->call(0x1addd5u,0x1adddau,0x106230u,child);
        refresh(kept);
        this->test8(gp[eax]);
        if(gp[eax]&0xffu)this->reached(Branch::ranged);
        return gp[eax]&0xffu;
    }
    std::uint32_t kind_word(std::uint32_t word){return word&0xffffu;}
    bool kind_compared(std::uint32_t kind,std::uint32_t value){this->flags.minus(kind&0xffffu,value,2);return (kind&0xffffu)==value;}   // CMP word,value
    std::uint32_t scaled_secondary(Kept &kept) {                                                         // CALL 106160
        this->reached(Branch::secondary);
        this->call(0x1ade6au,0x1ade6fu,0x106160u,scale_child);
        refresh(kept);
        return low(this->s.xmm[0]);
    }
    void secondary_inverse(std::uint32_t inverse) {                                                      // DIVSS XMM1,XMM0; MOVAPS XMM2,XMM1
        auto &x=this->s.xmm;
        x[1]=_mm_move_ss(x[1],lane0(inverse));x[2]=x[1];
    }
    bool kept_larger(std::uint32_t current,std::uint32_t widened_value) {                               // COMISS XMM3/XMM0,XMM1; (MOVAPS XMM1,...)
        auto &x=this->s.xmm;
        x[1]=_mm_move_ss(x[1],lane0(widened_value));
        auto &current_register=x[larger++==0u?3:0];
        this->flags.scalar_compare(current_register,x[1],false);
        const bool keep=!(this->flags.cf||this->flags.zf);
        if(keep)x[1]=current_register;
        return keep;
    }
    bool kept_widest(std::uint32_t kept_value,std::uint32_t widened_value) {                            // MULSS XMM0,XMM2; COMISS XMM1,XMM0
        auto &x=this->s.xmm;
        x[0]=_mm_move_ss(x[0],lane0(widened_value));
        this->flags.scalar_compare(x[1],x[0],false);
        const bool keep=!(this->flags.cf||this->flags.zf);
        if(keep){this->reached(Branch::kept_widest);(void)kept_value;}
        return keep;
    }
    std::uint32_t final_value(std::uint32_t v){this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0(v));return v;}   // MULSS XMM0,XMM2
    std::uint32_t leftover()const{return low(this->s.xmm[0]);}
    void answered(){set_low_byte(this->gp[eax],1u);}                                                     // MOV AL,1
    void restoring(unsigned n) {
        auto &gp=this->gp;
        if(n>=3u)gp[ebx]=this->pop(0x1adf50u);                                                           // POP EBX
        if(n>=2u){gp[edi]=this->pop(0x1adf51u);gp[ebp]=this->pop(0x1adf52u);}                           // POP EDI; POP EBP
        gp[ecx]=this->pop(0x1adf53u);exit=0x1adf54u;                                                     // POP ECX
    }
    void restoring_final(unsigned n) {                                                                   // POP EBX; POP EDI; (MOVAPS XMM0,XMM1)
        auto &gp=this->gp;
        static constexpr std::uint32_t pops[3]{0x1adef6u,0x1adf18u,0x1adf32u};
        final_exit=n;
        gp[ebx]=this->pop(pops[n]);gp[edi]=this->pop(pops[n]+1u);
        if(n==0u)this->s.xmm[0]=this->s.xmm[1];
    }
    void answered_final(unsigned n) {                                                                    // MOV AL,1; POP EBP; POP ECX
        auto &gp=this->gp;
        static constexpr std::uint32_t pops[3]{0x1adf02u,0x1adf25u,0x1adf3fu};
        set_low_byte(gp[eax],1u);
        gp[ebp]=this->pop(pops[n]);gp[ecx]=this->pop(pops[n]+1u);exit=pops[n]+2u;
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::weapon_ranges::measure(memory,*this,gp[ecx],Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]});
        this->ret(exit,4u);this->publish(exit);
    }
};
template<class State,class Reference,class Scale,class Melee,class Dispatch> bool bridge(State &s,Reference reference,Scale scale,Melee melee,Dispatch dispatch) {
    return run_owner(s,0x1adc90u,0x1adf54u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Scale,Melee,Dispatch>(v,c,t,scale,melee,dispatch);});
}
} // namespace halo2::native_weapon_ranges_detail
