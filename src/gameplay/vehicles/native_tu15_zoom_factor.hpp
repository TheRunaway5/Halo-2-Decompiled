#pragma once
// Boundary adapter for zoom factor, CAD60
// (native_tu15_zoom_factor_systems.hpp). Its callers and the children keep the
// original ABI, which the adapter owns: EAX the unit handle and BX the zoom
// level, XMM0 the resulting scalar magnification (RET 0); ECX pushed for a
// 4-byte local slot below saved ESI and EDI; 106040 goes through the dispatch
// (EAX the weapon handle, AL the answer) and 106160 is a direct tail call
// (EAX the weapon handle, ECX EBX, after popping EDI, ESI and ADD ESP,4).
// XMM0 and XMM1 keep their live upper lanes across 106040 and CVTSI2SS/DIVSS.
// Each access publishes its original instruction.
// HALO2_REFERENCE_ZOOM_FACTOR=1 restores the translated body.
// HALO2_VERIFY_ZOOM_FACTOR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/vehicles/native_tu15_zoom_factor_systems.hpp"

namespace halo2::native_zoom_factor_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_zoom_factor;
using Access=logic::zoom_factor::Access;
using Exit=logic::zoom_factor::Exit;
using Kept=logic::zoom_factor::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ZOOM_FACTOR");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ZOOM_FACTOR");return v;}
enum Branch : unsigned {no_slot, no_weapon, weapon_zoom, binoculars_disabled, unzoomed, single_level, multi_level, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Zoom factor"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline __m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{
    0xcad61u,0xcad70u,0xcad76u,0xcad7cu,0xcad80u,0xcad94u,
    0xcadb9u,0xcadc5u,0xcadc9u,0xcadcbu,0xcadd9u,0xcadddu,0xcade5u,
    0xcadefu,0xcadf5u,0xcadfdu,0xcae18u,0xcae20u,0xcae2cu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::min_zoom_add)+1u);
template<class State,class Tail,class Dispatch> struct Adapter : Port<State> {
    Tail &tail_child;        // 106160 (direct tail call)
    Dispatch &dispatch;      // 106040
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Tail &k,Dispatch &d):Port<State>(s,c,t,{}),tail_child(k),dispatch(d){}
    std::uint32_t local_flag()const{return this->gp[esp]+logic::local_flag_offset;}
    void refresh(Kept &kept){auto &gp=this->gp;kept.zoom_reg=gp[ebx];kept.weapon=gp[esi];kept.objects=gp[edi];}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case objects_load:gp[edi]=v;break;
        case objects_data:gp[ecx]=v;gp[eax]=3u*gp[eax];break;
        case weapon_objects_data:case definitions_load:case globals_load:case camera_globals:gp[ecx]=v;break;
        case unit_load:case weapon_tag:case zoom_levels:gp[eax]=v;break;
        case weapon_slot:gp[ecx]=(gp[ecx]&0xffff0000u)|(sx8(v)&0xffffu);break;
        case weapon_handle:gp[esi]=v;break;
        case weapon_object:case weapon_definition:gp[edx]=v;break;
        case weapon_flags:set_low_byte(gp[eax],v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void reserve_slot(){this->push(0xcad60u,this->gp[ecx]);}                                            // PUSH ECX
    void set_default_weight(std::uint32_t bits){this->s.xmm[1]=scalar(bits);}                           // MOVSS XMM1,[46A0D0]
    void save_esi(){this->push(0xcad69u,this->gp[esi]);}                                                // PUSH ESI
    std::uint32_t unit_index(std::uint32_t handle) {                                                    // AND EAX,FFFF; PUSH EDI; LEA EAX,[EAX+EAX*2]
        const auto idx=this->flags.logic(handle&logic::handle_mask,4);
        this->gp[eax]=idx;
        return idx;
    }
    void save_edi() {                                                                                   // PUSH EDI; then LEA EAX,[EAX+EAX*2] happens after objects_data
        this->push(0xcad6fu,this->gp[edi]);
    }
    // Wait: LEA EAX,[EAX+EAX*2] runs at 0xcad79u (after objects_data at 0xcad76u); keep EAX exact on fault!
    std::uint32_t check_slot(std::uint32_t) {                                                           // CMP CX,-1
        const auto cx=this->gp[ecx]&0xffffu;
        this->flags.minus(cx,logic::no_slot,2);
        if(this->flags.zf)this->reached(Branch::no_slot);
        return cx;
    }
    void copy_default_result(){this->s.xmm[0]=this->s.xmm[1];}                                          // MOVAPS XMM0,XMM1
    std::uint32_t slot_index(std::uint32_t cx){return this->gp[edx]=sx16(cx);}                          // MOVSX EDX,CX
    bool has_weapon(std::uint32_t weapon) {                                                             // CMP ESI,-1
        this->compare32(weapon,logic::none);
        if(this->flags.zf){this->reached(Branch::no_weapon);return false;}
        return true;
    }
    bool weapon_owns_zoom(Kept &kept) {                                                                 // MOV EAX,ESI; CALL 106040; TEST AL,AL
        this->gp[eax]=kept.weapon;
        auto route=[&](State &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site);};
        this->call_routed(route,0xcada2u,0xcada7u,0x106040u);
        refresh(kept);
        this->test8(this->gp[eax]);
        if(!this->flags.zf){this->reached(Branch::weapon_zoom);return true;}
        return false;
    }
    std::uint32_t weapon_index(std::uint32_t weapon) {                                                  // AND ESI,FFFF; LEA EAX,[ESI+ESI*2]
        auto &gp=this->gp;
        gp[esi]=this->flags.logic(weapon&logic::handle_mask,4);
        gp[eax]=3u*gp[esi];
        return gp[esi];
    }
    std::uint32_t tag_index(std::uint32_t tag) {                                                        // AND EAX,FFFF; SHL EAX,4
        const auto idx=this->flags.logic(tag&logic::handle_mask,4);
        this->gp[eax]=this->flags.shift(idx,4u,4);
        return idx;
    }
    std::uint32_t mask_weapon_flag(std::uint32_t raw) {                                                 // AND AL,1
        const auto bit=this->flags.logic((raw&0xffu)&1u,1);
        set_low_byte(this->gp[eax],bit);
        return bit;
    }
    bool flag_stops_zoom(std::uint32_t bit) {                                                           // JNE CAE34
        if(bit!=0u){this->reached(Branch::binoculars_disabled);return true;}
        return false;
    }
    bool is_unzoomed(std::uint32_t zoom_reg) {                                                          // CMP BX,-1 (before globals_load)
        this->flags.minus(zoom_reg&0xffffu,logic::no_slot,2);
        if(this->flags.zf){this->reached(Branch::unzoomed);return true;}
        return false;
    }
    bool single_level(std::uint32_t levels) {                                                           // CMP EAX,1
        this->compare32(levels,1u);
        if(this->flags.zf){this->reached(Branch::single_level);return true;}
        this->reached(Branch::multi_level);
        return false;
    }
    void compute_fraction(std::uint32_t zoom_reg,std::uint32_t levels) {                                // MOVSX EDX,BX; DEC EAX; CVTSI2SS x2; DIVSS
        auto &gp=this->gp;
        gp[edx]=sx16(zoom_reg);
        gp[eax]=this->flags.increment(levels,4,true);
        this->s.xmm[1]=_mm_cvtsi32_ss(this->s.xmm[1],std::int32_t(gp[edx]));
        this->s.xmm[0]=_mm_cvtsi32_ss(this->s.xmm[0],std::int32_t(gp[eax]));
        this->s.xmm[1]=_mm_div_ss(this->s.xmm[1],this->s.xmm[0]);
    }
    void load_max(std::uint32_t bits){this->s.xmm[0]=scalar(bits);}                                     // MOVSS XMM0,[ECX+C0]
    void subtract_and_scale(std::uint32_t bits) {                                                       // SUBSS XMM0,[ECX+BC]; MULSS XMM0,XMM1
        this->s.xmm[0]=_mm_sub_ss(this->s.xmm[0],scalar(bits));
        this->s.xmm[0]=_mm_mul_ss(this->s.xmm[0],this->s.xmm[1]);
    }
    void add_min(std::uint32_t bits){this->s.xmm[0]=_mm_add_ss(this->s.xmm[0],scalar(bits));}           // ADDSS XMM0,[ECX+BC]
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        Kept kept{gp[ebx],gp[esi],gp[edi]};
        const auto exit=logic::zoom_factor::compute(memory,*this,gp[eax],kept);
        if(exit==Exit::tail_weapon) {                                                                   // POP EDI; MOV EAX,ESI; MOV ECX,EBX; POP ESI; ADD ESP,4; JMP 106160
            gp[edi]=this->pop(0xcadabu);
            gp[eax]=kept.weapon;
            gp[ecx]=kept.zoom_reg;
            gp[esi]=this->pop(0xcadb0u);
            gp[esp]=this->flags.plus(gp[esp],4u,4);
            if(this->cursor)*this->cursor=0xcadb4u;
            this->enter(0x106160u,tail_child);
            return;
        }
        gp[edi]=this->pop(0xcae34u);
        gp[esi]=this->pop(0xcae35u);
        gp[ecx]=this->pop(0xcae36u);
        this->ret(0xcae37u,0u);
        this->publish(0xcae37u);
    }
};
template<class State,class Reference,class Tail,class Dispatch>
bool bridge(State &s,Reference reference,Tail tail_child,Dispatch dispatch) {
    return run_owner(s,0xcad60u,0xcae37u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Tail,Dispatch>(v,c,t,tail_child,dispatch);});
}
} // namespace halo2::native_zoom_factor_detail
