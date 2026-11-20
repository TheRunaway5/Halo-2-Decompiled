#pragma once
// Boundary adapter for the atmospheric fog mixers
// (native_tu15_fog_mixers_systems.hpp), entered at 131610 in the campaign
// environment unit. Its caller (131AC0) and its child (the interpolator
// evaluation 137AC0, entered as the unit's direct body) are still translated
// TU1.5 code, so this adapter owns that ABI and nothing else:
//   - the arguments: EDX the list's length, [EBP+8] the list, [EBP+C] the
//     output array; the result: EAX the slots written; RET 8;
//   - the stack frame: PUSH EBP; MOV EBP,ESP; AND ESP,-8; SUB ESP,1BC, then
//     EBX, ESI and EDI saved below it. Frame::local gives a local's address
//     from the stack pointer the last child left; the arguments are EBP's;
//   - the original's register file at the interpolator call: the index and
//     the interpolator's value pushed, the scenario in EDI and the entry in
//     EBX; the values each path leaves in the scratch registers (the clamped
//     indices, the cursors of the dword loops, the string instructions'
//     ESI/EDI/ECX) and in XMM0-XMM5 (the weight, the unit weight, zero, the
//     remaining share, the blend scale and each loop's last dword); the
//     flags of the instruction that decides the call (TEST ECX,ECX) and of
//     each exit (TEST EDI / TEST EDX, or the list count's DEC);
//   - the direction flag: REP STOSD/MOVSD step downward when it is set;
//   - the exits: POP EDI/ESI/EBX, MOV ESP,EBP, POP EBP and RET 8;
//   - the diagnostic cursor: each access publishes its original instruction
//     (the role -> instruction table below), and the RET publishes the exit;
//   - every store goes through the unit's store helper.
// Delete the adapter when the caller and the child go native.
// HALO2_REFERENCE_FOG_MIXERS=1 restores the translated body.
// HALO2_VERIFY_FOG_MIXERS=1 records the native run (the child genuine, once)
// and replays the unmodified translated body against it, as in
// native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_fog_mixers_systems.hpp"

namespace halo2::native_fog_mixers_detail {
using native_object_tick_detail::Trace;
using native_object_tick_detail::sx8;using native_object_tick_detail::sx16;using native_object_tick_detail::si;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_atmospheric_fog;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_FOG_MIXERS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_FOG_MIXERS");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    no_scenario, no_clusters, no_fog, negative_fog, fog_beyond, null_entry, no_mixers,
    source_negative, source_beyond, interpolator_negative, interpolator_beyond, null_state, interpolated,
    clamp_zero, clamp_one, clamp_kept, another_mixer, no_weight, blend_none, weight_skipped, source_missing,
    field_skipped, blend_dword, mix_skipped, mix_dword, another_cluster, branch_count
};
static_assert(branch_count<=64u);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Fog mixers verify: %llu calls; %llu ordered accesses (%llu stores) and %llu child "
            "boundaries identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)children.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 load_ss(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
// MOVSS into a register's low lane (arithmetic results keep the upper lanes).
inline __m128 low_lane(__m128 reg,std::uint32_t bits){return _mm_move_ss(reg,load_ss(bits));}

// ---- 131610 -------------------------------------------------------------------
inline constexpr Edge mixer_edges[]{
    {0x1318f0u,0x131810u,another_mixer},{0x131a96u,0x131770u,another_cluster},
};
// The field table's stores after the first two, by dword of the table.
inline constexpr std::uint32_t field_store_pcs[36]{
    0x131647u,0x13164bu,0x13164fu,0x131657u,0u,0x13165bu,0x13165fu,0x131667u,0x13166bu,0x131673u,0x131677u,0x13167fu,
    0x131683u,0x13168bu,0x13168fu,0x131697u,0x13169bu,0x1316a6u,0x1316adu,0x1316b8u,0x1316bfu,0x1316cau,0x1316d1u,
    0x1316dcu,0x1316e3u,0x1316eeu,0x1316f5u,0u,0x131700u,0x13170bu,0x131712u,0x13171du,0x131724u,0x13172fu,0x131736u,
    0x131741u,
};
template<class State,class Interpolate> struct FogMixers : Port<State> {
    using Access=logic::fog_mixers::Access;
    using Local=logic::fog_mixers::Local;
    using Argument=logic::fog_mixers::Argument;
    using Clamp=logic::fog_mixers::Clamp;
    Interpolate &interpolate_child;
    std::uint32_t locals{};      // the Locals as the original addresses them (ESP after the saves)
    bool blend_fields{},mix_fields{};
    FogMixers(State &s,std::uint32_t *c,Trace<State> *t,Interpolate &i):Port<State>(s,c,t,mixer_edges),interpolate_child(i){}
    std::uint32_t blended_base()const{return locals+std::uint32_t(offsetof(logic::Locals,blended));}
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0xfffffffcu:4u;}
    // The end of a field of the blend (INC ESI; CMP ESI,12) or the output
    // (INC EDI; CMP EDI,12); the first field is XOR ESI/EDI.
    void next_field(unsigned reg,bool &started) {
        auto &gp=this->gp;
        if(!started){started=true;gp[reg]=this->flags.logic(0,4);return;}
        gp[reg]=this->flags.increment(gp[reg],4,false);this->compare32(gp[reg],logic::mixed_field_count);
    }
    std::uint32_t instruction(Access a,std::uint32_t address) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        // Prologue runs at the start of run() before local() is computed.
        case field_offset_early:return 0x13162eu;
        case field_size_early:return 0x131632u;
        case scenario:return 0x131639u;
        case slots_clear:return 0x131643u;
        case field_store:return field_store_pcs[(address-(locals+std::uint32_t(offsetof(logic::Locals,fields))))/4u];
        case output_argument:this->test32(gp[edx]);return 0x131756u;                       // TEST EDX,EDX
        case list_argument:return 0x131759u;
        case output_store:return 0x13175cu;
        case list_store:return 0x131760u;
        case clusters_left_store:return 0x131764u;
        case list_entry:return 0x131770u;
        case entry_cluster:return 0x131774u;
        case structure_bsp:return 0x131776u;
        case clusters:return 0x131782u;
        case fog_index:return 0x131788u;
        case fog_count:return 0x1317a0u;
        case fog_palette:return 0x1317acu;
        case fog_entry_store:return 0x1317bcu;
        case mixer_count:return 0x1317c6u;
        case unit_weight:return 0x1317d4u;
        // XORPS XMM3; XOR EDX,EDX; CMP EAX,ECX.
        case weight_0_clear:
            x[3]=_mm_setzero_ps();gp[edx]=this->flags.logic(0,4);this->compare32(gp[eax],gp[ecx]);
            return 0x1317e3u;
        case weight_sum_clear:x[4]=x[2];return 0x1317eau;                                 // MOVAPS XMM4,XMM2
        case remaining_clear:return 0x1317f0u;
        case weight_1_clear:return 0x1317f6u;
        case mixer_index_clear:return 0x1317fau;
        case mixer_offset_clear:return 0x131804u;
        case mixers:return 0x131810u;
        case mixer_offset:return 0x131816u;
        case mixer_source:return 0x13181au;
        case palette_count:return 0x13182au;
        case mixer_interpolator:return 0x131845u;
        case interpolator_count:return 0x131852u;
        case interpolators:return 0x131869u;
        case interpolator_value:return 0x13187fu;
        case remaining_reload:return 0x131888u;
        case unit_weight_reload:return 0x13188eu;
        case mixer_index_reload:return 0x1318abu;
        case weight_sum:x[1]=x[0];return 0x1318b2u;                                        // MOVAPS XMM1,XMM0
        case weight_sum_store:return 0x1318b8u;
        case weight_store:return 0x1318c9u;
        case remaining_store:return 0x1318cfu;
        case mixer_index_next:return 0x1318d8u;
        case mixer_offset_next:return 0x1318dcu;
        case mixer_count_next:return 0x1318e0u;
        case mixer_index_store:return 0x1318ecu;
        case mixer_offset_store:return 0x1318f0u;
        case weight_total:return 0x1318fau;
        // XOR EAX,EAX; MOV ECX,3D; LEA EDI,[ESP+D0] before the first.
        case blended_clear:
            if(address==blended_base()){gp[eax]=this->flags.logic(0,4);gp[ecx]=logic::entry_dwords;gp[edi]=blended_base();}
            return 0x131917u;
        case blend_mixer_count:return 0x131919u;
        case blend_index_clear:return 0x131928u;
        case blend_count_store:return 0x13192cu;
        case blend_offset_clear:return 0x131936u;
        case blend_weight:return 0x131940u;
        case blend_mixers:return 0x13194fu;
        case blend_offset:return 0x131955u;
        case blend_source:return 0x131959u;
        case blend_scenario:return 0x131964u;
        case blend_palette_count:return 0x13196au;
        case blend_palette:return 0x131972u;
        case blend_field_size:next_field(esi,blend_fields);return 0x131984u;
        case blend_field_offset:return 0x13198cu;
        case blend_source_field:return 0x1319b0u;
        case blend_accumulator:return 0x1319bdu;
        case blend_accumulator_store:return 0x1319c1u;
        case blend_entry_reload:return 0x1319cbu;
        case blend_count_reload:next_field(esi,blend_fields);return 0x1319d5u;
        case blend_index_reload:return 0x1319d9u;
        case blend_offset_reload:return 0x1319ddu;
        case blend_index_store:return 0x1319e7u;
        case blend_offset_store:return 0x1319ebu;
        case output_count:return 0x1319f5u;
        case output_entry:return 0x1319f9u;
        case output_slot:return 0x1319fdu;
        case link_store:return 0x131a01u;
        // MOV ECX,3D; MOV ESI,EBX before the first.
        case copy:
            if(address==gp[ebx] && !copying){copying=true;gp[ecx]=logic::entry_dwords;gp[esi]=gp[ebx];}
            return 0x131a0bu;
        case mix_field_size:
            if(copying){copying=false;mix_fields=false;}
            next_field(edi,mix_fields);
            return 0x131a10u;
        case mix_field_offset:return 0x131a18u;
        case mix_output:return 0x131a1cu;
        case mix_entry:return 0x131a2bu;
        case mix_original:return 0x131a40u;
        case mix_blended:return 0x131a44u;
        case mix_store:return 0x131a55u;
        case mix_entry_reload:return 0x131a60u;
        case output_count_reload:next_field(edi,mix_fields);return 0x131a6au;
        case output_slot_reload:return 0x131a6eu;
        case scenario_reload:return 0x131a72u;
        case output_count_store:return 0x131a7eu;
        case output_slot_store:return 0x131a82u;
        case next_entry:return 0x131a86u;
        case clusters_left:return 0x131a8au;
        case next_entry_store:return 0x131a92u;
        case clusters_left_next:return 0x131a96u;
        case result:return 0x131aa0u;
        }
        return 0;
    }
    bool copying{};
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        // XOR ESI,ESI; TEST EDI,EDI.
        case scenario:gp[edi]=v;gp[esi]=this->flags.logic(0,4);this->test32(v);if(!v)this->reached(no_scenario);break;
        case output_argument:case entry_cluster:case mixers:case mixer_index_reload:case blend_index_reload:
        case output_slot_reload:case result:
            gp[eax]=v;
            break;
        case list_argument:case blend_mixers:case blend_scenario:case blend_field_size:case blend_count_reload:
        case output_count:case mix_field_size:
            gp[ecx]=v;
            break;
        case list_entry:case interpolator_value:case blend_offset:gp[edx]=v;break;
        case mixer_offset:case mix_output:gp[esi]=v;break;
        case output_slot:case scenario_reload:gp[edi]=v;break;
        case blend_entry_reload:case mix_entry_reload:gp[ebx]=v;break;
        case structure_bsp:gp[ecx]=v;gp[eax]=this->flags.product(gp[eax],std::uint32_t(sizeof(logic::Cluster)));break;
        case clusters:gp[eax]=this->flags.plus(gp[eax],v,4);break;
        // MOV AL; CMP AL,FF; MOVSX EAX,AL; XOR ECX,ECX; CMP EAX,ECX.
        case fog_index:
            set_low_byte(gp[eax],v);this->flags.minus(v,logic::no_fog,1);
            if(v==logic::no_fog){this->reached(no_fog);break;}
            gp[eax]=sx8(v);gp[ecx]=this->flags.logic(0,4);this->compare32(gp[eax],gp[ecx]);
            if(si(gp[eax])<0)this->reached(negative_fog);
            break;
        case fog_count:this->compare32(gp[eax],v);if(si(gp[eax])>=si(v))this->reached(fog_beyond);break;
        // MOV EDX; IMUL EAX,EAX,F4; ADD EAX,EDX; MOV EBX,EAX.
        case fog_palette:
            gp[edx]=v;gp[eax]=this->flags.plus(this->flags.product(gp[eax],std::uint32_t(sizeof(logic::FogEntry))),v,4);
            gp[ebx]=gp[eax];
            if(!gp[ebx])this->reached(null_entry);
            break;
        case mixer_count:gp[eax]=v;this->compare32(v,gp[ecx]);if(si(v)<=0)this->reached(no_mixers);break;
        case unit_weight:case unit_weight_reload:
            x[2]=load_ss(v);
            if(a==unit_weight_reload)x[3]=_mm_setzero_ps();                              // XORPS XMM3
            break;
        // MOV CX; ADD EAX,ESI; TEST CX,CX; a negative index is XOR EDX,EDX,
        // then MOVSX ECX,CX; CMP EDX,ECX.
        case mixer_source:
            gp[ecx]=(gp[ecx]&0xffff0000u)|v;gp[eax]=this->flags.plus(gp[eax],gp[esi],4);this->flags.logic(v,2);
            if(v&0x8000u) {
                gp[edx]=this->flags.logic(0,4);gp[ecx]=sx16(v);this->compare32(gp[edx],gp[ecx]);
                this->reached(source_negative);
            }
            break;
        // MOV ESI; MOVSX EDX,CX; DEC ESI; CMP EDX,ESI; (MOV EDX,ESI); MOVSX
        // ECX,CX; CMP EDX,ECX.
        case palette_count:
            gp[esi]=v;gp[edx]=sx16(gp[ecx]);gp[esi]=this->flags.increment(gp[esi],4,true);this->compare32(gp[edx],gp[esi]);
            if(si(gp[edx])>si(gp[esi]))gp[edx]=gp[esi];
            gp[ecx]=sx16(gp[ecx]);this->compare32(gp[edx],gp[ecx]);
            if(gp[edx]!=gp[ecx])this->reached(source_beyond);
            break;
        case mixer_interpolator:
            gp[eax]=(gp[eax]&0xffff0000u)|v;this->flags.logic(v,2);
            if(v&0x8000u) {
                gp[ecx]=this->flags.logic(0,4);gp[eax]=sx16(v);this->compare32(gp[ecx],gp[eax]);
                this->reached(interpolator_negative);
            }
            break;
        case interpolator_count:
            gp[edx]=v;gp[ecx]=sx16(gp[eax]);gp[edx]=this->flags.increment(gp[edx],4,true);this->compare32(gp[ecx],gp[edx]);
            if(si(gp[ecx])>si(gp[edx]))gp[ecx]=gp[edx];
            gp[eax]=sx16(gp[eax]);this->compare32(gp[ecx],gp[eax]);
            if(gp[ecx]!=gp[eax])this->reached(interpolator_beyond);
            break;
        // MOV ECX; MOV EDX,EAX; SHL EDX,5; LEA ECX,[EDX+ECX+4]; TEST ECX,ECX;
        // MOVAPS XMM0,XMM3.
        case interpolators:
            gp[edx]=this->flags.shift(gp[eax],5,4);gp[ecx]=gp[edx]+v+4u;this->test32(gp[ecx]);x[0]=x[3];
            if(!gp[ecx])this->reached(null_state);
            break;
        case remaining_reload:x[4]=load_ss(v);break;
        // MOV EAX; MOV EDX; MOV ECX; INC EAX; ADD EDX,10; CMP EAX,ECX.
        case mixer_index_next:gp[eax]=v;break;
        case mixer_offset_next:gp[edx]=v;break;
        case mixer_count_next:
            gp[ecx]=v;gp[eax]=this->flags.increment(gp[eax],4,false);
            gp[edx]=this->flags.plus(gp[edx],std::uint32_t(sizeof(logic::FogMixer)),4);this->compare32(gp[eax],v);
            break;
        // COMISS XMM0,XMM3 (the sum against zero).
        case weight_total:
            x[0]=load_ss(v);this->flags.scalar_compare(x[0],x[3],false);
            if(this->flags.cf||this->flags.zf)this->reached(no_weight);
            break;
        // CMP ECX,EAX (zero); MOVAPS XMM1,XMM2 (DIVSS: blend_scale).
        case blend_mixer_count:
            gp[ecx]=v;this->compare32(v,gp[eax]);x[1]=x[2];
            if(si(v)<=0)this->reached(blend_none);
            break;
        case blend_weight:
            x[0]=load_ss(v);this->flags.scalar_compare(x[0],x[3],false);
            if(this->flags.cf||this->flags.zf)this->reached(weight_skipped);
            break;
        // MOVSX EAX; XOR EDI,EDI; TEST EAX,EAX.
        case blend_source:
            gp[eax]=sx16(v);gp[edi]=this->flags.logic(0,4);this->test32(gp[eax]);blend_fields=false;
            if(si(gp[eax])<0)this->reached(source_missing);
            break;
        case blend_palette_count:this->compare32(gp[eax],v);if(si(gp[eax])>=si(v))this->reached(source_missing);break;
        // IMUL EAX,EAX,F4; ADD EAX,EDX; MOV EDI,EAX.
        case blend_palette:
            gp[edx]=v;gp[eax]=this->flags.plus(this->flags.product(gp[eax],std::uint32_t(sizeof(logic::FogEntry))),v,4);gp[edi]=gp[eax];
            break;
        // MOV EAX; MOV EDX,EDI; LEA EBX,[ESP+D0]; SUB EDX,EBX; DEC ECX; SHR
        // ECX,2; LEA EAX,[ESP+EAX+D0]; INC ECX.
        case blend_field_offset:
            gp[ebx]=blended_base();gp[edx]=this->flags.minus(gp[edi],gp[ebx],4);
            gp[ecx]=this->flags.right(this->flags.increment(gp[ecx],4,true),2,4,false);gp[eax]=blended_base()+v;
            gp[ecx]=this->flags.increment(gp[ecx],4,false);
            break;
        case blend_source_field:x[5]=load_ss(v);break;
        // MOV ESI; INC EAX; ADD ESI,10; CMP EAX,ECX.
        case blend_offset_reload:
            gp[esi]=v;gp[eax]=this->flags.increment(gp[eax],4,false);
            gp[esi]=this->flags.plus(gp[esi],std::uint32_t(sizeof(logic::FogMixer)),4);this->compare32(gp[eax],gp[ecx]);
            break;
        case output_entry:gp[eax]=v;break;
        case mix_field_offset:gp[eax]=v;break;
        // TEST ECX,ECX on each field's size.
        default:break;
        }
        switch(a) {
        case blend_field_size:case mix_field_size:
            this->test32(v);if(si(v)<=0)this->reached(a==blend_field_size?field_skipped:mix_skipped);
            break;
        // ADD EAX,EBX; LEA EDX,[ESP+D0]; SUB EDX,EBX.
        case mix_output:gp[eax]=this->flags.plus(gp[eax],gp[ebx],4);gp[edx]=this->flags.minus(blended_base(),gp[ebx],4);break;
        // SUB ESI,[ESP+28]; DEC ECX; SHR ECX,2; INC ECX (MOVAPS/SUBSS XMM0: mixed).
        case mix_entry:
            gp[esi]=this->flags.minus(gp[esi],v,4);
            gp[ecx]=this->flags.increment(this->flags.right(this->flags.increment(gp[ecx],4,true),2,4,false),4,false);
            break;
        case mix_original:x[3]=load_ss(v);break;
        case mix_blended:x[1]=load_ss(v);break;
        case output_count_reload:gp[ecx]=v;break;
        // INC ECX; ADD EAX,F4.
        case scenario_reload:
            gp[ecx]=this->flags.increment(gp[ecx],4,false);gp[eax]=this->flags.plus(gp[eax],std::uint32_t(sizeof(logic::FogEntry)),4);
            break;
        case next_entry:gp[ecx]=v;break;
        // ADD ECX,18; DEC EAX.
        case clusters_left:
            gp[eax]=v;gp[ecx]=this->flags.plus(gp[ecx],std::uint32_t(sizeof(logic::ClusterDistance)),4);
            gp[eax]=this->flags.increment(gp[eax],4,true);
            break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case weight_sum_store:x[1]=low_lane(x[1],v);break;
        // MOVAPS XMM1,XMM2; SUBSS; MULSS; then MOVAPS XMM4,XMM1.
        case remaining_store:x[1]=low_lane(x[2],v);x[4]=x[1];break;
        // REP STOSD / REP MOVSD: the cursors step, ECX counts down.
        case blended_clear:gp[edi]+=step();--gp[ecx];break;
        case copy:gp[esi]+=step();gp[edi]+=step();--gp[ecx];break;
        // MOVSS XMM5 ... ADDSS; MOVSS [EAX],XMM5; ADD EAX,4; DEC ECX.
        case blend_accumulator_store:
            this->reached(blend_dword);
            x[5]=low_lane(x[5],v);gp[eax]=this->flags.plus(gp[eax],4u,4);gp[ecx]=this->flags.increment(gp[ecx],4,true);
            break;
        case mix_store:
            this->reached(mix_dword);
            gp[eax]=this->flags.plus(gp[eax],4u,4);gp[ecx]=this->flags.increment(gp[ecx],4,true);
            break;
        default:break;
        }
    }
    // Frame.
    std::uint32_t local(Local l)const {
        switch(l) {
        case Local::mixer_offset:return locals+offsetof(logic::Locals,mixer_offset);
        case Local::mixer_index:return locals+offsetof(logic::Locals,mixer_index);
        case Local::list_entry:return locals+offsetof(logic::Locals,list_entry);
        case Local::slots_written:return locals+offsetof(logic::Locals,slots_written);
        case Local::output_slot:return locals+offsetof(logic::Locals,output_slot);
        case Local::weight_sum:return locals+offsetof(logic::Locals,weight_sum);
        case Local::fog_entry:return locals+offsetof(logic::Locals,fog_entry);
        case Local::remaining:return locals+offsetof(logic::Locals,remaining);
        case Local::clusters_left:return locals+offsetof(logic::Locals,clusters_left);
        case Local::mixer_count:return locals+offsetof(logic::Locals,mixer_count);
        case Local::weights:return locals+offsetof(logic::Locals,weights);
        case Local::fields:return locals+offsetof(logic::Locals,fields);
        case Local::blended:return locals+offsetof(logic::Locals,blended);
        }
        return 0;
    }
    std::uint32_t argument(Argument a)const{return this->gp[ebp]+(a==Argument::list?8u:0xcu);}
    std::uint32_t string_step()const{return step();}
    // PUSH EDX (the value); PUSH EAX (the index); 137AC0. The walk lives in
    // EDI (scenario) and EBX (entry); the result is XMM0.
    std::uint32_t interpolate(logic::fog_mixers::Walk &walk,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        gp[edi]=walk.scenario;gp[ebx]=walk.entry;
        this->push(0x131881u,gp[edx]);this->push(0x131882u,gp[eax]);
        this->call(0x131883u,0x131888u,0x137ac0u,interpolate_child);
        locals=gp[esp];
        walk.scenario=gp[edi];walk.entry=gp[ebx];
        this->reached(interpolated);
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(this->s.xmm[0])));
    }
    // COMISS XMM3,XMM0, then COMISS XMM0,XMM2; MOVAPS XMM0 from the bound.
    void weighed(Clamp clamp,std::uint32_t) {
        auto &x=this->s.xmm;
        this->flags.scalar_compare(x[3],x[0],false);
        if(clamp==Clamp::zero){x[0]=x[3];this->reached(clamp_zero);return;}
        this->flags.scalar_compare(x[0],x[2],false);
        if(clamp==Clamp::one){x[0]=x[2];this->reached(clamp_one);return;}
        this->reached(clamp_kept);
    }
    void blend_scale(std::uint32_t scale){this->s.xmm[1]=low_lane(this->s.xmm[2],scale);}
    // MOVAPS XMM0,XMM2; SUBSS XMM0,XMM4 (the complement); the dword's
    // MULSS XMM3,XMM4 and result in XMM1 (both loaded by MOVSS).
    void mixed(const logic::fog_mixers::MixStep &step) {
        auto &x=this->s.xmm;
        x[0]=low_lane(x[2],step.complement);x[3]=load_ss(step.own_share);x[1]=load_ss(step.result);
    }
    void run() {
        auto &gp=this->gp;
        // PUSH EBP; MOV EBP,ESP; AND ESP,-8; SUB ESP,1BC; MOV EAX,4; PUSH
        // EBX/ESI/EDI; MOV EDI,18; MOV ECX,0C.
        this->push(0x131610u,gp[ebp]);gp[ebp]=gp[esp];
        gp[esp]=this->flags.logic(gp[esp]&0xfffffff8u,4);gp[esp]=this->flags.minus(gp[esp],0x1bcu,4);gp[eax]=4u;
        this->push(0x131621u,gp[ebx]);this->push(0x131622u,gp[esi]);this->push(0x131623u,gp[edi]);
        locals=gp[esp];gp[edi]=0x18u;gp[ecx]=0xcu;
        Memory<FogMixers> memory{*this};
        const auto exit=logic::fog_mixers::mix(memory,*this,gp[edx]);
        if(exit==logic::fog_mixers::Exit::mixed) {
            gp[edi]=this->pop(0x131aa4u);gp[esi]=this->pop(0x131aa5u);gp[ebx]=this->pop(0x131aa6u);
            gp[esp]=gp[ebp];gp[ebp]=this->pop(0x131aa9u);
            this->ret(0x131aaau,8);this->publish(0x131aaau);
            return;
        }
        if(exit==logic::fog_mixers::Exit::no_clusters){this->test32(gp[edx]);this->reached(no_clusters);}
        gp[edi]=this->pop(0x131aadu);gp[eax]=gp[esi];gp[esi]=this->pop(0x131ab0u);gp[ebx]=this->pop(0x131ab1u);
        gp[esp]=gp[ebp];gp[ebp]=this->pop(0x131ab4u);
        this->ret(0x131ab5u,8);this->publish(0x131ab5u);
    }
};

template<class State,class Reference,class Interpolate>
bool bridge(State &s,Reference reference,Interpolate interpolate) {
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x131610u,0x131ab5u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return FogMixers<State,Interpolate>(v,c,t,interpolate);});
    return true;
}
} // namespace halo2::native_fog_mixers_detail
