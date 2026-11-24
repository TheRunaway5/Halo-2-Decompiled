#pragma once
// Boundary adapter for the sound readiness, 1718C0
// (native_tu15_sound_readiness_systems.hpp). Its callers and the child keep
// the original ABI, which the adapter owns: the sound on the stack (RET 4),
// the answer in AL; a 0xC-byte frame below the saved EBX and EBP (and ESI and
// EDI while the permutations are walked); 21F260 gets the sound, the sample
// and the mode pushed (RET C), every register it returns coming back. Each
// access publishes its original instruction.
// HALO2_REFERENCE_SOUND_READINESS=1 restores the translated body.
// HALO2_VERIFY_SOUND_READINESS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "audio/native_tu15_sound_readiness_systems.hpp"

namespace halo2::native_sound_readiness_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_sound_readiness;
using Access=logic::sound_readiness::Access;
using Kept=logic::sound_readiness::Kept;
using Outcome=logic::sound_readiness::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SOUND_READINESS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SOUND_READINESS");return v;}
enum Branch : unsigned {no_permutations, ready, not_ready, branch_count};
static_assert(unsigned(Outcome::not_ready)==Branch::not_ready);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Sound readiness"};return v;}
inline constexpr std::uint32_t sites[]{0x1718c3u,0x1718cbu,0x1718d9u,0x1718ddu,0x1718e5u,0x1718ebu,0x1718efu,0x1718f5u,0x1718ffu,
    0x171903u,0x171907u,0x171912u,0x17191au,0x171920u,0x171924u,0x17192cu,0x171934u,0x171951u,0x171955u,0x171960u,0x171964u,0x17196bu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::index_store)+1u);
template<class State,class Request> struct Adapter : Port<State> {
    Request &request_child;
    std::uint32_t depth{};      // bytes below the return address
    std::uint32_t last_al{};
    bool walked{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Request &r):Port<State>(s,c,t,{}),request_child(r){}
    std::uint32_t argument()const{return this->gp[esp]+depth+4u;}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case instances:gp[ecx]=v;put(0x1718c9u,gp[ebx]);put(0x1718cau,gp[ebp]);break;                  // PUSH EBX; PUSH EBP
        case sound_argument:gp[ebp]=v;gp[eax]=fl.shift(fl.logic(v&0xffffu,4),4u,4);break;               // MOV EAX,EBP; AND; SHL EAX,4
        case definition_address:case definition_load:case sample_array:case chunks_load:case count_load:gp[eax]=v;break;
        case permutation_count:gp[ecx]=logic::widen8(v);gp[edx]=fl.logic(0u,4);this->test32(gp[ecx]);break;   // XOR EDX,EDX; TEST ECX,ECX
        case sound_globals:case globals_again:gp[ecx]=v;break;
        case first_permutation:gp[eax]=logic::widen16(v);break;
        case permutation_array:                                                                         // ADD EAX,EDX; LEA EAX,[EAX+EAX*2]; LEA EDI,[ESI+EAX*4]
            gp[esi]=v;gp[eax]=3u*fl.plus(gp[eax],gp[edx],4);gp[edi]=gp[esi]+4u*gp[eax];break;
        case chunk_count:gp[eax]=logic::widen16(v);gp[esi]=fl.logic(0u,4);this->test32(gp[eax]);break;  // XOR ESI,ESI; TEST EAX,EAX
        case first_chunk:gp[edx]=logic::widen16(v);break;
        case chunk_array:gp[eax]=v;gp[edx]=fl.shift(fl.plus(gp[edx],gp[esi],4),4u,4);break;              // ADD EDX,ESI; SHL EDX,4
        case chunk_sample:gp[eax]=logic::widen16(v);gp[edx]=3u*gp[eax];break;                          // LEA EDX,[EAX+EAX*2]
        case index_load:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void ready_set(const Kept &kept){set_low_byte(this->gp[ebx],kept.ready);}
    void saving(){put(0x1718fbu,this->gp[esi]);put(0x1718fcu,this->gp[edi]);walked=true;}
    // PUSH 8; LEA EAX,[EAX+EDX*4]; PUSH EAX; PUSH EBP; CALL 21F260.
    std::uint32_t request(std::uint32_t sound,std::uint32_t sample,std::uint32_t mode,Kept &kept) {
        auto &gp=this->gp;
        put(0x171937u,mode);gp[eax]=sample;put(0x17193cu,gp[eax]);put(0x17193du,sound);
        this->call(0x17193eu,0x171943u,0x21f260u,request_child);depth-=12u;
        last_al=gp[eax]&0xffu;
        kept={gp[ebx]&0xffu,gp[ebp],gp[esi],gp[edi]};
        return gp[eax];
    }
    // TEST BL,BL; TEST AL,2; MOV BL,1 or XOR BL,BL.
    void answered(const Kept &kept) {
        auto &gp=this->gp;auto &fl=this->flags;
        const auto prior=gp[ebx]&0xffu;
        this->test8(prior);
        if(prior)fl.logic(last_al&logic::ready_bit,1);
        if(kept.ready)set_low_byte(gp[ebx],1u);else set_low_byte(gp[ebx],fl.logic(0u,1));
    }
    void chunk_advanced(const Kept &,std::uint32_t count){auto &gp=this->gp;gp[esi]=this->flags.increment(gp[esi],4,false);this->flags.minus(gp[esi],count,4);}
    void index_advanced(std::uint32_t,std::uint32_t total){auto &gp=this->gp;gp[edx]=this->flags.increment(gp[edx],4,false);this->flags.minus(gp[edx],total,4);}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0xcu,4);depth=0xcu;                                                   // SUB ESP,C
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::sound_readiness::ready(memory,*this)));
        if(walked){gp[edi]=take(0x171971u);gp[esi]=take(0x171972u);}
        gp[ebp]=take(0x171973u);set_low_byte(gp[eax],gp[ebx]);gp[ebx]=take(0x171976u);
        gp[esp]=fl.plus(gp[esp],0xcu,4);                                                               // ADD ESP,C
        this->ret(0x17197au,4u);this->publish(0x17197au);
    }
};
template<class State,class Reference,class Request> bool bridge(State &s,Reference reference,Request request) {
    return run_owner(s,0x1718c0u,0x17197au,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Request>(v,c,t,request);});
}
} // namespace halo2::native_sound_readiness_detail
