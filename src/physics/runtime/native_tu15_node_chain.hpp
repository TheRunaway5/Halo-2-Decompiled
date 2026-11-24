#pragma once
// Boundary adapter for the node chain, entered at 1E1390
// (native_tu15_node_chain_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns:
//   - EAX the model tag, EBX the node matrices, ESI the node; four stack
//     arguments, RET 10; SUB ESP,A4 makes the frame (the parent and the
//     grandparent at [ESP+4] and [ESP+8] once EBP is saved; the goal, the
//     node's matrix and the result at [ESP+10], [ESP+44] and [ESP+78] once
//     EDI is too); the arguments and locals are addressed from the live
//     ESP, so a child that leaves the stack unbalanced moves them as it does
//     for the original;
//   - 144090 (through the unit's route) gets ECX the source and EAX the
//     destination; 145560 gets ECX, EDX and one stack argument; 122BE0 EAX
//     and ECX and two; 170730 five; every register a child returns comes
//     back;
//   - the x87 test runs on the logical x87 as the original's FLD m32, FSUB
//     m32, FABS, FLD m32, FCOMIP and FSTP ST0 do (two free registers are
//     required at admission); the blend leaves XMM0-XMM3 as the original;
//   - the flags of the last instruction that sets them (ADD ESP,A4 at the
//     exit);
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_NODE_CHAIN=1 restores the translated body.
// HALO2_VERIFY_NODE_CHAIN=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_node_chain_systems.hpp"
#include <bit>
#include <cmath>
#include <emmintrin.h>

namespace halo2::native_node_chain_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_node_chain;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_NODE_CHAIN");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_NODE_CHAIN");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_parent, no_grandparent, chained, full_weight, partial_weight, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Node chain verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge chain_edges[]{
    {0x1e13b2u,0x1e14e4u,no_parent},{0x1e13d7u,0x1e14e4u,no_grandparent},{0x1e13d7u,0x1e13e1u,chained},
    {0x1e1425u,0x1e149du,full_weight},{0x1e1425u,0x1e1431u,partial_weight},
};
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
// FLD m32, as the translated route converts it.
inline long double extended(std::uint32_t bits){return physics_step_detail::ext(physics_step_detail::f(bits));}
template<class State,class Route,class Transform,class Solve,class Store> struct Chain : Port<State> {
    using Access=logic::node_chain::Access;
    using Local=logic::node_chain::Local;
    using Kept=logic::node_chain::Kept;
    Route &route;Transform &transform_child;Solve &solve_child;Store &store_child;
    // The bytes below the return address: it sits at ESP+depth.
    std::uint32_t depth{};
    Chain(State &s,std::uint32_t *c,Trace<State> *t,Route &r,Transform &m,Solve &v,Store &p)
        :Port<State>(s,c,t,chain_edges),route(r),transform_child(m),solve_child(v),store_child(p){}
    std::uint32_t local(Local l)const {
        const auto entry=this->gp[esp]+depth;
        switch(l) {
        case Local::target:return entry+4u;
        case Local::offset:return entry+8u;
        case Local::weight:return entry+0xcu;
        case Local::poses:return entry+0x10u;
        case Local::parent:return entry-0xa4u;
        case Local::grandparent:return entry-0xa0u;
        case Local::goal:return entry-0x9cu;
        case Local::node_matrix:return entry-0x68u;
        case Local::result:return entry-0x34u;
        }
        return 0;
    }
    void save(std::uint32_t pc,std::uint32_t value){this->push(pc,value);depth+=4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case instances:return 0x1e1390u;
        // AND EAX,FFFF; SUB ESP,A4; SHL EAX,4; PUSH EBP.
        case tag:
            gp[eax]=this->flags.logic(gp[eax]&0xffffu,4);gp[esp]=this->flags.minus(gp[esp],0xa4u,4);depth=0xa4u;
            gp[eax]=this->flags.shift(gp[eax],logic::instance_shift,4);save(0x1e13a4u,gp[ebp]);return 0x1e13a5u;
        case nodes:return 0x1e13a9u;
        // LEA EDX,[ESI+ESI*2]; SHL EDX,5.
        case parent:gp[edx]=this->flags.shift(gp[esi]*3u,5u,4);return 0x1e13b2u;
        case parent_store:gp[ecx]=logic::widen16(gp[ecx]);return 0x1e13c4u;                     // MOVSX ECX,CX
        case grandparent:return 0x1e13ceu;
        case grandparent_store:return 0x1e13d7u;
        case target_argument:return 0x1e13e1u;
        case offset_argument:return 0x1e13f2u;
        case weight_x87:return 0x1e1416u;
        case one_x87:return 0x1e141du;
        case full_x87:return 0x1e1425u;
        case weight:return 0x1e1431u;
        case one:return 0x1e143au;
        case goal_x:return 0x1e1442u;
        case node_x:return 0x1e1448u;
        case goal_y:return 0x1e145du;
        case goal_x_store:return 0x1e1463u;
        case node_y:return 0x1e1469u;
        case goal_y_store:return 0x1e147au;
        case node_z:return 0x1e1480u;
        case goal_z:return 0x1e1489u;
        case goal_z_store:return 0x1e1497u;
        case grandparent_again:return 0x1e149du;
        case parent_again:return 0x1e14a2u;
        case poses_argument:return 0x1e14cbu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        auto &fp=this->s.fp;
        using enum Access;
        switch(a) {
        case instances:gp[ecx]=v;break;
        case tag:gp[ebp]=v;break;
        case nodes:gp[eax]=v;break;
        // MOV CX,[EDX+EAX+4]; CMP CX,-1.
        case parent:gp[ecx]=(gp[ecx]&0xffff0000u)|(v&0xffffu);this->flags.minus(v&0xffffu,logic::no_node,2);break;
        // MOVSX EAX,word; CMP AX,FFFF.
        case grandparent:gp[eax]=logic::widen16(v);this->flags.minus(v&0xffffu,logic::no_node,2);break;
        case target_argument:gp[ecx]=v;save(0x1e13e8u,gp[edi]);break;                              // PUSH EDI
        case offset_argument:gp[ecx]=v;break;
        // FLD [ESP+B8]; FSUB [46A0D0]; FABS; FLD [46A0EC].
        case weight_x87:fp.push(extended(v));break;
        case one_x87:fp.st(0)=physics_step_detail::sub(fp.st(0),extended(v));fp.st(0)=std::fabs(fp.st(0));break;
        case full_x87:fp.push(extended(v));break;
        // MOVSX ECX,word [ESP+C]; MOV EAX,[ESP+8]; IMUL ECX,ECX,34; IMUL EAX,EAX,34.
        case grandparent_again:gp[ecx]=logic::widen16(v);break;
        case parent_again:
            gp[eax]=v;gp[ecx]=this->flags.product(gp[ecx],logic::matrix_size);gp[eax]=this->flags.product(gp[eax],logic::matrix_size);
            break;
        case poses_argument:gp[ecx]=v;break;
        default:break;
        }
    }
    // LEA ECX,[ECX+ECX*2]; SHL ECX,5.
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        if(a==Access::parent_store)gp[ecx]=this->flags.shift(gp[ecx]*3u,5u,4);
    }
    void returned(Kept &kept)const{kept={this->gp[ebx],this->gp[esi],this->gp[ebp],this->gp[edi]};}
    // LEA EAX,[ESP+10]; CALL 144090.
    void copy_target(Kept &kept) {
        this->gp[eax]=local(Local::goal);this->call_routed(route,0x1e13edu,0x1e13f2u,0x144090u);returned(kept);
    }
    // LEA EDX,[ESP+10]; PUSH EDX; CALL 145560.
    void transform_goal(Kept &kept) {
        auto &gp=this->gp;
        gp[edx]=local(Local::goal);save(0x1e13fdu,gp[edx]);
        this->call(0x1e13feu,0x1e1403u,0x145560u,transform_child);depth-=4u;returned(kept);
    }
    // MOV EAX,ESI; IMUL EAX,EAX,34; LEA EDI,[EAX+EBX]; LEA EAX,[ESP+44]; MOV ECX,EDI; CALL 144090.
    void copy_node(Kept &kept) {
        auto &gp=this->gp;
        gp[eax]=this->flags.product(gp[esi],logic::matrix_size);gp[edi]=gp[eax]+gp[ebx];gp[eax]=local(Local::node_matrix);
        gp[ecx]=gp[edi];this->call_routed(route,0x1e1411u,0x1e1416u,0x144090u);returned(kept);
    }
    // FCOMIP ST,ST1; FSTP ST0; JA: the full weight above |weight - 1|.
    bool weight_full() {
        auto &fp=this->s.fp;
        this->s.flags=this->flags;this->s.compare_and_pop(fp.st(1));this->flags=this->s.flags;
        fp.st(0)=fp.st(0);fp.pop();
        return !this->flags.cf && !this->flags.zf;
    }
    void blended(const logic::node_chain::Blend &b) {
        auto &x=this->s.xmm;
        x[0]=lane0_only(b.xmm0);x[1]=lane0_only(b.xmm1);x[2]=lane0_only(b.xmm2);x[3]=lane0_only(b.xmm3);
    }
    // PUSH EDI; LEA EDX,[ESP+14]; ADD EAX,EBX; ADD ECX,EBX; PUSH EDX; CALL 122BE0.
    void solve(Kept &kept) {
        auto &gp=this->gp;
        save(0x1e14acu,gp[edi]);gp[edx]=local(Local::goal);
        gp[eax]=this->flags.plus(gp[eax],gp[ebx],4);gp[ecx]=this->flags.plus(gp[ecx],gp[ebx],4);save(0x1e14b5u,gp[edx]);
        this->call(0x1e14b6u,0x1e14bbu,0x122be0u,solve_child);depth-=8u;returned(kept);
    }
    // LEA EAX,[ESP+78]; PUSH EAX; LEA EDX,[ESP+48]; MOV ECX,EDI; CALL 145560.
    void compose(Kept &kept) {
        auto &gp=this->gp;
        gp[eax]=local(Local::result);save(0x1e14bfu,gp[eax]);gp[edx]=local(Local::node_matrix);gp[ecx]=gp[edi];
        this->call(0x1e14c6u,0x1e14cbu,0x145560u,transform_child);depth-=4u;returned(kept);
    }
    // PUSH EBX; PUSH ECX; PUSH ESI; LEA EDX,[ESP+84]; PUSH EDX; PUSH EBP; CALL 170730.
    void store_poses(Kept &kept) {
        auto &gp=this->gp;
        save(0x1e14d2u,gp[ebx]);save(0x1e14d3u,gp[ecx]);save(0x1e14d4u,gp[esi]);gp[edx]=local(Local::result);
        save(0x1e14dcu,gp[edx]);save(0x1e14ddu,gp[ebp]);
        this->call(0x1e14deu,0x1e14e3u,0x170730u,store_child);depth-=0x14u;returned(kept);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Chain> memory{*this};
        const logic::node_chain::Kept entry{gp[ebx],gp[esi],gp[ebp],gp[edi]};
        if(logic::node_chain::pose(memory,*this,gp[eax],entry)==logic::node_chain::Exit::posed)gp[edi]=this->pop(0x1e14e3u);
        gp[ebp]=this->pop(0x1e14e4u);gp[esp]=this->flags.plus(gp[esp],0xa4u,4);
        this->ret(0x1e14ebu,0x10u);this->publish(0x1e14ebu);
    }
};
template<class State,class Reference,class Route,class Transform,class Solve,class Store>
bool bridge(State &s,Reference reference,Route route,Transform transform,Solve solve,Store store) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.fp.count>6u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1e1390u,0x1e14ebu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            return Chain<State,Route,Transform,Solve,Store>(v,c,t,route,transform,solve,store);});
    return true;
}
} // namespace halo2::native_node_chain_detail
