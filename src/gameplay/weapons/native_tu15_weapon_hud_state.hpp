#pragma once
// Boundary adapter for the weapon HUD state, 13DA89
// (native_tu15_weapon_hud_state_systems.hpp). Its callers and the child keep
// the original ABI, which the adapter owns: EAX the state, ECX the weapon,
// the output's address on the stack (RET 4); EBP the frame (0x40 bytes:
// the status, then the spilled object), EBX, ESI and EDI saved; 1055F0 gets
// the weapon and the status's address pushed (RET 8), every register it
// returns coming back. Each access publishes its original instruction.
// HALO2_REFERENCE_WEAPON_HUD_STATE=1 restores the translated body.
// HALO2_VERIFY_WEAPON_HUD_STATE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/weapons/native_tu15_weapon_hud_state_systems.hpp"

namespace halo2::native_weapon_hud_state_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_weapon_hud_state;
using Access=logic::weapon_hud_state::Access;
using Kept=logic::weapon_hud_state::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_WEAPON_HUD_STATE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_WEAPON_HUD_STATE");return v;}
enum Branch : unsigned {filled, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Weapon HUD state"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
// Each role's instruction.
inline constexpr std::uint32_t sites[]{
    0x13da96u,0x13da9cu,0x13daabu,0x13daafu,                                        // objects .. object_spill
    0x13dabcu,0x13dabfu,0x13dac2u,0x13dac6u,0x13dacau,0x13daceu,0x13dad2u,0x13dad6u,0x13dadbu,0x13dadfu,0x13dae3u,
    0x13dae7u,0x13daeau,0x13daedu,0x13daf0u,0x13daf3u,0x13daf6u,0x13daf9u,0x13dafcu,0x13daffu,0x13db02u,0x13db07u,0x13db0cu,
    0x13db0fu,0x13db12u,0x13db18u,0x13db1du,0x13db22u,0x13db27u,0x13db2cu,0x13db2eu,0x13db31u,0x13db34u,0x13db38u,
    0x13db43u,0x13db43u,                                                            // position_read, position_write (by word)
    0x13db46u,0x13db4du,0x13db51u,0x13db57u,0x13db5cu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::output_store)+1u);
template<class State,class Query> struct Adapter : Port<State> {
    Query &query_child;
    std::uint32_t copied{};     // the target position's words copied
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Query &q):Port<State>(s,c,t,{}),query_child(q){}
    std::uint32_t status(std::uint32_t offset)const{return this->gp[ebp]-0x40u+offset;}
    std::uint32_t argument()const{return this->gp[ebp]+8u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        if(a==position_read || a==position_write)return sites[unsigned(a)]+copied;
        if(a==weapon_definition)gp[eax]=fl.shift(fl.logic(gp[eax]&gp[ebx],4),4u,4);                   // AND EAX,EBX; SHL EAX,4
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case objects:case instances:case output_address:gp[ecx]=v;break;
        case object_data:                                                                              // MOV EAX,EDI; MOV EBX,FFFF; AND EAX,EBX; LEA EAX,[EAX+EAX*2]
            gp[ecx]=v;gp[ebx]=logic::handle_mask;gp[eax]=3u*fl.logic(gp[edi]&gp[ebx],4);break;
        case object_address:case object_reload:case object_tag:case weapon_definition:case output_value:gp[eax]=v;break;
        case overheated:case magazine:case input:case charging:case target:case locking:case locked:set_low_byte(gp[eax],v);break;
        case loaded:case capacity:case reserve:case maximum:set_low_word(gp[eax],v);break;
        case heat:case age:case charge:case progress:this->s.xmm[0]=lane0_only(v);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        // Each MOVSD step: ESI/EDI by the direction flag.
        if(a==Access::position_write){const auto step=copy_step();this->gp[esi]+=step;this->gp[edi]+=step;++copied;}
    }
    // LEA EAX,[EBP-40]; PUSH EAX; PUSH EDI; CALL 1055F0.
    void query(std::uint32_t,std::uint32_t status_address,Kept &kept) {
        auto &gp=this->gp;
        gp[eax]=status_address;this->push(0x13dab5u,gp[eax]);this->push(0x13dab6u,gp[edi]);
        this->call(0x13dab7u,0x13dabcu,0x1055f0u,query_child);
        kept={gp[esi],gp[edi],gp[ebx]};
    }
    std::uint32_t copy_step()const{return (this->s.other_flags&0x400u)?0xfffffffcu:4u;}
    // LEA EDI,[ESI+2C]; LEA ESI,[EBP-28].
    void copy_started(){this->gp[edi]=this->gp[esi]+logic::state::target_position;this->gp[esi]=status(logic::status::target_position);}
    void leaving(){this->gp[edi]=this->pop(0x13db5au);this->gp[esi]=this->pop(0x13db5bu);}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        this->push(0x13da89u,gp[ebp]);gp[ebp]=gp[esp];gp[esp]=fl.minus(gp[esp],0x40u,4);              // PUSH EBP; MOV EBP,ESP; SUB ESP,40
        this->push(0x13da8fu,gp[ebx]);this->push(0x13da90u,gp[esi]);this->push(0x13da91u,gp[edi]);
        gp[esi]=gp[eax];gp[edi]=gp[ecx];
        Memory<Adapter> memory{*this};
        logic::weapon_hud_state::fill(memory,*this,gp[esi],gp[edi]);
        this->reached(Branch::filled);
        gp[ebx]=this->pop(0x13db5eu);gp[esp]=gp[ebp];gp[ebp]=this->pop(0x13db5fu);                       // POP EBX; LEAVE
        this->ret(0x13db60u,4u);this->publish(0x13db60u);
    }
};
template<class State,class Reference,class Query> bool bridge(State &s,Reference reference,Query query) {
    return run_owner(s,0x13da89u,0x13db60u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Query>(v,c,t,query);});
}
} // namespace halo2::native_weapon_hud_state_detail
