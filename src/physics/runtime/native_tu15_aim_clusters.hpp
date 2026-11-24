#pragma once
// Boundary adapter for the aim cluster flood, entered at 14D200
// (native_tu15_aim_clusters_systems.hpp). Its caller (the weapon targeting
// search 1AE090, combat action unit) is still translated, so the adapter
// owns that ABI and nothing else:
//   - DX = the start cluster, XMM5 = the cone spread; [ESP+4] the point,
//     [ESP+8] the direction, [ESP+C] the max distance, [ESP+10] the width,
//     [ESP+14] the capacity, [ESP+18] the output, [ESP+1C] the count output;
//     RET 1C;
//   - the 0x410-byte frame: SUB ESP,410, then EBX, EBP and ESI saved, EDI
//     saved just before the stamp store; the locals and the stack follow
//     the entry stack pointer;
//   - the exit: EAX = the reached count with AX clamped to the capacity
//     (CMP AX,CX / JLE / MOV AX,CX), ECX = the capacity (32 bits), EDX as
//     the walk leaves it (the BSP, then the portal table with the last far
//     side in DX), EDI/ESI/EBP restored before the active byte is cleared
//     and EBX after it, ADD ESP,410 decides the flags; XMM0-XMM4 and XMM6
//     as the last portal test leaves them (lane 0; the upper lanes zero);
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_AIM_CLUSTERS=1 restores the translated body.
// HALO2_VERIFY_AIM_CLUSTERS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp:
// same ordered accesses, stores and final registers, flags, XMM, x87 and
// MXCSR.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_aim_clusters_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_aim_clusters;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_AIM_CLUSTERS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_AIM_CLUSTERS");return p && *p;}();
    return value;
}
// Branch outcomes (verification coverage), read from the order of the
// original instructions.
enum Branch : unsigned {
    start_unvisited, start_visited, output_room, output_full, portals_here, no_portals, front_side, back_side,
    far_unvisited, far_visited, behind, beyond, outside_cone, inside, more_portals, last_portal,
    more_stacked, stack_empty, count_reported, count_not_reported, clamped, within_capacity, branch_count
};
static_assert(branch_count<=64u);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Aim clusters verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

// MOVSS xmm,m32 and the scalar ops leave the upper lanes zero here.
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr bool backend=logic::sse::available;

// Coverage edges: an access at `to` right after the access at `from`.
inline constexpr Edge flood_edges[]{
    {0x14d227u,0x14d230u,start_unvisited},{0x14d227u,0x14d237u,start_visited},
    {0x14d260u,0x14d26au,output_room},{0x14d260u,0x14d279u,output_full},
    {0x14d28eu,0x14d298u,portals_here},{0x14d28eu,0x14d240u,no_portals},{0x14d28eu,0x14d3dau,no_portals},
    {0x14d2bcu,0x14d2ccu,front_side},{0x14d2bcu,0x14d2c8u,back_side},
    {0x14d2d4u,0x14d2e1u,far_unvisited},{0x14d2d4u,0x14d3b7u,far_visited},
    {0x14d327u,0x14d3b7u,behind},{0x14d342u,0x14d3b7u,beyond},
    {0x14d35cu,0x14d3b7u,outside_cone},{0x14d35cu,0x14d3a1u,inside},
    {0x14d3c3u,0x14d2b0u,more_portals},{0x14d3c3u,0x14d3cdu,last_portal},
    {0x14d3cdu,0x14d240u,more_stacked},{0x14d3cdu,0x14d3dau,stack_empty},
    {0x14d28eu,0x14d240u,more_stacked},{0x14d28eu,0x14d3dau,stack_empty},
    {0x14d3edu,0x14d3f0u,count_reported},{0x14d3edu,0x14d3f3u,count_not_reported},
};
template<class State> struct Flood : Port<State> {
    using Access=logic::aim_clusters::Access;
    using Local=logic::aim_clusters::Local;
    std::uint32_t entry_sp{};bool ebx_restored{};
    Flood(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,flood_edges){}
    void restore_ebx(){if(!ebx_restored){ebx_restored=true;this->gp[ebx]=this->pop(0x14d3edu);}}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case stamp_read:return 0x14d209u;
        case stamp_store:this->push(0x14d21au,gp[edi]);return 0x14d21bu;
        case active_set:return 0x14d221u;
        case start_stamp:return 0x14d227u;
        case start_mark:return 0x14d230u;
        case stack_start:return 0x14d237u;
        case bsp:return 0x14d240u;
        case clusters:return 0x14d246u;
        case stack_pop:return 0x14d250u;
        case capacity_check:return 0x14d260u;
        case output_argument:return 0x14d26au;
        case output_store:return 0x14d274u;
        case portal_count:return 0x14d279u;
        case reached_store:return 0x14d282u;
        case portal_clear:return 0x14d286u;
        case portal_count_store:return 0x14d28eu;
        case portals:return 0x14d298u;
        case portal_indices:return 0x14d29bu;
        case portals_store:return 0x14d2a3u;
        case portal_index:return 0x14d2b0u;
        case portals_reload:return 0x14d2b8u;
        case front:return 0x14d2bcu;
        case back:return 0x14d2c8u;
        case stamp_check:return 0x14d2ccu;
        case other_stamp:return 0x14d2d4u;
        case point_argument:return 0x14d2e1u;
        case center_x:return 0x14d2e8u;
        case point_x:return 0x14d2edu;
        case center_y:return 0x14d2f1u;
        case point_y:return 0x14d2f6u;
        case center_z:return 0x14d2fbu;
        case point_z:return 0x14d300u;
        case direction_argument:return 0x14d305u;
        case direction_y:return 0x14d30cu;
        case direction_z:return 0x14d311u;
        case radius:return 0x14d316u;
        case direction_x:return 0x14d327u;
        case max_distance:return 0x14d342u;
        case width:return 0x14d353u;
        case cone_slope:return 0x14d35cu;
        case stamp_mark:return 0x14d3a1u;
        case other_mark:return 0x14d3aau;
        case stack_push:return 0x14d3b1u;
        case portal_read:return 0x14d3b7u;
        case portal_store:return 0x14d3bfu;
        case portal_bound:return 0x14d3c3u;
        case reached_reload:return 0x14d3cdu;
        case count_output_argument:return 0x14d3dau;
        // POP EDI, POP ESI, POP EBP, then the active byte is cleared; POP
        // EBX before the count is reported (or not).
        case active_clear:
            gp[edi]=this->pop(0x14d3e3u);gp[esi]=this->pop(0x14d3e4u);gp[ebp]=this->pop(0x14d3e5u);
            return 0x14d3e6u;
        case count_store:restore_ebx();return 0x14d3f0u;
        case capacity_result:restore_ebx();return 0x14d3f3u;
        }
        return 0;
    }
    // EDX is the only scratch register the walk leaves for the caller: the
    // BSP, then the portal table, then (DX) the far side of the last portal.
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case bsp:case portals:gp[edx]=v;break;
        case front:case back:gp[edx]=(gp[edx]&0xffff0000u)|(v&0xffffu);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // Frame. E is the stack pointer at entry (the return address); after
    // SUB ESP,410 and the four saves the locals are [ESP+10..+1F], the stack
    // [ESP+20] (to E), the arguments [E+4..+1C].
    std::uint32_t local(Local l)const {
        switch(l) {
        case Local::portal:return entry_sp-0x410u;
        case Local::portal_count:return entry_sp-0x40cu;
        case Local::reached:return entry_sp-0x408u;
        case Local::portals:return entry_sp-0x404u;
        case Local::stack:return entry_sp-0x400u;
        case Local::point:return entry_sp+0x4u;
        case Local::direction:return entry_sp+0x8u;
        case Local::max_distance:return entry_sp+0xcu;
        case Local::width:return entry_sp+0x10u;
        case Local::capacity:return entry_sp+0x14u;
        case Local::output:return entry_sp+0x18u;
        case Local::count_output:return entry_sp+0x1cu;
        }
        return 0;
    }
    // The registers each portal test leaves: XMM2-XMM4 the offset, XMM1 the
    // radius (or dz^2), XMM0 the dot product (or the spread bound), XMM6 the
    // bound of the comparison that decided.
    void tested(const logic::aim_clusters::PortalTest &t) {
        auto &x=this->s.xmm;
        using logic::aim_clusters::Test;
        x[2]=lane0_only(t.dx);x[3]=lane0_only(t.dy);x[4]=lane0_only(t.dz);
        switch(t.result) {
        case Test::behind:x[0]=lane0_only(t.along);x[1]=lane0_only(t.radius);x[6]=lane0_only(t.negative_radius);break;
        case Test::beyond:x[0]=lane0_only(t.along);x[1]=lane0_only(t.radius);x[6]=lane0_only(t.reach);break;
        case Test::outside_cone:case Test::inside:
            x[0]=lane0_only(t.spread_bound);x[1]=lane0_only(t.dz_squared);x[6]=lane0_only(t.cone_bound);break;
        }
    }
    void run() {
        auto &gp=this->gp;
        entry_sp=gp[esp];
        gp[esp]=this->flags.minus(gp[esp],0x410u,4);                               // SUB ESP,410
        this->push(0x14d206u,gp[ebx]);this->push(0x14d207u,gp[ebp]);this->push(0x14d208u,gp[esi]);
        Memory<Flood> memory{*this};
        const auto spread=std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(this->s.xmm[5])));
        const auto result=logic::aim_clusters::flood(memory,*this,gp[edx]&0xffffu,spread);
        // CMP AX,CX; JLE; MOV AX,CX: the count, clamped to the capacity.
        gp[ecx]=result.capacity;gp[eax]=result.reached;
        if(std::int16_t(result.reached)>std::int16_t(result.capacity)) {
            this->reached(clamped);gp[eax]=(gp[eax]&0xffff0000u)|(result.capacity&0xffffu);
        } else this->reached(within_capacity);
        gp[esp]=this->flags.plus(gp[esp],0x410u,4);                                // ADD ESP,410
        this->ret(0x14d408u,0x1cu);this->publish(0x14d408u);
    }
};

// Admission also requires masked SSE exceptions (COMISS) and no active MMX.
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if((_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x14d200u,0x14d408u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Flood<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_aim_clusters_detail
