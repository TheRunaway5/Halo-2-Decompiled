#pragma once
// Boundary adapter for the node hierarchy transform, entered at 170730
// (native_tu15_node_hierarchy_systems.hpp). Its callers and children keep
// the original ABI, which the adapter owns:
//   - [ESP+4..+14] the five arguments; RET 14; EBX/ESI saved first, EBP/EDI
//     once there are children; arguments are addressed from the live ESP,
//     so a child that leaves the stack unbalanced moves them as it does for
//     the original;
//   - 145560 gets ECX the parent transform, EDX the child's matrix (also
//     pushed), EDI the child's node record; the recursion gets EBX, ECX as
//     145560 left it, the child, the limit and the matrices; every register
//     a child returns comes back to the walk;
//   - flags: the TEST/CMP that decided the last branch;
//   - the cursor: each access publishes its original instruction.
// HALO2_REFERENCE_NODE_HIERARCHY=1 restores the translated body.
// HALO2_VERIFY_NODE_HIERARCHY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_node_hierarchy_systems.hpp"

namespace halo2::native_node_hierarchy_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_node_hierarchy;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_NODE_HIERARCHY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_NODE_HIERARCHY");return p && *p;}();
    return value;
}
enum Branch : unsigned {
    leaf, has_children, past_count, within_count, past_limit, within_limit, no_grandchildren, grandchildren,
    next_child, last_child, branch_count
};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Node hierarchy verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}

inline constexpr Edge hierarchy_edges[]{
    {0x170743u,0x170798u,leaf},{0x170743u,0x17074cu,has_children},
    {0x170752u,0x170796u,past_count},{0x170752u,0x170757u,within_count},
    {0x170757u,0x170796u,past_limit},{0x170757u,0x17075du,within_limit},
    {0x170779u,0x17078eu,no_grandchildren},{0x170779u,0x170780u,grandchildren},
    {0x17078eu,0x170752u,next_child},{0x17078eu,0x170796u,last_child},
};
template<class State,class Combine,class Descend> struct Hierarchy : Port<State> {
    using Access=logic::node_hierarchy::Access;
    using Local=logic::node_hierarchy::Local;
    using Kept=logic::node_hierarchy::Kept;
    Combine &combine_child;Descend &descend_child;
    // E: where the return address sits; the walk's ESP is E-10 between the
    // saves and the restores.
    std::uint32_t frame{};
    Hierarchy(State &s,std::uint32_t *c,Trace<State> *t,Combine &m,Descend &d)
        :Port<State>(s,c,t,hierarchy_edges),combine_child(m),descend_child(d){}
    std::uint32_t local(Local l)const{return frame+4u+4u*std::uint32_t(l);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case node_argument:return 0x170730u;
        case model_argument:this->push(0x170734u,gp[ebx]);return 0x170735u;
        case nodes_table:return 0x170739u;
        // LEA EAX,[EAX+EAX*2]; SHL EAX,5; PUSH ESI.
        case first_child:
            gp[eax]=this->flags.shift(gp[eax]*3u,5u,4);this->push(0x170742u,gp[esi]);
            return 0x170743u;
        case matrices_argument:this->push(0x17074cu,gp[ebp]);return 0x17074du;
        case node_count:return 0x170752u;
        case limit_argument:return 0x170757u;
        case nodes_reload:return 0x17075du;
        case parent_argument:return 0x170760u;
        case grandchild:return 0x170779u;
        case limit_for_children:return 0x170780u;
        case next_sibling:return 0x17078eu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case node_argument:case limit_for_children:gp[eax]=v;break;
        case model_argument:gp[ebx]=v;break;
        case nodes_table:case parent_argument:gp[ecx]=v;break;
        case first_child:case next_sibling:gp[esi]=logic::widen16(v);this->test32(gp[esi]);break;
        case matrices_argument:gp[ebp]=v;this->push(0x170751u,gp[edi]);break;        // PUSH EDI
        case node_count:case limit_argument:this->compare32(gp[esi],v);break;
        case nodes_reload:gp[edx]=v;break;
        case grandchild:this->flags.minus(v,0xffffffffu,2);break;                     // CMP WORD,-1
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void returned(Kept &kept) {
        frame=this->gp[esp]+0x10u;
        kept={this->gp[ebx],this->gp[ebp],this->gp[edi],this->gp[esi]};
    }
    // LEA EDI,[ESI+ESI*2]; SHL EDI,5; ADD EDI,EDX; MOV EDX,ESI; IMUL EDX,34;
    // ADD EDX,EBP; PUSH EDX; CALL 145560.
    void combine(Kept &kept) {
        auto &gp=this->gp;
        gp[edi]=this->flags.plus(this->flags.shift(gp[esi]*3u,5u,4),gp[edx],4);
        gp[edx]=this->flags.plus(this->flags.product(gp[esi],logic::matrix_size),gp[ebp],4);
        this->push(0x170773u,gp[edx]);
        this->call(0x170774u,0x170779u,0x145560u,combine_child);returned(kept);
    }
    // PUSH EBP, EAX (the limit), ESI, ECX, EBX; CALL 170730.
    void descend(Kept &kept) {
        auto &gp=this->gp;
        this->push(0x170784u,gp[ebp]);this->push(0x170785u,gp[eax]);this->push(0x170786u,gp[esi]);
        this->push(0x170787u,gp[ecx]);this->push(0x170788u,gp[ebx]);
        this->call(0x170789u,0x17078eu,0x170730u,descend_child);returned(kept);
    }
    void run() {
        auto &gp=this->gp;
        frame=gp[esp];
        Memory<Hierarchy> memory{*this};
        if(logic::node_hierarchy::propagate(memory,*this)==logic::node_hierarchy::Exit::walked) {
            gp[edi]=this->pop(0x170796u);gp[ebp]=this->pop(0x170797u);
        }
        gp[esi]=this->pop(0x170798u);gp[ebx]=this->pop(0x170799u);
        this->ret(0x17079au,0x14u);this->publish(0x17079au);
    }
};

template<class State,class Reference,class Combine,class Descend>
bool bridge(State &s,Reference reference,Combine combine,Descend descend) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x170730u,0x17079au,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Hierarchy<State,Combine,Descend>(v,c,t,combine,descend);});
    return true;
}
} // namespace halo2::native_node_hierarchy_detail
