#pragma once
// Boundary adapter for the unit weapon controls, C7DD0
// (native_tu15_unit_weapon_controls_systems.hpp). Its callers and the
// children keep the original ABI, which the adapter owns: the unit's handle
// on the stack, AL the answer (RET 4); 0x28 bytes of frame with EBX, EBP,
// ESI and EDI saved below them, read from the ESP each child left. E8E10
// (ECX the action, the unit pushed, RET 4), 116480, 113190 (the unit
// pushed, RET 4), CE3A0 (EAX the unit, CX the grenade, EBX 0), CAAA0,
// 105500 (weapon, word and scale pushed, RET C) and D3A80 (the unit pushed,
// RET 4) are direct; 1164D0, 116400 and 116440 (0.5 pushed, RET 4) and
// 18C000 (EAX 0x4000095, ESI the unit, EDI the request) go through the
// unit's dispatch; every register each returns comes back. XMM0 and XMM1
// hold what the original's scalar moves leave in them at each call. Each
// access publishes its original instruction.
// HALO2_REFERENCE_UNIT_WEAPON_CONTROLS=1 restores the translated body.
// HALO2_VERIFY_UNIT_WEAPON_CONTROLS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_unit_weapon_controls_systems.hpp"

namespace halo2::native_unit_weapon_controls_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_unit_weapon_controls;
using Access=logic::unit_weapon_controls::Access;
using Request=logic::unit_weapon_controls::Request;
using Kept=logic::unit_weapon_controls::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_UNIT_WEAPON_CONTROLS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_UNIT_WEAPON_CONTROLS");return v;}
enum Branch : unsigned {counters, stance, stunned, weapon_change, trigger, grenade, zoom, weapon_sound, scaled, sound, hand, blocking,
    change_active, pair, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Unit weapon controls"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline std::uint32_t low_word(std::uint32_t reg,std::uint32_t v){return (reg&0xffff0000u)|(v&0xffffu);}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline constexpr std::uint32_t sites[]{0xc7dd3u,0xc7dd9u,0xc7ddcu,0xc7de3u,0xc7df5u,0xc7df9u,0xc7dfdu,0xc7e09u,0xc7e0du,0xc7e1au,
    0xc7e1eu,0xc7e29u,0xc7e35u,0xc7e53u,0xc7e58u,0xc7e64u,0xc7e6au,0xc7e6fu,0xc7e7bu,0xc7e81u,0xc7e86u,0xc7e92u,0xc7e9fu,0xc7ee6u,
    0xc7eebu,0xc7efcu,0xc7f0eu,0xc7f30u,0xc7f3bu,0xc7f45u,0xc7f4bu,0xc7f4eu,0xc7f7au,0xc7f99u,0xc7f9fu,0xc7fa7u,0xc7fb5u,0xc7fbdu,
    0xc7fceu,0xc7fd4u,0xc7fe1u,0xc7feeu,0xc8002u,0xc800du,0xc8015u,0xc801bu,0xc801eu,0xc8022u,0xc8026u,0xc803au,0xc8044u,0xc804eu,
    0xc8059u,0xc805du,0xc805fu,0xc806du,0xc8071u,0xc8080u,0xc8088u,0xc808eu,0xc80a3u,0xc80a9u,0xc80b1u,0xc80b9u,0xc80bfu,0xc80dfu,
    0xc80ecu,0xc80f4u,0xc80f8u,0xc80feu,0xc810bu,0xc810fu,0xc8122u,0xc8130u,0xc813eu,0xc8148u,0xc814eu,0xc8151u,0xc8155u,0xc8159u,
    0xc8166u,0xc8173u,0xc817au,0xc817eu,0xc8188u,0xc8194u,0xc8197u,0xc819bu,0xc819fu,0xc81a6u,0xc81b2u,0xc81b9u,0xc81c5u,0xc81ccu,
    0xc81d8u,0xc81fcu,0xc8200u,0xc820au,0xc831bu,0xc832fu,0xc8335u,0xc833cu,0xc8346u,0xc834eu,0xc8352u,0xc8356u,0xc835cu,0xc836cu,
    0xc8376u,0xc8380u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::changed_load)+1u);
// E8E10's sites: the PUSH EBX and the CALL, the first and second of a pair.
struct RequestSite {std::uint32_t push,call;};
inline constexpr RequestSite request_sites[4][2]{{{0xc7e3du,0xc7e43u},{0xc7e48u,0xc7e4eu}},{{0xc7f14u,0xc7f1au},{0xc7f1fu,0xc7f25u}},
    {{0xc7f6eu,0xc7f6fu},{0xc7f6eu,0xc7f6fu}},{{0xc7f86u,0xc7f89u},{0xc7f8eu,0xc7f94u}}};
template<class State,class Children,class Dispatch> struct Adapter : Port<State> {
    Children &children;      // the direct children, by entry address
    Dispatch &dispatch;
    std::uint32_t depth{};   // bytes pushed below the saved registers (negative before them)
    unsigned paired{};       // the next request of a pair is its second
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k,Dispatch &d):Port<State>(s,c,t,{}),children(k),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept) {
        const auto &gp=this->gp;
        kept.handle=kept.first=gp[ebx];kept.unit=gp[ebp];kept.definition=kept.hand=gp[esi];kept.weapon_hand=kept.headers=gp[edi];
    }
    void direct(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){children(v,target);};
        this->call(pc,pc+5u,target,child);depth-=popped;
        refresh(kept);
    }
    void routed(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){dispatch(v,target,pc);};
        this->call(pc,pc+5u,target,child);depth-=popped;
        refresh(kept);
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers_load:case instances_load:case grenade_argument:case zoom_offset:case zoom_unit:case weapon_datums:case weapon_instances:
        case zoom_out_sound:case zoom_in_sound:case game_globals_load:case global_zoom_out:case global_zoom_in:case hand_datums:
        case busy_count:case action_datums:case bits_first:case bits_second:case bits_third:case hand_action_bits:case weapon_reload:
            gp[ecx]=v;break;
        case block_first:case block_second:case block_third:gp[ecx]=sx16(v);break;                       // MOVSX ECX,word
        case desired_weapon:gp[ecx]=sx8(v);break;                                                          // MOVSX ECX,byte
        case desired_grenade_word:case hand_slot_word:gp[ecx]=low_word(gp[ecx],sx8(v));break;               // MOVSX CX,byte
        case object_type:case object_flags:set_low_byte(gp[ecx],v);break;                                  // MOV CL
        case datums_load:case definition_flags:case time_global:case time_load:case weapon_object:case hand_offset:
        case hand_weapon_handle:case action_offset:case action_unit:case hand_controls:case scale_load:case pair_argument:
            gp[edx]=v;break;
        case current_zoom:case zoom_slot:gp[edx]=low_word(gp[edx],sx8(v));break;                           // MOVSX DX,byte
        case handle_load:gp[ebx]=v;break;
        case requested_zoom:gp[ebx]=low_word(gp[ebx],sx8(v));break;                                         // MOVSX BX,byte
        case unit_load:gp[ebp]=v;break;
        case unit_tag:case unit_state:case stance_controls:case weapon_flags:case chosen_argument:case zoom_argument:case zoom_datums:
        case zoom_weapon:case weapon_tag:case weapon_definition_load:case information_load:case global_levels:case hands_flags:
        case hand_unit:case hand_argument:case controls_word:
            gp[eax]=v;break;
        case hand_weapon:gp[eax]=sx8(v);break;                                                             // MOVSX EAX,byte
        case zoom_levels:gp[eax]=low_word(gp[eax],v);break;                                                // MOV AX,word
        case first_counter:case second_counter:case third_counter:case desired_grenade:case hand_slot:case first_load:case changed_load:
            set_low_byte(gp[eax],v);break;                                                                 // MOV AL
        case definition_load:case sound_argument:case definition_reload:case hand_reload:gp[esi]=v;break;
        case zoom_headers:case hand_headers:gp[edi]=v;break;
        case hand_action_block:gp[edi]=sx16(v);break;                                                      // MOVSX EDI,word
        case default_scale:case first_scale:case second_scale:this->s.xmm[0]=lane0(v);break;               // MOVSS XMM0
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The frame and the unit.
    void framing() {                                                                                     // SUB ESP,28
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);depth=0u-16u;
    }
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / ESI / EDI
        auto &gp=this->gp;
        static constexpr std::uint32_t pcs[]{0xc7de2u,0xc7df4u,0xc7e08u,0xc7e19u};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t header_offset(std::uint32_t handle) {                                                 // MOV EAX,EBX; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]; SHL EAX,2
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=3u*fl.logic(handle&logic::handle_mask,4);
        return gp[eax]=fl.shift(gp[eax],2u,4);
    }
    std::uint32_t tag_offset(std::uint32_t tag) {                                                       // AND EAX,FFFF; SHL EAX,4
        auto &fl=this->flags;
        return this->gp[eax]=fl.shift(fl.logic(tag&logic::handle_mask,4),4u,4);
    }
    bool shifted(unsigned reg,std::uint32_t v,unsigned n) {                                             // SHR reg,n; TEST low byte,1
        auto &gp=this->gp;
        gp[reg]=this->flags.right(v,n,4,false);this->flags.logic(gp[reg]&1u,1);
        return gp[reg]&1u;
    }
    bool passive(std::uint32_t flags_value){return shifted(edx,flags_value,logic::definition::passive_bit);}
    // The timers.
    bool counting(std::uint32_t v){this->flags.logic(v,1);return v!=0u && !(v&0x80u);}                    // TEST AL,AL; JLE
    bool running(std::uint32_t v){this->flags.logic(v,1);return v!=0u;}                                   // TEST AL,AL; JBE
    std::uint32_t counted_down(std::uint32_t v) {                                                        // DEC AL
        set_low_byte(this->gp[eax],this->flags.increment(v,1,true));
        this->reached(Branch::counters);
        return low_byte(this->gp[eax]);
    }
    void request(Kept &kept,Request where,std::uint32_t action) {                                       // PUSH EBX; MOV ECX,action (XOR ECX,ECX); CALL E8E10
        auto &gp=this->gp;
        const auto site=request_sites[unsigned(where)][where==Request::weapon_changed?0u:paired];
        paired=where==Request::weapon_changed?paired:paired^1u;
        put(site.push,kept.handle);
        gp[ecx]=where==Request::triggered && action==0u?this->flags.logic(0u,4):action;
        direct(site.call,0xe8e10u,4u,kept);
        if(where==Request::stunned)this->reached(Branch::stunned);
        if(where==Request::weapon_changed)this->reached(Branch::weapon_change);
        if(where==Request::triggered)this->reached(Branch::trigger);
    }
    // The stance.
    bool state(std::uint32_t v){return shifted(eax,v,logic::unit::state_bit);}
    bool stance(std::uint32_t controls){return this->flags.logic(controls&(1u<<logic::unit::stance_bit),4)!=0u;}   // TEST EAX,8000000
    bool biped(std::uint32_t) {                                                                          // MOV EDX,1; SHL EDX,CL; TEST DL,1
        auto &gp=this->gp;
        gp[edx]=this->flags.shift(1u,low_byte(gp[ecx]),4);this->flags.logic(gp[edx]&1u,1);
        return gp[edx]&1u;
    }
    bool biped_stance(std::uint32_t controls) {                                                         // TEST AH,40
        return this->flags.logic((controls>>8)&(1u<<(logic::unit::biped_stance_bit-8u)),1)!=0u;
    }
    bool in_stance(Kept &kept,bool second) {                                                             // MOV EAX,EBX; CALL 1164D0 / 116480; TEST AL,AL
        auto &gp=this->gp;
        gp[eax]=kept.handle;
        if(second)routed(0xc7ed1u,0x1164d0u,0u,kept);else direct(0xc7eb8u,0x116480u,0u,kept);
        this->test8(gp[eax]);
        return low_byte(gp[eax])!=0u;
    }
    void blend_stance(Kept &kept,bool second) {                                                          // PUSH 3F000000; MOV EAX,EBX; CALL 116440 / 116400
        put(second?0xc7edau:0xc7ec1u,logic::half);this->gp[eax]=kept.handle;
        routed(second?0xc7ee1u:0xc7ec8u,second?0x116440u:0x116400u,4u,kept);
        this->reached(Branch::stance);
    }
    // Weapon changes.
    bool weapons_fixed(std::uint32_t v){return shifted(eax,v,logic::definition::no_weapon_changes_bit);}
    bool still(std::uint32_t v) {                                                                        // SHR CL,2; TEST CL,1
        auto &gp=this->gp;
        set_low_byte(gp[ecx],this->flags.right(v&0xffu,logic::unit::still_bit,1,false));this->flags.logic(gp[ecx]&1u,1);
        return gp[ecx]&1u;
    }
    bool stunned(std::uint32_t v){return this->flags.logic(v&logic::unit::stunned_bit,1)!=0u;}              // TEST byte,8
    std::uint32_t first_weapon_hand(){return this->gp[edi]=this->flags.logic(0u,4);}                         // XOR EDI,EDI
    bool holding(std::uint32_t current){this->compare32(current,logic::none);return current!=logic::none;}   // CMP EAX,-1
    bool changing(std::uint32_t desired,std::uint32_t current){this->compare32(desired,current);return desired!=current;}   // CMP ECX,EAX
    bool hand_is(std::uint32_t hand,std::uint32_t n) {                                                   // TEST EDI,EDI / CMP EDI,1
        if(n==0u)this->test32(hand);else this->compare32(hand,n);
        return hand==n;
    }
    std::uint32_t next_weapon_hand(std::uint32_t hand) {                                                 // INC EDI; CMP EDI,2
        auto &gp=this->gp;
        gp[edi]=this->flags.increment(hand,4,false);this->compare32(gp[edi],2u);
        return gp[edi];
    }
    bool triggered(std::uint32_t controls){return this->flags.logic(controls&(1u<<logic::unit::trigger_bit),4)!=0u;}   // TEST dword,10000000
    // The grenade.
    bool differ(std::uint32_t wanted,std::uint32_t current){this->flags.minus(wanted,current,1);return wanted!=current;}   // CMP AL,byte
    bool busy(Kept &kept,std::uint32_t handle) {                                                         // PUSH ECX; CALL 113190; TEST AL,AL
        put(0xc7fabu,handle);direct(0xc7facu,0x113190u,4u,kept);
        this->test8(this->gp[eax]);
        return low_byte(this->gp[eax])!=0u;
    }
    std::uint32_t chosen_grenade(Kept &kept) {                                                           // XOR EBX,EBX; CALL CE3A0; CMP AX,FFFF
        auto &gp=this->gp;
        gp[ebx]=this->flags.logic(0u,4);kept.handle=kept.first=0u;
        direct(0xc7fc3u,0xce3a0u,0u,kept);
        const auto chosen=gp[eax]&0xffffu;
        this->flags.minus(chosen,0xffffu,2);
        if(chosen!=0xffffu)this->reached(Branch::grenade);
        return chosen;
    }
    // The zoom.
    bool can_zoom(Kept &kept) {                                                                          // CALL CAAA0; TEST AL,AL
        direct(0xc7fd8u,0xcaaa0u,0u,kept);
        this->test8(this->gp[eax]);
        return low_byte(this->gp[eax])!=0u;
    }
    std::uint32_t unzoomed(){this->gp[ebx]=this->flags.logic(0xffffffffu,4);return logic::unnamed;}          // OR EBX,-1
    bool zoom_changes(std::uint32_t current,std::uint32_t requested) {                                   // CMP DX,BX
        const auto &gp=this->gp;
        this->flags.minus(gp[edx]&0xffffu,gp[ebx]&0xffffu,2);
        if(current!=requested)this->reached(Branch::zoom);
        return current!=requested;
    }
    bool zooming_out(std::uint32_t requested){this->flags.minus(low_byte(this->gp[ebx]),0xffu,1);return requested==logic::unnamed;}   // CMP BL,FF
    std::uint32_t cleared(){this->s.xmm[0]=_mm_setzero_ps();return 0u;}                                   // XORPS XMM0,XMM0
    std::uint32_t unarmed(std::uint32_t) {                                                               // OR EAX,-1; CMP DX,-1
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(0xffffffffu,4);this->flags.minus(gp[edx]&0xffffu,0xffffu,2);
        return logic::none;
    }
    std::uint32_t zoom_index(std::uint32_t){return this->gp[edx]=sx16(this->gp[edx]);}                        // MOVSX EDX,DX
    bool armed(std::uint32_t weapon){this->compare32(weapon,logic::none);return weapon!=logic::none;}         // CMP EAX,-1
    std::uint32_t weapon_index(std::uint32_t weapon) {                                                   // AND EAX,FFFF; LEA EAX,[EAX+EAX*2]
        return this->gp[eax]=3u*this->flags.logic(weapon&logic::handle_mask,4);
    }
    bool has_levels(std::uint32_t levels) {                                                              // CMP word,0; JLE
        this->flags.minus(levels,0u,2);
        const bool has=std::int16_t(levels)>0;
        if(has)this->reached(Branch::weapon_sound);
        return has;
    }
    bool several(std::uint32_t levels,unsigned width) {                                                 // CMP AX,1 / CMP EAX,1; JLE
        this->flags.minus(levels,1u,width);
        return width==2u?std::int16_t(levels)>1:std::int32_t(levels)>1;
    }
    std::uint32_t widened(std::uint32_t levels){return this->gp[eax]=sx16(levels);}                          // MOVSX EAX,AX
    void scaled(std::uint32_t,std::uint32_t steps,std::uint32_t scale) {                                 // MOVSX EDX,BL; DEC EAX; CVTSI2SS XMM0,EDX; CVTSI2SS XMM1,EAX; DIVSS
        auto &gp=this->gp;auto &x=this->s.xmm;
        gp[edx]=sx8(gp[ebx]);gp[eax]=this->flags.increment(gp[eax],4,true);
        x[0]=_mm_move_ss(x[0],lane0(scale));x[1]=_mm_move_ss(x[1],lane0(steps));
        this->reached(Branch::scaled);
    }
    bool sounded(std::uint32_t sound){this->compare32(sound,logic::none);return sound!=logic::none;}          // CMP ECX,-1
    void requesting() {                                                                                  // MOV EAX,4000095; LEA EDI,[ESP+28]
        auto &gp=this->gp;
        gp[eax]=logic::zoom_sound;gp[edi]=local(logic::local::request);
    }
    void play(Kept &kept){routed(0xc8106u,0x18c000u,0u,kept);this->reached(Branch::sound);}                  // CALL 18C000
    // The hands.
    bool hands_passive(std::uint32_t v){return shifted(eax,v,logic::definition::passive_bit);}
    std::uint32_t first_hand(){return this->gp[esi]=this->flags.logic(0u,4);}                                // XOR ESI,ESI
    std::uint32_t is_first(std::uint32_t hand) {                                                         // TEST ESI,ESI; SETZ BL
        auto &gp=this->gp;
        this->test32(hand);set_low_byte(gp[ebx],this->flags.zf?1u:0u);
        return gp[ebx];
    }
    bool named(std::uint32_t slot){this->flags.minus(slot,logic::unnamed,1);return slot!=logic::unnamed;}     // CMP AL,FF
    bool indexed(std::uint32_t index){this->flags.minus(this->gp[ecx]&0xffffu,0xffffu,2);return index!=logic::none;}   // CMP CX,-1
    std::uint32_t hand_index(std::uint32_t){return this->gp[ecx]=sx16(this->gp[ecx]);}                        // MOVSX ECX,CX
    bool hand_busy(Kept &kept,std::uint32_t handle) {                                                    // PUSH EAX; CALL 113190
        put(0xc8182u,handle);direct(0xc8183u,0x113190u,4u,kept);
        return low_byte(this->gp[eax])!=0u;
    }
    bool counting_busy(std::uint32_t count){this->test32(count);return std::int32_t(count)>0;}               // TEST ECX,ECX; JLE
    bool unblocked(){set_low_byte(this->gp[eax],this->flags.logic(0u,1));return false;}                     // XOR AL,AL
    bool action_active(std::uint32_t bits,unsigned n) {                                                  // SHR ECX,n; TEST CL,1
        const bool active=shifted(ecx,bits,n);
        if(active)this->reached(Branch::blocking);
        return active;
    }
    // MOV CL,BL; NEG CL; SBB ECX,ECX; AND ECX,-A; ADD ECX,12; ADD EDI,EDX; MOV ESI,ECX; AND ECX,1F; MOV EDX,1; SAR ESI,5; SHL EDX,CL
    std::uint32_t weapon_change(std::uint32_t first,std::uint32_t,std::uint32_t unit_actions) {
        auto &gp=this->gp;auto &fl=this->flags;
        const auto change=low_byte(first)?logic::action::first_weapon:logic::action::second_weapon;
        gp[edi]=fl.plus(gp[edi],unit_actions,4);
        gp[esi]=std::uint32_t(std::int32_t(change)>>5);gp[ecx]=change&31u;
        gp[edx]=fl.shift(1u,gp[ecx],4);
        return change;
    }
    bool change_active(std::uint32_t bits,std::uint32_t change) {                                        // TEST EDX,ECX
        const bool active=this->flags.logic((1u<<(change&31u))&bits,4)!=0u;
        if(active)this->reached(Branch::change_active);
        return active;
    }
    bool blocked(){set_low_byte(this->gp[eax],1u);return true;}                                         // MOV AL,1
    bool second_hand(std::uint32_t first){this->test8(first);return low_byte(first)==0u;}                    // TEST BL,BL
    void mapped(std::uint32_t controls,unsigned seventh,std::uint32_t control_word) {                     // the word in CX, EBX and EDX as shifted
        auto &gp=this->gp;
        gp[ecx]=control_word;gp[ebx]=controls>>seventh;gp[edx]=controls>>logic::unit::zoom_control_bit;
    }
    bool was_first(std::uint32_t first){this->test8(first);return low_byte(first)!=0u;}                      // TEST AL,AL
    void control(Kept &kept,std::uint32_t weapon,std::uint32_t control_word,std::uint32_t scale) {        // PUSH EDX; PUSH EAX; PUSH ECX; CALL 105500
        put(0xc8360u,scale);put(0xc8361u,control_word);put(0xc8362u,weapon);
        direct(0xc8363u,0x105500u,12u,kept);
        this->reached(Branch::hand);
    }
    std::uint32_t next_hand(std::uint32_t hand) {                                                        // INC ESI; CMP ESI,2
        auto &gp=this->gp;
        gp[esi]=this->flags.increment(hand,4,false);this->compare32(gp[esi],2u);
        return gp[esi];
    }
    void pair(Kept &kept,std::uint32_t handle) {                                                         // PUSH EDX; CALL D3A80
        put(0xc837au,handle);direct(0xc837bu,0xd3a80u,4u,kept);
        this->reached(Branch::pair);
    }
    void finished(std::uint32_t) {                                                                       // POP EDI; POP ESI; POP EBP; POP EBX; ADD ESP,28
        auto &gp=this->gp;
        gp[edi]=this->pop(0xc8384u);gp[esi]=this->pop(0xc8385u);gp[ebp]=this->pop(0xc8386u);gp[ebx]=this->pop(0xc8387u);
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::unit_weapon_controls::update(memory,*this,Kept{gp[ebx],gp[ebx],gp[ebp],gp[esi],gp[esi],gp[edi],gp[edi]});
        this->ret(0xc838bu,4u);this->publish(0xc838bu);
    }
};
template<class State,class Reference,class Children,class Dispatch> bool bridge(State &s,Reference reference,Children children,Dispatch dispatch) {
    return run_owner(s,0xc7dd0u,0xc838bu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children,Dispatch>(v,c,t,children,dispatch);});
}
} // namespace halo2::native_unit_weapon_controls_detail
