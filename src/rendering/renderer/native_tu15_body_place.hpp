#pragma once
// Boundary adapter for the body place, 1D9190
// (native_tu15_body_place_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the matrix, ESI the model,
// EDI the body index (RET); EBP saved and ESP aligned down to 16 below 0xA0
// bytes of frame, read from the ESP each child left; 2E4470 (ECX the
// inverse, the offset pushed) and 2E4350 (ECX the quaternion, the rows
// pushed) through the unit's dispatch and 2E4650 (ECX the rows, the inverse
// pushed), each RET 4, and 2E58F0 (the position, the quaternion, the rate and
// the body pushed, the caller cleaning) direct, every register each returns
// coming back. XMM0 and XMM1 hold what the original's moves leave in them.
// Each access publishes its original instruction.
// HALO2_REFERENCE_BODY_PLACE=1 restores the translated body.
// HALO2_VERIFY_BODY_PLACE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "rendering/renderer/native_tu15_body_place_systems.hpp"

namespace halo2::native_body_place_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_body_place;
using Access=logic::body_place::Access;
using Kept=logic::body_place::Kept;
using Outcome=logic::body_place::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_BODY_PLACE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_BODY_PLACE");return v;}
enum Branch : unsigned {plain, offset, branch_count};
static_assert(unsigned(Outcome::offset)==Branch::offset);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Body place"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
// Each component load and the XMM register it fills (0 or 1); each component store.
inline constexpr std::uint32_t component_loads[]{0x1d919cu,0x1d91b1u,0x1d91bcu,0x1d91c7u,0x1d91d2u,0x1d91ddu,0x1d91ecu,0x1d91f7u,0x1d9202u,
    0x1d920du,0x1d9218u,0x1d9223u};
inline constexpr unsigned component_registers[]{1,1,1,1,1,1,0,1,0,1,0,1};
inline constexpr std::uint32_t component_stores[]{0x1d91abu,0x1d91b6u,0x1d91c1u,0x1d91ccu,0x1d91d7u,0x1d91f1u,0x1d91fcu,0x1d9207u,0x1d9212u,
    0x1d921du,0x1d922bu,0x1d9239u,0x1d923fu,0x1d9245u,0x1d924bu,0x1d9251u};
template<class State,class Compose,class Place,class Dispatch> struct Adapter : Port<State> {
    Compose &compose;Place &place_child;Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below the aligned frame
    unsigned loads{},stores{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Compose &a,Place &p,Dispatch &d):Port<State>(s,c,t,{}),compose(a),place_child(p),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){kept={this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case component_load:return component_loads[loads];
        case time_address:return 0x1d91a1u;
        case rate_word:return 0x1d91a7u;
        case component_store:return component_stores[stores++];
        case rate_store:return 0x1d91e6u;
        case flags:return 0x1d9228u;
        case position_load:return 0x1d9257u;
        case position_store:return 0x1d925cu;
        case bodies:return 0x1d928cu;
        case body_entry:return 0x1d9295u;
        case rate_load:return 0x1d9299u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case component_load:this->s.xmm[component_registers[loads++]]=lane0(v);break;                // MOVSS
        case time_address:case rate_load:gp[ecx]=v;break;
        case rate_word:gp[edx]=std::uint32_t(std::int32_t(std::int16_t(v)));break;                     // MOVSX
        case flags:case body_entry:gp[eax]=v;break;
        case bodies:gp[edx]=v;break;
        default:break;
        }
    }
    void loaded_vector(Access,std::uint32_t,__m128 v){this->s.xmm[0]=v;}                               // MOVAPS XMM0
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // PUSH EBP; MOV EBP,ESP; AND ESP,-16; SUB ESP,A0
        auto &gp=this->gp;auto &fl=this->flags;
        this->push(0x1d9190u,gp[ebp]);gp[ebp]=gp[esp];
        gp[esp]=fl.minus(fl.logic(gp[esp]&0xfffffff0u,4),logic::local::size,4);
    }
    std::uint32_t rate(std::uint32_t) {                                                                 // CVTSI2SS XMM0,EDX
        auto &x=this->s.xmm[0];
        x=_mm_cvtsi32_ss(x,int(this->gp[edx]));
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));
    }
    std::uint32_t offset_tested(std::uint32_t flags_value) {                                            // XORPS XMM0; SHR EAX,6; TEST AL,1
        auto &gp=this->gp;
        this->s.xmm[0]=_mm_setzero_ps();
        gp[eax]=this->flags.right(flags_value,logic::model::offset_bit,4,false);
        this->flags.logic(gp[eax]&1u,1);
        return gp[eax];
    }
    void offset(std::uint32_t offset_address,Kept &kept) {
        auto &gp=this->gp;
        gp[ecx]=offset_address;put(0x1d9266u,gp[ecx]);gp[ecx]=local(logic::local::inverse);              // LEA ECX,[ESI+30]; PUSH; LEA ECX,[ESP+64]
        auto inverse=[&](State &v){dispatch(v,0x2e4470u,0x1d926bu);};
        this->call(0x1d926bu,0x1d9270u,0x2e4470u,inverse);depth-=4u;
        refresh(kept);
        gp[edx]=local(logic::local::inverse);put(0x1d9274u,gp[edx]);gp[ecx]=local(logic::local::forward); // LEA EDX; PUSH; LEA ECX,[ESP+24]
        this->call(0x1d9279u,0x1d927eu,0x2e4650u,compose);depth-=4u;
        refresh(kept);
        this->reached(Branch::offset);
    }
    void rotation(Kept &kept) {                                                                          // LEA EAX,[ESP+20]; PUSH; LEA ECX,[ESP+14]; CALL 2E4350
        auto &gp=this->gp;
        gp[eax]=local(logic::local::forward);put(0x1d9282u,gp[eax]);gp[ecx]=local(logic::local::quaternion);
        auto quaternion=[&](State &v){dispatch(v,0x2e4350u,0x1d9287u);};
        this->call(0x1d9287u,0x1d928cu,0x2e4350u,quaternion);depth-=4u;
        refresh(kept);
    }
    std::uint32_t indexed(std::uint32_t index) {                                                        // LEA ECX,[EDI+EDI*2]; SHL ECX,5
        static_assert((3u<<5)==logic::body::size);
        return this->gp[ecx]=this->flags.shift(3u*index,5u,4);
    }
    void placed(std::uint32_t body_address,std::uint32_t rate_value,Kept &kept) {                      // PUSH body, rate, quaternion, position; CALL 2E58F0; ADD ESP,10
        auto &gp=this->gp;
        put(0x1d929du,body_address);put(0x1d929eu,rate_value);
        gp[edx]=local(logic::local::quaternion);put(0x1d92a3u,gp[edx]);
        gp[eax]=local(logic::local::position);put(0x1d92a8u,gp[eax]);
        this->call(0x1d92a9u,0x1d92aeu,0x2e58f0u,place_child);
        refresh(kept);
        gp[esp]=this->flags.plus(gp[esp],0x10u,4);depth-=0x10u;
    }
    void restoring() {                                                                                   // MOV ESP,EBP; POP EBP
        auto &gp=this->gp;
        gp[esp]=gp[ebp];gp[ebp]=this->pop(0x1d92b3u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::body_place::place(memory,*this,gp[eax],Kept{gp[esi],gp[edi]});
        this->reached(unsigned(outcome));
        this->ret(0x1d92b4u,0u);this->publish(0x1d92b4u);
    }
};
template<class State,class Reference,class Compose,class Place,class Dispatch>
bool bridge(State &s,Reference reference,Compose compose,Place place,Dispatch dispatch) {
    return run_owner(s,0x1d9190u,0x1d92b4u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Compose,Place,Dispatch>(v,c,t,compose,place,dispatch);});
}
} // namespace halo2::native_body_place_detail
