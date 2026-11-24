#pragma once
// Boundary adapter for the player biped query, entered at 152E00
// (native_tu15_player_biped_systems.hpp). Its callers and E65A0 keep the
// original ABI, which the adapter owns: EAX the player handle; EBX and ESI
// saved; E65A0 gets EAX the unit, and every register it returns comes back;
// the result in AL (XOR AL,AL; MOV AL,1) above the rest of EAX as the
// original leaves it; ECX (the player, then the type byte over the unit's
// tripled index) and EDX (the unit) as the original leaves them; the flags
// of the last instruction that sets them (the SHL's included); RET. Each
// access publishes its original instruction.
// HALO2_REFERENCE_PLAYER_BIPED=1 restores the translated body.
// HALO2_VERIFY_PLAYER_BIPED=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_player_biped_systems.hpp"

namespace halo2::native_player_biped_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_player_biped;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_PLAYER_BIPED");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_PLAYER_BIPED");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_unit, not_biped, parented, other_state, answered, qualifies, branch_count};
static_assert(unsigned(logic::player_biped::Outcome::qualifies)==Branch::qualifies);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Player biped verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Answer> struct PlayerBiped : Port<State> {
    using Access=logic::player_biped::Access;
    Answer &answer_child;
    PlayerBiped(State &s,std::uint32_t *c,Trace<State> *t,Answer &a):Port<State>(s,c,t,{}),answer_child(a){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case players:return 0x152e00u;
        case player_datums:return 0x152e06u;
        case unit:return 0x152e18u;
        // PUSH EBX; PUSH ESI.
        case headers:this->push(0x152e22u,this->gp[ebx]);this->push(0x152e23u,this->gp[esi]);return 0x152e24u;
        case header_datums:return 0x152e2au;
        case type:return 0x152e3bu;
        case object_address:return 0x152e4au;
        case parent:return 0x152e4du;
        case state:return 0x152e53u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case players:gp[ecx]=v;break;
        // MOV EDX,[ECX+44]; AND EAX,FFFF; IMUL EAX,EAX,21C; ADD EAX,EDX; MOV ECX,EAX.
        case player_datums:gp[edx]=v;gp[eax]=fl.plus(fl.product(fl.logic(gp[eax]&0xffffu,4),logic::player_size),v,4);
            gp[ecx]=gp[eax];break;
        // MOV EDX,[ECX+2C]; XOR AL,AL; CMP EDX,-1.
        case unit:gp[edx]=v;set_low_byte(gp[eax],fl.logic(0u,1));fl.minus(v,logic::none,4);break;
        case headers:gp[esi]=v;break;
        // MOV ESI,[ESI+44]; MOV ECX,EDX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2]; LEA ESI,[ESI+ECX*4].
        case header_datums:gp[ecx]=fl.logic(gp[edx]&0xffffu,4)*3u;gp[esi]=v+4u*gp[ecx];break;
        // MOV CL,[ESI+3]; MOV EBX,1; SHL EBX,CL; TEST BL,1.
        case type:set_low_byte(gp[ecx],v);gp[ebx]=fl.shift(1u,v&0xffu,4);this->test8(gp[ebx]&1u);break;
        case object_address:gp[esi]=v;break;
        case parent:fl.minus(v,logic::none,4);break;                                                // CMP [ESI+14],-1
        case state:fl.minus(v&0xffu,logic::wanted_state,1);break;                                   // CMP byte [ESI+3DC],1
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOV EAX,EDX; CALL E65A0; TEST AL,AL.
    std::uint32_t answer(std::uint32_t unit) {
        auto &gp=this->gp;
        gp[eax]=unit;this->call(0x152e5eu,0x152e63u,0xe65a0u,answer_child);this->test8(gp[eax]);
        return gp[eax]&0xffu;
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::player_biped::Outcome;
        Memory<PlayerBiped> memory{*this};
        const auto outcome=logic::player_biped::test(memory,*this,gp[eax]);
        this->reached(unsigned(outcome));
        switch(outcome) {
        case Outcome::no_unit:break;
        case Outcome::qualifies:
            gp[esi]=this->pop(0x152e67u);set_low_byte(gp[eax],1u);gp[ebx]=this->pop(0x152e6au);    // POP ESI; MOV AL,1; POP EBX
            this->ret(0x152e6bu,0);this->publish(0x152e6bu);return;
        case Outcome::parented:case Outcome::other_state:case Outcome::answered:
            set_low_byte(gp[eax],this->flags.logic(0u,1));[[fallthrough]];                         // XOR AL,AL
        case Outcome::not_biped:
            gp[esi]=this->pop(0x152e6eu);gp[ebx]=this->pop(0x152e6fu);break;
        }
        this->ret(0x152e70u,0);this->publish(0x152e70u);
    }
};
template<class State,class Reference,class Answer> bool bridge(State &s,Reference reference,Answer answer) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x152e00u,0x152e70u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return PlayerBiped<State,Answer>(v,c,t,answer);});
    return true;
}
} // namespace halo2::native_player_biped_detail
