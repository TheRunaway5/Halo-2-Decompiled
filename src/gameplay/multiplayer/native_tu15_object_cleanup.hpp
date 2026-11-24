#pragma once
// Boundary adapter for the multiplayer object cleanup, entered at 15C4E0
// (native_tu15_object_cleanup_systems.hpp). Its callers and children keep
// the original ABI, which the adapter owns:
//   - no arguments; RET; SUB ESP,1C, EBP/ESI saved before the iterator,
//     EBX/EDI once there is an object; the frame is addressed from the live
//     ESP, so a child that leaves the stack unbalanced moves it as it does
//     for the original;
//   - the children through the unit's routes: BCEE0 (the iterator pushed),
//     105FD0 and 164750 (EAX the handle), the variant's check (ECX the
//     variant, the handle pushed, the target read from its table +0x98),
//     BBBC0 (DL zero) and BA570 (the handle pushed); each gets the
//     original's registers (the IDIV quotients and remainders, BL, the
//     tests), flags and XMM (XMM0 the age with the upper lanes a child
//     left, XMM1 the timeout), and every register it returns comes back;
//   - the exit: the saved registers restored, ADD ESP,1C decides the flags;
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_CLEANUP=1 restores the translated body.
// HALO2_VERIFY_OBJECT_CLEANUP=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/multiplayer/native_tu15_object_cleanup_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_object_cleanup_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_cleanup;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_CLEANUP");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_CLEANUP");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    quiet, active, none_found, found, other_slot, own_slot, item, biped, other_type, weapon, plain_item, tied, untied,
    held_item, checked, kept_free, aged_item, corpse_flagged, corpse_unflagged, corpse_expired, corpse_kept,
    other_flagged, other_unflagged, not_started, started, other_expired, other_kept, removed, next_object, last_object,
    branch_count
};
static_assert(branch_count<=64u);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object cleanup verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline __m128 with_lane0(__m128 x,std::uint32_t bits){return _mm_move_ss(x,lane0_only(bits));}
inline constexpr bool backend=logic::sse::available;

inline constexpr Edge cleanup_edges[]{
    {0x15c4e5u,0x15c754u,quiet},{0x15c4e5u,0x15c4f4u,active},
    {0x15c545u,0x15c74fu,none_found},{0x15c545u,0x15c54fu,found},
    {0x15c55cu,0x15c739u,other_slot},{0x15c55cu,0x15c57au,own_slot},
    {0x15c57au,0x15c59eu,item},{0x15c57au,0x15c5d9u,item},{0x15c57au,0x15c67cu,biped},{0x15c57au,0x15c6cau,other_type},
    {0x15c57au,0x15c59eu,weapon},{0x15c57au,0x15c5d9u,plain_item},
    {0x15c5ceu,0x15c63cu,tied},{0x15c5ceu,0x15c5d9u,untied},
    {0x15c5e2u,0x15c739u,held_item},{0x15c5d9u,0x15c5f8u,checked},{0x15c5e2u,0x15c5f8u,checked},
    {0x15c5fcu,0x15c739u,kept_free},{0x15c60du,0x15c739u,kept_free},{0x15c60du,0x15c61au,aged_item},
    {0x15c67cu,0x15c68eu,corpse_flagged},{0x15c67cu,0x15c739u,corpse_unflagged},
    {0x15c6abu,0x15c71du,corpse_expired},{0x15c6abu,0x15c6b5u,corpse_kept},
    {0x15c6cau,0x15c6d4u,other_flagged},{0x15c6cau,0x15c739u,other_unflagged},
    {0x15c6d7u,0x15c739u,not_started},{0x15c6d7u,0x15c6dfu,started},
    {0x15c6f6u,0x15c71du,other_expired},{0x15c6f6u,0x15c700u,other_kept},
    {0x15c72au,0x15c72fu,removed},
    {0x15c743u,0x15c551u,next_object},{0x15c743u,0x15c74du,last_object},
};
template<class State,class Route,class Free> struct Cleanup : Port<State> {
    using Access=logic::object_cleanup::Access;
    using Local=logic::object_cleanup::Local;
    using Kept=logic::object_cleanup::Kept;
    using Age=logic::object_cleanup::Age;
    Route &route;Free &free_child;
    // E: where the return address sits; ESP is E-24 before the loop's saves
    // and E-2C inside it.
    std::uint32_t frame{};bool saved{},first{true};
    Cleanup(State &s,std::uint32_t *c,Trace<State> *t,Route &r,Free &f):Port<State>(s,c,t,cleanup_edges),route(r),free_child(f){}
    std::uint32_t local(Local l)const {
        switch(l) {
        case Local::tick:return frame-0x1cu;
        case Local::setting:return frame-0x18u;
        case Local::current:return frame-0x14u;
        case Local::iterator:return frame-0x10u;
        case Local::iterator_byte:return frame-0xcu;
        case Local::iterator_word:return frame-0xau;
        case Local::iterator_handle:return frame-8u;
        case Local::iterator_marker:return frame-4u;
        }
        return 0;
    }
    void refresh(Kept &kept,std::uint32_t below) {
        frame=this->gp[esp]+below;
        kept={this->gp[ebp],this->gp[esi],this->gp[edi],this->gp[ebx]};
    }
    // CDQ; IDIV divisor: EAX the quotient, EDX the remainder.
    void divide(std::uint32_t divisor) {
        auto &gp=this->gp;
        const auto dividend=std::int32_t(gp[eax]);
        gp[eax]=std::uint32_t(dividend/std::int32_t(divisor));gp[edx]=std::uint32_t(dividend%std::int32_t(divisor));
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case engine:return 0x15c4e0u;
        case mode:return 0x15c4e5u;
        case variant:return 0x15c4f4u;
        case setting:return 0x15c4fau;
        case game_time:this->push(0x15c500u,gp[ebp]);return 0x15c501u;
        case tick:return 0x15c507u;
        case tick_store:return 0x15c50au;
        case setting_store:
            this->push(0x15c50eu,gp[esi]);gp[ecx]=local(Local::iterator);this->push(0x15c513u,gp[ecx]);
            return 0x15c514u;
        case marker_store:return 0x15c518u;
        case kind_store:return 0x15c520u;
        case byte_clear:return 0x15c528u;
        case word_clear:return 0x15c52du;
        case handle_clear:return 0x15c534u;
        case current_store:return 0x15c545u;
        case slot_tick:
            if(!saved){saved=true;this->push(0x15c54fu,gp[ebx]);this->push(0x15c550u,gp[edi]);}
            return 0x15c551u;
        case slot_handle:return 0x15c55cu;
        case type:return 0x15c57au;
        case headers:return 0x15c59eu;
        case rows:return 0x15c5a4u;
        case weapon_data:gp[eax]=gp[edi]*3u;return 0x15c5aau;                                        // LEA EAX,[EDI+EDI*2]
        case weapon_tag:return 0x15c5aeu;
        case instances:return 0x15c5b0u;
        case weapon_definition:gp[ecx]=this->flags.shift(this->flags.logic(gp[ecx]&0xffffu,4),4u,4);return 0x15c5bfu;
        case engine_field:return 0x15c5c3u;
        case holder_handle:return 0x15c5cau;
        case holder:return 0x15c5d9u;
        case item_flags:return 0x15c5e2u;
        case checked_handle:return 0x15c5f8u;
        case item_flags_again:return 0x15c60du;
        case dropped:return 0x15c61au;
        case item_tick:return 0x15c620u;
        case item_setting:return 0x15c626u;
        case item_timeout:return 0x15c62au;
        case variant_again:return 0x15c63cu;
        case variant_index_read:return 0x15c642u;
        case variant_object:return 0x15c648u;
        case check_handle:return 0x15c64fu;
        case vtable:return 0x15c653u;
        case check_target:this->push(0x15c655u,gp[esi]);return 0x15c656u;
        case fade_flags:return 0x15c67cu;
        case created:return 0x15c68eu;
        case corpse_tick:return 0x15c694u;
        case corpse_setting:return 0x15c69au;
        case corpse_timeout:return 0x15c69eu;
        case corpse_seconds:return 0x15c6abu;
        case hold_setting:return 0x15c6b5u;
        case held:return 0x15c6b9u;
        case hold_timeout:return 0x15c6bfu;
        case object_flags:return 0x15c6cau;
        case start_tick:return 0x15c6d4u;
        case other_tick:return 0x15c6d7u;
        case other_created:return 0x15c6dfu;
        case other_setting:return 0x15c6e5u;
        case other_timeout:return 0x15c6e9u;
        case other_seconds:return 0x15c6f6u;
        case late_timeout:return 0x15c700u;
        case age_tick:return 0x15c709u;
        case age_seconds:return 0x15c713u;
        case delete_handle:return 0x15c71du;
        case time_reload:return 0x15c72fu;
        case current_next:return 0x15c743u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case engine:case setting:case weapon_data:case weapon_definition:case holder_handle:case checked_handle:
        case dropped:case item_setting:case vtable:case created:case corpse_setting:case held:case start_tick:
            gp[eax]=v;break;
        // MOV CL,[EAX+C]; SUB ESP,1C; CMP CL,4.
        case mode:
            gp[ecx]=(gp[ecx]&0xffffff00u)|(v&0xffu);gp[esp]=this->flags.minus(gp[esp],0x1cu,4);
            this->flags.minus(v&0xffu,logic::quiet_mode,1);break;
        case variant:case rows:case instances:case variant_index_read:case hold_setting:case other_setting:gp[edx]=v;break;
        case game_time:case time_reload:gp[ebp]=v;break;
        case tick:case headers:case weapon_tag:case variant_again:case variant_object:case other_tick:
            gp[ecx]=v;if(a==other_tick)this->compare32(v,gp[eax]);break;
        // MOV EAX,[EBP+8]; CDQ; MOV ECX,1E; IDIV ECX.
        case slot_tick:gp[eax]=v;gp[ecx]=logic::slot_period;divide(gp[ecx]);break;
        // MOV EDI,[handle]; AND EDI,FFFF; MOV EAX,EDI; MOV EBX,1E; MOV ECX,EDX;
        // CDQ; IDIV EBX; CMP EDX,ECX.
        case slot_handle:
            gp[edi]=this->flags.logic(v&0xffffu,4);gp[eax]=gp[edi];gp[ebx]=logic::slot_period;gp[ecx]=gp[edx];
            divide(gp[ebx]);this->compare32(gp[edx],gp[ecx]);break;
        // MOVSX ECX,[ESI+AA]; MOV EAX,1; SHL EAX,CL; TEST AL,1C; items: MOV
        // EDX,1; SHL EDX,CL; MOV BL,1; TEST DL,4; otherwise TEST AL,1.
        case type: {
            gp[ecx]=std::uint32_t(std::int32_t(std::int8_t(v)));
            gp[eax]=this->flags.shift(1u,gp[ecx]&0xffu,4);this->test8(gp[eax]&logic::object_type::items);
            if(gp[eax]&0xffu&logic::object_type::items) {
                gp[edx]=this->flags.shift(1u,gp[ecx]&0xffu,4);gp[ebx]=(gp[ebx]&0xffffff00u)|1u;
                this->test8(gp[edx]&logic::object_type::weapon);
            } else this->test8(gp[eax]&logic::object_type::biped);
            break;
        }
        case engine_field:gp[edi]=std::uint32_t(std::int32_t(std::int16_t(v)));break;
        case holder:this->flags.minus(v&0xffu,0xffu,1);break;
        // MOV CL,[ESI+12C]; SHR CL,2; TEST CL,1; (TEST BL,BL).
        case item_flags:case fade_flags: {
            const auto shifted=this->flags.right(v&0xffu,2u,1,false);
            gp[ecx]=(gp[ecx]&0xffffff00u)|(shifted&0xffu);this->test8(shifted&1u);
            if(a==item_flags && !(shifted&1u))this->test8(gp[ebx]);
            break;
        }
        case item_flags_again:this->test8(v&1u);break;
        case item_tick:case corpse_tick:gp[edx]=this->flags.minus(v,gp[eax],4);break;              // SUB EDX,EAX
        case check_handle:case delete_handle:gp[esi]=v;break;
        case object_flags:gp[eax]=this->flags.right(v,14u,4,false);this->test8(gp[eax]&1u);break;
        case other_created:gp[ecx]=this->flags.minus(gp[ecx],v,4);break;
        case age_tick:gp[ecx]=this->flags.minus(v,gp[eax],4);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // BCEE0: the first time after the setup (its argument was pushed
    // early); then LEA EAX (the iterator); PUSH EAX. MOV ESI,EAX; TEST ESI,ESI.
    std::uint32_t iterate(Kept &kept) {
        auto &gp=this->gp;
        if(first) {
            first=false;
            this->call_routed(route,0x15c53cu,0x15c541u,0xbcee0u);
            gp[esi]=gp[eax];this->test32(gp[esi]);refresh(kept,0x24u);
        } else {
            gp[eax]=local(Local::iterator);this->push(0x15c739u,gp[eax]);
            this->call_routed(route,0x15c73au,0x15c73fu,0xbcee0u);
            gp[esi]=gp[eax];this->test32(gp[esi]);refresh(kept,0x2cu);
        }
        return gp[esi];
    }
    // CALL 105FD0; TEST EDI,EDI; MOV BL,AL.
    std::uint32_t holder_check(Kept &kept) {
        auto &gp=this->gp;
        this->call_routed(route,0x15c5ceu,0x15c5d3u,0x105fd0u);
        this->test32(gp[edi]);gp[ebx]=(gp[ebx]&0xffffff00u)|(gp[eax]&0xffu);refresh(kept,0x2cu);
        return gp[eax]&0xffu;
    }
    // CALL 164750 (EAX the handle; from ESI on the engine path); TEST AL,AL
    // (and TEST BL,BL when free on the item path).
    std::uint32_t free_check(Kept &kept,bool engine_path) {
        auto &gp=this->gp;
        if(engine_path){gp[eax]=gp[esi];this->call(0x15c666u,0x15c66bu,0x164750u,free_child);}
        else this->call(0x15c5fcu,0x15c601u,0x164750u,free_child);
        this->test8(gp[eax]);
        if(!engine_path && (gp[eax]&0xffu))this->test8(gp[ebx]);
        refresh(kept,0x2cu);
        return gp[eax]&0xffu;
    }
    // CALL [EAX+98] (the handle pushed); TEST AL,AL.
    std::uint32_t engine_check(Kept &kept,std::uint32_t target) {
        auto &gp=this->gp;
        this->call_routed(route,0x15c656u,0x15c65cu,target);
        this->test8(gp[eax]);refresh(kept,0x2cu);
        return gp[eax]&0xffu;
    }
    // MOVSS XMM1 (the timeout); CVTSI2SS and MULSS XMM0 (the age, upper
    // lanes kept); COMISS XMM1,XMM0.
    void aged(Age,std::uint32_t timeout,std::uint32_t age) {
        auto &x=this->s.xmm;
        x[1]=lane0_only(timeout);x[0]=with_lane0(x[0],age);
        this->flags.scalar_compare(x[1],x[0],false);
    }
    // PUSH ESI; XOR DL,DL; CALL BBBC0; PUSH ESI; CALL BA570.
    void remove(Kept &kept) {
        auto &gp=this->gp;
        this->push(0x15c721u,gp[esi]);gp[edx]=(gp[edx]&0xffffff00u)|this->flags.logic(0,1);
        this->call_routed(route,0x15c724u,0x15c729u,0xbbbc0u);
        this->push(0x15c729u,gp[esi]);
        this->call_routed(route,0x15c72au,0x15c72fu,0xba570u);
        refresh(kept,0x2cu);
    }
    void run() {
        auto &gp=this->gp;
        frame=gp[esp];
        Memory<Cleanup> memory{*this};
        const auto exit=logic::object_cleanup::sweep(memory,*this);
        if(exit==logic::object_cleanup::Exit::swept){gp[edi]=this->pop(0x15c74du);gp[ebx]=this->pop(0x15c74eu);}
        if(exit!=logic::object_cleanup::Exit::quiet){gp[esi]=this->pop(0x15c74fu);gp[ebp]=this->pop(0x15c750u);}
        gp[esp]=this->flags.plus(gp[esp],0x1cu,4);
        this->ret(0x15c754u,0);this->publish(0x15c754u);
    }
};

template<class State,class Reference,class Route,class Free>
bool bridge(State &s,Reference reference,Route route,Free free) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if((_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x15c4e0u,0x15c754u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Cleanup<State,Route,Free>(v,c,t,route,free);});
    return true;
}
} // namespace halo2::native_object_cleanup_detail
