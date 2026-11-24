#pragma once
// Boundary adapter for the placement repair, 175380
// (native_tu15_placement_repair_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX the record (RET); EBX
// saved (ESI and EDI past the first check); 175300 (EBX the record), B1720
// (ESI and EDI the unit vectors) and B16C0 (EAX the vector) direct, every
// register each returns coming back; XMM0 to XMM2 as the clamps leave them;
// the tail JMP 1722C0 (EDX the record) returns to this owner's caller. Each
// access publishes its original instruction.
// HALO2_REFERENCE_PLACEMENT_REPAIR=1 restores the translated body.
// HALO2_VERIFY_PLACEMENT_REPAIR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_placement_repair_systems.hpp"

namespace halo2::native_placement_repair_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_placement_repair;
using Access=logic::placement_repair::Access;
using Kept=logic::placement_repair::Kept;
using Outcome=logic::placement_repair::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_PLACEMENT_REPAIR");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_PLACEMENT_REPAIR");return v;}
enum Branch : unsigned {sane, repaired, reported, defaults, zeroed_vector, branch_count};
static_assert(unsigned(Outcome::reported)==Branch::reported);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Placement repair"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0x1753a1u,0u,0u,0x1753adu,0u,0u,0x1753ccu,0u,0x1753d8u,0u,0u,0u,0x1754b1u,0u,0u,0x1754c7u,0x1754cfu,
    0x1754d7u,0x1754dcu,0x1754ecu,0x1754f4u,0x1754fau,0x1754ffu,0x17550fu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::first_scalar_store)+1u);
// The repeated roles' instructions, in order.
inline constexpr std::uint32_t first_reads[3]{0x1753a6u,0x1753aau,0x1753b6u},first_stores[3]{0x1753a8u,0x1753b3u,0x1753b9u},
    second_reads[3]{0x1753bcu,0x1753c0u,0x1753c6u},second_stores[3]{0x1753beu,0x1753c3u,0x1753c9u},
    points[6]{0x1753d4u,0x1753f9u,0x17541bu,0x17543du,0x17545fu,0x175481u},point_stores[6]{0x1753fdu,0x17541fu,0x175441u,0x175463u,0x175485u,0x1754a3u},
    point_agains[6]{0x1753e7u,0x175409u,0x17542bu,0x17544du,0x17546fu,0x17548cu},point_lasts[6]{0x1753f4u,0x175416u,0x175438u,0x17545au,0x17547cu,0x175499u},
    zero_reads[3]{0x1754b7u,0x1754bbu,0x1754c1u},zero_stores[3]{0x1754b9u,0x1754beu,0x1754c4u};
template<class State,class Check,class Units,class Finite,class Tail> struct Adapter : Port<State> {
    Check &check_child;
    Units &units_child;
    Finite &finite_child;
    Tail &tail_child;
    unsigned counts[unsigned(Access::first_scalar_store)+1u]{},checks{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Check &k,Units &u,Finite &f,Tail &x)
        :Port<State>(s,c,t,{}),check_child(k),units_child(u),finite_child(f),tail_child(x){}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        const auto k=counts[unsigned(a)]++;
        switch(a) {
        case first_read:return first_reads[k];
        case first_store:return first_stores[k];
        case second_read:return second_reads[k];
        case second_store:return second_stores[k];
        case point:return points[k];
        case point_store:return point_stores[k];
        case point_again:return point_agains[counts[unsigned(point)]-1u];
        case point_last:return point_lasts[counts[unsigned(point)]-1u];
        case zero_read:return zero_reads[k];
        case zero_store:return zero_stores[k];
        default:return sites[unsigned(a)];
        }
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        if(a==first_pointer)gp[eax]=v;                                                                   // MOV EAX,[474D08]
        if(a==zero_pointer)gp[edx]=v;                                                                    // MOV EDX,[474D04]
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==0u){this->push(0x175380u,gp[ebx]);gp[ebx]=gp[eax];}                                     // PUSH EBX; MOV EBX,EAX
        else{this->push(0x175390u,gp[esi]);this->push(0x175391u,gp[edi]);}                             // PUSH ESI; PUSH EDI
    }
    void restoring(){auto &gp=this->gp;gp[edi]=this->pop(0x1754d3u);gp[esi]=this->pop(0x1754d4u);}   // POP EDI; POP ESI
    std::uint32_t sane(Kept &kept) {                                                                   // CALL 175300; TEST AL,AL
        auto &gp=this->gp;
        gp[ebx]=kept.record;
        const auto site=checks++?0x175514u:0x175383u;
        this->call(site,site+5u,0x175300u,check_child);
        refresh(kept);
        this->test8(gp[eax]);
        return gp[eax];
    }
    std::uint32_t unit_vectors(Kept &kept) {                                                           // LEA EDI,[EBX+38]; LEA ESI,[EBX+2C]; CALL B1720
        auto &gp=this->gp;
        gp[edi]=kept.record+logic::record::second_vector;gp[esi]=kept.record+logic::record::first_vector;
        this->call(0x175398u,0x17539du,0xb1720u,units_child);
        refresh(kept);
        this->test8(gp[eax]);
        if(!(gp[eax]&0xffu))this->reached(Branch::defaults);
        return gp[eax];
    }
    void finite_vector(const Kept &kept) {                                                             // LEA ESI,[EBX+44]; MOV EAX,ESI
        auto &gp=this->gp;
        gp[esi]=kept.record+logic::record::finite_vector;gp[eax]=gp[esi];
    }
    std::uint32_t finite(Kept &kept) {                                                                 // CALL B16C0; TEST AL,AL
        auto &gp=this->gp;
        this->call(0x1754a8u,0x1754adu,0xb16c0u,finite_child);
        refresh(kept);
        this->test8(gp[eax]);
        if(!(gp[eax]&0xffu))this->reached(Branch::zeroed_vector);
        return gp[eax];
    }
    void copied(unsigned n,std::uint32_t value) {                                                      // the copies' registers
        static constexpr unsigned registers[11]{ecx,edx,eax,ecx,edx,eax,ecx,0u,eax,ecx,edx};
        this->gp[registers[n]]=value;
    }
    void xmm(unsigned n,std::uint32_t value){this->s.xmm[n]=lane0(value);}
    void zeroed(){this->s.xmm[0]=_mm_setzero_ps();}                                                    // XORPS XMM0,XMM0
    void compared(std::uint32_t a,std::uint32_t b){this->flags.scalar_compare(lane0(a),lane0(b),false);}   // COMISS
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::placement_repair::repair(memory,*this,gp[eax]);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::reported) {                                                                 // MOV EDX,EBX; POP EBX; JMP 1722C0
            gp[edx]=gp[ebx];gp[ebx]=this->pop(0x17551fu);
            if(this->cursor)*this->cursor=0x175520u;
            this->enter(0x1722c0u,tail_child);
            return;
        }
        gp[ebx]=this->pop(0x175525u);
        this->ret(0x175526u,0u);this->publish(0x175526u);
    }
};
template<class State,class Reference,class Check,class Units,class Finite,class Tail>
bool bridge(State &s,Reference reference,Check check,Units units,Finite finite,Tail tail) {
    return run_owner(s,0x175380u,0x175526u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Check,Units,Finite,Tail>(v,c,t,check,units,finite,tail);});
}
} // namespace halo2::native_placement_repair_detail
