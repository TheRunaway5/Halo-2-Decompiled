#pragma once
// Boundary adapter for the cluster distance flood, entered at 131470
// (native_tu15_cluster_distance_flood_systems.hpp). Its caller (131AC0, the
// campaign environment unit) is still translated, so the adapter owns that
// ABI and nothing else:
//   - EAX = the start cluster; [ESP+4] = the point, [ESP+8] = the output;
//     AX = the count of clusters reached (EAX is the frame's count, ECX the
//     next entry, EDX the queue position, as the original leaves them);
//     ADD ESP,454 decides the flags; EBX/EBP/ESI/EDI restored; RET 8;
//   - the 0x454-byte frame: the register saves, the visited set's REP STOSD
//     clear in the live direction flag, and the addresses of the flood's
//     locals, which follow the live stack pointer;
//   - XMM0 (XORPS, the 14DD70 result, the MOVSS reload of a kept distance)
//     and XMM7 (the limit, MOVSS) exactly as the original leaves them;
//   - the 14DD70 call at its original boundary: EAX = the BSP, ECX = the
//     portal, EDX = the point, CMP ESI,-1 flags, PortalWalk in EBP/EDI/ESI/
//     EBX, the result in XMM0;
//   - the cursor: each access publishes its original instruction.
// The proven leaf kernel (kernel_14dd70, native_tu15_matrix_leaves.hpp) runs
// 14DD70 inline on this state under the scene device binding's admission
// (x87 room for its two logical slots, masked x87 exceptions, no MMX, a
// literal cursor or the census diagnostic); otherwise the unit's own dispatch
// (the required owner round trip) runs it. The round trip saves and reloads
// the complete ABI unchanged under masked controls, so both routes leave the
// same state.
// HALO2_REFERENCE_CLUSTER_DISTANCE=1 restores the translated body.
// HALO2_REFERENCE_CLUSTER_DISTANCE_CHILD=1 keeps the native flood but enters
// 14DD70 through the dispatch round trip.
// HALO2_VERIFY_CLUSTER_DISTANCE=1 records the native run (14DD70 genuine,
// once) and replays the unmodified translated body against it, as in
// native_tu15_object_tick.hpp: same ordered accesses, stores, child entry ABI
// and final registers, flags, XMM, x87 and MXCSR.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_cluster_distance_flood_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_cluster_distance_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_cluster_distance;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_CLUSTER_DISTANCE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_CLUSTER_DISTANCE");return p && *p;}();
    return value;
}
inline bool routed_child_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_CLUSTER_DISTANCE_CHILD");return p && *p;}();
    return value;
}
// Branch outcomes (verification coverage), read from the order of the
// original instructions, the direction flag and the 14DD70 route.
enum Branch : unsigned {
    clear_up, clear_down, no_portals, portals, near_side, far_side, closed_portal, visited_nearer, visited_kept,
    appended, beyond_limit, more_portals, last_portal, more_entries, last_entry, inline_child, routed_child, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Cluster distance verify: %llu calls; %llu ordered accesses (%llu stores) and %llu child "
            "boundaries identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)children.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

// MOVSS xmm,m32: the bits in lane 0, the upper lanes zero.
inline __m128 load_ss(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t lane0(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
inline constexpr bool backend=logic::sse::available;

// 14DD70 inline: the leaf kernel on this state with the scene device
// binding's admission, outside the leaf reference/verify modes (the leaf
// verifier re-enters a translated body this unit does not hold).
template<class State> bool point_distance(State &v) {
    if(routed_child_mode() || native_tu15_leaves::verify_mode() || !native_tu15_leaves::x87_room(v,2u))return false;
    return native_tu15_leaves::run(v,[](auto &io,auto &m){native_tu15_leaves::kernel_14dd70(io,m);},
        [](auto &){throw std::runtime_error("Inline 14DD70 has no translated reference route");},0u);
}

inline constexpr Edge flood_edges[]{
    {0x131512u,0x1315ecu,no_portals},{0x131512u,0x13151cu,portals},
    {0x131547u,0x131554u,near_side},{0x131547u,0x1315cau,near_side},{0x131547u,0x13154bu,far_side},
    {0x131547u,0x1315cau,closed_portal},{0x13154bu,0x1315cau,closed_portal},
    {0x131577u,0x1315a5u,appended},{0x131577u,0x1315cau,beyond_limit},
    {0x13158au,0x131599u,visited_nearer},{0x13158au,0x131594u,visited_kept},
    {0x1315d7u,0x131529u,more_portals},{0x1315d7u,0x1315e1u,last_portal},
    {0x1315f0u,0x1314e1u,more_entries},{0x1315f0u,0x1315fcu,last_entry},
};
template<class State,class Dispatch> struct Flood : Port<State> {
    using Access=logic::cluster_distance::Access;
    using Local=logic::cluster_distance::Local;
    Dispatch &dispatch;
    std::uint32_t entry_sp{};bool frame_built{};
    Flood(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,flood_edges),dispatch(d){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case limit:return 0x131479u;
        case output_argument:this->push(0x131481u,gp[ebx]);return 0x131482u;
        case bsp:this->push(0x131489u,gp[ebp]);return 0x13148au;
        case position_clear:return 0x1314a3u;
        case start_index:return 0x1314a7u;
        case start_visited:return 0x1314bdu;
        case start_cluster:return 0x1314c5u;
        case start_visited_mark:return 0x1314c9u;
        case start_distance:return 0x1314cbu;
        case start_link:return 0x1314d0u;
        case count_start:return 0x1314d7u;
        case position_read:return 0x1314e1u;
        case clusters:return 0x1314e5u;
        case position_store:return 0x1314eeu;
        case entry_cluster:return 0x1314f2u;
        case portal_count:return 0x1314fau;
        case current_store:return 0x13150au;
        case next_store:return 0x13150eu;
        case portal_clear:return 0x131512u;
        case count_for_tail:return 0x13151cu;
        case current_reload:return 0x131529u;
        case portal_indices:return 0x131530u;
        case portal_index:return 0x131536u;
        case portals:return 0x13153au;
        case front:return 0x131540u;
        case entry_cluster_again:return 0x131547u;
        case back:return 0x13154bu;
        case point_argument:return 0x131554u;
        case visited:return 0x131577u;
        case entry_index:return 0x13157bu;
        case output_for_update:return 0x131580u;
        case visited_distance:return 0x13158au;
        case kept_distance:return 0x131594u;
        case visited_distance_store:return 0x131599u;
        case count_for_append:return 0x1315a5u;
        case entry_index_store:return 0x1315a9u;
        case count_store:return 0x1315afu;
        case visited_again:return 0x1315b3u;
        case append_cluster:return 0x1315b5u;
        case append_distance:return 0x1315b8u;
        case append_link:return 0x1315bdu;
        case visited_mark:return 0x1315c8u;
        case portal_read:return 0x1315cau;
        case portal_count_again:return 0x1315ceu;
        case portal_store:return 0x1315d7u;
        case next_reload:return 0x1315e1u;
        case output_reload:return 0x1315e5u;
        case position_check:return 0x1315ecu;
        case count_check:return 0x1315f0u;
        }
        return 0;
    }
    // Only the registers that reach the 14DD70 call or the exit are kept:
    // XMM7 (the limit), XMM0 (a kept distance), ECX (the next entry), EDX
    // (the queue position) and EAX (the count).
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case limit:this->s.xmm[7]=load_ss(v);break;
        case kept_distance:this->s.xmm[0]=load_ss(v);break;
        case next_reload:gp[ecx]=v;break;
        case position_check:gp[edx]=v;break;
        case count_check:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v){if(a==Access::next_store)this->gp[ecx]=v;}
    // Frame. After the four register saves, ESP is 0x464 below the entry:
    // the locals sit at [ESP+10..+20], the visited set at [ESP+24], the
    // entry indices at [ESP+64], the arguments at [ESP+468] and [ESP+46C].
    // The prologue reads the output argument before the frame is complete
    // (the original's [ESP+460] after PUSH EBX): the entry's [ESP+8].
    std::uint32_t local(Local l)const {
        const auto sp=this->gp[esp];
        switch(l) {
        case Local::count:return sp+0x10u;
        case Local::position:return sp+0x14u;
        case Local::portal:return sp+0x18u;
        case Local::current_entry:return sp+0x1cu;
        case Local::next_entry:return sp+0x20u;
        case Local::visited:return sp+0x24u;
        case Local::entry_indices:return sp+0x64u;
        case Local::point:return sp+0x468u;
        case Local::output:return frame_built?sp+0x46cu:entry_sp+8u;
        }
        return 0;
    }
    // PUSH ESI, PUSH EDI, then REP STOSD of 16 zero dwords from [ESP+24],
    // stepping in the direction the flag gives.
    void clear_visited() {
        auto &gp=this->gp;
        this->push(0x131490u,gp[esi]);this->push(0x131495u,gp[edi]);frame_built=true;
        const bool down=(this->s.other_flags&0x400u)!=0;
        this->reached(down?clear_down:clear_up);
        auto at=gp[esp]+0x24u;
        for(unsigned n=0;n<16;++n){this->store(0x1314a1u,at,4,0);at+=down?0xfffffffcu:4u;}
    }
    // The walk lives in EBP (BSP), EDI (cluster), ESI (far cluster) and EBX
    // (tail). MOV EAX,EBP; ECX = the portal; EDX = the point; CMP ESI,-1.
    std::uint32_t portal_distance(std::uint32_t portal,std::uint32_t point,logic::cluster_distance::PortalWalk &w) {
        auto &gp=this->gp;
        gp[ebp]=w.bsp;gp[edi]=w.cluster;gp[esi]=w.other;gp[ebx]=w.tail;
        gp[eax]=gp[ebp];gp[ecx]=portal;gp[edx]=point;this->compare32(gp[esi],logic::none);
        this->call(0x13155du,0x131562u,0x14dd70u,[&](State &v){
            if(point_distance(v))this->reached(inline_child);
            else {this->reached(routed_child);dispatch(v,0x14dd70u,0x13155du);}
        });
        w.bsp=gp[ebp];w.cluster=gp[edi];w.other=gp[esi];w.tail=gp[ebx];
        return lane0(this->s.xmm[0]);
    }
    void run() {
        auto &gp=this->gp;
        entry_sp=gp[esp];gp[esp]-=0x454u;
        this->s.xmm[0]=_mm_setzero_ps();  // XORPS XMM0,XMM0
        Memory<Flood> memory{*this};
        logic::cluster_distance::flood(memory,*this,gp[eax]);
        gp[edi]=this->pop(0x1315fcu);gp[esi]=this->pop(0x1315fdu);gp[ebp]=this->pop(0x1315feu);gp[ebx]=this->pop(0x1315ffu);
        gp[esp]=this->flags.plus(gp[esp],0x454u,4);
        this->ret(0x131606u,8);this->publish(0x131606u);
    }
};

// Admission also requires masked SSE exceptions (COMISS) and no active MMX.
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if((_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x131470u,0x131606u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Flood<State,Dispatch>(v,c,t,dispatch);});
    return true;
}
} // namespace halo2::native_cluster_distance_detail
