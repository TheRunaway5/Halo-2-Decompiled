#pragma once
// Boundary adapter for the director player, entered at 18A240
// (native_tu15_engine_queries_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the record index (RET); ESI and EDI saved;
// EAX the answer, ECX and EDX the last values the original leaves them; the
// flags of the last test. Each access publishes its original instruction.
// HALO2_REFERENCE_DIRECTOR_PLAYER=1 restores the translated body.
// HALO2_VERIFY_DIRECTOR_PLAYER=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_engine_queries_systems.hpp"

namespace halo2::native_engine_queries_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace director_player {
namespace logic=halo2::native_engine_queries::director_player;
using Access=logic::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_DIRECTOR_PLAYER");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_DIRECTOR_PLAYER");return v;}
enum Branch : unsigned {no_player, live, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Director player"};return v;}
template<class State> struct Adapter : Port<State> {
    bool saved_edi{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case records:return 0x18a240u;
        // IMUL EAX,EAX,94; LEA ECX,[EAX+ECX+14]; PUSH ESI.
        case player:gp[eax]=fl.product(gp[eax],logic::record_size);gp[ecx]=gp[eax]+gp[ecx]+0x14u;this->push(0x18a250u,gp[esi]);
            return 0x18a251u;
        case players:this->push(0x18a25bu,gp[edi]);saved_edi=true;return 0x18a25cu;                    // PUSH EDI
        case limit:return 0x18a262u;
        case datum_size:return 0x18a271u;
        case data:return 0x18a277u;
        case salt:return 0x18a27au;
        case unit:return 0x18a28eu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case records:gp[ecx]=v;break;
        case player:gp[esi]=v;gp[eax]=fl.logic(0xffffffffu,4);this->compare32(v,gp[eax]);break;        // OR EAX,-1; CMP ESI,EAX
        case players:gp[edi]=v;break;
        // MOV ECX,[EDI+38]; MOV EDX,ESI; AND EDX,FFFF; CMP EDX,ECX.
        case limit:gp[ecx]=v;gp[edx]=fl.logic(gp[esi]&0xffffu,4);this->compare32(gp[edx],v);break;
        case datum_size:gp[ecx]=fl.product(v,gp[edx]);break;                                           // IMUL ECX,EDX
        case data:gp[ecx]=fl.plus(gp[ecx],v,4);break;                                                  // ADD ECX,[EDI+44]
        case salt:gp[edx]=(gp[edx]&0xffff0000u)|(v&0xffffu);fl.logic(v&0xffffu,2);                     // MOV DX; TEST DX,DX
            if(v&0xffffu) {
                gp[edi]=gp[esi];gp[edx]=native_object_update::widen16(v&0xffffu);gp[edi]=fl.right(gp[edi],16u,4,true);  // MOV EDI,ESI; MOVSX; SAR EDI,10
                this->compare32(gp[edx],gp[edi]);
            }
            break;
        case unit:this->compare32(v,native_object_update::none);break;                                                // CMP [ECX+2C],EAX
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto answer=logic::of(memory,*this,gp[eax]);
        gp[eax]=answer;
        this->reached(answer==native_object_update::none?Branch::no_player:Branch::live);
        if(saved_edi)gp[edi]=this->pop(0x18a295u);
        gp[esi]=this->pop(0x18a296u);
        this->ret(0x18a297u,0u);this->publish(0x18a297u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x18a240u,0x18a297u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace director_player
} // namespace halo2::native_engine_queries_detail
