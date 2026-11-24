#pragma once
// Boundary adapter for the object team, C8E70
// (native_tu15_object_team_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the object (RET); a 0x1C-byte
// frame (the second team, the iterator) below the saved ESI and EDI (and EBP,
// EBX as the paths need them); D2B60 (EAX the object, EBX the iterator) and
// D2BD0 (EDX the iterator) direct, every register each returns coming back.
// Each access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_TEAM=1 restores the translated body.
// HALO2_VERIFY_OBJECT_TEAM=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/objects/native_tu15_object_team_systems.hpp"

namespace halo2::native_object_team_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_object_team;
using Access=logic::object_team::Access;
using Kept=logic::object_team::Kept;
using Outcome=logic::object_team::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_OBJECT_TEAM");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_OBJECT_TEAM");return v;}
enum Branch : unsigned {held, player_team, actor_team, untouched, no_team, riders_team, second_seat, branch_count};
static_assert(unsigned(Outcome::riders_team)==Branch::riders_team);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Object team"};return v;}
inline void set_low_word(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff0000u)|(value&0xffffu);}
inline constexpr std::uint32_t sites[]{0xc8e70u,0xc8e7au,0xc8e89u,0xc8e8du,0xc8e9fu,0xc8eadu,0xc8eb3u,0xc8ec2u,0xc8eccu,0xc8ed9u,
    0xc8ee3u,0xc8ee9u,0xc8ef8u,0xc8efeu,0xc8f0bu,0xc8f1du,0xc8f35u,0xc8f46u,0xc8f54u,0xc8f58u,0xc8f68u,0xc8f6eu,0xc8f7fu,0xc8f83u,
    0xc8f96u,0xc8f9au,0xc8fa0u,0xc8fabu,0xc8fb2u,0xc8fb9u,0xc8fd1u,0xc8fddu,0xc8ff7u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::team_store)+1u);
template<class State,class Carrier,class Next> struct Adapter : Port<State> {
    Carrier &carrier_child;
    Next &next_child;
    std::uint32_t depth{};      // bytes below the return address
    unsigned walks{};
    bool ebp_saved{},ebx_saved{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Carrier &a,Next &n):Port<State>(s,c,t,{}),carrier_child(a),next_child(n){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:case player:case actor:case seat_load:case seat_flags:gp[ecx]=v;break;
        case datums:gp[esi]=v;break;
        case object_address:gp[edi]=v;break;
        case flags_byte:set_low_byte(gp[ecx],v);break;
        case players_array:case actors_array:case rider_object:gp[edx]=v;break;
        case players_data:gp[eax]=v;gp[ecx]=fl.product(fl.logic(gp[ecx]&0xffffu,4),logic::player_size);break;   // AND ECX,FFFF; IMUL ECX,21C
        case actors_data:gp[eax]=v;gp[ecx]=fl.product(fl.logic(gp[ecx]&0xffffu,4),logic::actor_size);break;     // AND ECX,FFFF; IMUL ECX,888
        case player_team:set_low_word(gp[ecx],logic::widen8(v));break;                                // MOVSX CX,byte
        case actor_team:set_low_word(gp[ecx],v);break;
        case instances:gp[ebx]=v;break;
        case second_load:case rider_load:case vehicle_load:case vehicle_object:case vehicle_tag:case definition_address:case seats_load:
        case second_final:gp[eax]=v;break;
        case second_team:set_low_word(gp[edx],v);break;
        case first_team:set_low_word(gp[ebp],v);break;
        default:break;
        }
        if(a==vehicle_object||a==definition_address)(void)0;
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                   // SUB ESP,1C; PUSH ESI
        auto &gp=this->gp;
        gp[esp]=this->flags.minus(gp[esp],0x1cu,4);depth=0x1cu;put(0xc8e79u,gp[esi]);
    }
    void indexing(std::uint32_t object_handle) {                                                       // MOV ECX,EAX; AND; LEA EDX; PUSH EDI
        auto &gp=this->gp;
        gp[ecx]=this->flags.logic(object_handle&0xffffu,4);gp[edx]=3u*gp[ecx];put(0xc8e88u,gp[edi]);
    }
    void held_tested(std::uint32_t byte) {                                                             // SHR CL,2; TEST CL,1
        auto &gp=this->gp;auto &fl=this->flags;
        set_low_byte(gp[ecx],fl.right(byte&0xffu,2u,1,false));fl.logic(gp[ecx]&1u,1);
    }
    void saving_ebp(std::uint32_t player_handle,const Kept &kept) {                                   // PUSH EBP; OR EBP,-1; CMP ECX,EBP
        auto &gp=this->gp;
        put(0xc8ea5u,gp[ebp]);ebp_saved=true;gp[ebp]=this->flags.logic(kept.first,4);this->compare32(player_handle,gp[ebp]);
    }
    void leaving_ebp(unsigned which,std::uint32_t){this->gp[ebp]=take(which?0xc8efdu:0xc8ecbu);ebp_saved=false;}
    void actor_tested(std::uint32_t actor_handle){this->compare32(actor_handle,this->gp[ebp]);}       // CMP ECX,EBP
    void type_tested(std::uint32_t type){this->flags.minus(type&0xffu,logic::object::vehicle,1);}     // CMP byte [EDI+AA],1
    void saving_ebx(){auto &gp=this->gp;put(0xc8f18u,gp[ebx]);ebx_saved=true;gp[ebx]=local(logic::local::iterator);}   // PUSH EBX; LEA EBX
    void carrier(std::uint32_t object_handle,Kept &kept) {                                             // CALL D2B60 (EAX the object, EBX the iterator)
        this->gp[eax]=object_handle;this->gp[ebx]=kept.held;
        this->call(0xc8f21u,0xc8f26u,0xd2b60u,carrier_child);
        refresh(kept);
    }
    std::uint32_t next(Kept &kept) {                                                                   // MOV EDX,EBX (LEA EDX,[ESP+14]); CALL D2BD0; TEST AL,AL
        auto &gp=this->gp;
        const bool first=walks++==0u;
        gp[edx]=first?kept.held:local(logic::local::iterator);
        this->call(first?0xc8f28u:0xc8fc4u,first?0xc8f2du:0xc8fc9u,0xd2bd0u,next_child);
        refresh(kept);
        this->test8(gp[eax]&0xffu);
        return gp[eax];
    }
    void first_tested(std::uint32_t first){this->flags.minus(first&0xffffu,0xffffu,2);}               // CMP BP,FFFF
    void second_tested(std::uint32_t second){this->flags.minus(second&0xffffu,0xffffu,2);}            // CMP AX,FFFF
    void seat_tested(std::uint32_t rider,std::uint32_t seat) {                                         // AND EAX,FFFF; CMP CX,FFFF; LEA EDX,[EAX+EAX*2]
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.logic(rider&0xffffu,4);fl.minus(seat&0xffffu,0xffffu,2);gp[edx]=3u*gp[eax];
    }
    void vehicle_tested(std::uint32_t vehicle){this->compare32(vehicle,logic::none);}                 // CMP EAX,-1
    void seat_flags_tested(std::uint32_t flags,std::uint32_t) {                                        // SHR ECX,B; TEST CL,1
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.right(flags,logic::second_team_seat,4,false);fl.logic(gp[ecx]&1u,1);
        if(gp[ecx]&1u)this->reached(Branch::second_seat);
    }
    void decided(bool none_team) {                                                                     // POP EBX; POP EBP (before the NONE store)
        auto &gp=this->gp;
        if(none_team){gp[ebx]=take(0xc8fdbu);gp[ebp]=take(0xc8fdcu);ebx_saved=ebp_saved=false;}
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::object_team::update(memory,*this,gp[eax],Kept{gp[ebx],gp[ebp],gp[esi],gp[edi]});
        this->reached(unsigned(outcome));
        std::uint32_t at=0xc9000u;
        switch(outcome) {
        case Outcome::player_team:at=0xc8ed3u;break;
        case Outcome::actor_team:at=0xc8f05u;break;
        case Outcome::no_team:at=0xc8fe6u;break;
        case Outcome::riders_team:gp[ebx]=take(0xc8ffeu);gp[ebp]=take(0xc8fffu);break;
        case Outcome::untouched:gp[ebp]=take(0xc8fffu);break;
        default:break;
        }
        gp[edi]=take(at);gp[esi]=take(at+1u);
        gp[esp]=fl.plus(gp[esp],0x1cu,4);                                                              // ADD ESP,1C
        this->ret(at+5u,0u);this->publish(at+5u);
    }
};
template<class State,class Reference,class Carrier,class Next> bool bridge(State &s,Reference reference,Carrier carrier,Next next) {
    return run_owner(s,0xc8e70u,0xc9005u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Carrier,Next>(v,c,t,carrier,next);});
}
} // namespace halo2::native_object_team_detail
