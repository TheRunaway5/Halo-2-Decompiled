#pragma once
// Boundary adapter for the child agents, 2F20F0
// (native_tu15_child_agents_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: the body, the other body, the
// dispatcher and the fourth argument on the stack (RET, the caller
// cleaning); an EBP frame, ESP aligned down to 16 below the exception
// registration (state -1, handler 37E516, the link) and 0x98 bytes, EBX,
// ESI and EDI saved below them, the frame read from the ESP each child left;
// 2E8FC0 (ECX the wrapper, 0 pushed, RET 4), 2E8CF0 (ECX the wrapper) and
// 2E12A0 (ECX the wrapper, 0 pushed, RET 4) through the unit's dispatch,
// the other's type (vtable +0x14, ECX the shape) and the agent (the stack
// body, the other, the dispatcher and the fourth pushed, the caller
// cleaning) through it at their call instructions, every register each
// returns coming back. XMM0 to XMM3 hold what the original's moves and
// arithmetic leave in them. fs:[0] is read and written at fs_base. Each
// access publishes its original instruction.
// HALO2_REFERENCE_CHILD_AGENTS=1 restores the translated body.
// HALO2_VERIFY_CHILD_AGENTS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_child_agents_systems.hpp"

namespace halo2::native_child_agents_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_child_agents;
using Access=logic::child_agents::Access;
using Kept=logic::child_agents::Kept;
using Outcome=logic::child_agents::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_CHILD_AGENTS");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_CHILD_AGENTS");return v;}
enum Branch : unsigned {empty, collided, owned, branch_count};
static_assert(unsigned(Outcome::collided)==Branch::collided);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Child agents"};return v;}
inline constexpr std::uint32_t sites[]{0x2f20fdu,0x2f2104u,0x2f2113u,0x2f2116u,0u,0u,0x2f2141u,0x2f2155u,0x2f2166u,0x2f2169u,0x2f216fu,
    0x2f2173u,0x2f2177u,0x2f2179u,0x2f2184u,0x2f2187u,0x2f218bu,0x2f2198u,0x2f21a0u,0u,0x2f21a8u,0x2f21b0u,0x2f21d6u,0x2f21e6u,0x2f21eeu,
    0x2f21f1u,0x2f21fdu,0x2f2201u,0x2f220au,0x2f220eu,0x2f2215u,0x2f2218u,0x2f221fu,0x2f222cu,0x2f2230u,0x2f2242u,0x2f224au,0x2f2250u,
    0x2f225du,0x2f225du,0x2f226du,0x2f2276u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::link_restore)+1u);
inline constexpr std::uint32_t row_loads[]{0x2f2119u,0x2f2121u,0x2f212au,0x2f2133u,0x2f213cu,0x2f214bu};
inline constexpr std::uint32_t row_stores[]{0x2f211cu,0x2f2125u,0x2f212eu,0x2f2137u,0x2f2143u,0x2f2159u};
inline constexpr std::uint32_t rotation_loads[]{0x2f21a3u,0x2f21abu,0x2f21c7u};
template<class State,class Dispatch> struct Adapter : Port<State> {
    Dispatch &dispatch;
    std::uint32_t depth{};      // bytes pushed below the saved registers (negative before EDI)
    unsigned rows_loaded{},rows_stored{},rotations{};
    bool looping{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Dispatch &d):Port<State>(s,c,t,{}),dispatch(d){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+depth+offset;}
    std::uint32_t argument(unsigned n)const{return this->gp[ebp]+8u+4u*n;}
    std::uint32_t exception_list()const{return this->s.fs_base;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    void refresh(Kept &kept) {
        auto &gp=this->gp;
        kept.dispatcher=gp[ebx];
        if(looping){kept.index=gp[esi];kept.child=gp[edi];}else{kept.body=gp[esi];kept.list=gp[edi];}
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case motion_row:return row_loads[rows_loaded++];
        case copy_store:return row_stores[rows_stored++];
        case child_offset:rotations=0u;return sites[unsigned(a)];
        case rotation_load:return rotation_loads[rotations++];
        default:return sites[unsigned(a)];
        }
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case link_load:case motion_load:case count_load:case body_again:case motion_again:case agent_load:case list_again:
        case wrapper_flags:gp[eax]=v;break;
        case body_argument:gp[esi]=v;break;
        case list_load:gp[edi]=v;break;
        case other_argument:case other_shape:case child_shape:case agents_load:case other_again:case count_again:case link_again:
            gp[ecx]=v;break;
        case other_vtable:case other_type_load:case fourth_argument:gp[edx]=v;break;
        case dispatcher_argument:gp[ebx]=v;break;
        default:break;
        }
    }
    void loaded_vector(Access a,std::uint32_t,__m128 v){if(a==Access::motion_row)this->s.xmm[0]=v;}   // MOVAPS XMM0
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing() {                                                                                     // PUSH EBP; MOV EBP,ESP; AND ESP,-16; PUSH -1; PUSH handler
        auto &gp=this->gp;
        this->push(0x2f20f0u,gp[ebp]);gp[ebp]=gp[esp];gp[esp]=this->flags.logic(gp[esp]&0xfffffff0u,4);
        this->push(0x2f20f6u,logic::no_state);this->push(0x2f20f8u,logic::handler);
    }
    std::uint32_t registration(std::uint32_t link){this->push(0x2f2103u,link);return this->gp[esp];}  // PUSH EAX; MOV FS:[0],ESP
    void saving(unsigned n) {
        auto &gp=this->gp;
        if(n==0u) {                                                                                       // SUB ESP,98; PUSH EBX; PUSH ESI
            gp[esp]=this->flags.minus(gp[esp],logic::local::size,4);
            this->push(0x2f2111u,gp[ebx]);this->push(0x2f2112u,gp[esi]);depth=0u-4u;
        } else{this->push(0x2f2140u,gp[edi]);depth=0u;}                                                   // PUSH EDI
    }
    void wrapper_pushed(){put(0x2f214fu,0u);this->gp[ecx]=local(logic::local::wrapper);}               // PUSH 0; LEA ECX,[ESP+1C]
    void constructed(Kept &kept) {                                                                       // CALL 2E8FC0
        auto child=[&](State &v){dispatch(v,0x2e8fc0u,0x2f2161u);};
        this->call(0x2f2161u,0x2f2166u,0x2e8fc0u,child);depth-=4u;
        refresh(kept);
    }
    std::uint32_t motion_address(){return this->gp[eax]=local(logic::local::motion);}                  // LEA EAX,[ESP+40]
    std::uint32_t typed(std::uint32_t target,Kept &kept) {                                              // CALL [EDX+14]
        auto child=[&](State &v){dispatch(v,target,0x2f2184u);};
        this->call(0x2f2184u,0x2f2187u,target,child);
        refresh(kept);
        return this->gp[eax];
    }
    void counted(std::uint32_t count) {                                                                 // XOR ESI,ESI; TEST EAX,EAX
        this->gp[esi]=this->flags.logic(0u,4);this->test32(count);
    }
    std::uint32_t children(std::uint32_t list_address) {                                                // ADD EDI,10
        looping=true;
        return this->gp[edi]=this->flags.plus(list_address,logic::list::children,4);
    }
    void translated(std::uint32_t motion_now){this->gp[eax]=this->flags.plus(motion_now,logic::motion::translation,4);}  // ADD EAX,50
    void placed(const logic::Placed &p) {                                                                // the shuffles and arithmetic's registers
        auto &x=this->s.xmm;
        x[0]=p.translation;x[1]=p.turned;x[2]=p.z_term;x[3]=p.y_term;
    }
    std::uint32_t wrapper_address() {                                                                    // LEA EDX,[ESP+18]; LEA ECX,[ESP+18]
        auto &gp=this->gp;
        gp[edx]=gp[ecx]=local(logic::local::wrapper);
        return gp[edx];
    }
    std::uint32_t wrapper_type(Kept &kept) {                                                             // CALL 2E8CF0
        auto child=[&](State &v){dispatch(v,0x2e8cf0u,0x2f2205u);};
        this->call(0x2f2205u,0x2f220au,0x2e8cf0u,child);
        refresh(kept);
        return this->gp[eax];
    }
    std::uint32_t agent_slot(std::uint32_t type,std::uint32_t other_type) {                             // SHL EAX,5; ADD EAX,EDX
        auto &fl=this->flags;
        return this->gp[eax]=fl.plus(fl.shift(type,logic::dispatcher::type_bits,4),other_type,4);
    }
    void agent(std::uint32_t target,std::uint32_t fourth,std::uint32_t other,Kept &kept) {             // PUSH EDX, EBX, ECX; LEA EDX,[ESP+3C]; PUSH EDX; CALL EAX
        auto &gp=this->gp;
        put(0x2f2222u,fourth);put(0x2f2223u,kept.dispatcher);put(0x2f2224u,other);
        gp[edx]=local(logic::local::body);put(0x2f2229u,gp[edx]);
        auto child=[&](State &v){dispatch(v,target,0x2f222au);};
        this->call(0x2f222au,0x2f222cu,target,child);
        refresh(kept);
    }
    void advanced(Kept &kept,std::uint32_t count) {                                                     // ADD ESP,10; INC ESI; ADD EDI,10; CMP ESI,ECX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.plus(gp[esp],0x10u,4);depth-=0x10u;
        kept.index=gp[esi]=fl.increment(kept.index,4,false);
        kept.child=gp[edi]=fl.plus(kept.child,logic::child::size,4);
        this->compare32(kept.index,count);
    }
    void flag_tested(std::uint32_t flags_value){this->flags.logic((flags_value>>8)&0xffu,1);}          // TEST AH,AH
    std::uint32_t anded(std::uint32_t v,std::uint32_t mask){return this->flags.logic(v&mask&0xffu,1);}  // AND byte [ESP+1D],7F
    void destroyed(Kept &kept) {                                                                         // PUSH 0; LEA ECX,[ESP+1C]; CALL 2E12A0
        put(0x2f2262u,0u);this->gp[ecx]=local(logic::local::wrapper);
        auto child=[&](State &v){dispatch(v,0x2e12a0u,0x2f2268u);};
        this->call(0x2f2268u,0x2f226du,0x2e12a0u,child);depth-=4u;
        refresh(kept);
        this->reached(Branch::owned);
    }
    void restoring(unsigned n) {
        auto &gp=this->gp;
        if(n==0u){gp[edi]=this->pop(0x2f2274u);gp[esi]=this->pop(0x2f2275u);}                          // POP EDI; POP ESI
        else{gp[ebx]=this->pop(0x2f227du);gp[esp]=gp[ebp];gp[ebp]=this->pop(0x2f2280u);}                // POP EBX; MOV ESP,EBP; POP EBP
    }
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::child_agents::collide(memory,*this,Kept{gp[ebx],gp[esi],gp[esi],gp[edi],gp[edi]});
        this->reached(unsigned(outcome));
        this->ret(0x2f2281u,0u);this->publish(0x2f2281u);
    }
};
template<class State,class Reference,class Dispatch> bool bridge(State &s,Reference reference,Dispatch dispatch) {
    return run_owner(s,0x2f20f0u,0x2f2281u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Dispatch>(v,c,t,dispatch);});
}
} // namespace halo2::native_child_agents_detail
