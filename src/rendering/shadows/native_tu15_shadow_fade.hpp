#pragma once
// Boundary adapter for the object shadow fade, entered at 3D7E0
// (native_tu15_shadow_fade_systems.hpp). Its callers and children keep the
// original ABI, which the adapter owns:
//   - [ESP+4] the object, [ESP+8] the fade output, [ESP+C] the selector
//     output; RET C; SUB ESP,1C, then EBX/EBP/ESI/EDI saved between the
//     first loads and restored before the unit fade; locals and arguments
//     are addressed from the live ESP, so a child that leaves the stack
//     unbalanced moves them as it does for the original;
//   - the children: 30C40 (ECX the object, EAX the position output, one
//     pushed argument), 4BAE0 (XMM1 the scaled distance, two pushed
//     pointers), BCFB0 (ECX the object) and 3D7E0 for the parent; each gets
//     the original's registers, flags and XMM, and every register it
//     returns comes back to the fade;
//   - XMM: scalar results keep the upper lanes a child left (CVTSI2SS,
//     MULSS); MOVSS loads, MOVAPS copies and XORPS replace them;
//   - the x87 distance pushes at most five values: admission requires
//     that many free registers (a stack fault is the translated route's);
//     the logical status keeps C1 of the final store;
//   - the exit: AL = shown with EAX's upper bytes from the selector load,
//     ECX the selector output, ADD ESP,1C decides the flags;
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_SHADOW_FADE=1 restores the translated body.
// HALO2_VERIFY_SHADOW_FADE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "rendering/shadows/native_tu15_shadow_fade_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_shadow_fade_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_shadow_fade;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_SHADOW_FADE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_SHADOW_FADE");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    unmeasured, measured_flags, other_mode, center_mode, position_mode, below_threshold, above_threshold,
    selector_set, selector_clear, timed, untimed, age_negative, age_above_one, age_within,
    camouflage_floor, camouflage_not_floor, camouflage_ceiling, camouflage_within, camouflage_equal,
    camouflage_unequal, shown_no_parent_check, no_parent, has_parent, unit, not_unit, branch_count
};
static_assert(branch_count<=64u);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Shadow fade verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline __m128 with_lane0(__m128 x,std::uint32_t bits){return _mm_move_ss(x,lane0_only(bits));}
inline constexpr bool backend=logic::sse::available && logic::scalar::available;

inline constexpr Edge fade_edges[]{
    {0x3d832u,0x3d9c1u,unmeasured},{0x3d832u,0x3d83cu,measured_flags},
    {0x3d83cu,0x3d9c1u,other_mode},{0x3d83cu,0x3d852u,center_mode},{0x3d83cu,0x3d8dcu,position_mode},
    {0x3d83cu,0x3d8e3u,position_mode},
    {0x3d955u,0x3d9c1u,below_threshold},{0x3d955u,0x3d95du,above_threshold},
    {0x3d95du,0x3da67u,selector_set},{0x3d95du,0x3d96bu,selector_clear},
    {0x3d971u,0x3d980u,timed},{0x3d971u,0x3d9ebu,untimed},
    {0x3d9acu,0x3d9b7u,age_negative},{0x3d9acu,0x3d9d5u,age_above_one},{0x3d9acu,0x3d9ebu,age_within},
    {0x3d9f2u,0x3da13u,camouflage_floor},{0x3d9f2u,0x3d9fbu,camouflage_not_floor},
    {0x3d9fbu,0x3da13u,camouflage_ceiling},{0x3d9fbu,0x3da0bu,camouflage_within},
    {0x3da13u,0x3da23u,camouflage_equal},{0x3da13u,0x3da3cu,camouflage_unequal},
    {0x3da13u,0x3da6fu,shown_no_parent_check},{0x3da32u,0x3da6fu,shown_no_parent_check},
    {0x3da3cu,0x3da6fu,no_parent},{0x3da3cu,0x3da42u,has_parent},
    {0x3da85u,0x3da8bu,unit},{0x3da85u,0x3dac5u,not_unit},
};
template<class State,class Position,class Level,class Parent,class Recurse> struct Fade : Port<State> {
    using Access=logic::shadow_fade::Access;
    using Local=logic::shadow_fade::Local;
    using Kept=logic::shadow_fade::Kept;
    using Clamp=logic::shadow_fade::Clamp;
    Position &position_child;Level &level_child;Parent &parent_child;Recurse &recurse;
    // E: where the return address sits for the original's addressing; the
    // walk's ESP is E-2C between the saves and the restores.
    std::uint32_t frame{};
    Fade(State &s,std::uint32_t *c,Trace<State> *t,Position &p,Level &l,Parent &a,Recurse &r)
        :Port<State>(s,c,t,fade_edges),position_child(p),level_child(l),parent_child(a),recurse(r){}
    std::uint32_t local(Local l)const {
        switch(l) {
        case Local::object:return frame+0x4u;
        case Local::fade_output:return frame+0x8u;
        case Local::selector_output:return frame+0xcu;
        case Local::fade:return frame-0x18u;
        case Local::selector:return frame-0x14u;
        case Local::cleared:return frame-0x10u;
        case Local::position:return frame-0xcu;
        case Local::level_byte:return frame-0x19u;
        }
        return 0;
    }
    void set_dl(bool v){this->gp[edx]=(this->gp[edx]&0xffffff00u)|(v?1u:0u);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case tag_instances:return 0x3d7e3u;
        // XORPS XMM0,XMM0; PUSH EBX.
        case object_argument:this->s.xmm[0]=_mm_setzero_ps();this->push(0x3d7ecu,gp[ebx]);return 0x3d7edu;
        case headers:this->push(0x3d7f1u,gp[ebp]);return 0x3d7f2u;
        // PUSH ESI; MOV EAX,EBX; AND EAX,FFFF; PUSH EDI; LEA EDI,[EAX+EAX*2].
        case header_rows:
            this->push(0x3d7f8u,gp[esi]);gp[eax]=this->flags.logic(gp[ebx]&0xffffu,4);
            this->push(0x3d800u,gp[edi]);gp[edi]=gp[eax]*3u;
            return 0x3d804u;
        case object_data:gp[edi]=this->flags.shift(gp[edi],2u,4);return 0x3d80au;              // SHL EDI,2
        case definition_handle:return 0x3d80eu;
        // LEA EAX,[EDI+EAX+8]; AND ECX,FFFF; SHL ECX,4.
        case definition:
            gp[eax]=gp[edi]+gp[eax]+logic::header_data;
            gp[ecx]=this->flags.shift(this->flags.logic(gp[ecx]&0xffffu,4),4u,4);
            return 0x3d81du;
        case selector:return 0x3d821u;
        case fade_clear:return 0x3d828u;
        case object_flags:return 0x3d82eu;
        case selector_store:return 0x3d832u;
        case distance_mode:return 0x3d83cu;
        case center_object:return 0x3d852u;
        case center_x:return 0x3d857u;
        case center_y:return 0x3d859u;
        case center_z:return 0x3d85cu;
        case position_x_store:return 0x3d85fu;
        case position_x:return 0x3d863u;
        case camera_x:return 0x3d867u;
        case position_y_store:return 0x3d86du;
        case position_y:return 0x3d871u;
        case position_z_store:return 0x3d875u;
        case camera_y:return 0x3d879u;
        case position_z:gp[ecx]=local(Local::object);return 0x3d883u;                         // LEA ECX
        case camera_z:this->push(0x3d887u,gp[ecx]);return 0x3d888u;
        case distance_scale:gp[edx]=local(Local::level_byte);this->push(0x3d892u,gp[edx]);return 0x3d8abu;
        case distance_store:return 0x3d8b1u;
        case distance_load:return 0x3d8b5u;
        case level:return 0x3d8c0u;
        case position_x_child:return 0x3d8e8u;
        case camera_x_child:return 0x3d8ecu;
        case position_y_child:gp[edx]=local(Local::object);return 0x3d8f6u;                   // LEA EDX
        case camera_y_child:this->push(0x3d8fau,gp[edx]);return 0x3d8fbu;
        case position_z_child:gp[eax]=local(Local::level_byte);return 0x3d905u;               // LEA EAX
        case camera_z_child:this->push(0x3d909u,gp[eax]);return 0x3d90au;
        case distance_scale_child:return 0x3d928u;
        case distance_store_child:return 0x3d92eu;
        case distance_load_child:return 0x3d932u;
        case level_child:return 0x3d93du;
        case level_scale:return 0x3d946u;
        case fade_threshold:return 0x3d94eu;
        case fade_store:return 0x3d955u;
        case selector_check:return 0x3d95du;
        case fade_flags:return 0x3d96bu;
        case one:return 0x3d971u;
        case game_time:return 0x3d980u;
        case now:return 0x3d985u;
        case created:return 0x3d988u;
        case seconds_per_tick:return 0x3d992u;
        case age_delay:return 0x3d997u;
        case age_store:return 0x3d9acu;
        case age_floor_store:return 0x3d9b7u;
        case age_one_store:return 0x3d9d5u;
        case one_unmeasured:return 0x3d9c1u;
        case rows_again:return 0x3d9ebu;
        case object_again:return 0x3d9eeu;
        case camouflage_floor:return 0x3d9f2u;
        case camouflage_ceiling:return 0x3d9fbu;
        case camouflage_value:return 0x3da0bu;
        case camouflage_equal:return 0x3da13u;
        case camouflage_fraction:return 0x3da23u;
        case camouflage_store:return 0x3da32u;
        case parent:return 0x3da3cu;
        case parent_selector_output:return 0x3da42u;
        case parent_fade:return 0x3da59u;
        case headers_again:return 0x3da5fu;
        case one_after:return 0x3da67u;
        case rows_final:return 0x3da6fu;
        case type:return 0x3da72u;
        case object_clear:this->s.xmm[3]=_mm_setzero_ps();return 0x3da8bu;                    // XORPS XMM3,XMM3
        case cleared_store:return 0x3da90u;
        case unit_object:return 0x3da96u;
        case unit_camouflage:return 0x3da99u;
        case selector_final:return 0x3dac5u;
        case fade_output:return 0x3dac9u;
        case fade_result_store:return 0x3dad0u;
        case selector_output:return 0x3dad4u;
        case selector_result_store:return 0x3dadbu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case tag_instances:case center_y:gp[edx]=v;break;
        case object_argument:gp[ebx]=v;break;
        case headers:case headers_again:gp[ebp]=v;if(a==headers_again)set_dl(gp[eax]&0xffu);break;   // MOV DL,AL
        case header_rows:case center_z:case game_time:case rows_again:case rows_final:case unit_object:
            gp[eax]=v;break;
        case object_data:gp[esi]=v;break;
        case definition_handle:case definition:case center_x:case now:case object_again:case fade_output:
        case parent_selector_output:
            gp[ecx]=v;
            if(a==fade_output)this->compare32(gp[eax],logic::selector_twelve);                  // CMP EAX,C
            break;
        case selector:gp[edx]=logic::shadow_fade::widen8(v);break;
        case object_flags:this->test8(v&1u);break;
        // MOVSX ECX,[ECX+18]; SUB ECX,0; then mode 0: TEST EDX,EDX (and CMP
        // EDX,1 when not zero); otherwise SUB ECX,2.
        case distance_mode:
            gp[ecx]=this->flags.minus(logic::shadow_fade::widen16(v),0,4);
            if(gp[ecx]==0){this->test32(gp[edx]);if(gp[edx])this->compare32(gp[edx],1);}
            else gp[ecx]=this->flags.minus(gp[ecx],2,4);
            break;
        case center_object:gp[eax]=this->flags.plus(v,logic::object::center,4);break;            // ADD EAX,30
        case distance_load:case distance_load_child:case one:case one_unmeasured:case one_after:
            x[1]=lane0_only(v);
            // MOV AL,[ESI+10A] was before: SHR AL,2; TEST DL,AL.
            if(a==one) {
                const auto shifted=this->flags.right(gp[eax]&0xffu,2u,1,false);
                gp[eax]=(gp[eax]&0xffffff00u)|(shifted&0xffu);this->test8(gp[edx]&shifted);
            }
            break;
        case level:gp[eax]=v;break;                                                              // MOVZX EAX
        case level_child:gp[ecx]=v;break;                                                        // MOVZX ECX
        case fade_threshold:this->flags.scalar_compare(x[0],lane0_only(v),false);break;
        case selector_check:case selector_final:gp[eax]=v;if(a==selector_check)this->test32(v);break;
        case fade_flags:gp[eax]=(gp[eax]&0xffffff00u)|(v&0xffu);break;                           // MOV AL
        case created:gp[ecx]=this->flags.minus(gp[ecx],v,4);break;                              // SUB ECX,[ESI+BC]
        // UCOMISS XMM2,[ECX+2B0]; LAHF; TEST AH,44.
        case camouflage_equal: {
            this->flags.scalar_compare(x[2],lane0_only(v),true);
            const auto ah=(this->flags.bits()|2u)&0xffu;
            gp[eax]=(gp[eax]&0xffff00ffu)|(ah<<8);this->test8(ah&0x44u);
            break;
        }
        case parent:this->compare32(v,logic::none);break;
        case parent_fade:x[0]=lane0_only(v);break;
        // MOV CL,[EAX+EDI+3]; ADD EAX,EDI; MOV EBX,1; POP EDI; SHL EBX,CL;
        // POP ESI; POP EBP; TEST BL,3; POP EBX.
        case type: {
            gp[ecx]=(gp[ecx]&0xffffff00u)|(v&0xffu);gp[eax]=this->flags.plus(gp[eax],gp[edi],4);
            gp[edi]=this->pop(0x3da7du);
            const auto bit=this->flags.shift(1u,v&0xffu,4);
            gp[esi]=this->pop(0x3da80u);gp[ebp]=this->pop(0x3da81u);
            this->test8(bit&logic::object_type::units);
            gp[ebx]=this->pop(0x3da85u);
            break;
        }
        // MOV ECX,[selector output]; SETZ AL.
        case selector_output:gp[ecx]=v;gp[eax]=(gp[eax]&0xffffff00u)|(this->flags.zf?1u:0u);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void returned(Kept &kept,std::uint32_t saved_below) {
        frame=this->gp[esp]+saved_below;
        kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};
    }
    // LEA ECX (the object slot); PUSH ECX; LEA EAX (the position); MOV
    // ECX,EBX; CALL 30C40.
    void position(Kept &kept) {
        auto &gp=this->gp;
        gp[ecx]=local(Local::object);this->push(0x3d8dcu,gp[ecx]);
        gp[eax]=local(Local::position);gp[ecx]=gp[ebx];
        this->call(0x3d8e3u,0x3d8e8u,0x30c40u,position_child);returned(kept,0x2cu);
    }
    // The logical x87: the pushes and pops balance; C1 is the final store's.
    void rounded(const logic::scalar::Rounded &r) {
        auto &fp=this->s.fp;
        fp.status=std::uint16_t((fp.status&~0x200u)|(r.store_status&0x200u));
    }
    void leveled(Kept &kept,bool center) {
        if(center)this->call(0x3d8bbu,0x3d8c0u,0x4bae0u,level_child);
        else this->call(0x3d938u,0x3d93du,0x4bae0u,level_child);
        returned(kept,0x2cu);
    }
    // CVTSI2SS XMM0 and MULSS keep XMM0's upper lanes.
    void scaled(std::uint32_t fade){this->s.xmm[0]=with_lane0(this->s.xmm[0],fade);}
    void shown(bool v){set_dl(v);}
    // XMM2 ends zero (XORPS); when timed XMM0 is the fade (MOVAPS from
    // XMM1, a MOVSS load, then scalar ops).
    void aged(bool timed,std::uint32_t fade) {
        auto &x=this->s.xmm;
        x[2]=_mm_setzero_ps();
        if(timed)x[0]=lane0_only(fade);
    }
    void camouflage_clamped(Clamp c,std::uint32_t clamped) {
        auto &x=this->s.xmm;
        if(c==Clamp::ceiling)x[2]=x[1];
        else if(c==Clamp::value)x[2]=lane0_only(clamped);
    }
    // MOVAPS XMM2,XMM1; SUBSS; MULSS XMM2,XMM0; MOVAPS XMM0,XMM2.
    void camouflage_faded(std::uint32_t fade) {
        auto &x=this->s.xmm;
        x[2]=lane0_only(fade);x[0]=x[2];
    }
    // PUSH ECX (selector output); LEA EDX (the fade); PUSH EDX; MOV ECX,EBX;
    // CALL BCFB0; PUSH EAX; CALL 3D7E0.
    std::uint32_t parent_shown(Kept &kept) {
        auto &gp=this->gp;
        this->push(0x3da46u,gp[ecx]);gp[edx]=local(Local::fade);this->push(0x3da4bu,gp[edx]);gp[ecx]=gp[ebx];
        this->call(0x3da4eu,0x3da53u,0xbcfb0u,parent_child);
        this->push(0x3da53u,gp[eax]);
        this->call(0x3da54u,0x3da59u,0x3d7e0u,recurse);returned(kept,0x2cu);
        return gp[eax];
    }
    // XMM2 the clamp (XMM3 zero, XMM1 one, or the camouflage), XMM1 and
    // XMM0 the fade; XOR DL,DL when hidden.
    void unit_faded(Clamp c,std::uint32_t clamped,std::uint32_t fade,bool shown_now) {
        auto &x=this->s.xmm;
        if(c==Clamp::floor)x[2]=x[3];
        else if(c==Clamp::ceiling)x[2]=x[1];
        else x[2]=lane0_only(clamped);
        x[1]=lane0_only(fade);x[0]=x[1];
        if(!shown_now)set_dl(false);
    }
    void run() {
        auto &gp=this->gp;
        frame=gp[esp];
        gp[esp]=this->flags.minus(gp[esp],0x1cu,4);                                          // SUB ESP,1C
        Memory<Fade> memory{*this};
        logic::shadow_fade::evaluate(memory,*this);
        // MOV AL,DL; ADD ESP,1C; RET C.
        gp[eax]=(gp[eax]&0xffffff00u)|(gp[edx]&0xffu);
        gp[esp]=this->flags.plus(gp[esp],0x1cu,4);
        this->ret(0x3dae2u,0xcu);this->publish(0x3dae2u);
    }
};

// Admission also requires masked SSE exceptions, no active MMX and five
// free x87 registers.
template<class State,class Reference,class Position,class Level,class Parent,class Recurse>
bool bridge(State &s,Reference reference,Position position,Level level,Parent parent,Recurse recurse) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.fp.count>3u)return false;
    if((_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x3d7e0u,0x3dae2u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return Fade<State,Position,Level,Parent,Recurse>(v,c,t,position,level,parent,recurse);});
    return true;
}
} // namespace halo2::native_shadow_fade_detail
