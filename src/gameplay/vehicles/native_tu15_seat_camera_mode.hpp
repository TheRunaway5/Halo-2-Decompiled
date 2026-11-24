#pragma once
// Boundary adapter for the seat camera mode, entered at 158450
// (native_tu15_seat_camera_mode_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EDI the seat, the forced
// byte on the stack (RET 4); a word of frame (the pushed ECX); EBX and ESI
// saved; 158340 gets EAX and EBX the unit and the word's address pushed
// (RET 4) and answers AX, 158900 EAX the seat, EDX the state and the flag
// pushed (RET 4), every register each returns coming back; EAX, ECX and EDX
// as the original leaves them; the flags of the last comparison (or the
// last child's). Each access publishes its original instruction.
// HALO2_REFERENCE_SEAT_CAMERA_MODE=1 restores the translated body.
// HALO2_VERIFY_SEAT_CAMERA_MODE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/vehicles/native_tu15_seat_camera_mode_systems.hpp"

namespace halo2::native_seat_camera_mode_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_seat_camera_mode;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_SEAT_CAMERA_MODE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_SEAT_CAMERA_MODE");return p && *p;}();
    return value;
}
enum Branch : unsigned {unchanged, kept, mode_one, other_mode, branch_count};
static_assert(unsigned(logic::seat_camera_mode::Outcome::other_mode)==Branch::other_mode);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Seat camera mode verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t mode_one_clears[]{0x1584d5u,0x1584d8u,0x1584dbu,0x1584deu,0x1584e1u,0x1584e5u,0x1584ebu,0x1584f0u,0x1584ffu,0x158506u};
inline constexpr std::uint32_t other_clears[]{0x158540u,0x158545u};
template<class State,class Route,class Enter> struct SeatCameraMode : Port<State> {
    using Access=logic::seat_camera_mode::Access;
    using Kept=logic::seat_camera_mode::Kept;
    Route &route;
    Enter &enter_child;
    bool to_mode_one{},forced_value{},unit_loaded{};
    unsigned clears{};
    SeatCameraMode(State &s,std::uint32_t *c,Trace<State> *t,Route &r,Enter &e):Port<State>(s,c,t,{}),route(r),enter_child(e){}
    std::uint32_t argument()const{return this->gp[esp]+0x10u;}
    std::uint32_t local()const{return this->gp[esp]+8u;}
    void refresh(Kept &kept){kept={this->gp[edi],this->gp[esi],this->gp[ebx]&0xffffu};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case seats:return 0x158464u;
        case seat_player:return 0x158469u;
        case players:return 0x158477u;
        case player_data:return 0x15847du;
        case unit:gp[eax]=fl.product(fl.logic(gp[eax]&0xffffu,4),logic::player_size);return 0x15848bu;   // AND EAX,FFFF; IMUL EAX,21C
        case forced:return 0x1584a0u;
        case word:return 0x1584a4u;
        case remembered:return 0x1584afu;
        case state:return to_mode_one?0x1584c3u:0x15852au;
        case clear_store:
            if(to_mode_one) {
                if(!clears){this->s.xmm[0]=_mm_setzero_ps();fl.minus(gp[ecx]&0xffu,0u,1);}                // XORPS; CMP CL,DL
                if(clears==6u)set_low_byte(gp[edx],forced_value?0u:1u);                                    // SETZ DL
                return mode_one_clears[clears++];
            }
            if(!clears){this->s.xmm[0]=_mm_setzero_ps();fl.minus(gp[ecx]&0xffu,0u,1);
                set_low_byte(gp[eax],forced_value?0u:1u);gp[edx]=logic::other_state;}                   // XORPS; CMP; SETZ AL; MOV EDX
            return other_clears[clears++];
        case one:return 0x1584f5u;
        // PUSH EDX; MOV EDX,245EA0.
        case blend_store:this->push(0x15850cu,gp[edx]);gp[edx]=logic::mode_one_state;return 0x158512u;
        case remember_store:return to_mode_one&&clears?0x15851cu:0x158552u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case seats:gp[eax]=v;break;
        case seat_player:gp[eax]=v;fl.minus(v,logic::none,4);set_low_byte(gp[ecx],v!=logic::none?1u:0u);
            this->test8(gp[ecx]);break;                                                                // CMP EAX,-1; SETNZ CL; TEST CL,CL
        case players:gp[ecx]=v;break;
        case player_data:gp[edx]=v;break;
        case unit:gp[eax]=v;unit_loaded=true;break;
        case forced:set_low_byte(gp[ecx],v);forced_value=v&0xffu;break;
        case word:gp[ebx]=(gp[ebx]&0xffff0000u)|(v&0xffffu);gp[edx]=fl.logic(0u,4);break;             // MOV BX; XOR EDX,EDX
        case remembered:fl.minus(v,gp[ebx]&0xffffu,2);break;                                           // CMP word [ESI+58],BX
        case state:fl.minus(v,to_mode_one?logic::other_state:logic::mode_one_state,4);break;
        case one:this->s.xmm[0]=lane0_only(v);gp[eax]=gp[edi];break;                                   // MOVSS; MOV EAX,EDI
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // CMP EDI,-1; (OR EAX,-1); LEA ECX,[ESP+8]; PUSH ECX; MOV EBX,EAX; CALL 158340; CMP CL,DL; CMP AX,1.
    std::uint32_t mode(Kept &kept,std::uint32_t unit_handle) {
        auto &gp=this->gp;auto &fl=this->flags;
        if(!unit_loaded)gp[eax]=fl.logic(0xffffffffu,4);                                              // OR EAX,-1
        else gp[eax]=unit_handle;
        gp[ecx]=gp[esp]+8u;this->push(0x158498u,gp[ecx]);gp[ebx]=gp[eax];
        this->call_routed(route,0x15849bu,0x1584a0u,0x158340u);
        refresh(kept);
        to_mode_one=(gp[eax]&0xffffu)==logic::mode_one;
        return gp[eax];
    }
    // (PUSH EAX; MOV EAX,EDI;) CALL 158900.
    void enter(Kept &kept,std::uint32_t,bool) {
        auto &gp=this->gp;
        if(to_mode_one)this->call(0x158517u,0x15851cu,0x158900u,enter_child);
        else{this->push(0x15854au,gp[eax]);gp[eax]=kept.seat;this->call(0x15854du,0x158552u,0x158900u,enter_child);}
        refresh(kept);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        this->push(0x158450u,gp[ecx]);this->push(0x158451u,gp[ebx]);this->push(0x158452u,gp[esi]);
        gp[esi]=fl.plus(fl.shift(5u*gp[edi],6u,4),logic::globals::cameras,4);                          // LEA; SHL ESI,6; ADD
        fl.minus(gp[edi],logic::none,4);                                                              // CMP EDI,-1
        Memory<SeatCameraMode> memory{*this};
        const auto outcome=logic::seat_camera_mode::update(memory,*this,gp[edi]);
        this->reached(unsigned(outcome));
        const auto exit=outcome==logic::seat_camera_mode::Outcome::mode_one?0x158520u:0x158556u;
        gp[esi]=this->pop(exit);gp[ebx]=this->pop(exit+1u);gp[ecx]=this->pop(exit+2u);
        this->ret(exit+3u,4u);this->publish(exit+3u);
    }
};
template<class State,class Reference,class Route,class Enter> bool bridge(State &s,Reference reference,Route route,Enter enter) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x158450u,0x158559u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return SeatCameraMode<State,Route,Enter>(v,c,t,route,enter);});
    return true;
}
} // namespace halo2::native_seat_camera_mode_detail
