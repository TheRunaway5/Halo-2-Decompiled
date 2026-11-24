#pragma once
// Boundary adapter for the teleporter update, 1A6830
// (native_tu15_teleporter_update_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: no arguments (RET); a
// 0x10-byte frame (the iterator) below the saved EBX, EBP, ESI and EDI;
// 1A8F10 (ESI the iterator) through the dispatch, 1A6930 (EAX the entry) and
// 1A6A40 (the entry pushed, RET 4) direct, every register each returns
// coming back. Each access publishes its original instruction.
// HALO2_REFERENCE_TELEPORTER_UPDATE=1 restores the translated body.
// HALO2_VERIFY_TELEPORTER_UPDATE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/world/native_tu15_teleporter_update_systems.hpp"

namespace halo2::native_teleporter_update_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_teleporter_update;
using Access=logic::teleporter_update::Access;
using Kept=logic::teleporter_update::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_TELEPORTER_UPDATE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_TELEPORTER_UPDATE");return v;}
enum Branch : unsigned {updated, marked_walk, touched, cleared_walk, cleared, inside, teleported, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Teleporter update"};return v;}
inline constexpr std::uint32_t sites[]{0x1a6835u,0x1a6844u,0x1a6848u,0x1a684cu,0x1a6860u,0x1a6864u,0x1a6864u,0x1a6875u,0x1a687bu,
    0x1a6897u,0x1a68acu,0x1a68b0u,0x1a68b4u,0x1a68d0u,0x1a68d4u,0x1a68d9u,0x1a68d9u,0x1a68eau,0x1a690du,0x1a6913u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::count_last)+1u);
// The iterator's calls, in the order the walks reach them.
enum class Walk {first_marking,marking,first_clearing,clearing};
template<class State,class Dispatch,class Touch,class Teleport> struct Adapter : Port<State> {
    Dispatch &dispatch;
    Touch &touch_child;
    Teleport &teleport_child;
    std::uint32_t depth{};      // bytes below the return address
    Walk walk{Walk::first_marking};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d,Touch &o,Teleport &p)
        :Port<State>(s,c,t,{}),dispatch(d),touch_child(o),teleport_child(p){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case players_array:gp[ebp]=v;break;
        case record_load:case record_again:case count_load:case count_again:case count_final:case count_last:gp[eax]=v;break;
        case session_address:case session_again:gp[edi]=v;break;
        case inside_test:this->flags.logic(v&gp[ebx]&0xffu,1);if(v&gp[ebx]&0xffu)this->reached(Branch::inside);break;   // TEST byte [EAX+2],BL
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t){if(a==Access::teleported_store)this->reached(Branch::cleared);}
    void prepared(const Kept &kept) {                                                                  // PUSH ESI; OR EBX,-1; PUSH EDI; LEA ESI,[ESP+10]
        auto &gp=this->gp;
        put(0x1a683bu,gp[esi]);gp[ebx]=this->flags.logic(kept.entry,4);put(0x1a683fu,gp[edi]);gp[esi]=local(logic::local::record);
    }
    std::uint32_t next(Kept &kept) {
        auto &gp=this->gp;
        std::uint32_t pc=0x1a6850u;
        switch(walk) {
        case Walk::first_marking:break;
        case Walk::marking:gp[esi]=local(logic::local::record);pc=0x1a686cu;break;
        case Walk::first_clearing:pc=0x1a68b8u;break;
        case Walk::clearing:gp[esi]=local(logic::local::record);pc=0x1a68e1u;break;
        }
        auto child=[&](State &v){dispatch(v,0x1a8f10u,pc);};
        this->call(pc,pc+5u,0x1a8f10u,child);
        refresh(kept);
        this->test8(gp[eax]&0xffu);                                                                    // TEST AL,AL
        if(walk==Walk::first_marking)walk=Walk::marking;
        if(walk==Walk::first_clearing)walk=Walk::clearing;
        return gp[eax];
    }
    void masking(const Kept &kept){this->gp[edi]=kept.held;this->reached(Branch::marked_walk);}        // MOV EDI,FF7F
    void masked(std::uint32_t value){this->flags.logic(value&0xffffu,2);}                               // AND word [EAX+2]
    void counting(std::uint32_t count){this->gp[esi]=this->flags.logic(0u,4);this->test32(count);}     // XOR ESI,ESI; TEST EAX,EAX
    void touch(Kept &kept) {                                                                           // (LEA EBX,[EDI+AFC]); MOV EAX,EBX; CALL 1A6930
        auto &gp=this->gp;
        gp[ebx]=kept.entry;gp[eax]=kept.entry;
        this->call(0x1a6892u,0x1a6897u,0x1a6930u,touch_child);
        refresh(kept);
        this->reached(Branch::touched);
    }
    void advanced(const Kept &kept,std::uint32_t count) {                                              // INC ESI; ADD EBX,10; CMP ESI,EAX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esi]=fl.increment(kept.index-1u,4,false);gp[ebx]=fl.plus(kept.entry-logic::teleporter_size,logic::teleporter_size,4);
        this->compare32(kept.index,count);
    }
    void reset(const Kept &kept){this->gp[ebx]=this->flags.logic(kept.entry,4);}                       // OR EBX,-1
    void iterating(){this->gp[esi]=local(logic::local::record);walk=Walk::first_clearing;}            // LEA ESI,[ESP+10]
    void clearing(const Kept &kept){this->gp[ebx]=kept.entry;this->gp[ebp]=kept.array;this->reached(Branch::cleared_walk);}   // MOV BL,80; MOV EBP,FEFF
    void offsets(const Kept &kept){this->gp[ebx]=this->flags.logic(kept.entry,4);}                     // XOR EBX,EBX
    void teleport(std::uint32_t entry,Kept &kept) {                                                    // LEA EAX,[EBX+EDI+AFC]; PUSH EAX; CALL 1A6A40
        auto &gp=this->gp;
        gp[eax]=entry;put(0x1a6907u,gp[eax]);
        this->call(0x1a6908u,0x1a690du,0x1a6a40u,teleport_child);depth-=4u;
        refresh(kept);
        this->reached(Branch::teleported);
    }
    void restoring() {
        auto &gp=this->gp;
        gp[edi]=take(0x1a6921u);gp[esi]=take(0x1a6922u);gp[ebp]=take(0x1a6923u);gp[ebx]=take(0x1a6924u);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x10u,4);depth=0x10u;                                                 // SUB ESP,10
        put(0x1a6833u,gp[ebx]);put(0x1a6834u,gp[ebp]);
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::teleporter_update::update(memory,*this,Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]})));
        gp[esp]=fl.plus(gp[esp],0x10u,4);                                                              // ADD ESP,10
        this->ret(0x1a6928u,0u);this->publish(0x1a6928u);
    }
};
template<class State,class Reference,class Dispatch,class Touch,class Teleport>
bool bridge(State &s,Reference reference,Dispatch dispatch,Touch touch,Teleport teleport) {
    return run_owner(s,0x1a6830u,0x1a6928u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch,Touch,Teleport>(v,c,t,dispatch,touch,teleport);});
}
} // namespace halo2::native_teleporter_update_detail
