#pragma once
// Boundary adapter for the node matrix gather, entered at 16B240
// (native_tu15_node_gather_systems.hpp). Its callers and the child 145560
// keep the original ABI, which the adapter owns:
//   - EAX the model tag; [EBP+8..+18] the five arguments; RET 14;
//   - the frame: PUSH EBP; MOV EBP,ESP; PUSH ECX (the position); PUSH EBX,
//     and ESI/EDI once there are nodes; the frame is addressed through the
//     live EBP, as the original addresses it (a child that returns another
//     EBP moves it), and the exit is MOV ESP,EBP; POP EBP;
//   - 145560 gets ECX the transform, EDX the source matrix, EAX the node
//     map and the destination pushed; every register it returns comes back
//     (EBX is the node count);
//   - the copy: REP MOVSD with ECX 13, ESI/EDI stepping by the direction
//     flag, leaving ECX zero and ESI/EDI past the matrix;
//   - flags: CMP EDX,-1, TEST ECX,ECX, IMUL/ADD of the source, INC/ADD of
//     the frame words and the CMP that ends the loop, as the original;
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_NODE_GATHER=1 restores the translated body.
// HALO2_VERIFY_NODE_GATHER=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_node_gather_systems.hpp"

namespace halo2::native_node_gather_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_node_gather;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_NODE_GATHER");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_NODE_GATHER");return p && *p;}();
    return value;
}
enum Branch : unsigned {empty, nodes, skipped, mapped, transformed, copied, more_nodes, last_node, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Node gather verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

inline constexpr Edge gather_edges[]{
    {0x16b261u,0x16b2b4u,empty},{0x16b261u,0x16b269u,nodes},
    {0x16b277u,0x16b2a6u,skipped},{0x16b277u,0x16b27fu,mapped},
    {0x16b27fu,0x16b286u,transformed},{0x16b27fu,0x16b296u,copied},
    {0x16b2adu,0x16b271u,more_nodes},{0x16b2adu,0x16b2b2u,last_node},
};
template<class State,class Transform> struct Gather : Port<State> {
    using Access=logic::node_gather::Access;
    using Local=logic::node_gather::Local;
    Transform &transform_child;
    Gather(State &s,std::uint32_t *c,Trace<State> *t,Transform &x):Port<State>(s,c,t,gather_edges),transform_child(x){}
    std::uint32_t local(Local l)const {
        const auto frame=this->gp[ebp];
        switch(l) {
        case Local::position:return frame-4u;
        case Local::transform:return frame+0x8u;
        case Local::destination:return frame+0xcu;
        case Local::capacity:return frame+0x10u;
        case Local::sources:return frame+0x14u;
        case Local::node_map:return frame+0x18u;
        }
        return 0;
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // PUSH EBP; MOV EBP,ESP; PUSH ECX (the position).
        case tag_instances:
            this->push(0x16b240u,gp[ebp]);gp[ebp]=gp[esp];this->push(0x16b243u,gp[ecx]);
            return 0x16b244u;
        // AND EAX,FFFF; SHL EAX,4.
        case definition:gp[eax]=this->flags.shift(this->flags.logic(gp[eax]&0xffffu,4),4u,4);return 0x16b252u;
        case node_count:this->push(0x16b256u,gp[ebx]);return 0x16b257u;
        case capacity_compare:return 0x16b25au;
        case capacity_move:return 0x16b25du;
        case position_clear_read:case position_clear:return 0x16b261u;
        case destination_argument:return 0x16b269u;
        case destination_store:this->push(0x16b26cu,gp[esi]);return 0x16b26du;
        case node_map_argument:return 0x16b271u;
        case position_read:return 0x16b274u;
        case map_entry:return 0x16b277u;
        case transform_argument:return 0x16b27fu;
        case destination_push:return 0x16b286u;
        // IMUL EDX,EDX,34; ADD EDX,[sources].
        case sources_for_transform:gp[edx]=this->flags.product(gp[edx],logic::matrix_size);return 0x16b28cu;
        case destination_load:return 0x16b296u;
        // MOV ESI,EDX; IMUL ESI,ESI,34; ADD ESI,[sources].
        case sources_for_copy:gp[esi]=this->flags.product(gp[edx],logic::matrix_size);return 0x16b29eu;
        case copy_read:case copy_write:return 0x16b2a4u;
        case position_increment_read:case position_increment:return 0x16b2a6u;
        case destination_advance_read:case destination_advance:return 0x16b2a9u;
        case position_bound:return 0x16b2adu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case tag_instances:case position_read:case transform_argument:gp[ecx]=v;if(a==transform_argument)this->test32(v);break;
        case definition:case destination_argument:case node_map_argument:gp[eax]=v;break;
        case node_count:gp[ebx]=v;break;
        case capacity_compare:this->compare32(gp[ebx],v);break;
        case capacity_move:if(!this->flags.zf && this->flags.sf==this->flags.of)gp[ebx]=v;break;      // CMOVG
        case map_entry:gp[edx]=v;this->compare32(v,logic::none);break;
        case destination_push:this->push(0x16b286u,v);break;                                    // PUSH [EBP+10]
        case sources_for_transform:gp[edx]=this->flags.plus(gp[edx],v,4);break;
        case destination_load:gp[edi]=v;break;
        case sources_for_copy:gp[esi]=this->flags.plus(gp[esi],v,4);break;
        case position_bound:this->compare32(v,gp[ebx]);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case position_clear:this->flags.logic(0u,4);this->test32(gp[ebx]);break;             // AND ...,0; TEST EBX,EBX
        case destination_store:this->push(0x16b270u,gp[edi]);break;
        // Each MOVSD step: ESI/EDI by the direction flag; the count down.
        case copy_write: {
            const auto step=copy_step();
            gp[esi]+=step;gp[edi]+=step;--gp[ecx];
            break;
        }
        default:break;
        }
    }
    std::uint32_t copy_step()const{return (this->s.other_flags&0x400u)?0xfffffffcu:4u;}
    void copy_started(){this->push(0x16b2a1u,logic::matrix_words);this->gp[ecx]=this->pop(0x16b2a3u);}   // PUSH D; POP ECX
    // CALL 145560 (through the unit's route).
    std::uint32_t transform(std::uint32_t) {
        this->call_routed(transform_child,0x16b28fu,0x16b294u,0x145560u);
        return this->gp[ebx];
    }
    void run() {
        auto &gp=this->gp;
        Memory<Gather> memory{*this};
        if(logic::node_gather::gather(memory,*this,gp[eax])==logic::node_gather::Exit::gathered) {
            gp[edi]=this->pop(0x16b2b2u);gp[esi]=this->pop(0x16b2b3u);
        }
        gp[ebx]=this->pop(0x16b2b4u);
        // MOV ESP,EBP; POP EBP; RET 14.
        gp[esp]=gp[ebp];gp[ebp]=this->pop(0x16b2b5u);
        this->ret(0x16b2b6u,0x14u);this->publish(0x16b2b6u);
    }
};

template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x16b240u,0x16b2b6u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Gather<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_node_gather_detail
