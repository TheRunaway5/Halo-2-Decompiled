#pragma once
#include "physics/runtime/native_tu15_two_bone_solve.hpp"

namespace halo2::native_polygon_intersection {
using namespace native_object_tick_detail::boundary;
using native_tu15_leaves::ordered_mul_ss;
using native_world_leaves_detail::ordered_add_ss;

constexpr std::uint32_t entry=0x1f6220u, last=0x1f6606u;
constexpr std::uint32_t first_vertex=0x20u, vertex_stride=0x10u;
constexpr std::uint32_t vertex_count=0x14u, axis_pairs=0x44ac8cu;
constexpr std::uint32_t one=0x46a0d0u, negative_one=0x46a0dcu, minimum_normal=0x46a0ecu;

namespace x87 {
using native_live_support::x87::load_single;
using native_two_bone_solve::x87::square_root;

template<class State>
void square_component(State &s,std::uint32_t bits) {
    auto &component=s.fp.st(0);
    long double result;
    std::uint16_t status;
    asm volatile("fldt %2\n\tfmuls %3\n\tfnstsw %1\n\tfstpt %0"
        :"=m"(result),"=m"(status):"m"(component),"m"(bits):"st");
    component=result;
    s.fp.status=(s.fp.status&~0x200u)|(status&0x200u);
}

template<class State>
void sum_components(State &s) {
    const auto &right=s.fp.st(0);
    auto &left=s.fp.st(1);
    long double result;
    std::uint16_t status;
    asm volatile("fldt %2\n\tfldt %3\n\tfaddp %%st,%%st(1)\n\tfnstsw %1\n\tfstpt %0"
        :"=m"(result),"=m"(status):"m"(left),"m"(right):"st","st(1)");
    left=result;
    s.fp.status=(s.fp.status&~0x200u)|(status&0x200u);
    s.fp.pop();
}

template<class State>
void spill_length(State &s,std::uint32_t address) {
    using namespace physics_contact_runtime_detail;
    const auto prior=source_x87_exception_flags();
    const auto rounded=native_live_support::x87::store_single(s.fp.st(0));
    const auto produced=source_x87_exception_flags();
    if(produced!=prior)source_load_x87_exception_flags(prior);
    native_two_bone_solve::store_and_commit(s,0x1f6338u,address,std::uint32_t(rounded.bits),[&] {
        if(produced!=prior)source_load_x87_exception_flags(produced|source_x87_exception_flags());
        s.fp.status=(s.fp.status&~0x200u)|(rounded.rounded_up?0x200u:0u);
    });
}

template<class State>
void compare_minimum(State &s) {
    s.compare_and_pop(s.fp.st(1));
    s.fp.status&=~0x200u;
}
}

template<class State,bool Observed>
struct Frame : native_two_bone_solve::Frame<State,Observed> {
    using Base=native_two_bone_solve::Frame<State,Observed>;
    using Base::Base;
    using Base::s;
    using Base::g;
    using Base::x;
    using Base::flags;
    using Base::pc;
    using Base::access;
    using Base::after_access;
    using Base::word;
    using Base::scalar;
    using Base::store;
    using Base::store_lane;
    using Base::push;
    using Base::pop;

    // Offsets depend on the saved registers pushed by each phase.
    std::uint32_t local(std::uint32_t offset)const {return g[esp]+offset;}
    std::uint32_t narrow(std::uint32_t site,std::uint32_t address,unsigned size) {
        access(site);
        const auto value=physics_step_detail::r(s.memory,address,size);
        after_access();
        return value;
    }
    void store_byte(std::uint32_t site,std::uint32_t address,std::uint32_t value) {
        access(site);
        physics_step_detail::w(s.memory,s.observe,site,address,1,value);
        after_access();
    }
    static std::uint32_t signed_short(std::uint32_t value) {
        return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    }
    void latch_comparison(std::uint32_t load,std::uint32_t test) {
        pc(load);s.set(eax,flags.bits()|2u,1,8);
        pc(test);flags.logic(s.reg(eax,1,8)&0x44u,1);
    }

    // Save the ray and build the cross product of the first two polygon
    // edges. Read and write in source order for stack aliases and observers.
    void build_normal() {
        pc(entry);g[esp]=flags.minus(g[esp],0x90u,4);
        g[eax]=word(0x1f6226u,local(0x98u));
        x[5]=scalar(0x1f622du,g[eax]+8u);
        x[2]=scalar(0x1f6232u,g[eax]+0x18u);
        x[3]=scalar(0x1f6237u,g[eax]);
        x[4]=scalar(0x1f623bu,g[eax]+4u);
        x[0]=scalar(0x1f6240u,g[eax]+0x10u);
        x[1]=scalar(0x1f6245u,g[eax]+0x14u);
        pc(0x1f624au);x[2]=_mm_sub_ss(x[2],x[5]);
        pc(0x1f624eu);x[0]=_mm_sub_ss(x[0],x[3]);
        pc(0x1f6252u);x[1]=_mm_sub_ss(x[1],x[4]);
        store_lane(0x1f6256u,local(0x1cu),2);
        push(0x1f625cu,g[esi]);
        pc(0x1f625du);g[esi]=g[ecx];
        x[6]=scalar(0x1f625fu,g[esi]+first_vertex);
        x[2]=scalar(0x1f6264u,g[esi]+first_vertex+vertex_stride);
        x[7]=scalar(0x1f6269u,g[esi]+first_vertex+4u);
        store_lane(0x1f626eu,local(0x2cu),5);
        x[5]=scalar(0x1f6274u,g[esi]+first_vertex+2u*vertex_stride);
        pc(0x1f6279u);x[2]=_mm_sub_ss(x[2],x[6]);
        store_lane(0x1f627du,local(0x48u),2);
        x[2]=scalar(0x1f6283u,g[esi]+first_vertex+8u);
        store_lane(0x1f6288u,local(0x18u),0);
        x[0]=scalar(0x1f628eu,g[esi]+first_vertex+vertex_stride+8u);
        store_lane(0x1f6293u,local(0x1cu),1);
        x[1]=scalar(0x1f6299u,g[esi]+first_vertex+2u*vertex_stride+4u);
        pc(0x1f629eu);x[0]=_mm_sub_ss(x[0],x[2]);
        store_lane(0x1f62a2u,local(0x28u),4);
        x[4]=scalar(0x1f62a8u,g[esi]+first_vertex+2u*vertex_stride+8u);
        pc(0x1f62adu);x[5]=_mm_sub_ss(x[5],x[6]);
        store_lane(0x1f62b1u,local(0x30u),5);
        store_lane(0x1f62b7u,local(0x24u),3);
        x[3]=scalar(0x1f62bdu,g[esi]+first_vertex+vertex_stride+4u);
        pc(0x1f62c2u);x[4]=_mm_sub_ss(x[4],x[2]);
        pc(0x1f62c6u);x[1]=_mm_sub_ss(x[1],x[7]);
        pc(0x1f62cau);x[5]=x[1];
        pc(0x1f62cdu);x[5]=ordered_mul_ss(x[5],x[0]);
        x[0]=ordered_mul_ss(x[0],scalar(0x1f62d1u,local(0x30u)));
        pc(0x1f62d7u);x[3]=_mm_sub_ss(x[3],x[7]);
        pc(0x1f62dbu);x[2]=x[4];
        pc(0x1f62deu);x[2]=ordered_mul_ss(x[2],x[3]);
        x[3]=ordered_mul_ss(x[3],scalar(0x1f62e2u,local(0x30u)));
        pc(0x1f62e8u);x[2]=_mm_sub_ss(x[2],x[5]);
        x[5]=scalar(0x1f62ecu,local(0x48u));
        pc(0x1f62f2u);x[1]=ordered_mul_ss(x[1],x[5]);
        pc(0x1f62f6u);x[1]=_mm_sub_ss(x[1],x[3]);
        store_lane(0x1f62fau,local(0x10u),1);
    }

    void measure_normal() {
        s.fp.push(x87::load_single(word(0x1f6300u,local(0x10u))));
        x87::square_component(s,word(0x1f6304u,local(0x10u)));
        pc(0x1f6308u);x[4]=ordered_mul_ss(x[4],x[5]);
        pc(0x1f630cu);x[0]=_mm_sub_ss(x[0],x[4]);
        store_lane(0x1f6310u,local(0xcu),0);
        s.fp.push(x87::load_single(word(0x1f6316u,local(0xcu))));
        x87::square_component(s,word(0x1f631au,local(0xcu)));
        store_lane(0x1f631eu,local(0x8u),2);
        pc(0x1f6324u);s.set(ecx,flags.logic(0,1),1);
        store_byte(0x1f6326u,local(0x3cu),s.reg(ecx,1));
        pc(0x1f632au);x87::sum_components(s);
        s.fp.push(x87::load_single(word(0x1f632cu,local(0x8u))));
        x87::square_component(s,word(0x1f6330u,local(0x8u)));
        pc(0x1f6334u);x87::sum_components(s);
        pc(0x1f6336u);x87::square_root(s);
        access(0x1f6338u);
        x87::spill_length(s,local(4u));
        after_access();
        pc(0x1f633cu);s.fp.st(0)=std::fabs(s.fp.st(0));s.fp.status&=~0x200u;
        s.fp.push(x87::load_single(word(0x1f633eu,minimum_normal)));
        pc(0x1f6344u);x87::compare_minimum(s);
        pc(0x1f6346u);s.fp.status&=~0x200u;s.fp.pop();
        pc(0x1f6348u);
        if(flags.cf||flags.zf)normalize();
    }

    void normalize() {
        x[3]=scalar(0x1f634au,one);
        x[3]=_mm_div_ss(x[3],scalar(0x1f6352u,local(4u)));
        pc(0x1f6358u);x[4]=x[3];
        pc(0x1f635bu);x[4]=ordered_mul_ss(x[4],x[2]);
        pc(0x1f635fu);x[0]=ordered_mul_ss(x[0],x[3]);
        pc(0x1f6363u);x[1]=ordered_mul_ss(x[1],x[3]);
        store_lane(0x1f6367u,local(8u),4);
        store_lane(0x1f636du,local(0xcu),0);
        store_lane(0x1f6373u,local(0x10u),1);
        pc(0x1f6379u);x[2]=x[4];
    }

    void intersect_plane() {
        x[4]=scalar(0x1f637cu,local(0x24u));
        pc(0x1f6382u);x[3]=_mm_setzero_ps();
        store_lane(0x1f6385u,local(0x14u),3);
        x[3]=scalar(0x1f638bu,g[esi]+first_vertex+8u);
        pc(0x1f6390u);x[4]=_mm_sub_ss(x[4],x[6]);
        x[6]=scalar(0x1f6394u,local(0x28u));
        pc(0x1f639au);x[6]=_mm_sub_ss(x[6],x[7]);
        x[7]=scalar(0x1f639eu,local(0x2cu));
        pc(0x1f63a4u);x[7]=_mm_sub_ss(x[7],x[3]);
        pc(0x1f63a8u);x[3]=x[1];
        x[3]=ordered_mul_ss(x[3],scalar(0x1f63abu,local(0x20u)));
        pc(0x1f63b1u);x[5]=x[0];
        x[5]=ordered_mul_ss(x[5],scalar(0x1f63b4u,local(0x1cu)));
        pc(0x1f63bau);x[3]=ordered_add_ss(x[3],x[5]);
        pc(0x1f63beu);x[5]=x[2];
        x[5]=ordered_mul_ss(x[5],scalar(0x1f63c1u,local(0x18u)));
        pc(0x1f63c7u);x[3]=ordered_add_ss(x[3],x[5]);
        pc(0x1f63cbu);x[5]=_mm_setzero_ps();
        pc(0x1f63ceu);flags.scalar_compare(x[3],x[5],true);
        latch_comparison(0x1f63d1u,0x1f63d2u);
        pc(0x1f63d5u);
        if(flags.pf) {
            pc(0x1f63d7u);x[1]=ordered_mul_ss(x[1],x[7]);
            pc(0x1f63dbu);x[0]=ordered_mul_ss(x[0],x[6]);
            pc(0x1f63dfu);x[1]=ordered_add_ss(x[1],x[0]);
            x[0]=scalar(0x1f63e3u,negative_one);
            pc(0x1f63ebu);x[2]=ordered_mul_ss(x[2],x[4]);
            pc(0x1f63efu);x[1]=ordered_add_ss(x[1],x[2]);
            pc(0x1f63f3u);x[0]=_mm_div_ss(x[0],x[3]);
            pc(0x1f63f7u);x[1]=ordered_mul_ss(x[1],x[0]);
            store_lane(0x1f63fbu,local(4u),1);
            pc(0x1f6401u);
        } else {
            store_lane(0x1f6403u,local(4u),5);
            pc(0x1f6409u);x[1]=x[5];
        }
    }

    bool accept_fraction() {
        g[eax]=word(0x1f640cu,local(0xa0u));
        x[0]=scalar(0x1f6413u,g[eax]+0x14u);
        pc(0x1f6418u);flags.scalar_compare(x[0],x[1],false);
        pc(0x1f641bu);
        if(flags.cf||flags.zf)return false;
        pc(0x1f6421u);flags.scalar_compare(x[5],x[1],false);
        pc(0x1f6424u);
        if(flags.cf||flags.zf) {
            x[5]=scalar(0x1f6426u,one);
            pc(0x1f642eu);flags.scalar_compare(x[1],x[5],false);
            pc(0x1f6431u);
            if(flags.cf||flags.zf) {
                pc(0x1f6433u);x[5]=x[1];
            }
        }
        pc(0x1f6436u);flags.scalar_compare(x[5],x[1],true);
        latch_comparison(0x1f6439u,0x1f643au);
        pc(0x1f643du);
        return !flags.pf;
    }

    // Use the dominant normal axis and its sign to select the two
    // coordinates of the projected point and the winding of the polygon.
    void project_intersection() {
        pc(0x1f644cu);x[6]=_mm_setzero_ps();
        pc(0x1f644fu);g[eax]=signed_short(g[eax]);
        pc(0x1f6452u);g[ecx]=signed_short(g[eax]);
        x[0]=scalar(0x1f6455u,local(8u+g[ecx]*4u));
        pc(0x1f645bu);flags.scalar_compare(x[0],x[6],false);
        pc(0x1f645eu);
        if(!flags.cf&&!flags.zf) {
            pc(0x1f6460u);s.set(eax,1,1);
            pc(0x1f6462u);
        } else {
            pc(0x1f6464u);s.set(eax,flags.logic(0,1),1);
        }
        x[5]=scalar(0x1f6466u,local(4u));
        pc(0x1f646cu);g[edx]=s.reg(eax,1);
        pc(0x1f646fu);g[eax]=g[edx]+g[ecx]*2u;
        push(0x1f6472u,g[ebx]);
        pc(0x1f6473u);x[0]=x[5];
        x[0]=ordered_mul_ss(x[0],scalar(0x1f6476u,local(0x1cu)));
        x[0]=ordered_add_ss(x[0],scalar(0x1f647cu,local(0x28u)));
        store_lane(0x1f6482u,local(0x34u),0);
        x[0]=scalar(0x1f6488u,local(0x20u));
        pc(0x1f648eu);x[0]=ordered_mul_ss(x[0],x[5]);
        x[0]=ordered_add_ss(x[0],scalar(0x1f6492u,local(0x2cu)));
        pc(0x1f6498u);g[eax]*=3u;
        pc(0x1f649bu);g[eax]=flags.shift(g[eax],1,4);
        g[ebx]=signed_short(narrow(0x1f649du,axis_pairs+g[eax]+2u,2));
        push(0x1f64a4u,g[ebp]);
        g[ebp]=signed_short(narrow(0x1f64a5u,axis_pairs+g[eax],2));
        store_lane(0x1f64acu,local(0x3cu),0);
        x[0]=scalar(0x1f64b2u,local(0x28u));
        pc(0x1f64b8u);x[0]=ordered_mul_ss(x[0],x[5]);
        x[0]=ordered_add_ss(x[0],scalar(0x1f64bcu,local(0x34u)));
        pc(0x1f64c2u);g[ebp]=flags.shift(g[ebp],2,4);
        store_lane(0x1f64c5u,local(0x40u),0);
        x[0]=scalar(0x1f64cbu,local(0x38u+g[ebp]));
        push(0x1f64d1u,g[edi]);
        g[edi]=word(0x1f64d2u,g[esi]+vertex_count);
        pc(0x1f64d5u);g[ebx]=flags.shift(g[ebx],2,4);
        store_lane(0x1f64d8u,local(0x24u),0);
        x[0]=scalar(0x1f64deu,local(0x3cu+g[ebx]));
        pc(0x1f64e4u);g[ecx]=flags.logic(0,4);
        pc(0x1f64e6u);flags.minus(g[edi],4,4);
        store(0x1f64e9u,local(0x4cu),g[ebp]);
        store_lane(0x1f64edu,local(0x28u),0);
    }

    void project_vertex_group() {
        pc(0x1f64f5u);g[edi]=flags.plus(g[edi],0xfffffffcu,4);
        pc(0x1f64f8u);g[edx]=g[esi]+g[ebp]+first_vertex+2u*vertex_stride;
        pc(0x1f64fcu);g[ebp]=flags.minus(g[ebp],g[ebx],4);
        pc(0x1f64feu);g[edi]=flags.right(g[edi],2,4,false);
        pc(0x1f6501u);g[edi]=flags.increment(g[edi],4,false);
        store(0x1f6502u,local(0x10u),g[ebp]);
        pc(0x1f6506u);g[ebp]=g[edi]*4u;
        pc(0x1f650du);g[eax]=local(0x6cu);
        pc(0x1f6511u);g[ecx]=g[ebx]+g[esi]+first_vertex;
        store(0x1f6515u,local(0x50u),g[ebp]);
        pc(0x1f6519u);
        do {
            g[ebp]=word(0x1f6520u,local(0x10u));
            g[ebp]=word(0x1f6524u,g[ecx]+g[ebp]);
            store(0x1f6527u,g[eax]-12u,g[ebp]);
            g[ebp]=word(0x1f652au,g[ecx]);
            store(0x1f652cu,g[eax]-8u,g[ebp]);
            g[ebp]=word(0x1f652fu,g[edx]-vertex_stride);
            store(0x1f6532u,g[eax]-4u,g[ebp]);
            g[ebp]=word(0x1f6535u,g[ecx]+vertex_stride);
            store(0x1f6538u,g[eax],g[ebp]);
            g[ebp]=word(0x1f653au,g[edx]);
            store(0x1f653cu,g[eax]+4u,g[ebp]);
            g[ebp]=word(0x1f653fu,g[ecx]+2u*vertex_stride);
            store(0x1f6542u,g[eax]+8u,g[ebp]);
            g[ebp]=word(0x1f6545u,g[edx]+vertex_stride);
            store(0x1f6548u,g[eax]+12u,g[ebp]);
            g[ebp]=word(0x1f654bu,g[ecx]+3u*vertex_stride);
            store(0x1f654eu,g[eax]+16u,g[ebp]);
            pc(0x1f6551u);g[ecx]=flags.plus(g[ecx],4u*vertex_stride,4);
            pc(0x1f6554u);g[edx]=flags.plus(g[edx],4u*vertex_stride,4);
            pc(0x1f6557u);g[eax]=flags.plus(g[eax],0x20u,4);
            pc(0x1f655au);g[edi]=flags.increment(g[edi],4,true);
            pc(0x1f655bu);
        } while(!flags.zf);
        g[ecx]=word(0x1f655du,local(0x50u));
        g[ebp]=word(0x1f6561u,local(0x4cu));
    }

    void project_vertices() {
        pc(0x1f64f3u);
        if(flags.sf==flags.of)project_vertex_group();
        g[edx]=word(0x1f6565u,g[esi]+vertex_count);
        pc(0x1f6568u);flags.minus(g[ecx],g[edx],4);
        pc(0x1f656au);
        if(flags.sf==flags.of)return;
        pc(0x1f656cu);g[eax]=g[ecx]+2u;
        pc(0x1f656fu);g[eax]=flags.shift(g[eax],4,4);
        pc(0x1f6572u);g[eax]=flags.plus(g[eax],g[ebx],4);
        pc(0x1f6574u);g[eax]=flags.plus(g[eax],g[esi],4);
        pc(0x1f6576u);g[ebp]=flags.minus(g[ebp],g[ebx],4);
        pc(0x1f6578u);
        do {
            g[esi]=word(0x1f6580u,g[eax]+g[ebp]);
            store(0x1f6583u,local(0x60u+g[ecx]*8u),g[esi]);
            g[esi]=word(0x1f6587u,g[eax]);
            store(0x1f6589u,local(0x64u+g[ecx]*8u),g[esi]);
            pc(0x1f658du);g[ecx]=flags.increment(g[ecx],4,false);
            pc(0x1f658eu);g[eax]=flags.plus(g[eax],vertex_stride,4);
            pc(0x1f6591u);flags.minus(g[ecx],g[edx],4);
            pc(0x1f6593u);
        } while(flags.sf!=flags.of);
    }

    void accept_hit() {
        g[eax]=word(0x1f65acu,local(0xa0u));
        x[0]=scalar(0x1f65b3u,local(8u));
        store_lane(0x1f65b9u,g[eax]+0x14u,5);
        store_lane(0x1f65beu,g[eax],0);
        x[0]=scalar(0x1f65c2u,local(0xcu));
        store_lane(0x1f65c8u,g[eax]+4u,0);
        x[0]=scalar(0x1f65cdu,local(0x10u));
        store_lane(0x1f65d3u,g[eax]+8u,0);
        store_lane(0x1f65d8u,g[eax]+0xcu,6);
        g[eax]=word(0x1f65ddu,local(0x98u));
        pc(0x1f65e4u);s.set(ecx,1,1);
        store_byte(0x1f65e6u,g[eax],s.reg(ecx,1));
        g[esi]=pop(0x1f65e8u);
        pc(0x1f65e9u);g[esp]=flags.plus(g[esp],0x90u,4);
        (void)pop(0x1f65efu);
        g[esp]+=12u;
    }

    void reject_hit() {
        g[eax]=word(0x1f65f6u,local(0x98u));
        store_byte(0x1f65fdu,g[eax],s.reg(ecx,1));
        g[esi]=pop(0x1f65ffu);
        pc(0x1f6600u);g[esp]=flags.plus(g[esp],0x90u,4);
        (void)pop(last);
        g[esp]+=12u;
    }

    template<class DominantAxis,class ContainsPoint>
    void intersect(DominantAxis &dominant_axis,ContainsPoint &contains_point) {
        build_normal();
        measure_normal();
        intersect_plane();
        if(!accept_fraction()) {
            reject_hit();
            return;
        }
        pc(0x1f6443u);g[eax]=local(8u);
        push(0x1f6447u,0x1f644cu);
        dominant_axis(s);
        after_access();
        project_intersection();
        project_vertices();
        push(0x1f6595u,0);
        push(0x1f6597u,g[edx]);
        pc(0x1f6598u);g[ebx]=local(0x2cu);
        pc(0x1f659cu);g[edx]=local(0x68u);
        push(0x1f65a0u,0x1f65a5u);
        contains_point(s);
        after_access();
        pc(0x1f65a5u);flags.logic(s.reg(eax,1),1);
        g[edi]=pop(0x1f65a7u);
        g[ebp]=pop(0x1f65a8u);
        g[ebx]=pop(0x1f65a9u);
        pc(0x1f65aau);
        if(flags.zf) {
            s.set(ecx,narrow(0x1f65f2u,local(0x3cu),1),1);
            reject_hit();
        } else {
            accept_hit();
        }
    }
};

template<class State,class DominantAxis,class ContainsPoint>
bool bridge(State &s,DominantAxis dominant_axis,ContainsPoint contains_point) {
    static const bool reference=native_small_queries_detail::flag("HALO2_REFERENCE_POLYGON_INTERSECTION");
    if(reference||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(native_object_tick_detail::admitted(s,entry,last,cursor,false) &&
       (s.immutable_cursor.cursor || native_census_detail::uncursored(s,entry,last))) {
        Frame<State,false>{s,cursor}.intersect(dominant_axis,contains_point);
    } else {
        Frame<State,true>{s,nullptr}.intersect(dominant_axis,contains_point);
    }
    return true;
}
}
