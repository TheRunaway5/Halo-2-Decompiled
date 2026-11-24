#pragma once
// Boundary adapter for the object placement, B95D0
// (native_tu15_object_placement_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX, ECX and EDX the
// vectors (or NULL), three arguments on the stack (RET C); a 0x24-byte frame
// (the copies) below the saved EBX, EBP, ESI and EDI; 10E340 gets the
// position's and forward's copies pushed (RET 8); B9460 its nine arguments
// pushed (RET 24); BA570 the object pushed (RET 4); B9390 EAX the object;
// every register each returns coming back. Which children are direct and
// which go through the unit's dispatch differs by unit, so the bridge takes
// the unit's steps. Each access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_PLACEMENT=1 restores the translated body.
// HALO2_VERIFY_OBJECT_PLACEMENT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/objects/native_tu15_object_placement_systems.hpp"

namespace halo2::native_object_placement_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_object_placement;
using Access=logic::object_placement::Access;
using Kept=logic::object_placement::Kept;
using Outcome=logic::object_placement::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_OBJECT_PLACEMENT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_OBJECT_PLACEMENT");return v;}
enum Branch : unsigned {placed, refused_kept, refused, branch_count};
static_assert(unsigned(Outcome::refused)==Branch::refused);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Object placement"};return v;}
inline constexpr std::uint32_t sites[]{0xb95dfu,0xb95e5u,0xb95e7u,0xb95ebu,0xb95eeu,0xb95f1u,0xb95f5u,0xb9601u,0xb9603u,0xb9607u,0xb960au,
    0xb960du,0xb9611u,0xb961du,0xb961fu,0xb9622u,0xb9625u,0xb9629u,0xb962du,0xb963cu,0xb9640u,0xb9659u,0xb965fu,0xb9665u,
    0xb966bu,0xb9678u,0xb967cu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::object_link)+1u);
template<class State,class Steps> struct Adapter : Port<State> {
    Steps &steps;
    std::uint32_t depth{};      // bytes below the return address
    std::uint32_t up_pointer{}; // EDX at entry, for TEST EDX,EDX
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Steps &st):Port<State>(s,c,t,{}),steps(st){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t copy(std::uint32_t offset)const{return this->gp[esp]+depth-0x24u+offset;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case object_argument:gp[edi]=v;break;
        case position_x:case position_y:gp[ebp]=v;break;
        case position_z:case forward_x:case forward_y:case up_x:case b_argument:case object_address:gp[eax]=v;break;
        case forward_z:case up_y:case a_argument:case headers:gp[ecx]=v;break;
        case up_z:case game:gp[edx]=v;break;
        case mode:fl.minus(v&0xffu,logic::kept_mode,1);break;
        case datums:gp[edx]=v;gp[eax]=3u*fl.logic(gp[edi]&0xffffu,4);break;                          // MOV EAX,EDI; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]
        case object_link:                                                                            // CMP [EAX+D4],-1; SETNZ AL; TEST AL,AL
            fl.minus(v,logic::none,4);set_low_byte(gp[eax],v!=logic::none?1u:0u);this->test8(gp[eax]&0xffu);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void step(std::uint32_t pc,std::uint32_t target) {
        auto child=[&](State &v){steps(v,target,pc);};
        this->call(pc,pc+5u,target,child);
    }
    void refresh(Kept &kept){kept={this->gp[ebp],this->gp[esi],this->gp[ebx],this->gp[edi]};}
    // The copies' registers (LEA, or 0); TEST EDX,EDX before; PUSH ESI; PUSH EBP; CALL 10E340.
    void checked(Kept &kept) {
        auto &gp=this->gp;
        gp[ebp]=kept.position;gp[esi]=kept.forward;gp[ebx]=kept.up;this->test32(up_pointer);
        put(0xb9635u,gp[esi]);put(0xb9636u,gp[ebp]);
        step(0xb9637u,0x10e340u);depth-=8u;refresh(kept);
    }
    // PUSH 0; PUSH b; PUSH 1; PUSH 1; PUSH a; PUSH EBX; PUSH ESI; PUSH EBP; PUSH EDI; CALL B9460; TEST AL,AL.
    std::uint32_t place(Kept &kept,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        put(0xb9644u,0u);put(0xb9646u,gp[eax]);put(0xb9647u,1u);put(0xb9649u,1u);put(0xb964bu,gp[ecx]);
        put(0xb964cu,gp[ebx]);put(0xb964du,gp[esi]);put(0xb964eu,gp[ebp]);put(0xb964fu,gp[edi]);
        step(0xb9650u,0xb9460u);depth-=36u;
        this->test8(gp[eax]&0xffu);refresh(kept);
        return gp[eax];
    }
    void refused(Kept &kept){put(0xb968au,this->gp[edi]);step(0xb968bu,0xba570u);depth-=4u;refresh(kept);}
    void finish(Kept &){this->gp[eax]=this->gp[edi];step(0xb9692u,0xb9390u);}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        up_pointer=gp[edx];
        gp[esp]=fl.minus(gp[esp],0x24u,4);depth=0x24u;                                                // SUB ESP,24
        put(0xb95d3u,gp[ebx]);put(0xb95d4u,gp[ebp]);put(0xb95d5u,gp[esi]);
        gp[ebp]=fl.logic(0u,4);gp[esi]=fl.logic(0u,4);gp[ebx]=fl.logic(0u,4);this->test32(gp[eax]);   // XOR EBP,EBP; XOR ESI,ESI; XOR EBX,EBX; TEST EAX,EAX
        put(0xb95deu,gp[edi]);
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::object_placement::place(memory,*this,gp[eax],gp[ecx],up_pointer)));
        gp[edi]=take(0xb9697u);gp[esi]=take(0xb9698u);gp[ebp]=take(0xb9699u);gp[ebx]=take(0xb969au);
        gp[esp]=fl.plus(gp[esp],0x24u,4);                                                             // ADD ESP,24
        this->ret(0xb969eu,0xcu);this->publish(0xb969eu);
    }
};
template<class State,class Reference,class Steps> bool bridge(State &s,Reference reference,Steps steps) {
    return run_owner(s,0xb95d0u,0xb969eu,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Steps>(v,c,t,steps);});
}
} // namespace halo2::native_object_placement_detail
