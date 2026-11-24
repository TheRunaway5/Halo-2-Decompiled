#pragma once
// Boundary adapter for the unit reaction, E71A0
// (native_tu15_unit_reaction_systems.hpp). Its callers (the dispatch) and
// E72E0 keep the original ABI, which the adapter owns: EDI the unit (RET);
// ECX pushed (the pulse's value in its slot) and ESI saved; XMM1 zero, XMM2
// one and XMM0 as the original leaves it; E72E0 (the value, 3 and the unit
// pushed, EDX 2, RET 0xC) direct, every register it returns coming back.
// Each access publishes its original instruction.
// HALO2_REFERENCE_UNIT_REACTION=1 restores the translated body.
// HALO2_VERIFY_UNIT_REACTION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_unit_reaction_systems.hpp"

namespace halo2::native_unit_reaction_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_unit_reaction;
using Access=logic::unit_reaction::Access;
using Kept=logic::unit_reaction::Kept;
using Outcome=logic::unit_reaction::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_UNIT_REACTION");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_UNIT_REACTION");return v;}
enum Branch : unsigned {unmatched, active, pulsed, expired, counting, clamped, branch_count};
static_assert(unsigned(Outcome::expired)==Branch::expired);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Unit reaction"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline void set_high_byte(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff00ffu)|((value&0xffu)<<8);}
inline constexpr std::uint32_t sites[]{0xe71a1u,0xe71a7u,0xe71b8u,0xe71bcu,0xe726au,0xe7278u,0xe7280u,0xe728du,0xe7292u,0xe729fu,
    0xe725eu,0xe71d5u,0xe71dbu,0xe71e2u,0xe71fcu,0xe7201u,0xe7208u,0xe720du,0xe7224u,0xe722cu,0xe7238u,0xe7243u,0xe724eu,0xe72b2u,
    0xe7256u,0xe72b8u,0xe72cau};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::level_clear)+1u);
template<class State,class Pulse> struct Adapter : Port<State> {
    Pulse &pulse_child;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Pulse &p):Port<State>(s,c,t,{}),pulse_child(p){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[esi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers:case clock:case block_id:gp[ecx]=v;break;
        case datums:gp[edx]=v;break;
        case object_address:gp[esi]=v;break;
        case mode:gp[eax]=logic::widen8(v);break;                                                       // MOVSX EAX,byte [ESI+34A]
        case counter:set_low_byte(gp[eax],v);break;
        case block_offset:gp[eax]=logic::widen16(v);break;                                              // MOVSX EAX,word [ESI+12A]
        case one:this->s.xmm[2]=lane0(v);break;                                                         // MOVSS XMM2,[46A0D0]
        case level:gp[edx]=logic::widen8(v);break;                                                      // MOVSX EDX,byte [ESI+39F]
        case value_load:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saving(unsigned){put(0xe71a0u,this->gp[ecx]);}                                               // PUSH ECX
    void indexing(std::uint32_t unit_handle) {                                                         // XORPS XMM1; MOV EAX,EDI; AND; LEA; PUSH ESI
        auto &gp=this->gp;
        this->s.xmm[1]=_mm_setzero_ps();
        gp[eax]=3u*this->flags.logic(unit_handle&0xffffu,4);
        put(0xe71b7u,gp[esi]);
    }
    void mode_tested(std::uint32_t mode_value) {                                                       // XOR DL,DL; SUB EAX,0; (DEC EAX)
        auto &gp=this->gp;auto &fl=this->flags;
        set_low_byte(gp[edx],fl.logic(0u,1));
        gp[eax]=fl.minus(mode_value,0u,4);
        if(mode_value)gp[eax]=fl.increment(mode_value,4,true);
    }
    void counter_tested(std::uint32_t count){this->test8(count);}                                      // TEST AL,AL
    std::uint32_t counted(std::uint32_t count) {                                                       // INC AL; (MOVSX EAX,AL)
        auto &gp=this->gp;
        const auto next=this->flags.increment(count&0xffu,1,false)&0xffu;
        gp[eax]=logic::widen8(next);
        this->reached(Branch::counting);
        return next;
    }
    void converted(std::uint32_t value){this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0(value));}    // CVTSI2SS XMM0
    void multiplied(std::uint32_t value){this->s.xmm[0]=_mm_move_ss(this->s.xmm[0],lane0(value));}   // MULSS XMM0
    void weight_loaded(std::uint32_t weight){this->s.xmm[0]=lane0(weight);}                            // MOVSS XMM0,[EAX+1C]
    void compared(std::uint32_t a,std::uint32_t b,bool unordered) {                                    // COMISS; or UCOMISS; LAHF; TEST AH,44
        auto &gp=this->gp;auto &fl=this->flags;
        fl.scalar_compare(lane0(a),lane0(b),unordered);
        if(unordered){set_high_byte(gp[eax],fl.bits()|2u);fl.logic((gp[eax]>>8)&0x44u,1);}
    }
    void expired(){set_low_byte(this->gp[edx],1u);}                                                    // MOV DL,1
    void block_found(std::uint32_t offset,std::uint32_t unit_address,std::uint32_t id) {               // ADD EAX,ESI; CMP ECX,900000E; (CMP ECX,A00000F)
        auto &gp=this->gp;
        gp[eax]=this->flags.plus(offset,unit_address,4);
        this->compare32(id,logic::ids::first);
        if(id!=logic::ids::first)this->compare32(id,logic::ids::second);
    }
    void handle_tested(std::uint32_t handle){this->compare32(handle,logic::none);}                    // CMP [EAX],-1
    void high_tested(std::uint32_t high){this->flags.minus(high&0xffffu,0xffffu,2);}                  // CMP word [EAX+6],-1
    void pulse_tested(bool,std::uint32_t){this->test8(this->gp[edx]);}                                // TEST DL,DL
    void pulse(std::uint32_t unit_handle,std::uint32_t value,Kept &kept) {                             // PUSH EAX, 3, EDI; MOV EDX,2; CALL E72E0
        auto &gp=this->gp;
        if(value!=_mm_cvtsi128_si32(_mm_castps_si128(this->s.xmm[2])))this->reached(Branch::clamped);
        put(0xe72bcu,value);put(0xe72bdu,logic::pulse_kind);put(0xe72bfu,unit_handle);gp[edx]=logic::pulse_mode;
        this->call(0xe72c5u,0xe72cau,0xe72e0u,pulse_child);depth-=12u;
        refresh(kept);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::unit_reaction::update(memory,*this,gp[edi]);
        this->reached(unsigned(outcome));
        gp[esi]=take(0xe72d1u);gp[ecx]=take(0xe72d2u);
        this->ret(0xe72d3u,0u);this->publish(0xe72d3u);
    }
};
template<class State,class Reference,class Pulse> bool bridge(State &s,Reference reference,Pulse pulse) {
    return run_owner(s,0xe71a0u,0xe72d3u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Pulse>(v,c,t,pulse);});
}
} // namespace halo2::native_unit_reaction_detail
