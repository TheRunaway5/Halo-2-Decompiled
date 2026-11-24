#pragma once
// Boundary adapter for the input edges, entered at 1521B0
// (native_tu15_input_edges_systems.hpp). Its caller keeps the original ABI,
// which the adapter owns: EAX the player handle, ECX the input (RET); EBX
// and ESI saved; EAX the player, EDX the trigger word's high byte over the
// second trigger's held bit; the flags of the closing ADD ESP,8. The
// original keeps the first trigger's held and last-held bits, the second's
// last-held bit and the trigger word in four stack locals ([ESP+9], +0xA,
// +0xB, +0xC), stored and read back where it branches on them; the adapter
// makes those accesses at their instructions. Each access publishes its
// original instruction.
// HALO2_REFERENCE_INPUT_EDGES=1 restores the translated body.
// HALO2_VERIFY_INPUT_EDGES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/players/native_tu15_input_edges_systems.hpp"

namespace halo2::native_input_edges_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_input_edges;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_INPUT_EDGES");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_INPUT_EDGES");return p && *p;}();
    return value;
}
// A branch per change made with its bit set (the two latch clears: made).
enum Branch : unsigned {first_pressed, first_released, first_held, second_pressed, second_released, second_held,
    first_cleared, first_latch, second_cleared, second_latch, branch_count};
static_assert(unsigned(logic::input_edges::Change::second_latch)==Branch::second_latch);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Input edges verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
// Each change's instruction when it sets its bit and when it clears it.
struct ChangeSites {std::uint32_t set,clear;};
inline constexpr ChangeSites changes[]{{0x152239u,0x152239u},{0x152247u,0x152265u},{0x15226du,0x15224bu},{0x15225bu,0x152273u},
    {0x152288u,0x1522a8u},{0x1522b0u,0x15228cu},{0u,0x15229au},{0x1522a2u,0x1522b6u},{0u,0x1522c4u},{0x1522ccu,0x1522d6u}};
template<class State> struct InputEdges : Port<State> {
    using Access=logic::input_edges::Access;
    using Change=logic::input_edges::Change;
    Change change{};
    bool on{},first_held_local{},second_held_reg{},last_latch_set{};
    std::uint32_t last_byte{};
    InputEdges(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    // The locals, past the two saves.
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+offset;}
    std::uint32_t site()const{const auto &c=changes[unsigned(change)];return on?c.set:c.clear;}
    void changing(Change next,bool next_on) {
        change=next;on=next_on;
        if(on || change==Change::first_cleared || change==Change::second_cleared)this->reached(unsigned(change));
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case players:return 0x1521b3u;
        case player_data:this->push(0x1521c4u,this->gp[ebx]);return 0x1521c5u;                  // PUSH EBX
        case buttons:return 0x1521c8u;
        case held:this->push(0x1521ccu,this->gp[esi]);return 0x1521cdu;                         // PUSH ESI
        case buttons_store:return 0x1521dau;
        case last_triggers:return 0x1521dcu;
        case held_store:return 0x1521dfu;
        case triggers:return 0x1521e2u;
        case trigger_word:return 0x1521eeu;
        case last_again:return 0x1521f9u;
        case pressed_store:return 0x152239u;
        case latches:return 0x152290u;
        case latches_again:return 0x1522bfu;
        case bits:
            // The locals the original reads back before these.
            if(change==Change::first_released) {
                if(!first_held_local)this->read(0x15223fu,local(0xau),1);
                if(!on)this->read(0x152261u,local(9u),1);
            }
            if(change==Change::second_pressed && second_held_reg)this->read(0x152253u,local(0xbu),1);
            if(change==Change::second_released && !second_held_reg)this->read(0x152280u,local(0xbu),1);
            if(change==Change::first_cleared)this->read(0x152296u,local(9u),1);
            return site();
        case bits_store:return site();
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case players:gp[edx]=v;break;
        case player_data:gp[ebx]=v;break;
        case buttons:gp[edx]=v;break;
        case last_triggers:last_byte=v;break;
        // MOV DL,[ECX+1C]; AND DL,1; MOV [ESP+9],DL.
        case triggers:first_held_local=v&logic::first.held;this->store(0x1521e8u,local(9u),1,first_held_local?1u:0u);break;
        // XOR EDX,EDX; MOV DX,[ECX+1C]; AND BL,1; MOV [ESP+A],BL.
        case trigger_word:gp[edx]=v&0xffffu;this->store(0x1521f5u,local(0xau),1,last_byte&logic::first.held);break;
        // MOV BL,[EAX+3C]; SHR BL,4; AND BL,1; MOV [ESP+B],BL; MOV BL,[ESP+9]; MOV [ESP+C],EDX; SHR DL,4; AND DL,1;
        // TEST BL,BL; (MOV BL,[ESP+A]; TEST BL,BL;) MOV ESI,[ESP+C]; OR/AND ESI; MOV BL,[ESP+9].
        case last_again:{
            this->store(0x152202u,local(0xbu),1,(v>>4)&1u);
            const auto held_local=this->read(0x152206u,local(9u),1);
            this->store(0x15220au,local(0xcu),4,gp[edx]);
            second_held_reg=(gp[edx]>>4)&1u;
            set_low_byte(gp[edx],second_held_reg?1u:0u);
            const bool pressed=held_local && !this->read(0x152218u,local(0xau),1);
            this->read(pressed?0x152220u:0x152229u,local(0xcu),4);
            this->read(0x152233u,local(9u),1);
            break;
        }
        case latches_again:last_latch_set=v&logic::second.latch;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]-=8u;                                                                             // SUB ESP,8
        const auto handle=gp[eax];
        Memory<InputEdges> memory{*this};
        logic::input_edges::update(memory,*this,handle,gp[ecx]);
        // AND EAX,FFFF; IMUL EAX,21C; ADD EAX,EBX.
        gp[eax]=gp[ebx]+logic::player_size*(handle&0xffffu);
        const auto pop_pc=last_latch_set && second_held_reg?0x1522d0u:0x1522dau;
        gp[esi]=this->pop(pop_pc);gp[ebx]=this->pop(pop_pc+1u);
        gp[esp]=fl.plus(gp[esp],8u,4);                                                            // ADD ESP,8
        this->ret(pop_pc+5u,0u);this->publish(pop_pc+5u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1521b0u,0x1522dfu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return InputEdges<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_input_edges_detail
