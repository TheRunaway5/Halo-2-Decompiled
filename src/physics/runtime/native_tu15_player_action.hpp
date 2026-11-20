#pragma once
// Boundary adapter for the player action search
// (native_tu15_player_action_systems.hpp), entered at 153BA0 in the combat
// action unit. Its callers and its children (the sphere query BD080 and the
// tests 1540D0, 153F20, 154270 and 1541B0, entered as the unit's direct
// bodies) are still translated TU1.5 code, so this adapter owns that ABI and
// nothing else:
//   - the arguments: [ESP+4] the player handle, [ESP+8] the action context;
//     no result; RET 8;
//   - the stack frame: SUB ESP,120 makes the Locals; ESI, EBX, EBP and EDI
//     are saved below them, in that order, interleaved with the frame's
//     first stores. Frame::local and Frame::argument give the addresses as
//     the original addresses them, from the stack pointer the last child
//     left;
//   - the original's register file at every child call: the sphere query's
//     seven pushed arguments (pushed between the reads that load them) with
//     the pass in ESI; a test's two pushed arguments with the scan in EBX
//     (count) and EBP (position), the object in ESI, the header datums in EDI
//     and the handle in EAX; the values each path leaves in scratch
//     registers and XMM0-XMM4 (the reach, the squared distance and its
//     parts); the flags of the instruction that decides each call (the type
//     mask's AND, the type's CMP ECX,8);
//   - the original's type case table (153DE0), read and mapped through the
//     unit's canonical branch mapping, so unmapped and unexpected targets
//     raise the translated body's faults;
//   - the exits: POP ESI (and EDI, EBP, EBX after the searches), ADD ESP,120
//     (its flags) and RET 8;
//   - the diagnostic cursor: each access publishes its original instruction
//     (the role -> instruction table below), and the RET publishes the exit;
//   - every store goes through the unit's store helper.
// Delete the adapter when the callers and the children go native.
// HALO2_REFERENCE_PLAYER_ACTION=1 restores the translated body.
// HALO2_VERIFY_PLAYER_ACTION=1 records the native run (children genuine,
// once) and replays the unmodified translated body against it, as in
// native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_player_action_systems.hpp"

namespace halo2::native_player_action_detail {
using native_object_tick_detail::Trace;
using native_object_tick_detail::sx8;using native_object_tick_detail::sx16;using native_object_tick_detail::si;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_player_action;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PLAYER_ACTION");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PLAYER_ACTION");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    no_unit, riding, nothing_found, ignored, unmeasured, out_of_reach, within_reach, beyond_table,
    test_unit, test_vehicle, test_weapon, test_control, table_attached, no_attached, attached_added, attached_full,
    another_object, another_pass, searched, branch_count
};
static_assert(branch_count<=64u);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Player action verify: %llu calls; %llu ordered accesses (%llu stores) and %llu child "
            "boundaries identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)children.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 load_ss(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}

// ---- 153BA0 -------------------------------------------------------------------
// Coverage: the branch outcome is the original instruction that follows
// (the rest is read from the deciding compares, below).
inline constexpr Edge action_edges[]{
    {0x153dc8u,0x153c56u,another_pass},{0x153dbeu,0x153dc8u,searched},
};
template<class State,class Branch_,class Search,class Unit,class Vehicle,class Weapon,class Control>
struct ActionSearch : Port<State> {
    using Access=logic::action_context::Access;
    using Case=logic::action_context::Case;
    using Local=logic::action_context::Local;
    using Argument=logic::action_context::Argument;
    Branch_ &branch;Search &search;Unit &unit;Vehicle &vehicle;Weapon &weapon;Control &control;
    std::uint32_t locals{},arguments{};   // the Locals and the caller's arguments as the original addresses them
    bool framed{},saved_edi{},walking{},attached_pending{};
    Case current_case{};
    ActionSearch(State &s,std::uint32_t *c,Trace<State> *t,Branch_ &b,Search &q,Unit &u,Vehicle &v,Weapon &w,Control &k)
        :Port<State>(s,c,t,action_edges),branch(b),search(q),unit(u),vehicle(v),weapon(w),control(k){}
    // The end of a found object: INC EBP; CMP EBP,EBX. An attached list
    // that stopped with an object left was full.
    void next_object() {
        auto &gp=this->gp;
        if(attached_pending){attached_pending=false;this->reached(attached_full);}
        gp[ebp]=this->flags.increment(gp[ebp],4,false);this->compare32(gp[ebp],gp[ebx]);
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case player_argument:return 0x153ba0u;
        case player_table:return 0x153ba4u;
        case player_rows:return 0x153baau;
        case context_argument:return 0x153badu;
        // AND EAX,FFFF; IMUL EAX,21C; ADD EAX,EDX (the player); SUB ESP,120;
        // PUSH ESI; XOR EDX,EDX; MOV ESI,ECX (the context).
        case context_clear_0:
            gp[eax]=this->flags.product(this->flags.logic(gp[eax]&0xffffu,4),logic::player_size);
            gp[eax]=this->flags.plus(gp[eax],gp[edx],4);
            gp[esp]=this->flags.minus(gp[esp],std::uint32_t(sizeof(logic::Locals)),4);
            locals=gp[esp];arguments=locals+std::uint32_t(sizeof(logic::Locals))+4u;framed=true;
            this->push(0x153bc4u,gp[esi]);gp[edx]=this->flags.logic(0,4);gp[esi]=gp[ecx];
            return 0x153bc9u;
        case context_clear_4:return 0x153bcbu;
        case context_clear_8:return 0x153bceu;
        case interaction_clear:return 0x153bd1u;
        case object_clear:return 0x153bd3u;
        case object_none:gp[edx]=this->flags.logic(logic::none,4);return 0x153bd9u;       // OR EDX,-1
        case melee_none:return 0x153bdcu;
        case player_unit:return 0x153bdfu;
        case object_table:return 0x153beau;
        case object_headers:return 0x153bf0u;
        // AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case unit_object:gp[eax]=this->flags.logic(gp[eax]&0xffffu,4)*3u;return 0x153bfbu;
        case unit_parent:return 0x153bffu;
        case unit_radius:return 0x153c08u;
        case pickup_margin:return 0x153c0du;
        // LEA EDX,[EAX+28] (the location); PUSH EBX; ADD EAX,30 (the center).
        case pickup_reach_store:
            gp[edx]=gp[eax]+std::uint32_t(offsetof(logic::ObjectBounds,location));this->push(0x153c18u,gp[ebx]);
            gp[eax]=this->flags.plus(gp[eax],std::uint32_t(offsetof(logic::ObjectBounds,center)),4);
            return 0x153c1cu;
        case pass_radius_store:return 0x153c22u;
        case search_margin:return 0x153c28u;
        case weapon_types_store:gp[esi]=this->flags.logic(0,4);this->push(0x153c32u,gp[ebp]);return 0x153c33u;  // XOR ESI,ESI; PUSH EBP
        case other_types_store:return 0x153c3bu;
        case search_radius_store:return 0x153c43u;
        case location_store:return 0x153c49u;
        case center_store:return 0x153c4du;
        case pass_clear:return 0x153c51u;
        case pass_radius:if(!saved_edi){saved_edi=true;this->push(0x153c55u,gp[edi]);}return 0x153c56u;   // PUSH EDI, once
        case center_reload:return 0x153c5au;
        // PUSH 40; LEA EAX,[ESP+34] (the found list); PUSH EAX.
        case location_reload:
            this->push(0x153c5eu,logic::found_limit);gp[eax]=gp[esp]+0x34u;this->push(0x153c64u,gp[eax]);
            return 0x153c65u;
        case pass_types:this->push(0x153c69u,gp[ecx]);return 0x153c6au;                      // PUSH ECX (the radius)
        case found_handle:if(walking){next_object();this->reached(another_object);}walking=true;return 0x153c90u;
        case found_table:return 0x153c94u;
        case found_headers:return 0x153c9au;
        // MOV ECX,EAX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2].
        case found_object:gp[ecx]=this->flags.logic(gp[eax]&0xffffu,4)*3u;return 0x153ca8u;
        case found_type:return 0x153cacu;
        case center_again:return 0x153cceu;
        case position_x:return 0x153cd2u;
        case center_x:return 0x153cd7u;
        case position_z:return 0x153cdbu;
        case center_z:return 0x153ce0u;
        case position_y:return 0x153ce5u;
        case center_y:return 0x153ceau;
        case object_radius:return 0x153cefu;
        case reach_reload:return 0x153cf4u;
        case test_context:
            return current_case==Case::weapon?0x153d33u:current_case==Case::control?0x153d4au:
                   current_case==Case::vehicle?0x153d61u:0x153d78u;
        case test_player:
            return current_case==Case::weapon?0x153d3au:current_case==Case::control?0x153d51u:
                   current_case==Case::vehicle?0x153d68u:0x153d7fu;
        case first_attached:return 0x153d8fu;
        case attached_append:attached_pending=false;this->compare32(gp[ebx],logic::found_limit);return 0x153d9cu;   // CMP EBX,40
        case attached_object:return 0x153da8u;
        case attached_sibling:return 0x153dacu;
        case pass_reload:next_object();return 0x153dbeu;
        // ADD ESI,4; CMP ESI,8.
        case pass_store:
            gp[esi]=this->flags.plus(gp[esi],4u,4);this->compare32(gp[esi],logic::pass_count_bytes);
            return 0x153dc8u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case player_argument:case location_reload:case found_handle:case attached_object:gp[eax]=v;break;
        case player_table:case context_argument:case object_table:case object_headers:case pass_radius:case pass_types:
        case test_player:
            gp[ecx]=v;
            break;
        case player_rows:case center_reload:case found_table:case center_again:case test_context:gp[edx]=v;break;
        case found_headers:gp[edi]=v;break;
        case found_object:case pass_reload:gp[esi]=v;break;
        // CMP EAX,EDX (NONE).
        case player_unit:gp[eax]=v;this->compare32(v,gp[edx]);if(v==logic::none)this->reached(no_unit);break;
        case unit_object:gp[eax]=v;break;
        case unit_parent:this->compare32(v,gp[edx]);if(v!=logic::none)this->reached(riding);break;
        // MOVSX ECX; MOV EDX,1; SHL EDX,CL; TEST EDX,1430; then TEST EDX,184.
        case found_type: {
            gp[ecx]=sx8(v);gp[edx]=this->flags.shift(1u,low_byte(gp[ecx]),4);
            this->flags.logic(gp[edx]&logic::ignored_types,4);
            if(gp[edx]&logic::ignored_types){this->reached(ignored);break;}
            this->flags.logic(gp[edx]&logic::reach_types,4);
            if(!(gp[edx]&logic::reach_types))this->reached(unmeasured);
            break;
        }
        // CMP EAX,-1.
        case first_attached:
            gp[eax]=v;this->compare32(v,logic::none);attached_pending=v!=logic::none;
            if(v==logic::none)this->reached(no_attached);
            break;
        // INC EBX; CMP EAX,-1.
        case attached_sibling:
            gp[eax]=v;gp[ebx]=this->flags.increment(gp[ebx],4,false);this->compare32(v,logic::none);
            attached_pending=v!=logic::none;
            break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // MOVSS XMM0 (radius + 0.4), then ADDSS 3.0: XMM0 is each stored value.
        case pickup_reach_store:case search_radius_store:this->s.xmm[0]=load_ss(v);break;
        // AND EAX,FFFF; LEA EDX,[EAX+EAX*2].
        case attached_append:
            this->reached(attached_added);
            gp[eax]=this->flags.logic(gp[eax]&0xffffu,4);gp[edx]=gp[eax]*3u;
            break;
        default:break;
        }
    }
    // Frame.
    std::uint32_t argument(Argument a)const {
        // Before SUB ESP,120 the arguments follow the return address.
        const auto first=framed?arguments:this->gp[esp]+4u;
        return first+(a==Argument::player?0u:4u);
    }
    std::uint32_t local(Local l)const {
        switch(l) {
        case Local::center:return locals+offsetof(logic::Locals,center);
        case Local::pass:return locals+offsetof(logic::Locals,pass);
        case Local::pickup_reach:return locals+offsetof(logic::Locals,pickup_reach);
        case Local::location:return locals+offsetof(logic::Locals,location);
        case Local::radii:return locals+offsetof(logic::Locals,radii);
        case Local::type_masks:return locals+offsetof(logic::Locals,type_masks);
        case Local::found:return locals+offsetof(logic::Locals,found);
        }
        return 0;
    }
    // After a call the four register saves are below the locals.
    void after_call(){locals=this->gp[esp]+0x10u;arguments=locals+std::uint32_t(sizeof(logic::Locals))+4u;}
    // PUSH EDX (center); PUSH EAX (location); AND ECX,FFFFEBCF; PUSH ECX;
    // PUSH 0; BD080. The pass lives in ESI. MOVSX EBX,AX; XOR EBP,EBP;
    // TEST EBX,EBX.
    std::uint32_t find_objects(logic::action_context::Pass &pass,std::uint32_t types,std::uint32_t,std::uint32_t,
                               std::uint32_t,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        gp[esi]=pass.offset;
        this->push(0x153c6eu,gp[edx]);this->push(0x153c6fu,gp[eax]);
        gp[ecx]=this->flags.logic(types,4);this->push(0x153c76u,gp[ecx]);this->push(0x153c77u,0);
        this->call(0x153c79u,0x153c7eu,0xbd080u,search);after_call();
        pass.offset=gp[esi];
        const auto found=gp[eax]&0xffffu;
        gp[ebx]=sx16(found);gp[ebp]=this->flags.logic(0,4);this->test32(gp[ebx]);
        walking=false;
        if(si(gp[ebx])<=0)this->reached(nothing_found);
        return found;
    }
    // MOVSS/SUBSS/MULSS/ADDSS leave the reach in XMM0, its square in XMM1,
    // the parts in XMM2 (y) and XMM3 (z), the squared distance in XMM4;
    // COMISS XMM1,XMM4.
    void reach_measured(const logic::action_context::Reach &r) {
        auto &x=this->s.xmm;
        x[0]=load_ss(r.reach);x[1]=load_ss(r.reach_squared);x[2]=load_ss(r.dy);x[3]=load_ss(r.dz);
        x[4]=load_ss(r.distance_squared);
        this->flags.scalar_compare(x[1],x[4],false);
        this->reached(this->flags.cf?out_of_reach:within_reach);
    }
    // CMP ECX,8; JA (the default); JMP [153DE0+ECX*4].
    Case action_case(std::uint32_t type) {
        this->compare32(type,8u);
        if(type>8u){this->reached(beyond_table);current_case=Case::attached;return current_case;}
        const auto target=branch(this->read(0x153d2cu,0x153de0u+type*4u,4),this->s.image);
        switch(target) {
        case 0x153d78u:current_case=Case::unit;break;
        case 0x153d61u:current_case=Case::vehicle;break;
        case 0x153d33u:current_case=Case::weapon;break;
        case 0x153d4au:current_case=Case::control;break;
        case 0x153d8fu:current_case=Case::attached;this->reached(table_attached);break;
        default:throw std::runtime_error("Required original solver dispatch target unavailable");
        }
        return current_case;
    }
    // PUSH EDX (context); PUSH ECX (player); the test. The scan lives in EBX
    // (count) and EBP (position).
    void test(Case c,logic::action_context::Scan &scan,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        gp[ebx]=scan.count;gp[ebp]=scan.index;
        switch(c) {
        case Case::weapon:
            this->push(0x153d41u,gp[edx]);this->push(0x153d42u,gp[ecx]);this->call(0x153d43u,0x153d48u,0x154270u,weapon);
            this->reached(test_weapon);break;
        case Case::control:
            this->push(0x153d58u,gp[edx]);this->push(0x153d59u,gp[ecx]);this->call(0x153d5au,0x153d5fu,0x1541b0u,control);
            this->reached(test_control);break;
        case Case::vehicle:
            this->push(0x153d6fu,gp[edx]);this->push(0x153d70u,gp[ecx]);this->call(0x153d71u,0x153d76u,0x153f20u,vehicle);
            this->reached(test_vehicle);break;
        case Case::unit:
            this->push(0x153d86u,gp[edx]);this->push(0x153d87u,gp[ecx]);this->call(0x153d88u,0x153d8du,0x1540d0u,unit);
            this->reached(test_unit);break;
        case Case::attached:break;
        }
        after_call();
        scan.count=gp[ebx];scan.index=gp[ebp];
    }
    void run() {
        auto &gp=this->gp;
        Memory<ActionSearch> memory{*this};
        const auto exit=logic::action_context::find(memory,*this);
        if(exit==logic::action_context::Exit::searched) {
            gp[edi]=this->pop(0x153dd2u);gp[ebp]=this->pop(0x153dd3u);gp[ebx]=this->pop(0x153dd4u);
        }
        gp[esi]=this->pop(0x153dd5u);
        gp[esp]=this->flags.plus(gp[esp],std::uint32_t(sizeof(logic::Locals)),4);         // ADD ESP,120
        this->ret(0x153ddcu,8);this->publish(0x153ddcu);
    }
};

template<class State,class Reference,class Branch_,class Search,class Unit,class Vehicle,class Weapon,class Control>
bool bridge(State &s,Reference reference,Branch_ branch,Search search,Unit unit,Vehicle vehicle,Weapon weapon,Control control) {
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x153ba0u,0x153ddcu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return ActionSearch<State,Branch_,Search,Unit,Vehicle,Weapon,Control>(v,c,t,branch,search,unit,vehicle,weapon,control);});
    return true;
}
} // namespace halo2::native_player_action_detail
