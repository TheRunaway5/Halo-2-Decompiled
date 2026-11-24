#pragma once
// Boundary adapter for the machine address check, 69AD0
// (native_tu15_machine_address_systems.hpp). Its callers and 6C3F0 keep the
// original ABI, which the adapter owns: EAX the address (RET), the answer in
// AL; a 0xC-byte frame (the answer byte, this machine's address) below the
// saved EBX (and ESI and EDI around the comparison); REPE CMPSW in the string
// direction flag's direction; 6C3F0 (the network block pushed, RET 4)
// direct, every register it returns coming back. Each access publishes its
// original instruction.
// HALO2_REFERENCE_MACHINE_ADDRESS=1 restores the translated body.
// HALO2_VERIFY_MACHINE_ADDRESS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "runtime/native_tu15_machine_address_systems.hpp"

namespace halo2::native_machine_address_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_machine_address;
using Access=logic::machine_address::Access;
using Outcome=logic::machine_address::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_MACHINE_ADDRESS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_MACHINE_ADDRESS");return v;}
enum Branch : unsigned {offline, no_mode, same, peer, unknown, backward, branch_count};
static_assert(unsigned(Outcome::unknown)==Branch::unknown);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Machine address"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0x69ad3u,0x69ae0u,0x69ae6u,0x69aecu,0x69af6u,0x69af8u,0x69afeu,0x69b02u,0x69b14u,0x69b14u,0x69b25u,
    0x69b34u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::answer_load)+1u);
template<class State,class Peer> struct Adapter : Port<State> {
    Peer &peer_child;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Peer &p):Port<State>(s,c,t,{}),peer_child(p){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case networking:set_low_byte(gp[ecx],v);break;
        case network:gp[edx]=v;break;
        case mode:case machine_low:gp[ecx]=v;break;
        case machine_high:set_low_word(gp[eax],v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==0u){put(0x69ad9u,gp[ebx]);gp[ebx]=gp[eax];set_low_byte(gp[eax],this->flags.logic(0u,1));}   // PUSH EBX; MOV EBX,EAX; XOR AL,AL
        else{put(0x69afcu,gp[esi]);put(0x69afdu,gp[edi]);}                                                // PUSH ESI; PUSH EDI
    }
    void enabled_tested(std::uint32_t enabled){this->test8(enabled);}                                  // TEST CL,CL
    void mode_tested(std::uint32_t mode_value){this->test32(mode_value);}                              // TEST ECX,ECX
    void copying(std::uint32_t machine){this->gp[eax]=machine;}                                        // LEA EAX,[EDX+D]
    void comparing(std::uint32_t given,std::uint32_t copy) {                                           // MOV ECX,3; LEA EDI; MOV ESI,EBX; XOR EAX,EAX
        auto &gp=this->gp;
        gp[ecx]=logic::address_words;gp[edi]=copy;gp[esi]=given;gp[eax]=this->flags.logic(0u,4);
        if(step()!=2u)this->reached(Branch::backward);
    }
    std::uint32_t step()const{return (this->s.other_flags&0x400u)?0xfffffffeu:2u;}
    void compared(std::uint32_t given,std::uint32_t copy) {                                            // CMPSW: flags; ESI, EDI on; ECX down
        auto &gp=this->gp;
        this->flags.minus(given&0xffffu,copy&0xffffu,2);gp[esi]+=step();gp[edi]+=step();--gp[ecx];
    }
    void restoring(){auto &gp=this->gp;gp[edi]=take(0x69b17u);gp[esi]=take(0x69b18u);}                 // POP EDI; POP ESI
    std::uint32_t peer(std::uint32_t network_address) {                                                // PUSH EDX; CALL 6C3F0; TEST EAX,EAX
        auto &gp=this->gp;
        put(0x69b1bu,network_address);
        this->call(0x69b1cu,0x69b21u,0x6c3f0u,peer_child);depth-=4u;
        this->test32(gp[eax]);
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0xcu,4);depth=0xcu;                                                      // SUB ESP,C
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        const auto answer=logic::machine_address::check(memory,*this,gp[eax],outcome);
        this->reached(unsigned(outcome));
        set_low_byte(gp[eax],answer);
        std::uint32_t at=0x69b38u;
        if(outcome==Outcome::offline || outcome==Outcome::same)at=0x69b2fu;
        if(outcome==Outcome::peer)at=0x69b28u;
        gp[ebx]=take(at);
        gp[esp]=fl.plus(gp[esp],0xcu,4);                                                                 // ADD ESP,C
        this->ret(at+4u,0u);this->publish(at+4u);
    }
};
template<class State,class Reference,class Peer> bool bridge(State &s,Reference reference,Peer peer) {
    return run_owner(s,0x69ad0u,0x69b3cu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Peer>(v,c,t,peer);});
}
} // namespace halo2::native_machine_address_detail
