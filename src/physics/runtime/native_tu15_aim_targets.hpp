#pragma once
// Boundary adapter for the aim target chain, entered at 1AE2B0
// (native_tu15_aim_targets_systems.hpp). Its caller (the weapon targeting
// search 1AE090), its children 1AE500 and 1AE7F0 and, through the bound
// route, itself are entered with the original ABI, which the adapter owns:
//   - [ESP+4..+2C] the eleven arguments; RET 2C;
//   - the frame: PUSH ECX (the local slot), PUSH EBX, PUSH EBP, PUSH ESI,
//     PUSH EDI between the first loads, as the original places them; the
//     arguments are addressed from the live ESP, so a child that leaves the
//     stack unbalanced moves them as it does for the original;
//   - each child gets the original's registers (EAX/ECX/EDX as the loads
//     before the call leave them), flags (the COMISS of the cone test, TEST
//     AL,AL, CMP EAX,-1) and XMM (the aim test's values), and every register
//     it returns comes back to the walk;
//   - the exit: EAX = EDI (the count), ECX = the local slot (the current
//     object's data), EDX as the walk or the last child left it, the flags
//     of the CMP that ended the walk; XMM0-XMM5 as the last aim test left
//     them (lane 0; the upper lanes zero) unless a child ran after it;
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_AIM_TARGETS=1 restores the translated body.
// HALO2_VERIFY_AIM_TARGETS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp:
// same ordered accesses, stores, child entries and final registers, flags,
// XMM, x87 and MXCSR.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_aim_targets_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_aim_targets_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_aim_targets;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_AIM_TARGETS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_AIM_TARGETS");return p && *p;}();
    return value;
}
// Branch outcomes (verification coverage), read from the order of the
// original instructions.
enum Branch : unsigned {
    other_type, candidate_type, header_skipped, header_clear, object_skipped, machine, unit,
    unit_over_limit, unit_under_limit, behind, ahead, beyond, within_reach, outside_cone, inside,
    not_target, target, full_after_add, room_after_add, no_children, children, full_after_children,
    room_after_children, next_sibling, chain_end, branch_count
};
static_assert(branch_count<=64u);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Aim targets verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

// MOVSS xmm,m32, the scalar ops and the register copies leave the upper
// lanes zero here.
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr bool backend=logic::sse::available;

// Coverage edges: an access at `to` right after the access at `from` (a
// child's own accesses do not move `from`: its CALL's push stays it).
inline constexpr Edge walk_edges[]{
    {0x1ae2edu,0x1ae477u,other_type},{0x1ae2edu,0x1ae300u,candidate_type},
    {0x1ae300u,0x1ae477u,header_skipped},{0x1ae300u,0x1ae30au,header_clear},
    {0x1ae30eu,0x1ae477u,object_skipped},{0x1ae30eu,0x1ae332u,machine},{0x1ae30eu,0x1ae31du,unit},
    {0x1ae325u,0x1ae477u,unit_over_limit},{0x1ae325u,0x1ae332u,unit_under_limit},
    {0x1ae37cu,0x1ae477u,behind},{0x1ae37cu,0x1ae397u,ahead},
    {0x1ae397u,0x1ae477u,beyond},{0x1ae397u,0x1ae3a9u,within_reach},
    {0x1ae3e7u,0x1ae477u,outside_cone},{0x1ae3e7u,0x1ae3fau,inside},
    {0x1ae40au,0x1ae471u,not_target},{0x1ae40au,0x1ae413u,target},
    {0x1ae42au,0x1ae489u,full_after_add},{0x1ae42au,0x1ae436u,room_after_add},
    {0x1ae43au,0x1ae471u,no_children},{0x1ae43au,0x1ae442u,children},
    {0x1ae465u,0x1ae489u,full_after_children},{0x1ae465u,0x1ae471u,room_after_children},
    {0x1ae47bu,0x1ae2cdu,next_sibling},{0x1ae47bu,0x1ae489u,chain_end},
};
template<class State,class Target,class Add,class Chain> struct Walk : Port<State> {
    using Access=logic::aim_targets::Access;
    using Local=logic::aim_targets::Local;
    using Kept=logic::aim_targets::Kept;
    Target &target;Add &add;Chain &chain;
    // E: where the return address sits for the original's addressing. The
    // five saves put ESP at E-14 inside the walk; after a child, E follows
    // the ESP the child returned.
    std::uint32_t frame{};
    Walk(State &s,std::uint32_t *c,Trace<State> *t,Target &e,Add &a,Chain &h)
        :Port<State>(s,c,t,walk_edges),target(e),add(a),chain(h){}
    std::uint32_t local(Local l)const {
        if(l==Local::current)return frame-4u;
        return frame+4u*std::uint32_t(l);         // search is [E+4], ..., add_context [E+2C]
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case first_handle:this->push(0x1ae2b0u,gp[ecx]);this->push(0x1ae2b1u,gp[ebx]);return 0x1ae2b2u;
        case headers:this->push(0x1ae2b6u,gp[ebp]);return 0x1ae2b7u;
        case count_argument:this->push(0x1ae2bdu,gp[esi]);this->push(0x1ae2beu,gp[edi]);return 0x1ae2bfu;
        // MOV EAX,EBX; AND EAX,FFFF; LEA ESI,[EAX+EAX*2]; (the load); SHL ESI,2.
        case header_data:gp[eax]=this->flags.logic(gp[ebx]&0xffffu,4);gp[esi]=gp[eax]*3u;return 0x1ae2cdu;
        case object_data:gp[esi]=this->flags.shift(gp[esi],2u,4);return 0x1ae2d3u;
        case result_clear:return 0x1ae2d7u;
        case header_data_again:return 0x1ae2dcu;
        case object_data_again:return 0x1ae2dfu;
        case current_store:return 0x1ae2e5u;
        case object_store:return 0x1ae2e9u;
        case type:return 0x1ae2edu;
        case header_flags:return 0x1ae300u;
        case current_reload:return 0x1ae30au;
        case object_flags:return 0x1ae30eu;
        case unit_limit:return 0x1ae31du;
        case unit_value:return 0x1ae325u;
        case header_data_sphere:return 0x1ae332u;
        case object_data_sphere:return 0x1ae335u;
        case radius:return 0x1ae339u;
        case center_x:return 0x1ae33eu;
        case point_argument:return 0x1ae343u;
        case center_y:return 0x1ae347u;
        case center_z:return 0x1ae34cu;
        case direction_argument:return 0x1ae351u;
        case point_y:return 0x1ae355u;
        case point_z:return 0x1ae35au;
        case direction_y:return 0x1ae35fu;
        case direction_z:return 0x1ae364u;
        case point_x:return 0x1ae369u;
        case direction_x:return 0x1ae37cu;
        case max_distance:return 0x1ae397u;
        case width:return 0x1ae3a9u;
        case cone_slope:return 0x1ae3afu;
        case spread:return 0x1ae3e7u;
        case test_second:return 0x1ae3fau;
        case test_first:return 0x1ae3feu;
        case add_context:return 0x1ae413u;
        case add_point:return 0x1ae417u;
        case add_result:return 0x1ae41bu;
        // PUSH EBP, EDI, ESI, ECX (point), EDX (the result slot's word), EBX.
        case add_search:
            this->push(0x1ae41fu,gp[ebp]);this->push(0x1ae420u,gp[edi]);this->push(0x1ae421u,gp[esi]);
            this->push(0x1ae422u,gp[ecx]);this->push(0x1ae423u,gp[edx]);this->push(0x1ae424u,gp[ebx]);
            return 0x1ae425u;
        case object_for_children:return 0x1ae436u;
        case first_child:return 0x1ae43au;
        case recurse_second:return 0x1ae442u;
        case recurse_first:return 0x1ae446u;
        // The recursion's pushes interleave with its loads.
        case recurse_spread:
            this->push(0x1ae44au,gp[ebp]);this->push(0x1ae44bu,gp[edi]);this->push(0x1ae44cu,gp[ecx]);return 0x1ae44du;
        case recurse_width:this->push(0x1ae451u,gp[edx]);return 0x1ae452u;
        case recurse_distance:this->push(0x1ae456u,gp[ecx]);return 0x1ae457u;
        case recurse_point:this->push(0x1ae45bu,gp[edx]);return 0x1ae45cu;
        case headers_reload:return 0x1ae471u;
        case object_for_next:return 0x1ae477u;
        case next_sibling:return 0x1ae47bu;
        }
        return 0;
    }
    // The registers each load writes, and the flags of the instructions
    // right after it.
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case first_handle:gp[ebx]=v;break;
        case headers:case add_context:case headers_reload:gp[ebp]=v;break;
        case count_argument:gp[edi]=v;break;
        case header_data:case header_data_again:case header_data_sphere:case current_reload:case point_argument:
        case test_first:case object_for_children:case object_for_next:
            gp[eax]=v;break;
        case object_data:case add_point:case recurse_second:case recurse_spread:case recurse_distance:gp[ecx]=v;break;
        // MOV EDX,[...]; ADD EAX,ESI (the header).
        case object_data_again:gp[edx]=v;gp[eax]=this->flags.plus(gp[eax],gp[esi],4);break;
        // MOV CL,[EAX+3]; MOV EDX,1; SHL EDX,CL; TEST DL,83.
        case type:
            gp[ecx]=(gp[ecx]&0xffffff00u)|(v&0xffu);gp[edx]=this->flags.shift(1u,gp[ecx]&0xffu,4);
            this->test8(gp[edx]&logic::object_type::candidates);break;
        case header_flags:this->test8(v&0x10u);break;
        // TEST [EAX+4],1; then (when clear) TEST DL,3.
        case object_flags:this->test8(v&1u);if(!(v&1u))this->test8(gp[edx]&logic::object_type::units);break;
        case unit_limit:x[0]=lane0_only(v);break;
        case unit_value:this->flags.scalar_compare(x[0],lane0_only(v),false);break;
        case object_data_sphere:case direction_argument:gp[esi]=v;break;
        case test_second:case add_result:case recurse_first:case recurse_width:case recurse_point:gp[edx]=v;break;
        case add_search:case next_sibling:gp[ebx]=v;if(a==next_sibling)this->compare32(v,logic::none);break;
        case first_child:gp[eax]=v;this->compare32(v,logic::none);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The registers the aim test leaves: XMM2-XMM4 the offset, XMM1 the
    // radius (or the spread bound), XMM0 the distance along the ray (or the
    // spread), XMM5 the bound of the comparison that decided.
    void tested(const logic::aim_targets::SphereTest &t) {
        auto &x=this->s.xmm;
        using logic::aim_targets::Stage;
        x[2]=lane0_only(t.dx);x[3]=lane0_only(t.dy);x[4]=lane0_only(t.dz);
        switch(t.result) {
        case Stage::behind:
            x[0]=lane0_only(t.along);x[1]=lane0_only(t.radius);x[5]=lane0_only(t.negative_radius);
            this->flags.scalar_compare(x[0],x[5],false);break;
        case Stage::beyond:
            x[0]=lane0_only(t.along);x[1]=lane0_only(t.radius);x[5]=lane0_only(t.reach);
            this->flags.scalar_compare(x[5],x[0],false);break;
        case Stage::outside_cone:case Stage::inside:
            x[0]=lane0_only(t.spread);x[1]=lane0_only(t.spread_bound);x[5]=lane0_only(t.cone_bound);
            this->flags.scalar_compare(x[5],x[1],false);break;
        }
    }
    void returned(Kept &kept) {
        frame=this->gp[esp]+0x14u;
        kept={this->gp[ebx],this->gp[esi],this->gp[edi],this->gp[ebp]};
    }
    // LEA ECX,[ESP+3C] (the count slot); PUSH ECX, EDX, EAX, EBX; 1AE500;
    // TEST AL,AL.
    bool is_target(Kept &kept) {
        auto &gp=this->gp;
        gp[ecx]=local(Local::count);
        this->push(0x1ae406u,gp[ecx]);this->push(0x1ae407u,gp[edx]);this->push(0x1ae408u,gp[eax]);
        this->push(0x1ae409u,gp[ebx]);
        this->call(0x1ae40au,0x1ae40fu,0x1ae500u,target);returned(kept);
        this->test8(gp[eax]);
        return (gp[eax]&0xffu)!=0;
    }
    // PUSH EBX (the search); 1AE7F0; MOV EDI,EAX; CMP EDI,40.
    std::uint32_t add_target(Kept &kept) {
        auto &gp=this->gp;
        this->push(0x1ae429u,gp[ebx]);
        this->call(0x1ae42au,0x1ae42fu,0x1ae7f0u,add);
        gp[edi]=gp[eax];returned(kept);
        this->compare32(gp[edi],logic::target_limit);
        return gp[edi];
    }
    // PUSH ECX (distance), ESI, EDX (point), EAX (the child), EBX; 1AE2B0;
    // MOV EDI,EAX; CMP EDI,40.
    std::uint32_t attached(Kept &kept) {
        auto &gp=this->gp;
        this->push(0x1ae460u,gp[ecx]);this->push(0x1ae461u,gp[esi]);this->push(0x1ae462u,gp[edx]);
        this->push(0x1ae463u,gp[eax]);this->push(0x1ae464u,gp[ebx]);
        this->call(0x1ae465u,0x1ae46au,0x1ae2b0u,chain);
        gp[edi]=gp[eax];returned(kept);
        this->compare32(gp[edi],logic::target_limit);
        return gp[edi];
    }
    void run() {
        auto &gp=this->gp;
        frame=gp[esp];
        Memory<Walk> memory{*this};
        logic::aim_targets::evaluate_chain(memory,*this);
        // MOV EAX,EDI; POP EDI, ESI, EBP, EBX, ECX (the local); RET 2C.
        gp[eax]=gp[edi];
        gp[edi]=this->pop(0x1ae489u);gp[esi]=this->pop(0x1ae48au);gp[ebp]=this->pop(0x1ae48bu);
        gp[ebx]=this->pop(0x1ae48cu);gp[ecx]=this->pop(0x1ae48du);
        this->ret(0x1ae48eu,0x2cu);this->publish(0x1ae48eu);
    }
};

// Admission also requires masked SSE exceptions (COMISS) and no active MMX.
template<class State,class Reference,class Target,class Add,class Chain>
bool bridge(State &s,Reference reference,Target target,Add add,Chain chain) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if((_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1ae2b0u,0x1ae48eu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Walk<State,Target,Add,Chain>(v,c,t,target,add,chain);});
    return true;
}
} // namespace halo2::native_aim_targets_detail
