#pragma once
// Boundary adapter for the seat camera death, 158790
// (native_tu15_seat_camera_death_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: ECX the seat, AL the reset;
// ESI, EDI (and past the reset EBX) saved; 158450 gets EDI the seat and the
// forced byte pushed, 246CF0 EAX the view and EDX the seat and NONE pushed,
// 158900 EAX the seat and EDX the state and 1 pushed (each RET 4), every
// register each returns coming back. Each access publishes its original
// instruction.
// HALO2_REFERENCE_SEAT_CAMERA_DEATH=1 restores the translated body.
// HALO2_VERIFY_SEAT_CAMERA_DEATH=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/vehicles/native_tu15_seat_camera_death_systems.hpp"

namespace halo2::native_seat_camera_death_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_seat_camera_death;
using Access=logic::seat_camera_death::Access;
using Kept=logic::seat_camera_death::Kept;
using Outcome=logic::seat_camera_death::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SEAT_CAMERA_DEATH");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SEAT_CAMERA_DEATH");return v;}
enum Branch : unsigned {reset, controlled, stopped, dead, already_dead, revived, alive, branch_count};
static_assert(unsigned(Outcome::alive)==Branch::alive);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Seat camera death"};return v;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
template<class State,class Mode,class View,class Enter> struct Adapter : Port<State> {
    Mode &mode_child;
    View &view_child;
    Enter &enter_child;
    bool begun{},updated{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Mode &m,View &v,Enter &e)
        :Port<State>(s,c,t,{}),mode_child(m),view_child(v),enter_child(e){}
    // OR EAX,-1; CMP EDI,-1.
    void begin(){if(!begun){this->gp[eax]=this->flags.logic(0xffffffffu,4);this->flags.minus(this->gp[edi],logic::none,4);begun=true;}}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case view_clear:this->s.xmm[0]=_mm_setzero_ps();return 0x1587a7u;                             // XORPS XMM0,XMM0
        case view_second_clear:return 0x1587acu;
        case one:return 0x1587b1u;
        case state_store:return 0x1587bau;
        case fade_store:return 0x1587c1u;
        case fade_flag_store:return 0x1587c9u;
        case seats:begin();return 0x1587dau;
        case seat_player:return 0x1587dfu;
        case players:begin();return 0x1587e3u;
        case player_data:                                                                              // AND EAX,FFFF; IMUL EAX,EAX,21C; PUSH EBX
            gp[eax]=fl.product(fl.logic(gp[eax]&0xffffu,4),logic::player_size);this->push(0x1587f4u,gp[ebx]);return 0x1587f5u;
        case player_unit:return 0x1587f8u;
        case game:return 0x158801u;
        case mode:return 0x158807u;
        case player_flags:return 0x158816u;
        case control:return 0x158822u;
        case control_byte:return 0x158827u;
        case game_time:return 0x15882cu;
        case time:return 0x158832u;
        case zero:return 0x158837u;
        case state:this->test8(gp[ebx]&0xffu);return 0x158849u;                                       // TEST BL,BL
        }
        return 0u;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case one:case time:this->s.xmm[0]=lane0_only(v);break;
        case seats:case seat_player:gp[eax]=v;break;
        case players:gp[ecx]=v;break;
        case player_data:gp[ebx]=v;break;
        case player_unit:gp[edx]=v;gp[eax]=fl.plus(gp[eax],gp[ebx],4);fl.minus(v,logic::none,4);break;   // ADD EAX,EBX; CMP EDX,-1
        case game:gp[edx]=v;break;
        case mode:gp[ecx]=v;break;                                                                     // (SETZ BL: update_mode())
        case control:gp[eax]=v;break;
        case control_byte:fl.minus(v&0xffu,0u,1);break;                                                // CMP byte [EAX],0
        case game_time:gp[ecx]=v;break;
        case zero:fl.scalar_compare(this->s.xmm[0],lane0_only(v),false);break;                         // COMISS XMM0,[46A0E8]
        case state:gp[eax]=v;fl.minus(v,logic::states::dead,4);break;                                  // CMP EAX,247140
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void restoring(){this->gp[edi]=this->pop(0x1587b9u);}
    void refresh(Kept &kept){kept={this->gp[edi],this->gp[esi],this->gp[ebx]&0xffu};}
    // The first: BL whether the player is dead (SETZ BL, then the flag's test). PUSH forced; CALL 158450.
    void update_mode(std::uint32_t forced,Kept &kept) {
        if(!updated) {
            set_low_byte(this->gp[ebx],kept.dead);
            this->push(0x158840u,forced);this->call(0x158842u,0x158847u,0x158450u,mode_child);updated=true;
        } else {
            this->push(0x15887au,forced);this->call(0x15887cu,0x158881u,0x158450u,mode_child);
        }
        refresh(kept);
    }
    // PUSH -1; LEA EAX,[ESI+10]; MOV EDX,EDI; CALL 246CF0.
    void view(Kept &kept) {
        auto &gp=this->gp;
        this->push(0x158855u,logic::none);gp[eax]=kept.camera+logic::camera::view;gp[edx]=kept.seat;
        this->call(0x15885cu,0x158861u,0x246cf0u,view_child);
        refresh(kept);
    }
    // PUSH 1; MOV EDX,247140; MOV EAX,EDI; CALL 158900.
    void enter(Kept &kept,std::uint32_t state_value) {
        auto &gp=this->gp;
        this->push(0x158861u,1u);gp[edx]=state_value;gp[eax]=kept.seat;
        this->call(0x15886au,0x15886fu,0x158900u,enter_child);
        refresh(kept);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        this->push(0x158790u,gp[esi]);this->push(0x158791u,gp[edi]);
        gp[edi]=gp[ecx];gp[esi]=fl.shift(5u*gp[edi],6u,4);this->test8(gp[eax]&0xffu);                  // LEA ESI,[EDI+EDI*4]; SHL ESI,6; TEST AL,AL
        gp[esi]+=logic::globals::cameras;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::seat_camera_death::update(memory,*this,gp[edi],(gp[eax]&0xffu)!=0u);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::reset){gp[esi]=this->pop(0x1587d0u);this->ret(0x1587d1u,0u);this->publish(0x1587d1u);return;}
        const std::uint32_t exit=outcome==Outcome::dead?0x15886fu:0x158881u;
        gp[ebx]=this->pop(exit);gp[edi]=this->pop(exit+1u);gp[esi]=this->pop(exit+2u);
        this->ret(exit+3u,0u);this->publish(exit+3u);
    }
};
template<class State,class Reference,class Mode,class View,class Enter>
bool bridge(State &s,Reference reference,Mode mode_child,View view_child,Enter enter_child) {
    return run_owner(s,0x158790u,0x158884u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Mode,View,Enter>(v,c,t,mode_child,view_child,enter_child);});
}
} // namespace halo2::native_seat_camera_death_detail
