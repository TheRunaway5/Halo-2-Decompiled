#pragma once
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "physics/runtime/native_tu15_live_support_systems.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"

namespace halo2::native_two_bone_solve {
using namespace native_object_tick_detail::boundary;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

constexpr std::uint32_t entry=0x122be0u, exit=0x123146u;
enum Stack : std::uint32_t {
    target_length=0x0cu, first_length=0x10u, second_length=0x14u,
    remaining_offset=0x18u, y_offset=0x1cu, z_offset=0x20u,
    translation_address=0x24u, target_x=0x28u, target_y=0x2cu, target_z=0x30u,
    end_x=0x34u, end_y=0x38u, end_z=0x3cu,
    saved_goal=0x44u, destination=0x48u
};
constexpr std::uint32_t lateral_axis=0x10u, vertical_axis=0x1cu, translation=0x28u;
constexpr std::uint32_t minimum_length_squared=0x46a0ecu;

template<class State,class Commit>
void store_and_commit(State &s,std::uint32_t pc,std::uint32_t address,std::uint32_t value,Commit commit) {
    if(s.observe) {
        const SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &event) {
            commit();
            s.observe(event);
        };
        physics_step_detail::w(s.memory,observe,pc,address,4,value);
    } else {
        physics_step_detail::w(s.memory,{},pc,address,4,value);
        commit();
    }
}
template<class State>
void push_word(State &s,std::uint32_t pc,std::uint32_t value) {
    const auto address=s.gp[esp]-4u;
    store_and_commit(s,pc,address,value,[&]{s.gp[esp]=address;});
}
template<class State>
void copy_matrix_word(State &s) {
    const auto value=physics_step_detail::r(s.memory,s.gp[esi],4);
    store_and_commit(s,0x12313du,s.gp[edi],value,[&] {
        const auto stride=(s.other_flags&0x400u)?0xfffffffcu:4u;
        s.gp[esi]+=stride;
        s.gp[edi]+=stride;
        --s.gp[ecx];
    });
}

namespace x87 {
inline long double load(std::uint32_t bits) {
    return native_live_support::x87::load_single(bits);
}
template<class State> void square_root(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;
    std::uint16_t status;
    auto &top=s.fp.st(0);
    asm volatile("fldt %2\n\tfsqrt\n\tfnstsw %1\n\tfstpt %0"
        :"=m"(result),"=m"(status):"m"(top):"st");
    top=result;
    s.fp.status=(s.fp.status&~0x200u)|(status&0x200u);
#else
    s.fp.st(0)=physics_solver_detail::root(s.fp.st(0));
#endif
}
template<class State> void store_pop(State &s,std::uint32_t pc,std::uint32_t address) {
    using namespace physics_contact_runtime_detail;
    const auto initial=source_x87_exception_flags();
    const auto rounded=native_live_support::x87::store_single(s.fp.st(0));
    const auto produced=source_x87_exception_flags();
    if(produced!=initial)source_load_x87_exception_flags(initial);
    const auto commit=[&] {
        if(produced!=initial)source_load_x87_exception_flags(produced|source_x87_exception_flags());
        s.fp.status=(s.fp.status&~0x200u)|(rounded.rounded_up?0x200u:0u);
        s.fp.pop();
    };
    store_and_commit(s,pc,address,std::uint32_t(rounded.bits),commit);
}
}

// The solver owns caller-visible GP and vector state even at a rejected read
// or store. Only a sealed quiet cursor can skip delivery between accesses.
template<class State,bool Observed> struct Frame {
    State &s;
    std::uint32_t *cursor;
    decltype(State::gp) &g;
    decltype(State::xmm) &x;
    decltype(State::flags) &flags;
    bool reopened{};

    Frame(State &state,std::uint32_t *position):s(state),cursor(position),g(s.gp),x(s.xmm),flags(s.flags){}

    void pc(std::uint32_t site) {
        if constexpr(Observed)s.instruction_literal(site);
        else if(reopened)s.instruction_literal(site);
    }
    void after_access() {
        if constexpr(!Observed) {
            if(!cursor)return;
            const auto *policy=s.immutable_cursor.cursor;
            if((policy->first_trace && *policy->first_trace) || (policy->second_trace && *policy->second_trace)) {
                cursor=nullptr;
                reopened=true;
            }
        }
    }
    void access(std::uint32_t site) {
        pc(site);
        if(cursor)*cursor=site;
    }
    std::uint32_t local(Stack offset)const{return g[esp]+offset;}
    std::uint32_t word(std::uint32_t site,std::uint32_t address) {
        access(site);
        const auto value=physics_step_detail::r(s.memory,address,4);
        after_access();
        return value;
    }
    __m128 scalar(std::uint32_t site,std::uint32_t address) {
        return _mm_set_ss(native_tu15_leaves::as_float(word(site,address)));
    }
    void store(std::uint32_t site,std::uint32_t address,std::uint32_t value) {
        access(site);
        physics_step_detail::w(s.memory,s.observe,site,address,4,value);
        after_access();
    }
    void store_lane(std::uint32_t site,std::uint32_t address,unsigned lane) {
        store(site,address,native_tu15_leaves::low_bits(x[lane]));
    }
    void push(std::uint32_t site,std::uint32_t value) {
        access(site);
        push_word(s,site,value);
        after_access();
    }
    std::uint32_t pop(std::uint32_t site) {
        const auto value=word(site,g[esp]);
        g[esp]+=4u;
        return value;
    }
    template<class Normalize> void normalize(std::uint32_t site,std::uint32_t next,Normalize &child) {
        push(site,next);
        child(s);
        after_access();
    }
    void finish() {
        g[edi]=pop(0x123140u);
        g[esi]=pop(0x123141u);
        g[ebx]=pop(0x123142u);
        pc(0x123143u);g[esp]=flags.plus(g[esp],0x30u,4);
        (void)pop(exit);
        g[esp]+=8u;
    }
    void load_length(std::uint32_t site,Stack slot) {s.fp.push(x87::load(word(site,local(slot))));}
    void root_length(std::uint32_t site) {pc(site);x87::square_root(s);}
    void spill_length(std::uint32_t site,Stack slot) {
        access(site);
        x87::store_pop(s,site,local(slot));
        after_access();
    }
    void root_length(std::uint32_t load_site,std::uint32_t root_site,std::uint32_t spill_site,
                     Stack from,Stack to) {
        load_length(load_site,from);
        root_length(root_site);
        spill_length(spill_site,to);
    }

    void build_plane() {
        load_length(0x122cd5u,second_length);
        x[3]=scalar(0x122cd9u,0x46a0d0u);
        root_length(0x122ce1u);
        x[0]=scalar(0x122ce3u,g[edi]+translation);
        x[0]=_mm_sub_ss(x[0],scalar(0x122ce8u,g[ebx]+translation));
        x[1]=scalar(0x122cedu,g[edi]+translation+4u);
        x[1]=_mm_sub_ss(x[1],scalar(0x122cf2u,g[ebx]+translation+4u));
        x[2]=scalar(0x122cf7u,g[edi]+translation+8u);
        x[2]=_mm_sub_ss(x[2],scalar(0x122cfcu,g[ebx]+translation+8u));
        x[5]=scalar(0x122d01u,g[esi]+translation+4u);
        x[5]=_mm_sub_ss(x[5],scalar(0x122d06u,g[ebx]+translation+4u));
        x[6]=scalar(0x122d0bu,g[esi]+translation+8u);
        x[6]=_mm_sub_ss(x[6],scalar(0x122d10u,g[ebx]+translation+8u));
        x[4]=scalar(0x122d15u,g[eax]);
        x[4]=_mm_sub_ss(x[4],scalar(0x122d19u,g[ebx]+translation));
        pc(0x122d1eu);g[eax]=local(target_z);
        spill_length(0x122d22u,first_length);
        root_length(0x122d26u,0x122d2au,0x122d2cu,target_length,second_length);
        root_length(0x122d30u,0x122d34u,0x122d36u,remaining_offset,target_length);

        x[3]=_mm_div_ss(x[3],scalar(0x122d3au,local(target_length)));
        pc(0x122d40u);x[2]=ordered_mul_ss(x[2],x[3]);
        pc(0x122d44u);x[0]=ordered_mul_ss(x[0],x[3]);
        pc(0x122d48u);x[1]=ordered_mul_ss(x[1],x[3]);
        pc(0x122d4cu);x[7]=x[2];
        pc(0x122d4fu);x[3]=x[1];
        pc(0x122d52u);x[3]=ordered_mul_ss(x[3],x[6]);
        pc(0x122d56u);x[7]=ordered_mul_ss(x[7],x[5]);
        pc(0x122d5au);x[3]=_mm_sub_ss(x[3],x[7]);
        store_lane(0x122d5eu,local(target_z),3);
        pc(0x122d64u);x[3]=x[0];
        store_lane(0x122d67u,local(translation_address),0);
        store_lane(0x122d6du,local(target_y),2);
        store_lane(0x122d73u,local(target_x),1);
        pc(0x122d79u);x[2]=ordered_mul_ss(x[2],x[4]);
        pc(0x122d7du);x[3]=ordered_mul_ss(x[3],x[6]);
        pc(0x122d81u);x[0]=ordered_mul_ss(x[0],x[5]);
        pc(0x122d85u);x[1]=ordered_mul_ss(x[1],x[4]);
        pc(0x122d89u);x[2]=_mm_sub_ss(x[2],x[3]);
        pc(0x122d8du);x[0]=_mm_sub_ss(x[0],x[1]);
        store_lane(0x122d91u,local(end_x),2);
        store_lane(0x122d97u,local(end_y),0);
    }
    void limit_target_reach() {
        x[4]=scalar(0x122da2u,local(end_x));
        x[0]=scalar(0x122da8u,local(end_y));
        x[7]=scalar(0x122daeu,local(target_y));
        x[6]=scalar(0x122db4u,local(target_x));
        x[5]=scalar(0x122dbau,local(translation_address));
        pc(0x122dc0u);x[2]=x[0];
        pc(0x122dc3u);x[1]=x[4];
        pc(0x122dc6u);x[2]=ordered_mul_ss(x[2],x[6]);
        pc(0x122dcau);x[1]=ordered_mul_ss(x[1],x[7]);
        pc(0x122dceu);x[1]=_mm_sub_ss(x[1],x[2]);
        x[2]=scalar(0x122dd2u,local(target_z));
        pc(0x122dd8u);x[0]=ordered_mul_ss(x[0],x[5]);
        pc(0x122ddcu);x[3]=x[2];
        pc(0x122ddfu);x[3]=ordered_mul_ss(x[3],x[7]);
        pc(0x122de3u);x[0]=_mm_sub_ss(x[0],x[3]);
        pc(0x122de7u);x[3]=x[0];
        x[0]=scalar(0x122deau,local(second_length));
        x[0]=ordered_add_ss(x[0],scalar(0x122df0u,local(first_length)));
        x[0]=ordered_mul_ss(x[0],scalar(0x122df6u,0x46a1acu));
        pc(0x122dfeu);x[4]=ordered_mul_ss(x[4],x[5]);
        pc(0x122e02u);x[2]=ordered_mul_ss(x[2],x[6]);
        pc(0x122e06u);x[2]=_mm_sub_ss(x[2],x[4]);
        x[4]=scalar(0x122e0au,local(target_length));
        pc(0x122e10u);flags.scalar_compare(x[4],x[0],false);
        pc(0x122e13u);
        if(flags.cf||flags.zf)return;

        pc(0x122e15u);x[4]=x[0];
        pc(0x122e18u);x[4]=ordered_mul_ss(x[4],x[5]);
        x[4]=ordered_add_ss(x[4],scalar(0x122e1cu,g[ebx]+translation));
        store_lane(0x122e21u,g[edi]+translation,4);
        pc(0x122e26u);x[4]=x[0];
        pc(0x122e29u);x[4]=ordered_mul_ss(x[4],x[6]);
        x[4]=ordered_add_ss(x[4],scalar(0x122e2du,g[ebx]+translation+4u));
        store_lane(0x122e32u,g[edi]+translation+4u,4);
        pc(0x122e37u);x[4]=x[0];
        pc(0x122e3au);x[4]=ordered_mul_ss(x[4],x[7]);
        x[4]=ordered_add_ss(x[4],scalar(0x122e3eu,g[ebx]+translation+8u));
        store_lane(0x122e43u,g[edi]+translation+8u,4);
        store_lane(0x122e48u,local(target_length),0);
        pc(0x122e4eu);x[4]=x[0];
    }
    // Resolve the triangle in the plane defined by the links. The offset
    // magnitude crosses a single-precision x87 spill before scaling.
    void solve_plane_offset() {
        x[7]=scalar(0x122e51u,local(first_length));
        pc(0x122e57u);x[7]=ordered_mul_ss(x[7],x[7]);
        store_lane(0x122e5bu,local(remaining_offset),7);
        pc(0x122e61u);x[0]=x[4];
        pc(0x122e64u);x[0]=ordered_mul_ss(x[0],x[4]);
        x[4]=scalar(0x122e68u,local(second_length));
        pc(0x122e6eu);x[0]=ordered_add_ss(x[0],x[7]);
        pc(0x122e72u);x[7]=x[4];
        pc(0x122e75u);x[7]=ordered_mul_ss(x[7],x[4]);
        x[4]=scalar(0x122e79u,local(target_length));
        pc(0x122e7fu);x[0]=_mm_sub_ss(x[0],x[7]);
        pc(0x122e83u);x[7]=x[4];
        x[7]=ordered_mul_ss(x[7],scalar(0x122e86u,0x46a11cu));
        pc(0x122e8eu);x[0]=_mm_div_ss(x[0],x[7]);
        pc(0x122e92u);x[4]=_mm_sub_ss(x[4],x[0]);
        store_lane(0x122e96u,local(second_length),4);
        x[4]=scalar(0x122e9cu,local(remaining_offset));
        pc(0x122ea2u);x[7]=x[0];
        pc(0x122ea5u);x[7]=ordered_mul_ss(x[7],x[0]);
        pc(0x122ea9u);x[4]=_mm_sub_ss(x[4],x[7]);
        const auto threshold=scalar(0x122eadu,minimum_length_squared);
        flags.scalar_compare(x[4],threshold,false);
        store_lane(0x122eb4u,local(remaining_offset),4);
        pc(0x122ebau);
        if(flags.cf) {
            pc(0x122eceu);x[4]=_mm_setzero_ps();
            return;
        }
        root_length(0x122ebcu,0x122ec0u,0x122ec2u,remaining_offset,remaining_offset);
        x[4]=scalar(0x122ec6u,local(remaining_offset));
        pc(0x122eccu);
    }
    void joint_forward_axis() {
        pc(0x122ed1u);x[1]=ordered_mul_ss(x[1],x[4]);
        pc(0x122ed5u);x[7]=x[0];
        store_lane(0x122ed8u,local(remaining_offset),1);
        pc(0x122edeu);x[7]=ordered_mul_ss(x[7],x[5]);
        pc(0x122ee2u);x[7]=ordered_add_ss(x[7],x[1]);
        pc(0x122ee6u);x[1]=x[0];
        x[0]=ordered_mul_ss(x[0],scalar(0x122ee9u,local(target_y)));
        push(0x122eefu,g[ebp]);
        pc(0x122ef0u);g[ebp]=g[ebx]+4u;
        pc(0x122ef3u);x[3]=ordered_mul_ss(x[3],x[4]);
        pc(0x122ef7u);x[2]=ordered_mul_ss(x[2],x[4]);
        pc(0x122efbu);x[1]=ordered_mul_ss(x[1],x[6]);
        pc(0x122effu);x[1]=ordered_add_ss(x[1],x[3]);
        pc(0x122f03u);x[0]=ordered_add_ss(x[0],x[2]);
        pc(0x122f07u);g[eax]=g[ebp];
        pc(0x122f09u);g[edi]=g[ebx]+vertical_axis;
        store_lane(0x122f0cu,g[ebp],7);
        store_lane(0x122f11u,local(first_length),3);
        store_lane(0x122f17u,g[ebp]+4u,1);
        store_lane(0x122f1cu,local(z_offset),2);
        store_lane(0x122f22u,g[ebp]+8u,0);
    }

    // Project the old lateral axis onto the plane normal to the new forward
    // axis. The original computes the components as cyclic differences.
    void joint_side_axis() {
        const auto old_side=g[ebx]+lateral_axis;
        const auto forward=g[ebp];
        x[0]=scalar(0x122f2cu,old_side+4u);
        x[1]=scalar(0x122f31u,forward+4u);
        x[1]=ordered_mul_ss(x[1],scalar(0x122f36u,old_side));
        x[0]=ordered_mul_ss(x[0],scalar(0x122f3bu,forward));
        x[2]=scalar(0x122f40u,old_side+8u);
        x[2]=ordered_mul_ss(x[2],scalar(0x122f45u,forward));
        x[3]=scalar(0x122f4au,forward+8u);
        x[3]=ordered_mul_ss(x[3],scalar(0x122f4fu,old_side+4u));
        pc(0x122f54u);x[0]=_mm_sub_ss(x[0],x[1]);
        x[1]=scalar(0x122f58u,forward+8u);
        x[1]=ordered_mul_ss(x[1],scalar(0x122f5du,old_side));
        pc(0x122f62u);x[1]=_mm_sub_ss(x[1],x[2]);
        x[2]=scalar(0x122f66u,forward+4u);
        x[2]=ordered_mul_ss(x[2],scalar(0x122f6bu,old_side+8u));
        pc(0x122f70u);x[2]=_mm_sub_ss(x[2],x[3]);
        pc(0x122f74u);g[eax]=g[edi];
        store_lane(0x122f76u,g[edi],2);
        store_lane(0x122f7au,g[edi]+4u,1);
        store_lane(0x122f7fu,g[edi]+8u,0);
    }
    // Rebuild the joint's remaining axis from forward and normalized side.
    void joint_lateral_axis() {
        const auto forward=g[ebp];
        const auto side=g[edi];
        x[1]=scalar(0x122f89u,side+4u);
        x[1]=ordered_mul_ss(x[1],scalar(0x122f8eu,forward));
        x[0]=scalar(0x122f93u,forward+4u);
        x[0]=ordered_mul_ss(x[0],scalar(0x122f98u,side));
        x[2]=scalar(0x122f9cu,forward+8u);
        x[2]=ordered_mul_ss(x[2],scalar(0x122fa1u,side));
        x[3]=scalar(0x122fa5u,side+8u);
        x[3]=ordered_mul_ss(x[3],scalar(0x122faau,forward+4u));
        pc(0x122fafu);x[0]=_mm_sub_ss(x[0],x[1]);
        x[1]=scalar(0x122fb3u,side+8u);
        x[1]=ordered_mul_ss(x[1],scalar(0x122fb8u,forward));
        pc(0x122fbdu);x[1]=_mm_sub_ss(x[1],x[2]);
        x[2]=scalar(0x122fc1u,forward+8u);
        x[2]=ordered_mul_ss(x[2],scalar(0x122fc6u,side+4u));
        store_lane(0x122fcbu,g[ebx]+lateral_axis+4u,1);
        store_lane(0x122fd0u,g[ebx]+lateral_axis+8u,0);
        x[0]=scalar(0x122fd5u,local(second_length));
        pc(0x122fdbu);x[2]=_mm_sub_ss(x[2],x[3]);
        store_lane(0x122fdfu,g[ebx]+lateral_axis,2);
    }
    void locate_end() {
        pc(0x122fe4u);x[1]=x[0];
        x[1]=ordered_mul_ss(x[1],scalar(0x122fe7u,g[ebp]));
        x[1]=ordered_add_ss(x[1],scalar(0x122fecu,g[ebx]+translation));
        store_lane(0x122ff1u,local(end_x),1);
        x[1]=scalar(0x122ff7u,g[ebp]+4u);
        pc(0x122ffcu);x[1]=ordered_mul_ss(x[1],x[0]);
        x[1]=ordered_add_ss(x[1],scalar(0x123000u,g[ebx]+translation+4u));
        store_lane(0x123005u,local(end_y),1);
        x[1]=scalar(0x12300bu,g[ebp]+8u);
        pc(0x123010u);x[1]=ordered_mul_ss(x[1],x[0]);
        x[1]=ordered_add_ss(x[1],scalar(0x123014u,g[ebx]+translation+8u));
        x[0]=scalar(0x123019u,local(remaining_offset));
        store_lane(0x12301fu,local(end_z),1);
    }
    void parent_forward_axis() {
        pc(0x123025u);x[1]=x[0];
        x[1]=ordered_mul_ss(x[1],scalar(0x123028u,local(target_x)));
        x[1]=_mm_sub_ss(x[1],scalar(0x12302eu,local(y_offset)));
        pc(0x123034u);g[ebx]=g[esi]+4u;
        store_lane(0x123037u,g[ebx],1);
        pc(0x12303bu);g[edi]=g[esi]+vertical_axis;
        pc(0x12303eu);x[1]=x[0];
        x[1]=ordered_mul_ss(x[1],scalar(0x123041u,local(target_y)));
        x[0]=ordered_mul_ss(x[0],scalar(0x123047u,local(target_z)));
        x[1]=_mm_sub_ss(x[1],scalar(0x12304du,local(first_length)));
        x[0]=_mm_sub_ss(x[0],scalar(0x123053u,local(z_offset)));
        pc(0x123059u);g[eax]=g[ebx];
        store_lane(0x12305bu,g[ebx]+4u,1);
        store_lane(0x123060u,g[ebx]+8u,0);
    }
    void parent_side_axis() {
        const auto old_side=g[esi]+lateral_axis;
        const auto forward=g[ebx];
        x[0]=scalar(0x12306au,forward);
        x[1]=scalar(0x12306eu,old_side);
        x[1]=ordered_mul_ss(x[1],scalar(0x123073u,forward+4u));
        x[0]=ordered_mul_ss(x[0],scalar(0x123078u,old_side+4u));
        x[2]=scalar(0x12307du,forward);
        x[2]=ordered_mul_ss(x[2],scalar(0x123081u,old_side+8u));
        x[3]=scalar(0x123086u,forward+8u);
        x[3]=ordered_mul_ss(x[3],scalar(0x12308bu,old_side+4u));
        pc(0x123090u);x[0]=_mm_sub_ss(x[0],x[1]);
        x[1]=scalar(0x123094u,old_side);
        x[1]=ordered_mul_ss(x[1],scalar(0x123099u,forward+8u));
        pc(0x12309eu);x[1]=_mm_sub_ss(x[1],x[2]);
        x[2]=scalar(0x1230a2u,forward+4u);
        x[2]=ordered_mul_ss(x[2],scalar(0x1230a7u,old_side+8u));
        pc(0x1230acu);x[2]=_mm_sub_ss(x[2],x[3]);
        pc(0x1230b0u);g[eax]=g[edi];
        store_lane(0x1230b2u,g[edi],2);
        store_lane(0x1230b6u,g[edi]+4u,1);
        store_lane(0x1230bbu,g[edi]+8u,0);
    }
    void parent_lateral_axis() {
        const auto forward=g[ebx];
        const auto side=g[edi];
        x[0]=scalar(0x1230c5u,forward+4u);
        x[0]=ordered_mul_ss(x[0],scalar(0x1230cau,side));
        x[1]=scalar(0x1230ceu,forward);
        x[1]=ordered_mul_ss(x[1],scalar(0x1230d2u,side+4u));
        x[2]=scalar(0x1230d7u,forward+8u);
        x[2]=ordered_mul_ss(x[2],scalar(0x1230dcu,side));
        x[3]=scalar(0x1230e0u,forward+4u);
        x[3]=ordered_mul_ss(x[3],scalar(0x1230e5u,side+8u));
        g[eax]=word(0x1230eau,local(translation_address));
        g[ecx]=word(0x1230eeu,local(end_x));
        g[edx]=word(0x1230f2u,local(end_y));
        pc(0x1230f6u);x[0]=_mm_sub_ss(x[0],x[1]);
        x[1]=scalar(0x1230fau,forward);
        x[1]=ordered_mul_ss(x[1],scalar(0x1230feu,side+8u));
        pc(0x123103u);x[1]=_mm_sub_ss(x[1],x[2]);
        x[2]=scalar(0x123107u,forward+8u);
        x[2]=ordered_mul_ss(x[2],scalar(0x12310cu,side+4u));
        g[edi]=word(0x123111u,local(destination));
        store_lane(0x123115u,g[esi]+lateral_axis+4u,1);
        store_lane(0x12311au,g[esi]+lateral_axis+8u,0);
        pc(0x12311fu);x[2]=_mm_sub_ss(x[2],x[3]);
        store_lane(0x123123u,g[esi]+lateral_axis,2);
    }
    void commit_matrix() {
        g[esi]=word(0x123128u,local(saved_goal));
        store(0x12312cu,g[eax],g[ecx]);
        g[ecx]=word(0x12312eu,local(end_z));
        store(0x123132u,g[eax]+4u,g[edx]);
        store(0x123135u,g[eax]+8u,g[ecx]);
        pc(0x123138u);g[ecx]=13u;
        // REP MOVSD keeps two live indices and the remaining count even on
        // a partial fault. Retained traces publish after the last word too.
        access(0x12313du);
        while(g[ecx]) {
            copy_matrix_word(s);
            after_access();
            pc(0x12313du);
        }
        g[ebp]=pop(0x12313fu);
    }
    template<class Normalize> void publish(Normalize &child) {
        joint_side_axis();
        normalize(0x122f84u,0x122f89u,child);
        joint_lateral_axis();
        locate_end();
        parent_forward_axis();
        normalize(0x123065u,0x12306au,child);
        parent_side_axis();
        normalize(0x1230c0u,0x1230c5u,child);
        parent_lateral_axis();
        commit_matrix();
        finish();
    }

    // Measure the two links and the target displacement. The three dot
    // products follow the source's distinct summation orders.
    bool measure() {
        pc(0x122be0u);g[esp]=flags.minus(g[esp],0x30u,4);
        push(0x122be3u,g[ebx]);
        pc(0x122be4u);g[ebx]=g[ecx];
        g[ecx]=word(0x122be6u,g[esp]+0x3cu);
        push(0x122beau,g[esi]);
        pc(0x122bebu);g[esi]=g[eax];

        x[2]=scalar(0x122bedu,g[esi]+translation+4u);
        x[2]=_mm_sub_ss(x[2],scalar(0x122bf2u,g[ebx]+translation+4u));
        x[0]=scalar(0x122bf7u,g[esi]+translation+8u);
        x[0]=_mm_sub_ss(x[0],scalar(0x122bfcu,g[ebx]+translation+8u));
        x[1]=scalar(0x122c01u,g[esi]+translation);
        x[1]=_mm_sub_ss(x[1],scalar(0x122c06u,g[ebx]+translation));
        pc(0x122c0bu);g[eax]=g[esi]+translation;
        pc(0x122c0eu);x[3]=x[2];
        pc(0x122c11u);x[3]=ordered_mul_ss(x[3],x[2]);
        pc(0x122c15u);x[2]=x[1];
        pc(0x122c18u);x[2]=ordered_mul_ss(x[2],x[1]);

        x[1]=scalar(0x122c1cu,g[ecx]+translation+8u);
        x[1]=_mm_sub_ss(x[1],scalar(0x122c21u,g[eax]+8u));
        pc(0x122c26u);x[0]=ordered_mul_ss(x[0],x[0]);
        pc(0x122c2au);x[0]=ordered_add_ss(x[0],x[3]);
        x[3]=scalar(0x122c2eu,g[ecx]+translation+4u);
        x[3]=_mm_sub_ss(x[3],scalar(0x122c33u,g[eax]+4u));
        pc(0x122c38u);x[0]=ordered_add_ss(x[0],x[2]);
        x[2]=scalar(0x122c3cu,g[ecx]+translation);
        x[2]=_mm_sub_ss(x[2],scalar(0x122c41u,g[eax]));
        pc(0x122c45u);x[4]=x[3];
        pc(0x122c48u);x[4]=ordered_mul_ss(x[4],x[3]);
        pc(0x122c4cu);x[3]=x[2];
        pc(0x122c4fu);x[3]=ordered_mul_ss(x[3],x[2]);

        x[2]=scalar(0x122c53u,g[ebx]+translation+8u);
        pc(0x122c58u);x[1]=ordered_mul_ss(x[1],x[1]);
        pc(0x122c5cu);x[1]=ordered_add_ss(x[1],x[4]);
        x[4]=scalar(0x122c60u,g[ebx]+translation+4u);
        push(0x122c65u,g[edi]);
        g[edi]=word(0x122c66u,g[esp]+0x40u);
        x[4]=_mm_sub_ss(x[4],scalar(0x122c6au,g[edi]+translation+4u));
        x[2]=_mm_sub_ss(x[2],scalar(0x122c6fu,g[edi]+translation+8u));
        pc(0x122c74u);x[1]=ordered_add_ss(x[1],x[3]);
        x[3]=scalar(0x122c78u,g[ebx]+translation);
        x[3]=_mm_sub_ss(x[3],scalar(0x122c7du,g[edi]+translation));
        pc(0x122c82u);x[5]=x[4];
        pc(0x122c85u);x[5]=ordered_mul_ss(x[5],x[4]);
        pc(0x122c89u);x[4]=x[3];
        pc(0x122c8cu);x[2]=ordered_mul_ss(x[2],x[2]);
        pc(0x122c90u);x[4]=ordered_mul_ss(x[4],x[3]);
        x[3]=scalar(0x122c94u,minimum_length_squared);
        pc(0x122c9cu);flags.scalar_compare(x[0],x[3],false);
        pc(0x122c9fu);x[2]=ordered_add_ss(x[2],x[5]);
        pc(0x122ca3u);x[2]=ordered_add_ss(x[2],x[4]);

        store(0x122ca7u,local(z_offset),g[eax]);
        store_lane(0x122cabu,local(second_length),0);
        store_lane(0x122cb1u,local(target_length),1);
        store_lane(0x122cb7u,local(remaining_offset),2);
        pc(0x122cbdu);
        if(flags.cf||flags.zf)return false;
        pc(0x122cc3u);flags.scalar_compare(x[1],x[3],false);
        pc(0x122cc6u);
        if(flags.cf||flags.zf)return false;
        pc(0x122cccu);flags.scalar_compare(x[2],x[3],false);
        pc(0x122ccfu);
        return !flags.cf&&!flags.zf;
    }
    template<class Normalize> void solve(Normalize &child) {
        if(!measure()) {
            finish();
            return;
        }
        build_plane();
        normalize(0x122d9du,0x122da2u,child);
        limit_target_reach();
        solve_plane_offset();
        joint_forward_axis();
        normalize(0x122f27u,0x122f2cu,child);
        publish(child);
    }
};

template<class State> bool quiet(State &s,std::uint32_t *&cursor) {
    using native_object_tick_detail::admitted;
    return admitted(s,entry,exit,cursor,false) &&
        (s.immutable_cursor.cursor || native_census_detail::uncursored(s,entry,exit));
}
template<class State,class Normalize> bool bridge(State &s,Normalize child) {
    static const bool forced=native_small_queries_detail::flag("HALO2_REFERENCE_TWO_BONE_SOLVE");
    if(forced||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(quiet(s,cursor))Frame<State,false>{s,cursor}.solve(child);
    else Frame<State,true>{s,nullptr}.solve(child);
    return true;
}
}
