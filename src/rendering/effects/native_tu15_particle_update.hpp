#pragma once
// Boundary adapter for the particle location update
// (native_tu15_particle_update_systems.hpp): 251A60 with 250450 inline. Its
// caller (1778D0, the particle system update) and its children are still
// translated TU1.5 code in the particle frame unit, so the adapter owns that
// ABI and nothing else:
//   - the arguments and locals: their stack slots, the pushes of the inline
//     call's six arguments, the return addresses and RETs;
//   - the original's register file at every child call (13A6B0, 251650,
//     16E8A0, 2C40D0, 2516C0 and the three interface methods): each access
//     role's register effects, the "kept" values handed back after a call,
//     the SSE scratch values the logic reports, and the flags of the
//     instruction that decides them;
//   - the x87 tail: the radius scale the +4C method leaves in ST(0), the
//     logical x87 stack's checks and pops, and the C1 of the two stores;
//   - the diagnostic cursor (each access publishes its original instruction;
//     the RET publishes the exit), and every store through the unit's store
//     helper.
// Delete it when 1778D0 and the children go native.
//
// The children stay genuine and are entered through the unit's own route with
// the exact register, flag, XMM, x87 and stack ABI: 13A6B0, 251650, 16E8A0,
// 2C40D0 and 2516C0 through their bodies, the interface methods through the
// unit's dispatch. tools/native_effect_owner_replacements.py installs the
// bridge into the build-local specialized copy of the particle frame unit.
// HALO2_REFERENCE_EFFECT_OWNERS=1 (or a list holding 251a60) restores the
// translated bodies; HALO2_VERIFY_EFFECT_OWNERS=1 records each admitted call
// and replays the unmodified translated bodies against it, and prints
// "Particle locations verify: ..." at exit.
#include "rendering/effects/native_tu15_effect_owners.hpp"
#include "rendering/effects/native_tu15_particle_update_systems.hpp"
#include <array>
#include <atomic>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

namespace halo2::native_particle_update_detail {
namespace logic=halo2::native_effects::location_update;
using native_object_tick_detail::Trace;
using native_object_tick_detail::sx16;
using native_object_tick_detail::si;
using native_effect_owner_detail::owner_reference;
using native_effect_owner_detail::verify_mode;
using native_effect_owner_detail::scalar;
using native_effect_owner_detail::replace_low;
using native_effect_owner_detail::masked_controls;
using native_effect_owner_detail::Memory;
using halo2::native_effects::none;
using namespace native_object_tick_detail::boundary;

// Decisions reached (coverage). The adapter reads them from the logic's
// results and reports; the logic carries no instrumentation.
enum Branch : unsigned {
    // 251A60
    lu_no_emitters, lu_emitters, lu_sized, lu_unsized, lu_unit_range, lu_ranged, lu_lower_larger, lu_upper_larger,
    lu_size_grew, lu_size_kept, lu_more_emitters, lu_last_emitter, lu_drawn, lu_undrawn_kept, lu_undrawn_empty,
    lu_released, lu_release_more, lu_model, lu_sprite, lu_other,
    // 250450
    eu_physics, eu_no_physics, eu_idle, eu_no_particles, eu_aging, eu_cycled, eu_cycles_unflagged, eu_cycles_off,
    eu_lod_cycled, eu_lod_dead, eu_lod_unflagged, eu_skipped, eu_aged_out, eu_rested, eu_landed_moving, eu_moved,
    eu_rest_checked, eu_rest_killed, eu_rest_kept, eu_not_resting, eu_x_grew, eu_x_kept, eu_y_grew, eu_y_kept, eu_z_grew,
    eu_z_kept, eu_x_max_grew, eu_x_max_kept, eu_y_max_grew, eu_y_max_kept, eu_z_max_grew, eu_z_max_kept,
    eu_removed_linked, eu_removed_first, eu_more_particles, eu_physics_empty, eu_physics_moved, eu_running,
    eu_no_deaths, eu_stopped,
    branch_count
};
static_assert(branch_count<=64u);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)
            std::fprintf(stderr,"Particle locations verify: 251A60 %llu calls; %llu ordered accesses (%llu stores) "
                "and %llu child boundaries identical, coverage %llx\n",(unsigned long long)calls.load(),
                (unsigned long long)accesses.load(),(unsigned long long)stores.load(),
                (unsigned long long)children.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

inline constexpr char x87_stack_fault[]="Required original solver x87 stack-fault owner unavailable";

// ---- 251A60 / 250450 --------------------------------------------------------
// 251A60: EAX = the system definition, EBX = the location; [ESP+4] the
// particle system, +8 the time step; RET 8. Frame: SUB ESP,20 and EBP, ESI,
// EDI saved, so the locals are entry-0x20.. (index -0x20, largest size -0x1C,
// box -0x18..-4). The location stays in EBX, the definition in ESI, the
// emitter definition's offset in EDI and the emitter in EBP (the release
// walk: EDI the emitter, EBP the emitter array; the radius: ESI the particle
// tag, EDI the interface). XMM0..XMM5 hold the box after the emitters (and
// across the release walk), XMM6 the 0.5.
// 250450 (inline): EAX = the emitter definition; [ESP+4] the emitter, +8 the
// system, +C the location, +10 the system definition, +14 the time step, +18
// the box (its low byte reused as "a particle died"); RET 18. Frame: SUB
// ESP,18 and EBX, EBP saved (EDI and ESI once there are particles), locals
// entry-0x10 the physics definition, -0xC the interface, -0x14 the previous
// particle, -0x18 the next handle, -8 and -4 the two spills. The box stays in
// EBP, the particle in ESI, its handle in EDI and the mark (1) in EBX.
template<class State,class Interface,class Release,class Delete,class Physics,class Clear,class Method>
struct ParticleLocation : Port<State> {
    using LocationAccess=logic::LocationAccess;
    using EmitterAccess=logic::EmitterAccess;
    Interface &interface_child;Release &release_child;Delete &delete_child;Physics &physics_child;Clear &clear_child;
    Method &method_child;
    std::uint32_t location_entry{};  // 251A60's entry ESP (its return address)
    std::uint32_t emitter_entry{};   // 250450's entry ESP
    bool emitter_saves_edi{},emitter_saves_esi{};
    std::uint32_t physics_handle{},alive_byte{};
    unsigned x87_depth{};            // the logical x87 depth during the radius tail
    ParticleLocation(State &s,std::uint32_t *c,Trace<State> *t,Interface &i,Release &r,Delete &d,Physics &p,Clear &k,Method &m)
        :Port<State>(s,c,t,{}),interface_child(i),release_child(r),delete_child(d),physics_child(p),clear_child(k),method_child(m){}

    // ---- 251A60 accesses
    std::uint32_t instruction(LocationAccess a,std::uint32_t) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum LocationAccess;
        switch(a) {
        // SUB ESP,20; XORPS XMM0,XMM0.
        case position_x:gp[esp]-=0x20u;x[0]=_mm_setzero_ps();return 0x251a66u;
        case position_y:return 0x251a6bu;
        // PUSH EBP; PUSH ESI; MOV ESI,EAX.
        case first_emitter:this->push(0x251a70u,gp[ebp]);this->push(0x251a71u,gp[esi]);gp[esi]=gp[eax];return 0x251a74u;
        case size_store:this->push(0x251a77u,gp[edi]);return 0x251a78u;
        case position_z:return 0x251a7eu;
        case x_maximum_init:return 0x251a91u;
        case x_minimum_init:return 0x251a97u;
        case y_maximum_init:return 0x251a9du;
        case y_minimum_init:return 0x251aa3u;
        case z_maximum_init:return 0x251aa9u;
        case z_minimum_init:return 0x251aafu;
        case index_init:return 0x251ab5u;
        case emitter_table:return 0x251ac0u;
        case emitter_data:return 0x251ac6u;
        case index:return 0x251ad3u;
        case emitter_count:return 0x251ad9u;
        case emitter_definitions:return 0x251adeu;
        case size_function:return 0x251ae1u;
        case size_flags:gp[eax]=gp[eax]+gp[edi]+0x50u;return 0x251ae9u;
        case size_lower:return 0x251af3u;
        case size_upper:return 0x251b01u;
        case size_unit_upper:return 0x251b08u;
        case size_maximum:return 0x251b18u;
        case size_store_grown:return 0x251b1fu;
        case time_step:return 0x251b25u;
        case emitter_definitions_again:return 0x251b29u;
        // LEA ECX,[ESP+14]; PUSH ECX.
        case system:gp[ecx]=gp[esp]+0x14u;this->push(0x251b30u,gp[ecx]);return 0x251b31u;
        case next_emitter:return 0x251b41u;
        case index_reload:return 0x251b44u;
        case index_store:return 0x251b52u;
        case z_minimum:return 0x251b5cu;
        case z_maximum:return 0x251b62u;
        case y_minimum:return 0x251b68u;
        case y_maximum:return 0x251b6eu;
        case x_minimum:return 0x251b74u;
        case x_maximum:return 0x251b7au;
        case drawn_frame:return 0x251b80u;
        case frame_stamp:return 0x251b83u;
        case keep_flags:return 0x251b8du;
        case release_first:return 0x251b96u;
        case release_table:return 0x251b9eu;
        case release_data:return 0x251ba4u;
        case release_next:return 0x251bb7u;
        case half:return 0x251bbfu;
        case tag_table:return 0x251bc7u;
        case center_x:return 0x251be5u;
        case center_y:return 0x251beau;
        case center_z:return 0x251befu;
        case particle_tag:return 0x251bf4u;
        // MOVSX ECX,SI; SHL ECX,4.
        case group:gp[ecx]=this->flags.shift(sx16(gp[esi]),4,4);return 0x251bfdu;
        case extent_x:return 0x251c13u;
        case extent_y:return 0x251c19u;
        case extent_z:return 0x251c1fu;
        case vtable:return 0x251c3au;
        // PUSH ESI (the tag); MOV ECX,EDI.
        case select_method:this->push(0x251c3cu,gp[esi]);gp[ecx]=gp[edi];return 0x251c3fu;
        case radius_vtable:return 0x251c42u;
        case radius_method:gp[ecx]=gp[edi];return 0x251c46u;
        case size_for_radius:return 0x251c49u;
        case extent_z_value:return 0x251c4du;
        case extent_z_again:gp[edi]=this->pop(0x251c51u);return 0x251c52u;
        case extent_y_value:gp[esi]=this->pop(0x251c56u);return 0x251c57u;
        case extent_y_again:gp[ebp]=this->pop(0x251c5bu);return 0x251c5cu;
        case extent_x_value:return 0x251c62u;
        case extent_x_again:return 0x251c66u;
        case extent_spill:return 0x251c6eu;
        case radius:return 0x251c74u;
        }
        return 0;
    }
    void loaded(LocationAccess a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum LocationAccess;
        switch(a) {
        case position_x:x[2]=scalar(std::bit_cast<float>(v));break;
        case position_y:x[1]=scalar(std::bit_cast<float>(v));break;
        case first_emitter:case next_emitter:case emitter_definitions:case emitter_definitions_again:case frame_stamp:gp[eax]=v;break;
        // MOVSS XMM0; XOR EDI,EDI; MOVAPS XMM5/XMM4/XMM3 from XMM2/XMM1/XMM0.
        case position_z:x[0]=scalar(std::bit_cast<float>(v));gp[edi]=0;x[5]=x[2];x[4]=x[1];x[3]=x[0];break;
        case emitter_table:case size_function:case release_data:case system:gp[ecx]=v;break;
        // AND EAX,FFFF; IMUL EAX,4C; ADD EAX,EDX (the emitter).
        case emitter_data:gp[edx]=v;gp[eax]=(gp[eax]&0xffffu)*0x4cu+v;break;
        // MOV EBP,EAX after the index.
        case index:gp[edx]=v;gp[ebp]=gp[eax];break;
        // MOV AL,[ECX+1]; SHR AL,4.
        case size_flags:set_low_byte(gp[eax],(v&0xffu)>>4);break;
        case size_lower:x[1]=scalar(std::bit_cast<float>(v));break;
        case size_upper:case size_unit_upper:x[0]=scalar(std::bit_cast<float>(v));break;
        case time_step:case drawn_frame:case tag_table:gp[edx]=v;break;
        // INC EBP.
        case index_reload:gp[ebp]=v+1u;break;
        case z_minimum:x[3]=scalar(std::bit_cast<float>(v));break;
        case z_maximum:x[0]=scalar(std::bit_cast<float>(v));break;
        case y_minimum:x[4]=scalar(std::bit_cast<float>(v));break;
        case y_maximum:x[1]=scalar(std::bit_cast<float>(v));break;
        case x_minimum:x[5]=scalar(std::bit_cast<float>(v));break;
        case x_maximum:x[2]=scalar(std::bit_cast<float>(v));break;
        // MOV AL,[ESI+16]; SHR AL,1.
        case keep_flags:set_low_byte(gp[eax],(v&0xffu)>>1);break;
        case release_first:gp[edi]=v;break;
        case release_next:gp[edi]=v;if(v!=none)this->reached(lu_release_more);break;
        // CMP EDX,[ESI+30]: the emitter has a definition (index < count).
        case emitter_count:this->reached(si(gp[edx])<si(v)?lu_sized:lu_unsized);break;
        case release_table:gp[ebp]=v;break;
        case half:x[6]=scalar(std::bit_cast<float>(v));break;
        case particle_tag:gp[esi]=v;break;
        case group:gp[eax]=v;break;
        case vtable:gp[eax]=v;break;
        case radius_vtable:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(LocationAccess a,std::uint32_t,std::uint32_t v) {
        auto &x=this->s.xmm;
        using enum LocationAccess;
        // The center (ADDSS, MULSS into XMM5/4/3) and the half extent (SUBSS
        // into XMM2/1/0) keep their registers' upper lanes.
        switch(a) {
        case center_x:x[5]=replace_low(x[5],std::bit_cast<float>(v));break;
        case center_y:x[4]=replace_low(x[4],std::bit_cast<float>(v));break;
        case center_z:x[3]=replace_low(x[3],std::bit_cast<float>(v));break;
        case extent_x:x[2]=replace_low(x[2],std::bit_cast<float>(v));break;
        case extent_y:x[1]=replace_low(x[1],std::bit_cast<float>(v));break;
        case extent_z:x[0]=replace_low(x[0],std::bit_cast<float>(v));break;
        default:break;
        }
    }

    // ---- 251A60 frame
    std::uint32_t argument(logic::LocationArgument a){return location_entry+(a==logic::LocationArgument::system?4u:8u);}
    std::uint32_t local(logic::LocationLocal l) {
        using enum logic::LocationLocal;
        switch(l) {
        case index:return location_entry-0x20u;
        case size:return location_entry-0x1cu;
        case bounds:case extent_x:return location_entry-0x18u;
        case extent_y:return location_entry-0x14u;
        case extent_z:return location_entry-0x10u;
        }
        return 0;
    }
    // XMM1 = the lower value (or 0: XORPS), XMM0 = the upper value (or 1);
    // MOVAPS XMM0,XMM1 when the lower is greater.
    void size_checked(const logic::SizeScratch &size) {
        auto &x=this->s.xmm;
        if(size.unit_range)x[1]=_mm_setzero_ps();
        if(size.lower_greater)x[0]=x[1];
        this->reached(size.unit_range?lu_unit_range:lu_ranged);
        this->reached(size.lower_greater?lu_lower_larger:lu_upper_larger);
        this->reached(size.grew?lu_size_grew:lu_size_kept);
    }
    // PUSH EDX (time step), ESI, EBX, ECX (system); ADD EAX,EDI; PUSH EBP; CALL.
    void pass_emitter_arguments(const logic::EmitterCall &call,const logic::LocationKept &kept) {
        auto &gp=this->gp;
        gp[ebx]=kept.location;gp[esi]=kept.definition;gp[edi]=kept.emitter_offset;gp[ebp]=kept.emitter;
        this->push(0x251b35u,gp[edx]);this->push(0x251b36u,gp[esi]);this->push(0x251b37u,gp[ebx]);
        this->push(0x251b38u,gp[ecx]);gp[eax]=call.definition_entry;this->push(0x251b3bu,gp[ebp]);
        this->push(0x251b3cu,0x251b41u);
        emitter_entry=gp[esp];emitter_saves_edi=emitter_saves_esi=false;
        this->reached(lu_emitters);
    }
    // The registers 250450 restored (or left) come back to the walk.
    void emitter_updated(logic::LocationKept &kept) {
        const auto &gp=this->gp;
        kept.location=gp[ebx];kept.definition=gp[esi];kept.emitter_offset=gp[edi];kept.emitter=gp[ebp];
    }
    // INC EBP (the count); ADD EDI,B8.
    void emitter_counted(std::uint32_t count,std::uint32_t next,const logic::LocationKept &kept) {
        this->gp[ebp]=count;this->gp[edi]=kept.emitter_offset;
        this->reached(next==none?lu_last_emitter:lu_more_emitters);
    }
    // INC EDX.
    void stamp_compared(std::uint32_t drawn){this->gp[edx]=drawn;}
    // AND EDI,FFFF; IMUL EDI,4C; ADD EDI,ECX (its flags); CALL 2516C0 with
    // EDI the emitter and EBP the array. The box comes back from XMM0..XMM5.
    void release_emitter_particles(logic::LocationKept &kept,logic::Bounds &box) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        const auto index=this->flags.logic(gp[edi]&0xffffu,4);
        const auto scaled=this->flags.product(index,0x4cu);
        this->flags.plus(scaled,gp[ecx],4);
        gp[ebx]=kept.location;gp[esi]=kept.definition;gp[edi]=kept.released;gp[ebp]=kept.emitter_table;
        this->call(0x251bb2u,0x251bb7u,0x2516c0u,clear_child);
        kept.location=gp[ebx];kept.definition=gp[esi];kept.released=gp[edi];kept.emitter_table=gp[ebp];
        const auto lane=[&](unsigned n){return _mm_cvtss_f32(x[n]);};
        box={lane(5),lane(2),lane(4),lane(1),lane(3),lane(0)};
    }
    // XOR EDI,EDI; CMP EAX,'PRTM' (and 'prt3'); MOV EDI,interface.
    void group_compared(std::uint32_t group,const logic::LocationKept &kept) {
        this->flags.logic(0,4);
        this->flags.minus(group,logic::model_group,4);
        if(group!=logic::model_group)this->flags.minus(group,logic::sprite_group,4);
        this->gp[edi]=kept.interface;
        this->reached(group==logic::model_group?lu_model:group==logic::sprite_group?lu_sprite:lu_other);
    }
    void select_particle_tag(std::uint32_t method,logic::LocationKept &kept) {
        auto &gp=this->gp;
        gp[ebx]=kept.location;gp[esi]=kept.definition;gp[edi]=kept.interface;
        this->call(0x251c3fu,0x251c42u,method,[&](State &v){method_child(v,method,0x251c3fu);});
        kept.location=gp[ebx];kept.definition=gp[esi];kept.interface=gp[edi];
    }
    // The +4C method leaves the radius scale in ST(0); FMUL m32 (after its
    // read) takes it there.
    void radius_scale(std::uint32_t method,logic::LocationKept &kept) {
        auto &gp=this->gp;
        gp[ebx]=kept.location;gp[esi]=kept.definition;gp[edi]=kept.interface;
        this->call(0x251c46u,0x251c49u,method,[&](State &v){method_child(v,method,0x251c46u);});
        kept.location=gp[ebx];kept.definition=gp[esi];kept.interface=gp[edi];
        x87_depth=this->s.fp.count;
    }
    long double scale_on_stack(){return this->s.fp.st(0);}
    // FLD m32 pushes on the logical x87 stack: depth + 1 must fit.
    void x87_load(unsigned above) {
        if(x87_depth+above>8u)throw std::runtime_error(x87_stack_fault);
    }
    void extent_spilled(bool rounded_up){set_c1(rounded_up);}
    void set_c1(bool rounded_up) {
        auto &status=this->s.fp.status;
        status=std::uint16_t((status&~0x200u)|(rounded_up?0x200u:0u));
    }
    // FSTP: the radius scale leaves the logical stack; ADD ESP,20; RET 8.
    void leave_location_update(bool rounded_up) {
        set_c1(rounded_up);this->s.fp.pop();
        auto &gp=this->gp;
        gp[esp]=this->flags.plus(gp[esp],0x20u,4);
        this->ret(0x251c7au,8);this->publish(0x251c7au);
    }

    // ---- 250450 accesses
    std::uint32_t instruction(EmitterAccess a,std::uint32_t) {
        auto &gp=this->gp;
        using enum EmitterAccess;
        switch(a) {
        case physics_tag:return 0x250450u;
        // SUB ESP,18; PUSH EBX; PUSH EBP.
        case bounds_argument:gp[esp]-=0x18u;this->push(0x250459u,gp[ebx]);this->push(0x25045au,gp[ebp]);return 0x25045bu;
        case tag_table:return 0x250461u;
        case physics_definition:gp[eax]=(gp[eax]&0xffffu)<<4;return 0x25046fu;
        case physics_store:return 0x250473u;
        case physics_clear:return 0x250479u;
        case definition_argument:return 0x250481u;
        case particle_tag:return 0x250485u;
        case system_argument:return 0x25048du;
        case system_updating:gp[ecx]=gp[eax];return 0x250493u;
        case interface_store:return 0x2504a0u;
        case emitter_argument:return 0x2504aau;
        case first_particle:this->push(0x2504aeu,gp[edi]);emitter_saves_edi=true;return 0x2504afu;
        case previous_clear:return 0x2504b5u;
        case died_clear:return 0x2504bdu;
        case interface_reload:return 0x2504cbu;
        case particle_table:
            if(!emitter_saves_esi){this->push(0x2504c8u,gp[esi]);emitter_saves_esi=true;}
            return 0x2504d0u;
        case time_step:return 0x2504d5u;
        case one:return 0x2504dbu;
        case particle_data:return 0x2504eeu;
        case inverse_lifetime:return 0x2504f1u;
        case next:return 0x2504f6u;
        case age:return 0x2504fdu;
        case next_store:return 0x250505u;
        case age_store:return 0x250509u;
        case system_cycles:return 0x250510u;
        case system_cycling:return 0x25051au;
        case cycle_spill:return 0x25051fu;
        case cycle_reload:return 0x250525u;
        case definition_reload:return 0x250543u;
        case definition_cycles:return 0x250547u;
        case location_argument:return 0x25054cu;
        case location_lod:return 0x250550u;
        case zero:return 0x250555u;
        case lod_spill:return 0x25055eu;
        case lod_reload:return 0x250564u;
        case age_reload:return 0x25052fu;
        case age_fraction:return 0x25053cu;
        case dead_flags:case dead_flags_store:return 0x250570u;
        case died_set:return 0x250574u;
        case flags:return 0x250578u;
        case age_check:return 0x250584u;
        case velocity_x:return 0x250592u;
        case velocity_y:return 0x250597u;
        case velocity_z:return 0x25059cu;
        case rest_threshold:return 0x2505beu;
        // OR EAX,8.
        case rest_store:gp[eax]|=logic::particle_flags::resting;return 0x2505cau;
        case move_velocity_x:return 0x2505d0u;
        case move_position_x:return 0x2505d9u;
        case move_position_x_store:return 0x2505deu;
        case move_velocity_y:return 0x2505e3u;
        case move_position_y:return 0x2505ecu;
        case move_position_y_store:return 0x2505f1u;
        case move_velocity_z:return 0x2505f6u;
        case move_position_z:return 0x2505ffu;
        case move_position_z_store:return 0x250604u;
        case rotation_rate:return 0x250609u;
        // AND EAX,FFEF.
        case phase:gp[eax]&=0xffefu;return 0x250617u;
        case unlanded_store:return 0x25061cu;
        case phase_store:return 0x250620u;
        case vtable:return 0x25062eu;
        case rest_method:return 0x250630u;
        case killed_flags:case killed_flags_store:return 0x250637u;
        case alive:return 0x25063bu;
        case x_minimum:return 0x250644u;
        case x_check:return 0x250649u;
        case previous_store:return 0x25064du;
        case x_from_particle:return 0x250653u;
        case x_kept:return 0x25065au;
        case x_minimum_store:return 0x25065fu;
        case y_minimum:return 0x250664u;
        case y_check:return 0x250669u;
        case y_from_particle:return 0x25066fu;
        case y_kept:return 0x250676u;
        case y_minimum_store:return 0x25067bu;
        case z_minimum:return 0x250680u;
        case z_check:return 0x250685u;
        case z_from_particle:return 0x25068bu;
        case z_kept:return 0x250692u;
        case z_minimum_store:return 0x250697u;
        case x_maximum:return 0x25069cu;
        case x_maximum_check:return 0x2506a1u;
        case x_maximum_from_particle:return 0x2506a9u;
        case x_maximum_store:return 0x2506aeu;
        case y_maximum:return 0x2506b3u;
        case y_maximum_check:return 0x2506b8u;
        case y_maximum_from_particle:return 0x2506c0u;
        case y_maximum_store:return 0x2506c5u;
        case z_maximum:return 0x2506cau;
        case z_maximum_check:return 0x2506cfu;
        case z_maximum_store:return 0x2506d5u;
        case z_maximum_from_particle:return 0x2506dcu;
        case z_maximum_particle_store:return 0x2506e1u;
        case release_emitter:return 0x2506e8u;
        // PUSH EAX (the emitter).
        case release_system:this->push(0x2506ecu,gp[eax]);return 0x2506edu;
        case delete_table:return 0x2506f8u;
        case previous_reload:return 0x250705u;
        case link_next:return 0x250712u;
        case previous_link:return 0x250716u;
        case first_next:return 0x25071bu;
        case first_emitter:return 0x25071fu;
        case first_link:return 0x250723u;
        case next_reload:return 0x250726u;
        case system_reload:return 0x25072du;
        case physics_reload:
            if(emitter_saves_esi)gp[esi]=this->pop(0x250737u);
            return 0x250738u;
        case physics_emitter:return 0x250740u;
        case physics_first:return 0x250744u;
        case physics_time_step:return 0x25074cu;
        // PUSH ECX (the time step).
        case physics_location:this->push(0x250750u,gp[ecx]);return 0x250751u;
        case definition_final:return 0x25075cu;
        case definition_stops:return 0x250760u;
        case died_check:return 0x250766u;
        case system_final:return 0x25076eu;
        case system_flags:case system_flags_store:return 0x250772u;
        }
        return 0;
    }
    void loaded(EmitterAccess a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum EmitterAccess;
        const auto single=[&](unsigned n){x[n]=scalar(std::bit_cast<float>(v));};
        switch(a) {
        case physics_tag:gp[eax]=v;physics_handle=v;break;
        case bounds_argument:gp[ebp]=v;break;
        case tag_table:gp[ecx]=v;break;
        case physics_definition:gp[edx]=v;break;
        case definition_argument:case emitter_argument:case particle_table:case next:case location_argument:
        case release_emitter:case release_system:case previous_reload:case first_emitter:case system_final:gp[eax]=v;break;
        case physics_first:gp[eax]=v;if(v==none)this->reached(eu_physics_empty);break;
        case particle_tag:case system_argument:case definition_reload:case delete_table:case first_next:case system_reload:
        case definition_final:gp[edx]=v;break;
        // MOV AL; SHR AL,3 (the updating bit); MOV EBX,1.
        case system_updating:set_low_byte(gp[eax],(v&0xffu)>>3);gp[ebx]=1;break;
        case first_particle:gp[edi]=v;if(v==none)this->reached(eu_no_particles);break;
        case next_reload:gp[edi]=v;if(v!=none)this->reached(eu_more_particles);break;
        case physics_reload:gp[edi]=v;break;
        case interface_reload:case link_next:case physics_emitter:case physics_time_step:case physics_location:gp[ecx]=v;break;
        case time_step:single(3);break;
        case one:single(1);break;
        // MOV ESI,EDI; AND ESI,FFFF; SHL ESI,6; ADD ESI,[EAX+44].
        case particle_data:gp[esi]=((gp[edi]&0xffffu)<<6)+v;break;
        case inverse_lifetime:case cycle_reload:case lod_reload:case velocity_z:case move_velocity_x:case move_velocity_y:
        case move_velocity_z:case rotation_rate:case x_minimum:case x_from_particle:case x_kept:case y_minimum:
        case y_from_particle:case y_kept:case z_minimum:case z_from_particle:case z_kept:case x_maximum:
        case x_maximum_from_particle:case y_maximum:case y_maximum_from_particle:case z_maximum:case z_maximum_from_particle:
            single(0);break;
        // MOV AL; SHR AL,6 (the cycles bit).
        case system_cycles:set_low_byte(gp[eax],(v&0xffu)>>6);break;
        case location_lod:case age_reload:case velocity_x:single(2);break;
        case velocity_y:single(1);break;
        // MOV AX,[ESI+2].
        case flags:gp[eax]=(gp[eax]&0xffff0000u)|(v&0xffffu);break;
        case vtable:gp[edx]=v;break;
        case alive:alive_byte=v;break;
        // POP EDI follows the definition's flags test.
        case definition_stops:gp[edi]=this->pop(0x250763u);break;
        case died_check:set_low_byte(gp[eax],v);break;
        default:break;
        }
        if(a==previous_reload)gp[ebx]=1;
    }
    void stored(EmitterAccess a,std::uint32_t,std::uint32_t v) {
        auto &x=this->s.xmm;
        using enum EmitterAccess;
        switch(a) {
        case age_store:case move_position_x_store:case move_position_y_store:case move_position_z_store:case phase_store:
            x[0]=replace_low(x[0],std::bit_cast<float>(v));break;
        case age_fraction:x[2]=replace_low(x[2],std::bit_cast<float>(v));break;
        default:break;
        }
    }

    // ---- 250450 frame
    std::uint32_t argument(logic::Argument a){return emitter_entry+4u*(unsigned(a)+1u);}
    std::uint32_t local(logic::Local l) {
        using enum logic::Local;
        switch(l) {
        case physics:return emitter_entry-0x10u;
        case interface:return emitter_entry-0xcu;
        case previous:return emitter_entry-0x14u;
        case next:return emitter_entry-0x18u;
        case cycle_spill:return emitter_entry-8u;
        case lod_spill:return emitter_entry-4u;
        }
        return 0;
    }
    // CMP EAX,-1 (or AND EAX,FFFF; SHL EAX,4 for a physics tag); MOV
    // EDX,[EAX+4]; CALL 13A6B0. EAX comes back as the interface.
    std::uint32_t particle_interface(std::uint32_t,logic::EmitterKept &kept) {
        auto &gp=this->gp;
        if(physics_handle==none){this->flags.minus(physics_handle,none,4);this->reached(eu_no_physics);}
        else {
            this->flags.logic(physics_handle&0xffffu,4);this->flags.shift(physics_handle&0xffffu,4,4);
            this->reached(eu_physics);
        }
        gp[ebp]=kept.bounds;
        this->call(0x250488u,0x25048du,0x13a6b0u,interface_child);
        kept.bounds=gp[ebp];
        return gp[eax];
    }
    // CVTTSS2SI EAX; CVTSI2SS XMM0,EAX (lane 0 only).
    void cycle_converted(const logic::CycleScratch &cycle) {
        this->gp[eax]=std::uint32_t(cycle.whole);
        this->s.xmm[0]=replace_low(this->s.xmm[0],cycle.converted);
    }
    // XMM4 = the sum of squares, XMM2 = vy*vy, XMM1 = vz*vz (each from a
    // MOVSS-loaded source, so the upper lanes are zero).
    void speed_checked(const logic::SpeedScratch &speed) {
        auto &x=this->s.xmm;
        x[4]=scalar(speed.x_squared_sum);x[2]=scalar(speed.y_squared);x[1]=scalar(speed.z_squared);
    }
    // SHR AL,3; TEST BL,AL.
    void rest_flag_tested(std::uint32_t flags) {
        auto &gp=this->gp;
        const auto shifted=this->flags.right(flags&0xffu,3,1,false);
        set_low_byte(gp[eax],shifted);
        this->flags.logic(gp[ebx]&shifted,1);
    }
    // CALL [vtable+34] with ECX = the interface. AL comes back.
    bool resting_particle_dies(std::uint32_t interface,std::uint32_t,std::uint32_t method,logic::EmitterKept &kept) {
        auto &gp=this->gp;
        gp[ecx]=interface;gp[ebx]=kept.mark;gp[esi]=kept.particle;gp[edi]=kept.handle;gp[ebp]=kept.bounds;
        this->call(0x250630u,0x250633u,method,[&](State &v){method_child(v,method,0x250630u);});
        kept.mark=gp[ebx];kept.particle=gp[esi];kept.handle=gp[edi];kept.bounds=gp[ebp];
        return low_byte(gp[eax])!=0;
    }
    // TEST [ESI+2],BL decided the flags; MOV EBX,ESI; CALL 251650 (the
    // emitter pushed, EAX = the system).
    void release_particle(std::uint32_t,std::uint32_t,logic::EmitterKept &kept) {
        auto &gp=this->gp;
        this->flags.logic(alive_byte&kept.mark,1);
        gp[esi]=kept.particle;gp[ebx]=kept.particle;gp[edi]=kept.handle;gp[ebp]=kept.bounds;
        this->call(0x2506f3u,0x2506f8u,0x251650u,release_child);
        kept.handle=gp[edi];kept.bounds=gp[ebp];
    }
    // MOV EAX,EDI; CALL 16E8A0 (EDX = the particle array).
    void delete_particle(std::uint32_t,logic::EmitterKept &kept) {
        auto &gp=this->gp;
        gp[edi]=kept.handle;gp[ebp]=kept.bounds;gp[eax]=gp[edi];
        this->call(0x250700u,0x250705u,0x16e8a0u,delete_child);
        kept.bounds=gp[ebp];
    }
    void particle_stepped(const logic::ParticleStep &step) {
        using logic::Fate;using logic::Motion;
        switch(step.fate) {
        case Fate::aging:this->reached(eu_aging);break;
        case Fate::expired_cycled:this->reached(eu_cycled);break;
        case Fate::expired_lod_cycled:this->reached(eu_lod_cycled);break;
        case Fate::expired_died:
            if(!step.cycle_while_drawn)this->reached(eu_lod_unflagged);
            else this->reached(eu_lod_dead);
            break;
        }
        if(step.fate!=Fate::aging && step.fate!=Fate::expired_cycled)
            this->reached(step.cycles_flagged?eu_cycles_off:eu_cycles_unflagged);
        switch(step.motion) {
        case Motion::skipped:this->reached(step.aged_out?eu_aged_out:eu_skipped);break;
        case Motion::rested:this->reached(eu_rested);break;
        case Motion::still_landed:this->reached(eu_landed_moving);break;
        case Motion::moved:this->reached(eu_moved);break;
        }
        if(step.motion==Motion::skipped)
            this->reached(!step.rest_checked?eu_not_resting:step.rest_killed?eu_rest_killed:eu_rest_kept);
        if(step.rest_checked)this->reached(eu_rest_checked);
        if(!step.dead) {
            static constexpr unsigned grew[]{eu_x_grew,eu_y_grew,eu_z_grew,eu_x_max_grew,eu_y_max_grew,eu_z_max_grew};
            static constexpr unsigned kept[]{eu_x_kept,eu_y_kept,eu_z_kept,eu_x_max_kept,eu_y_max_kept,eu_z_max_kept};
            for(unsigned n=0;n<6;++n)this->reached(step.grew>>n&1u?grew[n]:kept[n]);
        } else this->reached(step.removed_first?eu_removed_first:eu_removed_linked);
    }
    // CMP EAX,-1 decided the flags; PUSH ECX (location); PUSH EDX (system);
    // CALL 2C40D0 with EAX = the first particle and EDI = the physics.
    void move_particles(std::uint32_t,std::uint32_t first,std::uint32_t system,std::uint32_t,std::uint32_t,
                        logic::EmitterKept &kept) {
        auto &gp=this->gp;
        this->flags.minus(first,none,4);
        gp[edx]=system;gp[ebx]=kept.mark;gp[ebp]=kept.bounds;
        this->push(0x250755u,gp[ecx]);this->push(0x250756u,gp[edx]);
        this->call(0x250757u,0x25075cu,0x2c40d0u,physics_child);
        kept.mark=gp[ebx];kept.bounds=gp[ebp];
        this->reached(eu_physics_moved);
    }
    // POP EBP; POP EBX; ADD ESP,18; RET 18.
    void leave_emitter_update(logic::Ending ending) {
        auto &gp=this->gp;
        using logic::Ending;
        switch(ending) {
        case Ending::idle:this->reached(eu_idle);break;
        case Ending::running:this->reached(eu_running);break;
        case Ending::no_deaths:this->reached(eu_no_deaths);break;
        case Ending::stopped:this->reached(eu_stopped);break;
        }
        gp[ebp]=this->pop(0x250776u);gp[ebx]=this->pop(0x250777u);
        gp[esp]=this->flags.plus(gp[esp],0x18u,4);
        this->ret(0x25077bu,0x18u);
    }

    void run() {
        auto &gp=this->gp;
        location_entry=gp[esp];
        Memory<ParticleLocation> memory{{*this}};
        logic::LocationKept kept{};
        kept.location=gp[ebx];kept.definition=gp[eax];
        const auto result=logic::run(memory,*this,kept);
        if(!result.emitters)this->reached(lu_no_emitters);
        if(!result.undrawn)this->reached(lu_drawn);
        else if(result.kept_undrawn)this->reached(lu_undrawn_kept);
        else this->reached(result.released?lu_released:lu_undrawn_empty);
    }
};

// ---- Admission and binding ------------------------------------------------
// The SSE arithmetic needs masked MXCSR exceptions and the x87 tail masked
// x87 exceptions; MMX must not own the x87 file. Neither body holds a live
// service PC.
template<class State,class Reference,class Interface,class Release,class Delete,class Physics,class Clear,class Method>
bool bridge_251a60(State &s,Reference reference,Interface interface,Release release,Delete erase,Physics physics,Clear clear,
                   Method method) {
    if(!native_effect_owner_detail::backend || !masked_controls())return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x250450u,0x251c7au,cursor,owner_reference(0x251a60u)))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return ParticleLocation<State,Interface,Release,Delete,Physics,Clear,Method>(v,c,t,interface,release,erase,physics,
                                                                                        clear,method);});
    return true;
}
} // namespace halo2::native_particle_update_detail
