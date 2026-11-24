#pragma once
// Boundary adapter for the pose sample, entered at 212D40
// (native_tu15_pose_sample_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the value, ECX the vector, EDX the record, the
// frame on the stack (RET 4); EBX, EBP, ESI and EDI saved; EAX and EDX the
// last two words read (the kind less three for other kinds); XMM0 zero;
// the flags of the last instruction that sets them (the frame's ADD of the
// base, or the kind's DECs). Each access publishes its original
// instruction.
// HALO2_REFERENCE_POSE_SAMPLE=1 restores the translated body.
// HALO2_VERIFY_POSE_SAMPLE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_pose_sample_systems.hpp"

namespace halo2::native_pose_sample_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_pose_sample;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_POSE_SAMPLE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_POSE_SAMPLE");return p && *p;}();
    return value;
}
enum Branch : unsigned {other_kind, pair, pair_value, triple_value, branch_count};
static_assert(unsigned(logic::pose_sample::Outcome::triple_value)==Branch::triple_value);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Pose sample verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
// Each kind's instructions: the description reads, the base, the component
// reads and stores, the value read and store, the epilogue.
struct KindSites {std::uint32_t description,second,start,offset,lead,base,component[3],store[3],value,value_store,pop;};
inline constexpr KindSites kinds[]{
    {0x212df9u,0x212dfcu,0x212e00u,0x212e09u,0x212e0cu,0x212e16u,{0x212e18u,0x212e1fu,0u},{0x212e1du,0x212e22u,0u},0u,0u,0x212e25u},
    {0x212dbdu,0x212dc0u,0x212dc4u,0x212dd4u,0x212dd7u,0x212ddeu,{0x212de0u,0x212de7u,0u},{0x212de5u,0x212deau,0u},0x212dedu,0x212df0u,0x212df2u},
    {0x212d7du,0x212d80u,0x212d84u,0x212d8au,0x212d8du,0x212d9cu,{0x212d9eu,0x212da5u,0x212dabu},{0x212da3u,0x212da8u,0x212daeu},0x212db1u,0x212db4u,0x212db6u}};
template<class State> struct PoseSample : Port<State> {
    using Access=logic::pose_sample::Access;
    std::uint32_t k{},kind_value{},into{},base_value{};
    bool started{};
    PoseSample(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    // [ESP+8] after PUSH EBX.
    std::uint32_t frame_slot()const{return this->gp[esp]+8u;}
    const KindSites &sites()const{return kinds[kind_value-1u];}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        static constexpr std::uint32_t copies[]{0x212d52u,0x212d58u,0x212d5eu},copy_stores[]{0x212d56u,0x212d5bu,0x212d61u};
        switch(a) {
        case frame_word:return 0x212d44u;
        case zero_vector:return 0x212d4du;
        case start_copy:return copies[k];
        case start_store:return copy_stores[k];
        case value_clear:return 0x212d64u;
        case Access::kind:return 0x212d68u;
        case description:return sites().description;
        case second:return sites().second;
        case start:return sites().start;
        case offset:return sites().offset;
        case lead:return sites().lead;
        case base:return sites().base;
        case component:return sites().component[k];
        case component_store:return sites().store[k];
        case value:return sites().value;
        case value_store:return sites().value_store;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV EBX,[ESP+8]; PUSH EBP; PUSH ESI; PUSH EDI; MOV EDI,EAX.
        case frame_word:gp[ebx]=v;this->push(0x212d48u,gp[ebp]);this->push(0x212d49u,gp[esi]);this->push(0x212d4au,gp[edi]);gp[edi]=gp[eax];break;
        case zero_vector:gp[eax]=v;gp[esi]=gp[ecx];break;                                           // MOV EAX,[474D04]; MOV ESI,ECX
        case start_copy:if(k==2)gp[eax]=v;else gp[ebp]=v;break;
        // MOVZX EAX,byte [EDX+9]; DEC EAX until the kind matches (three for the others).
        case Access::kind:{
            gp[eax]=v&0xffu;
            const auto decrements=(v&0xffu)>=1u && (v&0xffu)<=3u?(v&0xffu):3u;
            for(std::uint32_t d=0;d<decrements;++d)gp[eax]=fl.increment(gp[eax],4,true);
            kind_value=v&0xffu;break;
        }
        case description:gp[esi]=v;break;
        // The last two words read: EDX then EAX (kind 1), or both into EDX with the value in EAX.
        case component:if(kind_value==1u && k==1u)gp[eax]=v;else gp[edx]=v;break;
        case value:gp[eax]=v;break;
        case base:gp[esi]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void component(std::uint32_t index){k=index;}
    void located(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t sum,std::uint32_t base_address){into=sum;base_value=base_address;}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        using Outcome=logic::pose_sample::Outcome;
        this->s.xmm[0]=_mm_setzero_ps();this->push(0x212d43u,gp[ebx]);                               // XORPS XMM0,XMM0; PUSH EBX
        Memory<PoseSample> memory{*this};
        const auto outcome=logic::pose_sample::sample(memory,*this,gp[eax],gp[ecx],gp[edx]);
        this->reached(unsigned(outcome));
        std::uint32_t pop_pc=0x212e25u;
        if(outcome!=Outcome::other_kind){fl.plus(into,base_value,4);pop_pc=sites().pop;}            // ADD EAX,ESI
        gp[edi]=this->pop(pop_pc);gp[esi]=this->pop(pop_pc+1u);gp[ebp]=this->pop(pop_pc+2u);gp[ebx]=this->pop(pop_pc+3u);
        this->ret(pop_pc+4u,4u);this->publish(pop_pc+4u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x212d40u,0x212e29u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return PoseSample<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_pose_sample_detail
