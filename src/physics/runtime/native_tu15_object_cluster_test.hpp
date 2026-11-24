#pragma once
// Boundary adapter for the object cluster test, entered at B8D90
// (native_tu15_object_cluster_test_systems.hpp). Its callers and 10DDE0 keep
// the original ABI, which the adapter owns: EAX the object handle, the
// cluster set at [ESP+4] (its slot's low byte the result byte), RET 4; EBP,
// ESI and EDI saved (the cluster set, the header, the handle), EBX around
// 10DDE0; 10DDE0 gets EDI the handle, EBX the result byte's address and the
// cluster set pushed (and pops it), and every register it returns comes
// back; the result in AL above the rest of EAX as the original leaves it;
// ECX and EDX as the original leaves them; the flags of the last
// instruction that sets them. Each access publishes its original
// instruction.
// HALO2_REFERENCE_OBJECT_CLUSTER_TEST=1 restores the translated body.
// HALO2_VERIFY_OBJECT_CLUSTER_TEST=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_cluster_test_systems.hpp"

namespace halo2::native_object_cluster_test_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_cluster_test;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_CLUSTER_TEST");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_CLUSTER_TEST");return p && *p;}();
    return value;
}
enum Branch : unsigned {always, machine_refused, answered, no_cluster, looked_up, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object cluster test verify: %llu calls; %llu ordered accesses (%llu stores) "
            "identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Ask> struct ClusterTest : Port<State> {
    using Access=logic::object_cluster_test::Access;
    using Kept=logic::object_cluster_test::Kept;
    Ask &ask_child;std::uint32_t depth{};
    ClusterTest(State &s,std::uint32_t *c,Trace<State> *t,Ask &a):Port<State>(s,c,t,{}),ask_child(a){}
    std::uint32_t argument()const{return this->gp[esp]+depth+4u;}
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    void restore(std::uint32_t pc,unsigned r){this->gp[r]=this->pop(pc);depth-=4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0xb8d90u;
        case header_datums:return 0xb8d96u;
        case clusters_argument:save(0xb8d99u,this->gp[ebp]);return 0xb8d9au;                       // PUSH EBP
        case object_address:return 0xb8dadu;
        case result_clear:return 0xb8db0u;
        case object_flags:return 0xb8db5u;
        case type:return 0xb8dc6u;
        case cluster_check:return 0xb8de4u;
        case cluster:return 0xb8debu;
        case cluster_word:return 0xb8dfeu;
        case result_read:return 0xb8e0du;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;break;
        // MOV EBP,[ESP+8]; PUSH ESI; PUSH EDI; MOV EDI,EAX; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]; LEA ESI,[EDX+EAX*4].
        case clusters_argument:
            gp[ebp]=v;save(0xb8d9eu,gp[esi]);save(0xb8d9fu,gp[edi]);gp[edi]=gp[eax];
            gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;gp[esi]=gp[edx]+gp[eax]*4u;break;
        case object_address:gp[eax]=v;break;
        case object_flags:gp[eax]=fl.right(v,logic::always_bit,4,false);this->test8(gp[eax]&1u);break;   // SHR EAX,1; TEST AL,1
        // MOV CL,[ESI+3]; MOV EDX,1; SHL EDX,CL; TEST DL,DL.
        case type:set_low_byte(gp[ecx],v);gp[edx]=fl.shift(1u,v&0xffu,4);this->test8(gp[edx]);break;
        case cluster_check:fl.minus(v&0xffffu,0xffffu,2);break;                                        // CMP word,-1
        // MOVSX EAX; MOV ECX,EAX; AND ECX,1F; MOV EDX,1; SHL EDX,CL; SAR EAX,5.
        case cluster:
            gp[eax]=logic::widen16(v);gp[ecx]=fl.logic(gp[eax]&0x1fu,4);gp[edx]=fl.shift(1u,gp[ecx]&0xffu,4);
            gp[eax]=fl.right(gp[eax],5u,4,true);break;
        // MOV ECX,[EBP+EAX*4]; POP EDI; TEST ECX,EDX; POP ESI; (SETNZ AL: the logic's result); POP EBP.
        case cluster_word:
            gp[ecx]=v;restore(0xb8e02u,edi);this->test32(v&gp[edx]);restore(0xb8e05u,esi);restore(0xb8e09u,ebp);break;
        case result_read:set_low_byte(gp[eax],v);break;                                                // MOV AL,[ESP+10]
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH EBX; PUSH EBP; LEA EBX,[ESP+18]; CALL 10DDE0 (which pops the cluster set); TEST AL,AL; POP EBX.
    std::uint32_t ask(Kept &kept) {
        auto &gp=this->gp;
        save(0xb8dd4u,gp[ebx]);save(0xb8dd5u,gp[ebp]);gp[ebx]=argument();
        this->call(0xb8ddau,0xb8ddfu,0x10dde0u,ask_child);depth-=4u;
        this->test8(gp[eax]);const auto al=gp[eax]&0xffu;restore(0xb8de1u,ebx);
        kept={gp[esi],gp[ebp]};
        if(!al)this->reached(Branch::machine_refused);
        return al;
    }
    void run() {
        auto &gp=this->gp;
        Memory<ClusterTest> memory{*this};
        std::uint32_t result=0;
        using logic::object_cluster_test::Outcome;
        const auto outcome=logic::object_cluster_test::in(memory,*this,gp[eax],result);
        std::uint32_t exit=0;
        switch(outcome) {
        case Outcome::always:                                                                     // POP EDI; POP ESI; MOV AL,1; POP EBP
            this->reached(Branch::always);restore(0xb8dbeu,edi);restore(0xb8dbfu,esi);set_low_byte(gp[eax],1u);
            restore(0xb8dc2u,ebp);exit=0xb8dc3u;break;
        case Outcome::answered:case Outcome::no_cluster:                                          // POP EDI; POP ESI; POP EBP
            this->reached(outcome==Outcome::answered?Branch::answered:Branch::no_cluster);
            restore(0xb8e11u,edi);restore(0xb8e12u,esi);restore(0xb8e13u,ebp);exit=0xb8e14u;break;
        case Outcome::looked_up:this->reached(Branch::looked_up);set_low_byte(gp[eax],result);exit=0xb8e0au;break;
        }
        this->ret(exit,4u);this->publish(exit);
    }
};
template<class State,class Reference,class Ask> bool bridge(State &s,Reference reference,Ask ask) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xb8d90u,0xb8e14u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ClusterTest<State,Ask>(v,c,t,ask);});
    return true;
}
} // namespace halo2::native_object_cluster_test_detail
