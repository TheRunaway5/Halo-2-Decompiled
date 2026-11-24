#pragma once
// Boundary adapter for the animation busy check, 113190
// (native_tu15_animation_busy_systems.hpp). Its callers and C9590 keep the
// original ABI, which the adapter owns: the unit's handle on the stack, AL
// the answer (RET 4); 8 bytes of frame with EBX, EBP, ESI and EDI saved
// below them, read from the ESP C9590 left; C9590 gets EAX the unit and
// every register it returns comes back. The original's binary searches leave
// through one of several identical epilogues; the adapter publishes the one
// an unlisted channel animation reaches. Each access publishes its original
// instruction.
// HALO2_REFERENCE_ANIMATION_BUSY=1 restores the translated body.
// HALO2_VERIFY_ANIMATION_BUSY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_animation_busy_systems.hpp"

namespace halo2::native_animation_busy_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_animation_busy;
using Access=logic::animation_busy::Access;
using Kept=logic::animation_busy::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ANIMATION_BUSY");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ANIMATION_BUSY");return v;}
enum Branch : unsigned {no_player, first_timed, second_timing, posed, answered, pose_animation, weapon_animation, channel_weapon,
    channel_pose, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Animation busy"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline std::uint32_t low_word(std::uint32_t reg,std::uint32_t v){return (reg&0xffff0000u)|(v&0xffffu);}
inline constexpr std::uint32_t sites[]{0x113193u,0x113197u,0x1131a4u,0x1131adu,0x1131b1u,0x1131b8u,0x1131c1u,0x1131c8u,0x1131ccu,
    0x1131d0u,0x1131dbu,0x1131e0u,0x1131e5u,0x1131f2u,0x1131f6u,0x113207u,0x11321cu,0x113220u,0x11322du,0x113231u,0x113234u,
    0x113238u,0x113249u,0x11325eu,0x113262u,0x11326eu,0x113274u,0x11327bu,0x11327fu,0x11328cu,0x1132e6u,0x11353eu,0x113543u,
    0x113683u,0x113627u,0x11362fu,0x113634u,0u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::result_load)+1u);
// The epilogue (MOV AL,[ESP+11]) an unlisted channel animation leaves
// through: the highest name of each leaf of the original's search, signed.
struct Exit {std::int32_t last;std::uint32_t pc;};
inline constexpr Exit exits[]{{-1,0x113588u},{0x0a00022c,0x113588u},{0x0b00022d,0x1135a1u},{0x0c000074,0x1135d2u},
    {0x0c0006b2,0x1135ebu},{0x0d00022b,0x113619u},{0x0e00067c,0x113647u},{0x11000075,0x11366cu},{0x7fffffff,0x113688u}};
inline constexpr std::uint32_t matched_exit=0x113688u;
inline std::uint32_t exit_for(std::uint32_t name) {
    for(const auto &e:exits)if(std::int32_t(name)<=e.last)return e.pc;
    return matched_exit;
}
template<class State,class Child> struct Adapter : Port<State> {
    Child &child_body;
    std::uint32_t depth{};      // bytes pushed below the saved registers (negative before them)
    std::uint32_t exit_pc{matched_exit},return_pc{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Child &k):Port<State>(s,c,t,{}),child_body(k){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    std::uint32_t instruction(Access a,std::uint32_t){return a==Access::result_load?exit_pc:sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case argument_load:case unit_load:case hand_unit:case first_weapon_load:case first_object:case second_weapon_load:gp[ecx]=v;break;
        case headers_load:case second_datums:case second_object:case call_argument:case channel_load:gp[eax]=v;break;
        case datums_load:gp[ebp]=v;break;
        case channels_block:gp[ebx]=sx16(v);break;                                                         // MOVSX EBX,word
        case channel_animation:case player_load:gp[ebx]=v;break;
        case state_block:gp[edi]=sx16(v);break;                                                            // MOVSX EDI,word
        case state_animation:gp[esi]=v;break;
        case second_unit:gp[edx]=v;break;
        case first_slot:gp[ebx]=low_word(gp[ebx],sx8(v));break;                                             // MOVSX BX,byte
        case second_slot:gp[eax]=low_word(gp[eax],sx8(v));break;                                            // MOVSX AX,byte
        case no_player_spare:case first_spare:set_low_byte(gp[ebx],v);break;                               // MOV BL
        case posed_load:case channel_posed_load:case channel_posed_result:case result_load:set_low_byte(gp[eax],v);break;   // MOV AL
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // SUB ESP,8
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);depth=0u-16u;
    }
    void saving(unsigned n) {                                                                            // PUSH EBX / EBP / ESI / EDI
        static constexpr std::uint32_t pcs[]{0x11319cu,0x1131a3u,0x1131bfu,0x1131c0u};
        static constexpr unsigned regs[]{ebx,ebp,esi,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t indexed(std::uint32_t handle){return this->gp[ecx]=this->flags.logic(handle&logic::handle_mask,4);}   // AND ECX,FFFF
    std::uint32_t header_offset(std::uint32_t index){return this->gp[edx]=this->flags.shift(3u*index,2u,4);}   // LEA EDX,[ECX+ECX*2]; SHL EDX,2
    std::uint32_t state_address(std::uint32_t state_offset,std::uint32_t unit_address) {                // ADD EDI,ECX
        return this->gp[edi]=this->flags.plus(state_offset,unit_address,4);
    }
    bool played(std::uint32_t player) {                                                                  // CMP EBX,-1
        this->compare32(player,logic::none);
        if(player==logic::none)this->reached(Branch::no_player);
        return player!=logic::none;
    }
    std::uint32_t idle(std::uint32_t){return this->gp[ebx];}                                              // (MOV BL loaded)
    void datums_copied(){this->gp[ecx]=this->gp[ebp];}                                                   // MOV ECX,EBP
    bool first_named(std::uint32_t slot){this->flags.minus(this->gp[ebx]&0xffffu,0xffffu,2);return slot!=logic::none;}   // CMP BX,-1
    std::uint32_t first_index(std::uint32_t){return this->gp[ebx]=sx16(this->gp[ebx]);}                       // MOVSX EBX,BX
    bool armed(std::uint32_t weapon){this->compare32(weapon,logic::none);return weapon!=logic::none;}         // CMP ECX,-1
    std::uint32_t first_weapon_index(std::uint32_t weapon) {                                             // AND ECX,FFFF; LEA ECX,[ECX+ECX*2]
        return this->gp[ecx]=3u*this->flags.logic(weapon&logic::handle_mask,4);
    }
    std::uint32_t run_out(std::uint32_t timer) {                                                         // CMP word,0; SETLE BL
        auto &fl=this->flags;
        fl.minus(timer,0u,2);set_low_byte(this->gp[ebx],(fl.zf||fl.sf!=fl.of)?1u:0u);
        this->reached(Branch::first_timed);
        return this->gp[ebx];
    }
    bool second_named(std::uint32_t slot){this->flags.minus(this->gp[eax]&0xffffu,0xffffu,2);return slot!=logic::none;}   // CMP AX,FFFF
    std::uint32_t second_index(std::uint32_t){return this->gp[ecx]=sx16(this->gp[eax]);}                      // MOVSX ECX,AX
    std::uint32_t second_weapon_index(std::uint32_t weapon) {                                            // AND ECX,FFFF; LEA EDX,[ECX+ECX*2]
        auto &gp=this->gp;
        gp[ecx]=this->flags.logic(weapon&logic::handle_mask,4);
        return gp[edx]=3u*gp[ecx];
    }
    bool timing(std::uint32_t timer){this->flags.minus(timer,0u,2);return std::int16_t(timer)>0;}             // CMP word,0; JLE
    std::uint32_t not_idle() {                                                                           // XOR BL,BL
        set_low_byte(this->gp[ebx],this->flags.logic(0u,1));
        this->reached(Branch::second_timing);
        return this->gp[ebx];
    }
    bool posed(std::uint32_t flags_value) {                                                              // TEST byte,10
        const bool set=this->flags.logic(flags_value&logic::state::pose_bit,1)!=0u;
        if(set)this->reached(Branch::posed);
        return set;
    }
    bool answered(Kept &kept) {                                                                          // CALL C9590; TEST AL,AL
        auto &gp=this->gp;
        auto route=[&](State &v){child_body(v);};
        this->call(0x113283u,0x113288u,0xc9590u,route);
        kept.idle=gp[ebx];kept.state_animation=gp[esi];
        this->test8(gp[eax]);
        if(low_byte(gp[eax]))this->reached(Branch::answered);
        else if(logic::among(logic::pose_animations,kept.state_animation))this->reached(Branch::pose_animation);
        else if(logic::among(logic::weapon_animations,kept.state_animation))this->reached(Branch::weapon_animation);
        return low_byte(gp[eax])!=0u;
    }
    std::uint32_t clear(std::uint32_t v){this->test8(v);return this->flags.zf?1u:0u;}                         // TEST AL,AL / TEST BL,BL; SETZ
    void leaving_matched(){exit_pc=matched_exit;this->reached(Branch::channel_weapon);}
    void leaving(std::uint32_t channel){exit_pc=exit_for(channel);}
    bool unposed(std::uint32_t posed_value) {                                                            // POP EDI; TEST AL,AL; POP ESI
        auto &gp=this->gp;
        gp[edi]=this->pop(0x11362bu);this->test8(posed_value);gp[esi]=this->pop(0x11362eu);depth-=8u;
        this->reached(Branch::channel_pose);
        return this->flags.zf;
    }
    void finished_posed(std::uint32_t) {                                                                 // POP EBP; POP EBX; ADD ESP,8
        auto &gp=this->gp;
        gp[ebp]=this->pop(0x113638u);gp[ebx]=this->pop(0x113639u);
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);return_pc=0x11363du;
    }
    void finished(std::uint32_t) {                                                                       // POP EDI; POP ESI; POP EBP; POP EBX; ADD ESP,8
        auto &gp=this->gp;
        gp[edi]=this->pop(exit_pc+4u);gp[esi]=this->pop(exit_pc+5u);gp[ebp]=this->pop(exit_pc+6u);gp[ebx]=this->pop(exit_pc+7u);
        gp[esp]=this->flags.plus(gp[esp],logic::local::size,4);return_pc=exit_pc+0xbu;
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        logic::animation_busy::check(memory,*this,Kept{gp[ebx],gp[esi]});
        this->ret(return_pc,4u);this->publish(return_pc);
    }
};
template<class State,class Reference,class Child> bool bridge(State &s,Reference reference,Child child) {
    return run_owner(s,0x113190u,0x113693u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Child>(v,c,t,child);});
}
} // namespace halo2::native_animation_busy_detail
