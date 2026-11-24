#pragma once
// Boundary adapter for the item points, 15D890
// (native_tu15_item_points_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: the records on the stack (RET 4);
// 0x18 bytes of frame with ESI (and, past an empty scenario, EBX, EBP and
// EDI) saved below them, read from the ESP each child left. The engine's
// first method (ECX the engine; CALL [EAX]), 15CAB0 (EDX the answer, EDI the
// point's +4; one pushed, RET 4), D9110 (one pushed, RET 4) and 15D4F0 (two
// pushed, RET 8) go through the dispatch; 15BA10, 15D670 (EAX the index) and
// 161AE0 (EDI the object's position, XMM3 the radius) are direct; every
// register each returns comes back. The tick test keeps the original's two
// CDQ/IDIV (EAX and EDX as they leave them; a division fault as the unit's).
// Each exit leaves through the original's epilogue. Each access publishes
// its original instruction.
// HALO2_REFERENCE_ITEM_POINTS=1 restores the translated body.
// HALO2_VERIFY_ITEM_POINTS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_item_points_systems.hpp"
#include <limits>
#include <stdexcept>

namespace halo2::native_item_points_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_item_points;
using Access=logic::item_points::Access;
using Exit=logic::item_points::Exit;
using Kept=logic::item_points::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ITEM_POINTS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ITEM_POINTS");return v;}
enum Branch : unsigned {modeless, pointless, other_tick, no_engine, unspawned, counted, reset, flagged, restarted_moved, restarted_away,
    scaled, counting_down, deleted, held, respawned, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Item points"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline std::uint32_t sx16(std::uint32_t v){return std::uint32_t(std::int32_t(std::int16_t(v)));}
inline std::uint32_t low_word(std::uint32_t reg,std::uint32_t v){return (reg&0xffff0000u)|(v&0xffffu);}
inline __m128 lane0(std::uint32_t bits){return logic::sse::scalar(bits);}
inline constexpr std::uint32_t sites[]{
    0x15d890u,0x15d895u,0x15d8a4u,0x15d8aau,0x15d8b0u,0x15d8beu,0x15d8c2u,0x15d8c5u,0x15d8cbu,0x15d8d1u,0x15d8d5u,0x15d8d9u,
    0x15d8deu,0x15d8e9u,0x15d8edu,0x15d8f1u,0x15d900u,0x15d904u,0x15d912u,0x15d918u,0x15d92du,0x15d933u,0x15d939u,0x15d947u,
    0x15d949u,0x15d960u,0x15d964u,0x15d96cu,0x15d980u,0x15d985u,0x15d988u,0x15d98cu,0x15d993u,0x15d99du,0x15d9a8u,0x15d9aeu,
    0x15d9c4u,0x15d9c8u,0x15d9cdu,0x15d9d2u,0x15d9d7u,0x15d9dcu,0x15d9e1u,0x15da06u,0x15da0fu,0x15da13u,0x15da21u,0x15da25u,
    0x15da25u,0x15da30u,0x15da38u,0x15da3du,0x15da40u,0x15da44u,0x15da4au,0x15da4eu,0x15da57u,0x15da5bu,0x15da5fu,0x15da69u,
    0x15da73u,0x15da7du,0x15da8du,0x15daa0u,0x15dab1u,0x15dac4u,0x15dac9u,0x15daceu,0x15dad3u,0x15dad8u,0x15daddu,0x15daffu,
    0x15db0fu,0x15db16u,0x15db1cu,0x15db2fu,0x15db35u,0x15db50u,0x15db54u,0x15db59u,0x15db65u,0x15db6au,0x15db6eu,0x15db76u,
    0x15db7cu,0x15db93u,0x15db97u,0x15db9bu,0x15dbacu,0x15dbb0u,0x15dbb7u,0x15dbbbu,0x15dbc4u,0x15dbcfu,0x15dbd7u,0x15dbdbu,
    0x15dbdfu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::point_count_next)+1u);
static_assert(logic::point::size==9u<<4);
template<class State,class Children,class Dispatch> struct Adapter : Port<State> {
    Children &children;      // 15BA10, 15D670 and 161AE0
    Dispatch &dispatch;      // the engine's method, 15CAB0, D9110 and 15D4F0
    std::uint32_t depth{};   // bytes pushed beyond the four saved registers (negative before them)
    bool just_counted{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k,Dispatch &d):Port<State>(s,c,t,{}),children(k),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept){auto &gp=this->gp;kept.handle=gp[ebx];kept.point_address=gp[ebp];kept.index=gp[esi];kept.timer=gp[edi];}
    void direct(std::uint32_t pc,std::uint32_t target,Kept &kept) {
        auto child=[&](State &v){children(v,target);};
        this->call(pc,pc+5u,target,child);refresh(kept);
    }
    void routed(std::uint32_t pc,std::uint32_t next,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto route=[&](State &v){dispatch(v,target,pc);};
        this->call(pc,next,target,route);depth-=popped;refresh(kept);
    }
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case game_load:case definitions_load:case respawn_definition:case scenario_load:case time_now:case vtable_load:
        case objects_load:case object_load:case definition_load:case records_timer:case records_expired:case delete_handle:
        case index_load:case scenario_next:gp[eax]=v;break;
        case point_mode:gp[eax]=sx16(v);break;                                                             // MOVSX EAX,word
        case count_load_again:case timer_load:gp[eax]=low_word(gp[eax],v);break;                           // MOV AX,word
        case object_flags:case object_state:case expired_type:set_low_byte(gp[eax],v);break;               // MOV AL
        case game_mode:case type_load:case point_scale:set_low_byte(gp[ecx],v);break;                      // MOV CL
        case tags_load:case globals_definition:case time_load:case rate_load:case session_load:case engine_load:
        case records_argument:case objects_data:case records_count:case records_flag:case expired_definition:case game_scale_again:
        case records_respawn:gp[ecx]=v;break;
        case tick_rate:gp[ecx]=sx16(v);break;                                                              // MOVSX ECX,word
        case game_default_scale:case game_team_scale:gp[ecx]=sx8(v);break;                                 // MOVSX ECX,byte
        case globals_tag:case globals_block:case point_count:case points_load:case time_again:case engine_index:case game_again:
        case records_reset:case object_after:case object_skip:case game_scale:case object_after_scale:case records_delete:
            gp[edx]=v;break;
        case record_handle:case expired_handle:gp[ebx]=v;break;
        case object_reload:case expired_object:case timer_pointer_load:gp[edi]=v;break;
        case definition_radius:case expired_radius:x[3]=lane0(v);break;                                    // MOVSS XMM3
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // The frame, the definition and the points.
    void framing(){auto &gp=this->gp;gp[esp]=this->flags.minus(gp[esp],0x18u,4);depth=0u-16u;}               // SUB ESP,18
    void saving(unsigned n) {                                                                            // PUSH ESI / EBX / EBP / EDI
        static constexpr std::uint32_t pcs[]{0x15d8e4u,0x15d8fbu,0x15d8fcu,0x15d8fdu};
        static constexpr unsigned regs[]{esi,ebx,ebp,edi};
        this->push(pcs[n-1u],this->gp[regs[n-1u]]);depth+=4u;
    }
    bool mode_is(std::uint32_t v,std::uint32_t k) {                                                      // CMP CL,4
        this->flags.minus(v&0xffu,k,1);
        if((v&0xffu)==k)this->reached(Branch::modeless);
        return (v&0xffu)==k;
    }
    std::uint32_t tag_index(std::uint32_t tag) {                                                         // AND EDX,FFFF; SHL EDX,4
        auto &fl=this->flags;
        const auto index=fl.logic(tag&logic::handle_mask,4);
        this->gp[edx]=fl.shift(index,4u,4);
        return index;
    }
    std::uint32_t first_point(){return this->gp[esi]=this->flags.logic(0u,4);}                               // XOR ESI,ESI
    bool above(std::uint32_t a,std::uint32_t b) {                                                        // CMP EDX,ESI; JLE
        this->compare32(a,b);
        if(std::int32_t(a)<=std::int32_t(b))this->reached(Branch::pointless);
        return std::int32_t(a)>std::int32_t(b);
    }
    std::uint32_t point_address(std::uint32_t index,std::uint32_t points) {                              // LEA EBP,[ESI+ESI*8]; SHL EBP,4; ADD EBP,EDX
        auto &fl=this->flags;
        return this->gp[ebp]=fl.plus(fl.shift(9u*index,4u,4),points,4);
    }
    // CDQ; IDIV ECX: EDX:EAX by ECX, as the unit divides (a fault as its own).
    void divided(std::uint32_t denominator) {
        auto &gp=this->gp;
        gp[edx]=std::int32_t(gp[eax])<0?0xffffffffu:0u;
        const auto a=std::int64_t(std::int32_t(gp[eax])),b=std::int64_t(std::int32_t(denominator));
        if(!b||(a==std::numeric_limits<std::int32_t>::min()&&b==-1))
            throw std::runtime_error("Required original world integer division-fault owner unavailable");
        gp[eax]=std::uint32_t(a/b);gp[edx]=std::uint32_t(a%b);
    }
    bool this_tick(std::uint32_t,std::uint32_t index,std::uint32_t rate) {                              // CDQ; IDIV ECX; MOV EAX,ESI; MOV EDI,EDX; CDQ; IDIV ECX; CMP EDX,EDI
        auto &gp=this->gp;
        divided(rate);gp[eax]=index;gp[edi]=gp[edx];divided(rate);
        this->compare32(gp[edx],gp[edi]);
        if(gp[edx]!=gp[edi])this->reached(Branch::other_tick);
        return gp[edx]==gp[edi];
    }
    // The engine and 15CAB0.
    std::uint32_t unanswered(){return this->gp[eax]=this->flags.logic(logic::none,4);}                        // OR EAX,-1
    bool present(std::uint32_t engine){this->test32(engine);if(!engine)this->reached(Branch::no_engine);return engine!=0u;}   // TEST ECX,ECX
    std::uint32_t engine_answer(Kept &kept,std::uint32_t method){routed(0x15d949u,0x15d94bu,method,0u,kept);return this->gp[eax];}   // CALL [EAX]
    std::uint32_t spawn_answer(Kept &kept,std::uint32_t answer) {                                        // LEA EDI,[EBP+4]; PUSH 4; MOV EDX,EAX; CALL 15CAB0
        auto &gp=this->gp;
        gp[edi]=kept.point_address+logic::point::spawn;put(0x15d94eu,logic::spawn_kind);gp[edx]=answer;
        routed(0x15d952u,0x15d957u,0x15cab0u,4u,kept);
        if(gp[eax]==logic::none)this->reached(Branch::unspawned);
        return gp[eax];
    }
    bool named(std::uint32_t v){this->compare32(v,logic::none);return v!=logic::none;}                        // CMP reg,-1
    // The record's vehicle.
    std::uint32_t no_object(){return this->gp[edx]=this->flags.logic(0u,4);}                                  // XOR EDX,EDX
    std::uint32_t object_index(std::uint32_t handle) {                                                   // MOV EAX,EBX; AND EAX,FFFF; LEA EDX,[EAX+EAX*2]
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(handle&logic::handle_mask,4);gp[edx]=3u*gp[eax];
        return gp[eax];
    }
    bool byte_is(std::uint32_t v,std::uint32_t k){this->flags.minus(v&0xffu,k,1);return (v&0xffu)==k;}       // CMP byte,k
    bool word_nonzero(std::uint32_t v){this->flags.minus(v&0xffffu,0u,2);return (v&0xffffu)!=0u;}            // CMP word,0
    bool idle(Kept &kept,bool expired) {                                                                 // CALL 15BA10; TEST AL,AL
        direct(expired?0x15db8au:0x15d9bbu,0x15ba10u,kept);
        this->test8(this->gp[eax]);
        if(expired && low_byte(this->gp[eax]))this->reached(Branch::held);
        return low_byte(this->gp[eax])!=0u;
    }
    void position(std::uint32_t object){this->gp[edi]=this->flags.plus(object,logic::object::x,4);}          // ADD EDI,30
    bool beyond(const logic::item_points::Distance &d,std::uint32_t limit,bool near) {                  // COMISS XMM3,[limit]; JBE
        auto &x=this->s.xmm;auto &fl=this->flags;
        x[0]=lane0(d.dx);x[1]=lane0(d.dx2);x[2]=lane0(d.dy2);x[3]=lane0(d.d2);
        fl.scalar_compare(x[3],lane0(limit),false);
        const bool far=!(fl.cf||fl.zf);
        if(near && far)this->reached(Branch::restarted_away);
        return far;
    }
    bool occupied(Kept &kept,std::uint32_t,bool expired) {                                               // (ADD EDI,30); CALL 161AE0; TEST AL,AL
        auto &gp=this->gp;
        if(expired)gp[edi]=this->flags.plus(gp[edi],logic::object::x,4);
        direct(expired?0x15dba3u:0x15da18u,0x161ae0u,kept);
        this->test8(gp[eax]);
        if(expired && low_byte(gp[eax]))this->reached(Branch::held);
        return low_byte(gp[eax])!=0u;
    }
    std::uint32_t counted(std::uint32_t v){this->reached(Branch::counted);just_counted=true;return this->flags.increment(v&0xffffu,2,false);}   // INC word
    std::uint32_t count_address(std::uint32_t address) {                                                 // LEA EAX,[reg+ESI*8+6]
        if(!just_counted)this->reached(Branch::reset);
        just_counted=false;
        this->gp[eax]=address;
        return address;
    }
    bool word_above(std::uint32_t a,std::uint32_t b) {                                                   // CMP AX,word; JLE
        this->flags.minus(a&0xffffu,b&0xffffu,2);
        const bool past=std::int16_t(a)>std::int16_t(b);
        if(past)this->reached(Branch::flagged);
        return past;
    }
    // The timer.
    std::uint32_t timer_address(std::uint32_t address){return this->gp[edi]=address;}                        // LEA EDI,[EAX+ESI*8+4]
    unsigned mode_kind(std::uint32_t mode) {                                                             // SUB EAX,0; DEC EAX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.minus(mode,0u,4);
        if(gp[eax]==0u)return 0u;
        gp[eax]=fl.increment(gp[eax],4,true);
        return gp[eax]==0u?1u:2u;
    }
    bool moved(std::uint32_t) {                                                                          // SHR AL,2; TEST AL,1
        auto &gp=this->gp;auto &fl=this->flags;
        set_low_byte(gp[eax],fl.right(gp[eax]&0xffu,2u,1,false));
        const bool is=fl.logic(gp[eax]&1u,1)!=0u;
        if(is)this->reached(Branch::restarted_moved);
        return is;
    }
    bool item(std::uint32_t) {                                                                           // MOV EAX,1; SHL EAX,CL; TEST AL,1C
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.shift(1u,gp[ecx]&0xffu,4);
        return fl.logic(gp[eax]&logic::object::item_types,1)!=0u;
    }
    bool item_taken(std::uint32_t) {                                                                     // TEST AL,1; MOV CL,AL; SHR CL,1; TEST CL,1
        auto &gp=this->gp;auto &fl=this->flags;
        if(fl.logic(gp[eax]&1u,1))return true;
        set_low_byte(gp[ecx],gp[eax]);
        set_low_byte(gp[ecx],fl.right(gp[ecx]&0xffu,1u,1,false));
        return fl.logic(gp[ecx]&1u,1)!=0u;
    }
    std::uint32_t respawn_time(Kept &kept) {                                                             // MOV EAX,ESI; CALL 15D670
        auto &gp=this->gp;
        gp[eax]=kept.index;direct(0x15db0au,0x15d670u,kept);
        return gp[eax];
    }
    bool kind_default(std::uint32_t kind){this->test8(kind);return (kind&0xffu)==0u;}                        // TEST CL,CL
    bool kind_team(std::uint32_t kind) {                                                                 // CMP CL,1; JL; CMP CL,7; JG
        const auto k=std::int8_t(kind&0xffu);
        this->flags.minus(kind&0xffu,1u,1);
        if(k<1)return false;
        this->flags.minus(kind&0xffu,logic::setting::last_team_kind,1);
        return k<=std::int8_t(logic::setting::last_team_kind);
    }
    std::uint32_t scaled(std::uint32_t,std::uint32_t) {                                                  // DEC ECX; (OR EAX,-1 / SAR EAX,1 / SHL EAX,1)
        auto &gp=this->gp;auto &fl=this->flags;
        this->reached(Branch::scaled);
        gp[ecx]=fl.increment(gp[ecx],4,true);
        if(!gp[ecx])return gp[eax]=fl.logic(logic::none,4);
        gp[ecx]=fl.increment(gp[ecx],4,true);
        if(!gp[ecx])return gp[eax]=fl.right(gp[eax],1u,4,true);
        gp[ecx]=fl.increment(gp[ecx],4,true);
        if(!gp[ecx])gp[eax]=fl.shift(gp[eax],1u,4);
        return gp[eax];
    }
    void timer_cleared(){this->gp[eax]=this->flags.logic(0u,4);}                                              // XOR EAX,EAX
    bool word_positive(std::uint32_t v){this->flags.logic(v&0xffffu,2);return std::int16_t(v)>0;}             // TEST AX,AX; JLE
    std::uint32_t counted_down(std::uint32_t){this->reached(Branch::counting_down);return this->gp[eax]=this->flags.increment(this->gp[eax],4,true);}   // DEC EAX
    bool word_tested(std::uint32_t v){this->flags.logic(v&0xffffu,2);return (v&0xffffu)!=0u;}                // TEST AX,AX
    bool kept(std::uint32_t flags_value){return this->flags.logic(flags_value&logic::point::kept_bit,1)!=0u;}   // TEST byte,2
    void deleted(Kept &kept,std::uint32_t handle){put(0x15dbbeu,handle);routed(0x15dbbfu,0x15dbc4u,0xd9110u,4u,kept);this->reached(Branch::deleted);}   // PUSH EAX; CALL D9110
    void respawned(Kept &kept,std::uint32_t records) {                                                   // PUSH ESI; PUSH ECX; CALL 15D4F0
        put(0x15dbc8u,kept.index);put(0x15dbc9u,records);routed(0x15dbcau,0x15dbcfu,0x15d4f0u,8u,kept);
        this->reached(Branch::respawned);
    }
    std::uint32_t next_point(std::uint32_t index){return this->gp[eax]=this->flags.increment(index,4,false);}   // INC EAX
    std::uint32_t point_index(std::uint32_t v){return this->gp[esi]=sx16(v);}                                 // MOVSX ESI,AX
    bool below(std::uint32_t a,std::uint32_t b){this->compare32(a,b);return std::int32_t(a)<std::int32_t(b);}   // CMP ESI,[EAX+120]; JL
    void finish(Exit exit) {                                                                             // (POP EDI; POP EBP; POP EBX); (POP ESI); ADD ESP,18; RET 4
        auto &gp=this->gp;
        if(exit==Exit::done){gp[edi]=this->pop(0x15dbebu);gp[ebp]=this->pop(0x15dbecu);gp[ebx]=this->pop(0x15dbedu);}
        if(exit!=Exit::modeless)gp[esi]=this->pop(0x15dbeeu);
        gp[esp]=this->flags.plus(gp[esp],0x18u,4);
        this->ret(0x15dbf2u,4u);this->publish(0x15dbf2u);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        finish(logic::item_points::update(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]}));
    }
};
template<class State,class Reference,class Children,class Dispatch> bool bridge(State &s,Reference reference,Children children,Dispatch dispatch) {
    return run_owner(s,0x15d890u,0x15dbf2u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children,Dispatch>(v,c,t,children,dispatch);});
}
} // namespace halo2::native_item_points_detail
