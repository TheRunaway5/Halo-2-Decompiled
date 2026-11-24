#pragma once
// Boundary adapter for the team count, 15B620
// (native_tu15_team_count_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: three output addresses on the stack
// (RET C); a 0x20-byte frame below the saved EBX, EBP, ESI and EDI; 15B470
// gets the registers as the scan left them (EBX the array, EDI the player,
// ESI its handle, EBP its team's bit), every register it returns coming
// back. Each access publishes its original instruction.
// HALO2_REFERENCE_TEAM_COUNT=1 restores the translated body.
// HALO2_VERIFY_TEAM_COUNT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_team_count_systems.hpp"

namespace halo2::native_team_count_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_team_count;
using Access=logic::team_count::Access;
using Kept=logic::team_count::Kept;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_TEAM_COUNT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_TEAM_COUNT");return v;}
// The outcome, then each player's: inactive, a new team, not living, a new
// living team; and each output written.
enum Branch : unsigned {counted, inactive, new_team, not_living, new_living_team, teams_written, living_written, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Team count"};return v;}
inline constexpr std::uint32_t sites[]{0x15b624u,0x15b62fu,0x15b633u,0x15b637u,0x15b63bu,0x15b643u,0x15b647u,0x15b64eu,0x15b655u,0x15b671u,
    0x15b6a6u,0x15b6a9u,0x15b6afu,0x15b6b9u,0x15b6c1u,0x15b6c5u,0x15b6e9u,0x15b6fau,0x15b701u,0x15b705u,0x15b712u,0x15b71cu,
    0x15b722u,0x15b726u,0x15b730u,0x15b736u,0x15b67bu,0x15b687u,0x15b68bu,0x15b68du,0x15b695u,0x15b699u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::living_output)+1u);
template<class State,class Route> struct Adapter : Port<State> {
    Route &route;
    std::uint32_t depth{};      // bytes below the return address
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth-0x20u+offset;}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case bit:                                                                                      // MOV ECX,EAX; AND ECX,1F; MOV EDI,1; SHL EDI,CL; MOV ECX,EAX; SAR ECX,5
            gp[ecx]=fl.logic(gp[eax]&0x1fu,4);gp[edi]=fl.shift(1u,gp[ecx]&0xffu,4);gp[ecx]=fl.right(gp[eax],5u,4,true);break;
        case datum_size:fl.minus(gp[eax],logic::none,4);break;                                         // CMP EAX,-1
        case counted_load:case living_counted_load:gp[ecx]=gp[eax];break;                              // MOV ECX,EAX
        default:break;
        }
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case players:gp[ebx]=v;break;
        case index_load:gp[eax]=fl.increment(v,4,false);break;                                        // INC EAX
        case array_count:gp[esi]=v;fl.minus(gp[eax],v,4);break;                                        // CMP EAX,ESI
        case bitmap:gp[edx]=v;break;
        case bit:                                                                                      // TEST [EDX+ECX*4],EDI; then INC EAX; CMP EAX,ESI
            fl.logic(v&gp[edi],4);
            if(!(v&gp[edi])){gp[eax]=fl.increment(gp[eax],4,false);fl.minus(gp[eax],gp[esi],4);}
            break;
        case datum_size:gp[edi]=v;break;
        case array_data:gp[esi]=v;gp[edi]=fl.product(gp[edi],gp[eax]);break;                          // IMUL EDI,EAX
        case team:gp[ecx]=logic::widen8(v);gp[edi]=fl.plus(gp[edi],gp[esi],4);break;                  // ADD EDI,ESI
        case salt:gp[esi]=fl.logic(fl.shift(logic::widen16(v),16u,4)|gp[eax],4);break;               // SHL ESI,10; OR ESI,EAX
        case flags:                                                                                    // TEST [EDI+2],2; TEST ECX,ECX; CMP ECX,10; CMP ECX,-1
            fl.logic(v&logic::player::inactive,1);
            if(v&logic::player::inactive){this->reached(Branch::inactive);break;}
            this->test32(gp[ecx]);
            if(std::int32_t(gp[ecx])<0)break;
            fl.minus(gp[ecx],logic::teams_limit,4);
            if(std::int32_t(gp[ecx])>=std::int32_t(logic::teams_limit))break;
            fl.minus(gp[ecx],logic::none,4);break;
        case teams_load:gp[eax]=v;gp[ebp]=fl.shift(1u,gp[ecx]&0xffu,4);fl.logic(gp[ebp]&v,4);break;   // MOV EBP,1; SHL EBP,CL; TEST EBP,EAX
        case counted_load:gp[ecx]=fl.logic(gp[ecx]|gp[ebp],4);gp[eax]=fl.increment(v,4,false);break;  // OR ECX,EBP; INC EAX
        case flags_again:fl.logic(v&logic::player::unliving,1);if(v&logic::player::unliving)this->reached(Branch::not_living);break;
        case living_load:gp[eax]=v;fl.logic(v&gp[ebp],4);break;                                        // TEST EAX,EBP
        case last_argument:gp[edx]=v;break;
        case living_counted_load:gp[eax]=v;gp[ecx]=fl.logic(gp[ecx]|gp[ebp],4);break;                  // OR ECX,EBP
        case teams_argument:case living_argument:gp[eax]=v;this->test32(v);break;
        case counted_final:gp[ecx]=v;break;
        case living_counted_final:gp[edx]=v;break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case teams_store:this->reached(Branch::new_team);break;
        case living_store:gp[eax]=this->flags.increment(gp[eax],4,false);this->reached(Branch::new_living_team);break;   // INC EAX
        case teams_output:this->reached(Branch::teams_written);break;
        case living_output:this->reached(Branch::living_written);break;
        default:break;
        }
    }
    // PUSH EBP; XOR EAX,EAX; PUSH ESI; PUSH EDI.
    void saving(){auto &gp=this->gp;put(0x15b62au,gp[ebp]);gp[eax]=this->flags.logic(0u,4);put(0x15b62du,gp[esi]);put(0x15b62eu,gp[edi]);}
    void restoring() {
        auto &gp=this->gp;
        gp[edi]=take(0x15b681u);gp[esi]=take(0x15b682u);gp[ebp]=take(0x15b683u);gp[ebx]=take(0x15b684u);
    }
    std::uint32_t living(Kept &kept) {
        auto &gp=this->gp;
        this->call_routed(route,0x15b709u,0x15b70eu,0x15b470u);this->test8(gp[eax]&0xffu);
        kept={gp[ebx],gp[edi],gp[esi],gp[ebp]};
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x20u,4);depth=0x20u;                                                 // SUB ESP,20
        put(0x15b623u,gp[ebx]);
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::team_count::count(memory,*this)));
        gp[esp]=fl.plus(gp[esp],0x20u,4);                                                              // ADD ESP,20
        this->ret(0x15b69eu,0xcu);this->publish(0x15b69eu);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    return run_owner(s,0x15b620u,0x15b73bu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Route>(v,c,t,route);});
}
} // namespace halo2::native_team_count_detail
