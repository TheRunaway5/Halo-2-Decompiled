#pragma once
// Boundary adapter for the tagged play, entered at 1D6430
// (native_tu15_tagged_play_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EDI the object, three arguments
// (RET C); a 0x10-byte frame; EBX and ESI saved; 1D45A0 gets EAX the first
// argument, ESI the frame and the object and the key pushed (RET 8),
// 1E5FB0 EAX the first word, EBX the definition and the other words and
// arguments pushed (RET 10), every register each returns coming back; the
// flags of the closing ADD ESP,10. Each access publishes its original
// instruction.
// HALO2_REFERENCE_TAGGED_PLAY=1 restores the translated body.
// HALO2_VERIFY_TAGGED_PLAY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_tagged_play_systems.hpp"

namespace halo2::native_tagged_play_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_tagged_play;
using Access=logic::tagged_play::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_TAGGED_PLAY");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_TAGGED_PLAY");return v;}
enum Branch : unsigned {untagged, played, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Tagged play"};return v;}
template<class State,class Find,class Play> struct Adapter : Port<State> {
    Find &find_child;
    Play &play_child;
    std::uint32_t pushed{};      // the bytes below the frame the adapter has pushed
    bool saved{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Find &f,Play &p):Port<State>(s,c,t,{}),find_child(f),play_child(p){}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);pushed+=4u;}
    std::uint32_t local(std::uint32_t k)const{return this->gp[esp]+pushed+4u*k;}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+pushed+0x10u+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case tag:return 0x1d6430u;
        case first:return 0x1d643bu;
        case third:return 0x1d6450u;
        case second:return 0x1d6454u;
        case word2:return 0x1d6458u;
        case word1:put(0x1d645cu,gp[eax]);return 0x1d645du;                                       // PUSH EAX (the third)
        case tag_again:put(0x1d6461u,gp[ecx]);return 0x1d6462u;                                   // PUSH ECX (the second)
        case tags:put(0x1d6465u,gp[edx]);return 0x1d6466u;                                        // PUSH EDX (word 2)
        case definition:return 0x1d6475u;
        case word0:put(0x1d6479u,gp[eax]);return 0x1d647au;                                       // PUSH EAX (word 1)
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case tag:gp[eax]=v;gp[esp]=fl.minus(gp[esp],0x10u,4);this->compare32(v,logic::none);break;  // SUB ESP,10; CMP EAX,-1
        case first:case third:case word1:case word0:gp[eax]=v;break;
        case second:gp[ecx]=v;break;
        case word2:gp[edx]=v;break;
        case tag_again:gp[ecx]=v;break;
        case tags:gp[edx]=v;gp[ecx]=fl.shift(fl.logic(gp[ecx]&0xffffu,4),4u,4);break;                // AND ECX,FFFF; SHL ECX,4
        case definition:gp[ebx]=v;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH EBX; PUSH ESI; PUSH 7000101; PUSH EDI; LEA ESI,[ESP+10]; CALL 1D45A0.
    std::uint32_t found(std::uint32_t object_address,std::uint32_t) {
        auto &gp=this->gp;
        put(0x1d643fu,gp[ebx]);put(0x1d6440u,gp[esi]);saved=true;
        put(0x1d6441u,logic::entry_key);put(0x1d6446u,object_address);
        gp[esi]=gp[esp]+0x10u;
        this->call(0x1d644bu,0x1d6450u,0x1d45a0u,find_child);pushed-=8u;
        return gp[edi];
    }
    void played(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) {
        this->call(0x1d647eu,0x1d6483u,0x1e5fb0u,play_child);pushed-=0x10u;
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        Memory<Adapter> memory{*this};
        const bool played_entry=logic::tagged_play::play(memory,*this,gp[edi]);
        this->reached(played_entry?Branch::played:Branch::untagged);
        if(saved){gp[esi]=this->pop(0x1d6483u);gp[ebx]=this->pop(0x1d6484u);}
        gp[esp]=fl.plus(gp[esp],0x10u,4);                                                           // ADD ESP,10
        this->ret(0x1d6488u,0xcu);this->publish(0x1d6488u);
    }
};
template<class State,class Reference,class Find,class Play> bool bridge(State &s,Reference reference,Find find,Play play) {
    return run_owner(s,0x1d6430u,0x1d6488u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Find,Play>(v,c,t,find,play);});
}
} // namespace halo2::native_tagged_play_detail
