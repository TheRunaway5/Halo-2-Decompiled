#pragma once
// Boundary adapter for the weapon targeting search, entered at 1AE090
// (native_tu15_aim_search_systems.hpp). Its caller and children keep the
// original ABI, which the adapter owns:
//   - [ESP+4..+1C] the seven arguments; RET 1C;
//   - the frame: MOV EAX,2410 and the stack probe 32A660 (through the unit's
//     route), PUSH EBP, PUSH EDI (and XOR EDI,EDI) between the first loads,
//     PUSH EBX/ESI before the flood's arguments; locals and arguments are
//     addressed from the live ESP, so a child that leaves the stack
//     unbalanced moves them as it does for the original;
//   - each child gets the original's registers, flags and XMM (XMM0 zero,
//     XMM1 the angle, XMM2 the range, XMM5 the cosine for the flood), and
//     every register it returns comes back to the search;
//   - the x87: each FLD m32 / FSIN or FCOS / FSTP m32 pushes and pops the
//     logical stack, clears C1 on the load, keeps C1/C2 of the function and
//     C1 of the store;
//   - the exit: EAX = EDI, ESI/EBX restored unless the search closed early,
//     EDI/EBP restored, ADD ESP,2410 decides the flags; ECX/EDX as the last
//     load or child left them;
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_AIM_SEARCH=1 restores the translated body.
// HALO2_VERIFY_AIM_SEARCH=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_aim_search_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_aim_search_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_aim_search;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_AIM_SEARCH");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_AIM_SEARCH");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    range_middle_larger, range_last_larger, range_first_not_larger, range_first_larger, range_middle_third,
    range_last_again, angle_middle_larger, angle_last_larger, angle_first_not_larger, angle_first_larger,
    angle_middle_third, angle_last_again, closed, open, players_done, player, null_record, record, no_unit, unit,
    has_parent, root, marked, unmarked, no_objects, objects, full, room, more_objects, objects_done, branch_count
};
static_assert(branch_count<=64u);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Aim search verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr bool backend=logic::sse::available && logic::x87::available;

// Coverage edges: an access at `to` right after the access at `from` (a
// child's own accesses do not move `from`: its CALL's push stays it).
inline constexpr Edge search_edges[]{
    {0x1ae0aau,0x1ae0b2u,range_middle_larger},{0x1ae0aau,0x1ae0b7u,range_last_larger},
    {0x1ae0b7u,0x1ae0c6u,range_first_not_larger},{0x1ae0b7u,0x1ae0dbu,range_first_larger},
    {0x1ae0cbu,0x1ae0d6u,range_middle_third},{0x1ae0cbu,0x1ae0dbu,range_last_again},
    {0x1ae0e4u,0x1ae0eeu,angle_middle_larger},{0x1ae0e4u,0x1ae0f3u,angle_last_larger},
    {0x1ae0f3u,0x1ae0ffu,angle_first_not_larger},{0x1ae0f3u,0x1ae11au,angle_first_larger},
    {0x1ae104u,0x1ae10fu,angle_middle_third},{0x1ae104u,0x1ae11au,angle_last_again},
    {0x1ae11au,0x1ae296u,closed},{0x1ae11au,0x1ae12fu,open},
    {0x1ae1b6u,0x1ae225u,players_done},{0x1ae1b6u,0x1ae1c0u,player},
    {0x1ae1c3u,0x1ae225u,null_record},{0x1ae1c3u,0x1ae1cfu,record},
    {0x1ae1cfu,0x1ae1b6u,no_unit},{0x1ae1cfu,0x1ae1d7u,unit},
    {0x1ae1eeu,0x1ae1eau,has_parent},{0x1ae1eeu,0x1ae1f6u,root},
    {0x1ae203u,0x1ae1b6u,marked},{0x1ae203u,0x1ae20cu,unmarked},
    {0x1ae22du,0x1ae292u,no_objects},{0x1ae22du,0x1ae236u,objects},
    {0x1ae27du,0x1ae292u,full},{0x1ae27du,0x1ae289u,room},
    {0x1ae289u,0x1ae240u,more_objects},{0x1ae289u,0x1ae292u,objects_done},
};
template<class State,class Route,class Flood,class Query,class Next,class Chain> struct Search : Port<State> {
    using Access=logic::aim_search::Access;
    using Local=logic::aim_search::Local;
    using Kept=logic::aim_search::Kept;
    Route &route;Flood &flood_child;Query &query_child;Next &next;Chain &chain_child;
    // E: where the return address sits for the original's addressing. After
    // the probe ESP is E-2410; after the saves of EBX and ESI, E-2420 between
    // the calls. After a child, E follows the ESP the child returned.
    std::uint32_t frame{};
    Search(State &s,std::uint32_t *c,Trace<State> *t,Route &r,Flood &f,Query &q,Next &n,Chain &h)
        :Port<State>(s,c,t,search_edges),route(r),flood_child(f),query_child(q),next(n),chain_child(h){}
    std::uint32_t local(Local l)const {
        switch(l) {
        case Local::search:return frame+0x4u;
        case Local::start_cluster:return frame+0x8u;
        case Local::point:return frame+0xcu;
        case Local::direction:return frame+0x10u;
        case Local::test_first:return frame+0x14u;
        case Local::test_second:return frame+0x18u;
        case Local::add_context:return frame+0x1cu;
        case Local::angle:return frame-0x2410u;
        case Local::range:return frame-0x240cu;
        case Local::cosine:return frame-0x2408u;
        case Local::sine:return frame-0x2404u;
        case Local::clusters:return frame-0x2400u;
        case Local::objects:return frame-0x2000u;
        }
        return 0;
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // MOV EAX,2410; CALL 32A660 (the stack probe); PUSH EBP.
        case search_argument:
            gp[eax]=0x2410u;
            this->call_routed(route,0x1ae095u,0x1ae09au,0x32a660u);
            frame=gp[esp]+0x2410u;
            this->push(0x1ae09au,gp[ebp]);
            return 0x1ae09bu;
        case range_last:return 0x1ae0a2u;
        // PUSH EDI; XOR EDI,EDI.
        case range_middle:this->push(0x1ae0a7u,gp[edi]);gp[edi]=this->flags.logic(0,4);return 0x1ae0aau;
        case range_middle_again:return 0x1ae0b2u;
        case range_first:return 0x1ae0b7u;
        case range_last_again:return 0x1ae0c6u;
        case range_middle_compare:return 0x1ae0cbu;
        case range_middle_third:return 0x1ae0d6u;
        case angle_last:return 0x1ae0dbu;
        case angle_middle:return 0x1ae0e0u;
        case range_store:return 0x1ae0e4u;
        case angle_middle_again:return 0x1ae0eeu;
        case angle_first:return 0x1ae0f3u;
        case angle_last_again:return 0x1ae0ffu;
        case angle_middle_compare:return 0x1ae104u;
        case angle_middle_third:return 0x1ae10fu;
        case angle_store:return 0x1ae11au;
        case angle_for_sine:return 0x1ae12fu;
        case range_for_flood:return 0x1ae133u;
        // PUSH EBX, ESI, 0 (no count output); LEA EAX (the cluster list); PUSH EAX.
        case direction_for_flood:
            this->push(0x1ae139u,gp[ebx]);this->push(0x1ae13au,gp[esi]);this->push(0x1ae13bu,0);
            gp[eax]=local(Local::clusters);this->push(0x1ae141u,gp[eax]);
            return 0x1ae142u;
        case sine_store:this->push(0x1ae149u,logic::cluster_capacity);return 0x1ae14eu;
        case angle_for_cosine:return 0x1ae152u;
        case sine_for_flood:return 0x1ae156u;
        case point_for_flood:this->push(0x1ae15cu,gp[ecx]);return 0x1ae15du;          // the width
        case start_for_flood:this->push(0x1ae164u,gp[edx]);return 0x1ae165u;          // the max distance
        case cosine_store:this->push(0x1ae16cu,gp[eax]);this->push(0x1ae16du,gp[ecx]);return 0x1ae16eu;
        case spread_for_flood:return 0x1ae172u;
        case players_table:return 0x1ae19au;
        case count_store:gp[edx]=logic::widen16(gp[eax]);return 0x1ae1a3u;                    // MOVSX EDX,AX
        case active_set:return 0x1ae1a7u;
        case player_size:return 0x1ae1c0u;
        case player_data:return 0x1ae1c3u;
        case unit:return 0x1ae1cfu;
        case headers:return 0x1ae1d7u;
        case header_rows:return 0x1ae1ddu;
        // MOV ECX,EAX; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case object_data:gp[ecx]=gp[eax];gp[eax]=this->flags.logic(gp[eax]&0xffffu,4)*3u;return 0x1ae1eau;
        case parent:return 0x1ae1eeu;
        case stamp:return 0x1ae1f6u;
        case root_mark:gp[eax]=this->flags.logic(gp[ecx]&0xffffu,4);return 0x1ae203u;   // MOV EAX,ECX; AND EAX,FFFF
        case root_mark_store:return 0x1ae20cu;
        case count_read:return 0x1ae213u;
        case object_append:return 0x1ae217u;
        case count_update:gp[eax]=this->flags.increment(gp[eax],4,false);return 0x1ae21fu;
        case count_check:return 0x1ae225u;
        case active_clear:return 0x1ae22du;
        case spread_for_chains:return 0x1ae236u;
        case context_for_chain:return 0x1ae240u;
        case second_for_chain:return 0x1ae247u;
        case first_for_chain:return 0x1ae24eu;
        case width_for_chain:this->push(0x1ae255u,gp[eax]);return 0x1ae256u;
        case range_for_chain:this->push(0x1ae25au,gp[edi]);this->push(0x1ae25bu,gp[ecx]);return 0x1ae25cu;
        case direction_for_chain:this->push(0x1ae260u,gp[edx]);return 0x1ae261u;
        case point_for_chain:this->push(0x1ae268u,gp[ebx]);this->push(0x1ae269u,gp[eax]);return 0x1ae26au;
        case object_for_chain:this->push(0x1ae271u,gp[ecx]);return 0x1ae272u;
        case count_bound:return 0x1ae289u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case search_argument:gp[ebp]=v;break;
        case range_for_flood:case start_for_flood:case header_rows:case stamp:case first_for_chain:
        case direction_for_chain:
            gp[edx]=v;break;
        case direction_for_flood:case count_read:case context_for_chain:case width_for_chain:case point_for_chain:
            gp[eax]=v;break;
        case sine_for_flood:case point_for_flood:case player_size:case headers:case second_for_chain:case range_for_chain:
        case object_for_chain:
            gp[ecx]=v;break;
        case spread_for_flood:this->s.xmm[5]=lane0_only(v);break;
        case players_table:gp[esi]=v;break;
        // MOV EDX,[ESI+44]; IMUL ECX,EAX; ADD ECX,EDX; MOV EBX,EAX.
        case player_data:
            gp[edx]=v;gp[ecx]=this->flags.product(gp[ecx],gp[eax]);gp[ecx]=this->flags.plus(gp[ecx],gp[edx],4);
            gp[ebx]=gp[eax];break;
        case unit:case parent:gp[eax]=v;this->compare32(v,logic::none);break;
        case object_data:gp[eax]=v;break;
        case root_mark:this->compare32(v,gp[edx]);break;
        // MOV EAX,[count]; XOR ESI,ESI; TEST EAX,EAX.
        case count_check:gp[eax]=v;gp[esi]=this->flags.logic(0,4);this->test32(v);break;
        case spread_for_chains:gp[ebx]=v;break;
        // MOV EAX,[count]; INC ESI; CMP ESI,EAX.
        case count_bound:gp[eax]=v;gp[esi]=this->flags.increment(gp[esi],4,false);this->compare32(gp[esi],v);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        if(a==Access::active_set)this->gp[ebx]=this->flags.logic(this->gp[ebx]|0xffffffffu,4);   // OR EBX,-1
    }
    // XORPS XMM0,XMM0; COMISS XMM2 (the range),XMM0; then, when the range
    // is positive, COMISS XMM1 (the angle),XMM0.
    void limits(std::uint32_t range,std::uint32_t angle,bool reaches) {
        auto &x=this->s.xmm;
        x[0]=_mm_setzero_ps();x[1]=lane0_only(angle);x[2]=lane0_only(range);
        this->flags.scalar_compare(x[2],x[0],false);
        if(reaches)this->flags.scalar_compare(x[1],x[0],false);
    }
    // The logical x87 stack of FLD m32 (C1 cleared), the function (C1/C2),
    // FSTP m32 (C1) and the pop.
    void rounded(std::uint32_t angle,const logic::x87::Rounded &r) {
        auto &fp=this->s.fp;
        fp.push(static_cast<long double>(std::bit_cast<float>(angle)));
        fp.status=std::uint16_t((fp.status&~0x600u)|(r.function_status&0x600u));
        fp.status=std::uint16_t((fp.status&~0x200u)|(r.store_status&0x200u));
        fp.pop();
    }
    void returned(Kept &kept) {
        frame=this->gp[esp]+0x2420u;
        kept.search=this->gp[ebp];kept.targets=this->gp[edi];kept.players=this->gp[esi];kept.position=this->gp[esi];
    }
    // CALL 14D200 (DX the start cluster, XMM5 the spread).
    std::uint32_t flood(Kept &kept) {
        this->call(0x1ae178u,0x1ae17du,0x14d200u,flood_child);returned(kept);
        return this->gp[eax];
    }
    // LEA EDX (the object list); PUSH EDX, 800; MOVSX EAX,AX; LEA ECX (the
    // cluster list); PUSH ECX, EAX, 1 (collideable); CALL BD460.
    std::uint32_t query(Kept &kept,std::uint32_t) {
        auto &gp=this->gp;
        gp[edx]=local(Local::objects);this->push(0x1ae184u,gp[edx]);this->push(0x1ae185u,logic::object_capacity);
        gp[eax]=logic::widen16(gp[eax]);gp[ecx]=local(Local::clusters);
        this->push(0x1ae191u,gp[ecx]);this->push(0x1ae192u,gp[eax]);
        this->push(0x1ae193u,native_cluster_objects::partition_flags::collideable);
        this->call(0x1ae195u,0x1ae19au,0xbd460u,query_child);returned(kept);
        return gp[eax];
    }
    // LEA EDX,[EBX+1]; MOV ECX,ESI; CALL 16EA60; CMP EAX,-1.
    std::uint32_t next_player(Kept &kept) {
        auto &gp=this->gp;
        gp[edx]=gp[ebx]+1u;gp[ecx]=gp[esi];
        this->call(0x1ae1b6u,0x1ae1bbu,0x16ea60u,next);returned(kept);
        this->compare32(gp[eax],logic::none);
        return gp[eax];
    }
    // PUSH EDX (direction), EAX (point), ECX (the object), EBP (the search);
    // CALL 1AE2B0; MOV EDI,EAX; CMP EDI,40.
    std::uint32_t chain(Kept &kept) {
        auto &gp=this->gp;
        this->push(0x1ae279u,gp[edx]);this->push(0x1ae27au,gp[eax]);this->push(0x1ae27bu,gp[ecx]);
        this->push(0x1ae27cu,gp[ebp]);
        this->call(0x1ae27du,0x1ae282u,0x1ae2b0u,chain_child);
        gp[edi]=gp[eax];returned(kept);
        this->compare32(gp[edi],logic::target_limit);
        return gp[edi];
    }
    void run() {
        auto &gp=this->gp;
        frame=gp[esp];
        Memory<Search> memory{*this};
        const auto result=logic::aim_search::find_targets(memory,*this);
        if(result.exit!=logic::aim_search::Exit::closed){gp[esi]=this->pop(0x1ae292u);gp[ebx]=this->pop(0x1ae293u);}
        // MOV EAX,EDI; POP EDI; POP EBP; ADD ESP,2410; RET 1C.
        gp[eax]=gp[edi];
        gp[edi]=this->pop(0x1ae296u);gp[ebp]=this->pop(0x1ae297u);
        gp[esp]=this->flags.plus(gp[esp],0x2410u,4);
        this->ret(0x1ae29eu,0x1cu);this->publish(0x1ae29eu);
    }
};

// Admission also requires masked SSE exceptions (COMISS), no active MMX and
// a free x87 register (a full stack is the translated route's fault).
template<class State,class Reference,class Route,class Flood,class Query,class Next,class Chain>
bool bridge(State &s,Reference reference,Route route,Flood flood,Query query,Next next,Chain chain) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.fp.count>=8u)return false;
    if((_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1ae090u,0x1ae29eu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return Search<State,Route,Flood,Query,Next,Chain>(v,c,t,route,flood,query,next,chain);});
    return true;
}
} // namespace halo2::native_aim_search_detail
