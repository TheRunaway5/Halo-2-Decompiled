#pragma once
// Boundary adapter for the action allowed, 1EFD60
// (native_tu15_action_allowed_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EDX the action and the
// local player on the stack (RET 4), AL the answer; 0x54 bytes of frame with
// EBX, ESI, EDI (and, past the action's definition, EBP) saved below them,
// read from the ESP each child left. 13F620, CE230 (EAX the unit, CX the
// slot), D2AD0, D2B00, 161530, BCDF0 (one pushed, RET 4), 1055F0 (two
// pushed, RET 8) and 105FD0 go through the dispatch; 1F1160 (two pushed,
// RET 8), 1EFD10, 1EFCC0, 1AC79B and 1AC595 (one pushed, RET 4), CAD40,
// 165A50, 536E0, 1060A0 and 1060E0 are direct; every register each returns
// comes back. Each case leaves through its own copy of the epilogue, its
// answer's store and load among the pops. Each access publishes its
// original instruction.
// HALO2_REFERENCE_ACTION_ALLOWED=1 restores the translated body.
// HALO2_VERIFY_ACTION_ALLOWED=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_action_allowed_systems.hpp"

namespace halo2::native_action_allowed_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_action_allowed;
using Access=logic::action_allowed::Access;
using Exit=logic::action_allowed::Exit;
using Leave=logic::action_allowed::Leave;
using Kept=logic::action_allowed::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ACTION_ALLOWED");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ACTION_ALLOWED");return v;}
// One branch per leaving kind and per action granted.
enum Branch : unsigned {early, common, pending, granted, shared, flagged_bit, set_true, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Action allowed"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline std::uint32_t low_word(std::uint32_t reg,std::uint32_t v){return (reg&0xffff0000u)|(v&0xffffu);}
inline __m128 lane0(std::uint32_t bits){return logic::sse::scalar(bits);}
inline constexpr std::uint32_t sites[]{
    0x1efd63u,0x1efd69u,0x1efd80u,0x1efd87u,0x1efd8du,0x1efd91u,0x1efda1u,0x1efda9u,0x1efdc6u,0x1efdcau,0x1efdd2u,0x1efddcu,
    0x1efdeeu,0x1efdf1u,0x1efdfau,0x1efe04u,0x1efe0au,0x1efe18u,0x1efe22u,0x1efe39u,0x1efe46u,0x1efe4fu,0x1efe64u,0x1efe6bu,
    0x1efe76u,0x1efe7bu,0x1efe9bu,0x1efea8u,0x1efeb1u,0x1efec6u,0x1efecdu,0x1efed8u,0x1efeddu,0x1efefau,0x1eff04u,0x1eff0bu,
    0x1eff16u,0x1eff1bu,0x1eff38u,0x1eff42u,0x1eff49u,0x1eff54u,0x1eff59u,0x1eff69u,0x1eff7cu,0x1eff9bu,0x1effaeu,0x1effb8u,
    0x1effbfu,0x1effcau,0x1effcfu,0x1effecu,0x1efff8u,0x1f0013u,0x1f0017u,0x1f0019u,0x1f0027u,0x1f002bu,0x1f003cu,0x1f007fu,
    0x1f0083u,0x1f008eu,0x1f009du,0x1f00bau,0x1f00bfu,0x1f00cbu,0x1f00d9u,0x1f00ddu,0x1f00f3u,0x1f0107u,0x1f010cu,0x1f0118u,
    0x1f0126u,0x1f012au,0x1f0140u,0x1f0160u,0x1f016bu,0x1f0176u,0x1f0183u,0x1f018eu,0x1f019cu,0x1f01a7u,0x1f01b5u,0x1f01c5u,
    0x1f01cau,0x1f01d6u,0x1f01edu,0x1f01ffu,0x1f0217u,0x1f0229u,0x1f022fu,0x1f0232u,0x1f0240u,0x1f0244u,0x1f025au,0x1f0268u,
    0x1f026cu,0x1f0282u,0x1f0290u,0x1f0294u,0x1f02aeu,0x1f02bcu,0x1f02c0u,0x1f02e0u,0x1f02e7u,0x1f02f2u,0x1f02f7u,0x1f0307u,
    0x1f0319u,0x1f0329u,0x1f032eu,0x1f033au,0x1f034cu,0x1f0352u,0x1f0355u,0x1f0363u,0x1f0367u,0x1f037du,0x1f038bu,0x1f038fu,
    0x1f03a9u,0x1f03b5u,0x1f03c3u,0x1f03d5u,0x1f03e1u,0x1f03efu,0x1f03f4u,0x1f0400u,0x1f040eu,0x1f0412u,0x1f0428u,0x1f0436u,
    0x1f043au,0x1f0460u,0x1f0473u,0x1f0478u,0x1f0484u,0x1f048cu,0x1f0499u,0x1f04a6u,0x1f04bbu,0x1f04c0u,0x1f04ceu,0x1f04e1u,
    0x1f04e9u,0x1f04f6u,0x1f0505u,0x1f050au,0x1f051bu,0x1f0528u,0x1f0534u,0x1f0542u,0x1f0550u,0x1f0561u,0x1f0563u,0x1f0571u,
    0x1f0575u,0x1f0581u,0x1f0586u,0x1f0592u,0x1f059cu,0x1f05acu,0x1f05b5u,0x1f05bfu,0x1f05c8u,0x1f05d1u,0x1f05d7u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::answer_load)+1u);
// Each case's epilogue: its POP EBP, the POP that follows the answer (EDI or ESI or EBX), and its RET.
struct Epilogue {std::uint32_t pop_base,pop_next,ret;};
inline Epilogue epilogue(Leave leave,std::uint32_t action) {
    switch(leave) {
    case Leave::pending:
        switch(action){case 10:return {0x1efe7au,0x1efe82u,0x1efe88u};case 11:return {0x1efedcu,0x1efee4u,0x1efeeau};
            case 12:return {0x1eff1au,0x1eff22u,0x1eff28u};case 13:return {0x1eff58u,0x1eff60u,0x1eff66u};
            case 2:return {0x1effceu,0x1effd6u,0x1effdcu};default:return {0x1f02f6u,0x1f02feu,0x1f0304u};}
    case Leave::granted:
        switch(action){case 4:return {0x1f00b8u,0x1f00c3u,0x1f00c8u};case 15:return {0x1f0105u,0x1f0110u,0x1f0115u};
            case 17:return {0x1f01c3u,0x1f01ceu,0x1f01d3u};case 20:return {0x1f0327u,0x1f0332u,0x1f0337u};
            case 23:return {0x1f03edu,0x1f03f8u,0x1f03fdu};case 22:return {0x1f04b9u,0x1f04c4u,0x1f04c9u};
            case 21:return {0x1f0503u,0x1f050eu,0x1f0513u};default:return {0x1f057fu,0x1f058au,0x1f058fu};}
    case Leave::shared:return {0x1f0471u,0x1f047cu,0x1f0481u};
    case Leave::flag:
        switch(action){case 0:return {0x1f0166u,0x1f016au,0x1f0173u};case 1:return {0x1f017cu,0x1f0182u,0x1f018bu};
            default:return {0x1f0194u,0x1f019bu,0x1f01a4u};}
    default:return {0x1f05d6u,0x1f05dbu,0x1f05e1u};
    }
}
template<class State,class Children,class Dispatch> struct Adapter : Port<State> {
    Children &children;      // 1F1160, 1EFD10, 1EFCC0, 1AC79B, 1AC595, CAD40, 165A50, 536E0, 1060A0 and 1060E0
    Dispatch &dispatch;      // 13F620, CE230, D2AD0, D2B00, 161530, BCDF0, 1055F0 and 105FD0
    std::uint32_t depth{};   // bytes pushed beyond EBX, ESI, EDI and EBP (negative before them)
    std::uint32_t action{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k,Dispatch &d):Port<State>(s,c,t,{}),children(k),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){auto &gp=this->gp;kept.unit_address=gp[ebx];kept.objects=gp[ebp];kept.player_address=gp[esi];kept.controls_address=gp[edi];}
    void direct(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){children(v,target);};
        this->call(pc,pc+5u,target,child);depth-=popped;refresh(kept);
    }
    void routed(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto route=[&](State &v){dispatch(v,target,pc);};
        this->call(pc,pc+5u,target,route);depth-=popped;refresh(kept);
    }
    bool answered(){this->test8(this->gp[eax]);return low_byte(this->gp[eax])!=0u;}                       // TEST AL,AL
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case player_argument: case players_load: case unit_load: case unit_handle_10: case unit_handle_11: case unit_handle_14: case parent_tag_4: case definition_4: case seat_flags_4: case rider_seat_flags_4: case unit_15: case player_15: case unit_18: case player_30: case player_16: case unit_a_16: case unit_b_16: case unit_c_16: case address_d_16: case unit_a_23: case unit_a_9: case shared_unit_handle: case item_6: case item_tag_6:gp[eax]=v;break;
        case local_players: case actions_load: case actions_array: case game_load: case player_14: case player_2: case definitions_4: case objects_16: case address_a_16: case address_b_16: case address_c_16: case player_20: case objects_23: case address_a_23: case address_b_23: case address_b_9: case player_22: case definitions_6:gp[ecx]=v;break;
        case parent_object_4: case unit_address_15: case unit_address_18: case player_23: case address_a_9: case definition_6:gp[edx]=v;break;
        case objects_load: case unit_address: case unit_handle_4: case objects_data_16:gp[ebx]=v;break;
        case players_data: case objects_data:gp[ebp]=v;break;
        case controls_load: case player_handle: case parent_4: case player_18: case player_object_30: case unit_d_16: case unit_b_23: case unit_b_9:gp[esi]=v;break;
        case seats_4: case objects_data_23:gp[edi]=v;break;
        case seat_4:gp[ebx]=low_word(gp[ebx],v);break;                                                     // MOV BX,word
        case rider_seat_4:case bits_7:gp[eax]=low_word(gp[eax],v);break;                                   // MOV AX,word
        case maximum_23:gp[eax]=low_word(gp[eax],v);break;                                                 // MOV AX,word
        case loaded_23:gp[edx]=sx16(v);break;                                                              // MOVSX EDX,word
        case flags_21:gp[edx]=low_word(gp[edx],v);break;                                                   // MOV DX,word
        case slot_15:case slot_18:case slot_a_16:case slot_b_16:case slot_c_16:case slot_d_16:case slot_a_23:case slot_b_23:
        case slot_a_9:case slot_b_9:gp[ecx]=low_word(gp[ecx],sx8(v));break;                                // MOVSX CX,byte
        case answer_10:case answer_11:case answer_12:case answer_13:case answer_2:case answer_4:case answer_15:case flags_0:
        case flags_1:case flags_31:case count_17:case answer_17:case answer_16:case answer_20:case answer_23:case shared_answer:
        case answer_22:case answer_21:case answer_6:case answer_load:set_low_byte(gp[eax],v);break;         // MOV AL
        case threshold_22:case threshold_21:this->s.xmm[0]=lane0(v);break;                                 // MOVSS XMM0
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t){if(a==Access::answer_true)this->reached(Branch::set_true);}
    // The frame and the prologue.
    void framing(){auto &gp=this->gp;gp[esp]=this->flags.minus(gp[esp],0x54u,4);depth=0u-16u;}               // SUB ESP,54
    void saving(unsigned n) {                                                                            // PUSH EBX / ESI / EDI / EBP
        static constexpr std::uint32_t pcs[]{0x1efd67u,0x1efd68u,0x1efd6fu,0x1efdedu};
        static constexpr unsigned regs[]{ebx,esi,edi,ebp};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    std::uint32_t controls(std::uint32_t index,std::uint32_t base) {                                     // MOV EDI,EAX; IMUL EDI,EDI,1B0; ADD EDI,ESI
        auto &fl=this->flags;
        return this->gp[edi]=fl.plus(fl.product(index,logic::controls_size),base,4);
    }
    std::uint32_t no_player(){return this->gp[esi]=this->flags.logic(logic::none,4);}                         // OR ESI,-1
    bool named(std::uint32_t v){this->compare32(v,logic::none);return v!=logic::none;}                        // CMP reg,-1
    std::uint32_t no_definition(){return this->gp[eax]=this->flags.logic(0u,4);}                              // XOR EAX,EAX
    bool present(std::uint32_t v){this->test32(v);return v!=0u;}                                              // TEST reg,reg
    bool non_negative(std::uint32_t v){this->test32(v);return std::int32_t(v)>=0;}                            // TEST EDX,EDX; JL
    bool below(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)<std::int32_t(b);}   // CMP EDX,[ECX+420]; JGE
    std::uint32_t definition(std::uint32_t array,std::uint32_t index){return this->gp[eax]=array+logic::action_size*index;}   // LEA EAX x2
    bool flagged(std::uint32_t v){return this->flags.logic(v&1u,1)!=0u;}                                      // TEST byte,1
    bool mode_is(std::uint32_t v,std::uint32_t k){this->compare32(v,k);return v==k;}                          // CMP dword,k
    std::uint32_t player_offset(std::uint32_t handle) {                                                  // AND ESI,FFFF; IMUL ESI,ESI,21C
        auto &fl=this->flags;
        return this->gp[esi]=fl.product(fl.logic(handle&logic::handle_mask,4),logic::player_size);
    }
    std::uint32_t player_address(std::uint32_t offset,std::uint32_t players){return this->gp[esi]=this->flags.plus(offset,players,4);}   // ADD ESI,EBP
    std::uint32_t unit_index(std::uint32_t handle,std::uint32_t action_index) {                          // AND EAX,FFFF; CMP EDX,1F; LEA EAX,[EAX+EAX*2]
        const auto index=this->flags.logic(handle&logic::handle_mask,4);
        this->compare32(action_index,logic::last_action);
        this->gp[eax]=3u*index;
        return index;
    }
    bool listed(std::uint32_t action_index){return action_index<=logic::last_action;}                         // JA
    void jumped(std::uint32_t){}                                                                         // JMP [EDX*4+1F05E4]
    // The cases' questions.
    bool busy(Kept &kept,unsigned site) {                                                                // CALL 13F620; TEST AL,AL
        static constexpr std::uint32_t pcs[]{0x1efe29u,0x1efe8bu,0x1efeedu,0x1eff2bu,0x1eff8eu,0x1effdfu};
        routed(pcs[site],0x13f620u,0u,kept);
        return answered();
    }
    std::uint32_t none_seat(){return this->gp[eax]=this->flags.logic(logic::none,4);}                         // OR EAX,-1
    bool word_differs(std::uint32_t a,std::uint32_t b){this->flags.minus(a&0xffffu,b&0xffffu,2);return (a&0xffffu)!=(b&0xffffu);}   // CMP word
    bool differs(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return a!=b;}                          // CMP dword
    bool is(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return a==b;}                               // CMP dword,k
    void agreement_pushed(std::uint32_t mode) {                                                          // PUSH 0; PUSH mode
        const auto pc=mode==1u?0x1f0456u:mode==3u?0x1f0450u:0x1f045cu;
        put(pc,0u);put(pc+2u,mode);
    }
    bool agrees(Kept &kept,std::uint32_t,unsigned site) {                                                // (PUSH 1; PUSH -1;) CALL 1F1160; TEST AL,AL
        std::uint32_t pc=0x1f0464u;
        if(site<2u){const auto base=site?0x1efeb5u:0x1efe53u;put(base,1u);put(base+2u,logic::none);pc=base+4u;}
        direct(pc,0x1f1160u,8u,kept);
        return answered();
    }
    std::uint32_t asked(Kept &kept,std::uint32_t,std::uint32_t n){put(0x1eff6du,n);direct(0x1eff6fu,0x1efd10u,4u,kept);return this->gp[eax];}   // PUSH 10; CALL 1EFD10
    bool nonzero(std::uint32_t v){this->test32(v);return v!=0u;}                                              // TEST EAX,EAX
    bool seat_named(Kept &kept,std::uint32_t) {                                                          // CALL CAD40; CMP AX,FFFF
        direct(0x1eff80u,0xcad40u,0u,kept);
        return word_differs(this->gp[eax],logic::no_seat);
    }
    bool may(Kept &kept,std::uint32_t player_index,std::uint32_t n,unsigned site) {                      // PUSH n; (MOV ECX,ESI;) CALL 1EFCC0; TEST AL,AL
        static constexpr std::uint32_t pushes[]{0x1eff9fu,0x1f0152u,0x1f04aau};
        put(pushes[site],n);
        if(site==1u)this->gp[ecx]=player_index;
        direct(site==1u?0x1f0156u:pushes[site]+2u,0x1efcc0u,4u,kept);
        return answered();
    }
    bool answers(Kept &kept,std::uint32_t player_index,unsigned site) {                                  // PUSH reg; CALL 1AC79B / 1AC595; TEST AL,AL
        static constexpr std::uint32_t pushes[]{0x1f00f7u,0x1f0144u,0x1f021bu,0x1f030bu,0x1f033eu};
        put(pushes[site],player_index);
        direct(pushes[site]+1u,site==3u?0x1ac595u:0x1ac79bu,4u,kept);
        return answered();
    }
    // The weapons (CE230).
    std::uint32_t weapon_unit(std::uint32_t handle,unsigned site) {                                      // MOV ECX,reg; AND ECX,FFFF; LEA r,[ECX+ECX*2]
        auto &gp=this->gp;
        gp[ecx]=this->flags.logic(handle&logic::handle_mask,4);
        const auto triple=3u*gp[ecx];
        if(site<=1u||site==8u)gp[ecx]=triple;
        else if(site==7u||site==9u)gp[eax]=triple;
        else gp[edx]=triple;
        return handle&logic::handle_mask;
    }
    std::uint32_t weapon(Kept &kept,std::uint32_t handle,unsigned site) {                                // (LEA EDX; PUSH EDX;) (MOV EAX,ESI;) CALL CE230
        static constexpr std::uint32_t pcs[]{0x1f00e5u,0x1f0132u,0x1f024cu,0x1f0274u,0x1f029cu,0x1f02cau,0x1f036fu,0x1f039eu,0x1f041au,
            0x1f0444u};
        auto &gp=this->gp;
        if(site==7u){gp[edx]=local(logic::local::report);put(0x1f039bu,gp[edx]);}
        if(site==5u||site==7u||site==9u)gp[eax]=handle;
        routed(pcs[site],0xce230u,0u,kept);
        return gp[eax];
    }
    bool reloadable(Kept &kept){direct(0x1f02a1u,0x1060a0u,0u,kept);return answered();}                       // CALL 1060A0; TEST AL,AL
    std::uint32_t rounds(Kept &kept){this->gp[ecx]=this->flags.logic(0u,4);direct(0x1f02d1u,0x1060e0u,0u,kept);return this->gp[eax];}   // XOR ECX,ECX; CALL 1060E0
    void reported(Kept &kept,std::uint32_t weapon_handle){put(0x1f03a3u,weapon_handle);routed(0x1f03a4u,0x1055f0u,8u,kept);}   // PUSH EAX; CALL 1055F0
    bool switchable(Kept &kept){routed(0x1f0449u,0x105fd0u,0u,kept);return answered();}                       // CALL 105FD0; TEST AL,AL
    bool word_positive(std::uint32_t v){this->flags.minus(v&0xffffu,0u,2);return std::int16_t(v)>0;}         // CMP word,0; JLE
    bool word_nonzero(std::uint32_t v){this->flags.logic(v&0xffffu,2);return (v&0xffffu)!=0u;}               // TEST AX,AX
    bool word_compared_nonzero(std::uint32_t v){this->flags.minus(v&0xffffu,0u,2);return (v&0xffffu)!=0u;}    // CMP word,0; JE
    bool too_loaded(std::uint32_t loaded_value,std::uint32_t maximum) {                                  // MOVSX EAX,AX; SHL EDX,1; CMP EDX,EAX; JG
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=sx16(maximum);gp[edx]=fl.shift(loaded_value,1u,4);
        this->compare32(gp[edx],gp[eax]);
        return std::int32_t(gp[edx])>std::int32_t(gp[eax]);
    }
    // The rider seats.
    std::uint32_t parent_index(std::uint32_t parent) {                                                   // MOV EAX,ESI; AND EAX,FFFF; LEA ECX,[EAX+EAX*2]
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(parent&logic::handle_mask,4);gp[ecx]=3u*gp[eax];
        return gp[eax];
    }
    bool seat_held(std::uint32_t seat_value){return word_differs(seat_value,logic::no_seat);}                 // CMP BX,-1
    std::uint32_t tag_index(std::uint32_t tag) {                                                         // AND EAX,FFFF; SHL EAX,4
        auto &fl=this->flags;
        const auto index=fl.logic(tag&logic::handle_mask,4);
        this->gp[eax]=fl.shift(index,4u,4);
        return index;
    }
    std::uint32_t seat_offset(std::uint32_t seat_value,std::uint32_t seats) {                            // MOVSX EAX,BX; IMUL EAX,EAX,B0; ADD EAX,EDI
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.plus(fl.product(sx16(seat_value),logic::seat::size),seats,4);
        return sx16(seat_value);
    }
    bool seat_bit(std::uint32_t flags_value,unsigned bit,unsigned which) {                               // (MOV EDX/ECX,EAX;) SHR reg,bit; TEST reg8,1
        auto &gp=this->gp;auto &fl=this->flags;
        static constexpr unsigned regs[]{edx,ecx,eax,eax};
        const auto r=regs[which];
        gp[r]=fl.right(flags_value,bit,4,false);
        return fl.logic(gp[r]&1u,1)!=0u;
    }
    void riders_start(Kept &kept,std::uint32_t parent) {                                                 // LEA ECX,[ESP+18]; MOV EDX,ESI; CALL D2AD0; MOV ESI,ECX
        auto &gp=this->gp;
        gp[ecx]=local(logic::local::iterator);gp[edx]=parent;
        routed(0x1f006bu,0xd2ad0u,0u,kept);
        gp[esi]=gp[ecx];kept.player_address=gp[esi];
    }
    bool next_rider(Kept &kept,bool again) {                                                             // (LEA ESI,[ESP+18];) CALL D2B00; TEST EAX,EAX
        auto &gp=this->gp;
        if(again)gp[esi]=local(logic::local::iterator);
        routed(again?0x1f00afu:0x1f0072u,0xd2b00u,0u,kept);
        return present(gp[eax]);
    }
    std::uint32_t rider_offset(std::uint32_t seat_value) {                                               // MOVSX EDX,AX; IMUL EDX,EDX,B0
        return this->gp[edx]=this->flags.product(sx16(seat_value),logic::seat::size);
    }
    // The controls' questions.
    std::uint32_t flag_bit(std::uint32_t flags_value,unsigned k) {                                       // (SHR AL,k); AND AL,1
        auto &gp=this->gp;auto &fl=this->flags;
        auto v=flags_value&0xffu;
        if(k)v=fl.right(v,k,1,false);
        set_low_byte(gp[eax],fl.logic(v&1u,1));
        return gp[eax]&0xffu;
    }
    bool some(std::uint32_t v){this->test8(v);return (v&0xffu)!=0u;}                                          // TEST AL,AL; JBE
    bool ready(Kept &kept){routed(0x1f01e0u,0x161530u,0u,kept);return answered();}                            // CALL 161530; TEST AL,AL
    std::uint32_t player_slot(Kept &kept,std::uint32_t){direct(0x1f01f1u,0x165a50u,0u,kept);return this->gp[eax];}   // CALL 165A50
    bool object_ready(Kept &kept,std::uint32_t handle){this->gp[eax]=handle;direct(0x1f020du,0x536e0u,0u,kept);return answered();}   // MOV EAX,ESI; CALL 536E0
    void flags_cleared(){this->gp[edx]=this->flags.logic(0u,4);}                                              // XOR EDX,EDX
    bool driving(std::uint32_t){auto &gp=this->gp;auto &fl=this->flags;gp[edx]=fl.right(gp[edx],logic::unit::driving_bit,4,false);return fl.logic(gp[edx]&1u,1)!=0u;}   // SHR EDX,9; TEST DL,1
    bool fresh(std::uint32_t threshold,std::uint32_t timer) {                                            // COMISS XMM0,[EBX+F0]; JA
        auto &fl=this->flags;
        fl.scalar_compare(lane0(threshold),lane0(timer),false);
        return !(fl.cf||fl.zf);
    }
    void three(){this->gp[ecx]=3u;}                                                                           // MOV ECX,3
    bool word_is(std::uint32_t v,std::uint32_t k){this->flags.minus(v&0xffffu,k,2);return (v&0xffffu)==k;}   // CMP word,k
    bool word_below(std::uint32_t v,std::uint32_t k){this->flags.minus(v&0xffffu,k,2);return (v&0xffffu)<k;}  // CMP word,k; JB
    std::uint32_t item(Kept &kept,std::uint32_t){put(0x1f0556u,2u);routed(0x1f0558u,0xbcdf0u,4u,kept);return this->gp[eax];}   // PUSH 2; CALL BCDF0
    bool low_bits(std::uint32_t){auto &gp=this->gp;gp[ecx]=3u;return this->flags.logic(gp[eax]&3u,1)!=0u;}     // MOV ECX,3; TEST CL,AL
    bool high_bits(std::uint32_t v){return this->flags.logic(v&0xcu,1)!=0u;}                                  // TEST byte,0C
    bool byte_below(std::uint32_t v,std::uint32_t k){this->flags.minus(v&0xffu,k,1);return (v&0xffu)<k;}      // CMP byte,3; JB
    // The epilogues.
    void leaving(){this->gp[ebp]=this->pop(0x1f05d6u);depth-=4u;}                                             // POP EBP
    void popped_base(){this->gp[ebp]=this->pop(epilogue(Leave::pending,action).pop_base);depth-=4u;}           // POP EBP
    void popped_two() {                                                                                  // POP EBP; POP EDI
        auto &gp=this->gp;
        const auto e=epilogue(action<=1u||action==31u?Leave::flag:pending_or(action),action);
        gp[ebp]=this->pop(e.pop_base);gp[edi]=this->pop(e.pop_base+1u);depth-=8u;
    }
    Leave pending_or(std::uint32_t a)const {
        switch(a){case 4:case 15:case 17:case 20:case 23:case 22:case 21:case 6:return Leave::granted;default:return Leave::shared;}
    }
    void popped_player(){this->gp[esi]=this->pop(epilogue(Leave::flag,action).pop_next);depth-=4u;}            // POP ESI
    void finish(Exit exit) {
        auto &gp=this->gp;
        const auto e=epilogue(exit.leave,exit.action);
        switch(exit.leave) {
        case Leave::early:case Leave::common:                                                            // POP EDI; POP ESI; POP EBX
            this->reached(exit.leave==Leave::early?Branch::early:Branch::common);
            gp[edi]=this->pop(0x1f05dbu);gp[esi]=this->pop(0x1f05dcu);gp[ebx]=this->pop(0x1f05ddu);break;
        case Leave::pending:                                                                             // POP EDI; POP ESI; POP EBX
            this->reached(Branch::pending);
            gp[edi]=this->pop(e.pop_next);gp[esi]=this->pop(e.pop_next+1u);gp[ebx]=this->pop(e.pop_next+2u);break;
        case Leave::granted:case Leave::shared:                                                          // POP ESI; POP EBX
            this->reached(exit.leave==Leave::granted?Branch::granted:Branch::shared);
            gp[esi]=this->pop(e.pop_next);gp[ebx]=this->pop(e.pop_next+1u);break;
        case Leave::flag:                                                                                // POP EBX
            this->reached(Branch::flagged_bit);
            gp[ebx]=this->pop(e.ret-4u);break;
        }
        gp[esp]=this->flags.plus(gp[esp],0x54u,4);
        this->ret(e.ret,4u);this->publish(e.ret);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        action=gp[edx];
        finish(logic::action_allowed::allowed(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]},gp[edx]));
    }
};
template<class State,class Reference,class Children,class Dispatch> bool bridge(State &s,Reference reference,Children children,Dispatch dispatch) {
    return run_owner(s,0x1efd60u,0x1f05e1u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children,Dispatch>(v,c,t,children,dispatch);});
}
} // namespace halo2::native_action_allowed_detail
