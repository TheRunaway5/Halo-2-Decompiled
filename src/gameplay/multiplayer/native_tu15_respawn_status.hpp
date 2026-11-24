#pragma once
// Boundary adapter for the respawn status, 1618B0
// (native_tu15_respawn_status_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: the player's handle on the
// stack, EAX the status (RET 4); ECX pushed for two local bytes, then EBX
// and EDI (EBP and ESI on the player's path), read from the ESP each child
// left. The engine's method +0x94 (ECX the engine; two pushed, RET 8) and
// 160570 (ECX the player) go through the dispatch; 15BE80 (three pushed,
// RET C) is direct; every register each returns comes back. Each status
// leaves through the original's own epilogue (its pops and the arithmetic
// that builds EBX). Each access publishes its original instruction.
// HALO2_REFERENCE_RESPAWN_STATUS=1 restores the translated body.
// HALO2_VERIFY_RESPAWN_STATUS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_respawn_status_systems.hpp"

namespace halo2::native_respawn_status_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_respawn_status;
using Access=logic::respawn_status::Access;
using Exit=logic::respawn_status::Exit;
using Kept=logic::respawn_status::Kept;
using Outcome=logic::respawn_status::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_RESPAWN_STATUS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_RESPAWN_STATUS");return v;}
// One branch per exit, in the Exit order.
enum Branch : unsigned {engine_early, early_decided, early_undecided, teamless, quitting, flagged, respawning, engine, penalty_one,
    penalty, recent, mode, late_decided, late_second, late_undecided, branch_count};
static_assert(unsigned(Branch::branch_count)==unsigned(Exit::late_undecided)+1u);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Respawn status"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline constexpr std::uint32_t sites[]{0x1618b1u,0x1618b6u,0x1618bcu,0x1618c5u,0x1618ceu,0x1618d3u,0x1618d6u,0x1618deu,0x1618efu,
    0x1618f5u,0x16190fu,0x161917u,0x16193au,0x161940u,0x161953u,0x16195cu,0x161962u,0x16196bu,0x16199du,0x1619b2u,0x1619cfu,
    0x1619d5u,0x1619dcu,0x1619e7u,0x1619ecu,0x1619f5u,0x161a25u,0x161a28u,0x161a6bu,0x161a71u,0x161a77u,0x161a90u,0x161a98u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::late_second)+1u);
// Each exit's pops (ESI and EBP when pushed, then EDI, EBX and ECX) and its RET.
struct Epilogue {std::uint32_t esi,edi,ebx,ret;};
inline constexpr Epilogue epilogues[]{
    {0u,0x161ad4u,0x161ad7u,0x161ad9u},        // engine_early
    {0u,0x161922u,0x161928u,0x16192au},        // early_decided
    {0u,0x161932u,0x161935u,0x161937u},        // early_undecided
    {0x161974u,0x16197bu,0x16197eu,0x161980u}, // teamless
    {0x16198eu,0x161995u,0x161998u,0x16199au}, // quitting
    {0x1619a3u,0x1619aau,0x1619adu,0x1619afu}, // flagged
    {0x1619c0u,0x1619c7u,0x1619cau,0x1619ccu}, // respawning
    {0x161ad2u,0x161ad4u,0x161ad7u,0x161ad9u}, // engine
    {0x161a01u,0x161a08u,0x161a0bu,0x161a0du}, // penalty_one
    {0x161a18u,0x161a1au,0x161a20u,0x161a22u}, // penalty
    {0x161a3au,0x161a41u,0x161a44u,0x161a46u}, // recent
    {0x161a57u,0x161a5eu,0x161a61u,0x161a63u}, // mode
    {0x161aa2u,0x161aa4u,0x161aafu,0x161ab1u}, // late_decided
    {0x161ab6u,0x161ab8u,0x161ac3u,0x161ac5u}, // late_second
    {0x161ad2u,0x161ad4u,0x161ad7u,0x161ad9u}};// late_undecided
static_assert(sizeof(epilogues)/sizeof(epilogues[0])==unsigned(Branch::branch_count));
template<class State,class Children,class Dispatch> struct Adapter : Port<State> {
    Children &children;      // 15BE80
    Dispatch &dispatch;
    std::uint32_t depth{};   // bytes pushed beyond ECX, EBX and EDI (negative before them)
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k,Dispatch &d):Port<State>(s,c,t,{}),children(k),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){auto &gp=this->gp;kept.handle=gp[edi];kept.answer=gp[ebx];kept.player_address=gp[esi];}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case session_load:case unit_load:case respawn_load:case game_again:case time_now:gp[eax]=v;break;
        case engine_index:case engine_load:case game_load:case players_load:case time_load:gp[ecx]=v;break;
        case engine_table:case players_data:case session_again:case game_last:case late_player:gp[edx]=v;break;
        case player_argument:case late_mode:gp[edi]=v;break;
        case session_mode:gp[ebp]=sx16(v);break;                                                           // MOVSX EBP,word
        case time_rate:case penalty_kind:gp[eax]=sx16(v);break;                                            // MOVSX EAX,word
        case lives_load:gp[eax]=sx8(v);break;                                                              // MOVSX EAX,byte
        case override_load:case decision_mode:case early_decided:case late_decided:case late_second:set_low_byte(gp[eax],v);break;   // MOV AL
        case early_second:set_low_byte(gp[ecx],v);break;                                                   // MOV CL
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void reserving(){this->push(0x1618b0u,this->gp[ecx]);depth=0u-8u;}                                    // PUSH ECX
    void saving(unsigned n) {                                                                            // PUSH EBX / EDI / EBP / ESI
        static constexpr std::uint32_t pcs[]{0x1618c3u,0x1618c4u,0x161943u,0x161944u};
        static constexpr unsigned regs[]{ebx,edi,ebp,esi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    void override_pushed(){auto &gp=this->gp;gp[eax]=local(logic::local::override_byte);put(0x1618cdu,gp[eax]);}   // LEA EAX,[ESP+A]; PUSH EAX
    void player_pushed(Kept &kept){put(0x1618d5u,kept.handle);}                                           // PUSH EDI
    std::uint32_t engine_answer(Kept &kept,std::uint32_t method) {                                      // CALL [EDX+94]; MOV EBX,EAX
        auto &gp=this->gp;
        auto route=[&](State &v){dispatch(v,method,0x1618d6u);};
        this->call(0x1618d6u,0x1618dcu,method,route);depth-=8u;
        refresh(kept);
        return kept.answer=gp[ebx]=gp[eax];
    }
    bool set(std::uint32_t v){this->test8(v);return (v&0xffu)!=0u;}                                         // TEST AL,AL / TEST CL,CL
    bool answered(std::uint32_t v){this->compare32(v,logic::none);return v!=logic::none;}                  // CMP reg,-1
    void decide(Kept &kept,bool late) {                                                                  // (pushes); CALL 15BE80
        auto &gp=this->gp;
        if(!late) {
            gp[edx]=local(logic::local::second);put(0x161903u,gp[edx]);
            gp[eax]=local(logic::local::decided);put(0x161908u,gp[eax]);put(0x161909u,kept.handle);
        }
        const auto pc=late?0x161a8bu:0x16190au;
        auto child=[&](State &v){children(v,0x15be80u);};
        this->call(pc,pc+5u,0x15be80u,child);depth-=12u;
        refresh(kept);
    }
    std::uint32_t player_offset(std::uint32_t handle) {                                                  // MOV ESI,EDI; AND ESI,FFFF; IMUL ESI,ESI,21C
        auto &fl=this->flags;
        return this->gp[esi]=fl.product(fl.logic(handle&logic::handle_mask,4),logic::player_size);
    }
    std::uint32_t player_address(std::uint32_t offset,std::uint32_t data){return this->gp[esi]=this->flags.plus(offset,data,4);}   // ADD ESI,EDX
    bool teamless(std::uint32_t team){this->flags.minus(team,logic::no_team,1);return team==logic::no_team;}   // CMP byte,FF
    bool quitting(Kept &kept) {                                                                          // MOV ECX,EDI; CALL 160570; TEST AL,AL
        auto &gp=this->gp;
        gp[ecx]=kept.handle;
        auto route=[&](State &v){dispatch(v,0x160570u,0x161985u);};
        this->call(0x161985u,0x16198au,0x160570u,route);
        refresh(kept);
        return set(gp[eax]);
    }
    bool flagged(std::uint32_t flags_value){return this->flags.logic(flags_value&logic::player::flag_bit,1)!=0u;}   // TEST byte,40
    bool negative(std::uint32_t v){this->test32(v);return std::int32_t(v)<0;}                                // TEST EAX,EAX; JL
    std::uint32_t window(std::uint32_t rate){return this->gp[edi]=logic::window_seconds*rate;}              // LEA EDI,[EAX+EAX*2]
    bool below(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)<std::int32_t(b);}   // CMP; JGE
    bool penalized(std::uint32_t v){this->flags.minus(v,0u,4);return v!=0u;}                               // CMP dword,0
    bool kind_one(std::uint32_t kind){this->compare32(kind,1u);return kind==1u;}                             // CMP EAX,1
    std::uint32_t elapsed(std::uint32_t now,std::uint32_t stamp) {                                      // SUB EAX,[EDX+70]; MOV ECX,0; SETS CL; DEC ECX; AND EAX,ECX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.minus(now,stamp,4);
        gp[ecx]=fl.sf?1u:0u;gp[ecx]=fl.increment(gp[ecx],4,true);
        return gp[eax]=fl.logic(gp[eax]&gp[ecx],4);
    }
    void mode_tested(std::uint32_t mode) {                                                               // CMP EBP,-1; TEST EBP,EBP; CMP EBP,5
        this->compare32(mode,logic::none);
        if(mode==logic::none)return;
        this->test32(mode);
        if(mode==0u)return;
        this->compare32(mode,std::uint32_t(logic::mode_limit));
    }
    void late_pushed(std::uint32_t late) {                                                               // LEA EAX; PUSH EAX; LEA ECX; PUSH ECX; TEST EDI,EDI; PUSH EDX; SETZ BL
        auto &gp=this->gp;
        gp[eax]=local(logic::local::second);put(0x161a7fu,gp[eax]);
        gp[ecx]=local(logic::local::decided);put(0x161a84u,gp[ecx]);
        this->test32(late);put(0x161a87u,gp[edx]);set_low_byte(gp[ebx],this->flags.zf?1u:0u);
    }
    void finish(const Outcome &o) {
        auto &gp=this->gp;auto &fl=this->flags;
        this->reached(unsigned(o.exit));
        const auto &e=epilogues[unsigned(o.exit)];
        // The flags of the arithmetic each epilogue builds EBX with, as the
        // original orders it; EBX itself is the status.
        switch(o.exit) {
        case Exit::early_decided:fl.logic(0u,4);this->test8(gp[ecx]);break;                              // XOR EBX,EBX; TEST CL,CL; SETNZ BL
        case Exit::penalty:fl.logic(0u,4);this->compare32(gp[eax],2u);break;                             // XOR EBX,EBX; CMP EAX,2; SETZ BL
        case Exit::late_decided:case Exit::late_second:case Exit::late_undecided:                        // NEG BL
            set_low_byte(gp[ebx],fl.minus(0u,gp[ebx]&0xffu,1));break;
        default:break;
        }
        if(e.esi){gp[esi]=this->pop(e.esi);gp[ebp]=this->pop(e.esi+1u);}
        const auto unpaced=o.status>=logic::code::unpaced?logic::code::unpaced:0u;
        switch(o.exit) {
        case Exit::early_decided:fl.plus(o.status-logic::code::decided,logic::code::decided,4);break;     // ADD EBX,8
        case Exit::penalty:fl.plus(o.status-logic::code::penalty,logic::code::penalty,4);break;           // ADD EBX,16
        case Exit::late_decided:case Exit::late_second:case Exit::late_undecided:                        // SBB EBX,EBX; AND EBX,14; ADD EBX,k
            fl.logic(fl.borrow(gp[ebx],gp[ebx],4)&logic::code::unpaced,4);fl.plus(unpaced,o.status-unpaced,4);break;
        default:break;
        }
        gp[edi]=this->pop(e.edi);
        gp[ebx]=o.status;
        gp[eax]=gp[ebx];
        gp[ebx]=this->pop(e.ebx);gp[ecx]=this->pop(e.ebx+1u);
        this->ret(e.ret,4u);this->publish(e.ret);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        finish(logic::respawn_status::status(memory,*this,Kept{gp[edi],gp[ebx],gp[esi]}));
    }
};
template<class State,class Reference,class Children,class Dispatch> bool bridge(State &s,Reference reference,Children children,Dispatch dispatch) {
    return run_owner(s,0x1618b0u,0x161ad9u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children,Dispatch>(v,c,t,children,dispatch);});
}
} // namespace halo2::native_respawn_status_detail
