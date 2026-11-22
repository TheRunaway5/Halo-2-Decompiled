#include "halo2/native_campaign_ai_vehicle_movement_tu15.hpp"
#include "campaign/ai/native_campaign_ai_vehicle_candidate_geometry_tu15.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include <cstring>

namespace halo2::vehicle_movement_tu15 {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
using goal_tu15::Port;
using goal_tu15::Simd;
using goal_tu15::u32;
using physics_contact_runtime_detail::Flags;
using Owner=SourcePhysicsWorldRuntimeServices::RequiredOwner;
using vehicle_candidates_tu15::Planar;

struct Vector {
    u32 x,y,z;
};

inline u32 length3(NativeTu15PhysicsFrameFp &fp,Vector vector) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    u32 result;
    asm volatile("flds %[z]\n\tflds %[y]\n\tflds %[x]\n\tfld %%st(0)\n\tfmul %%st(1)"
                 "\n\tfld %%st(2)\n\tfmul %%st(3)\n\tfaddp\n\tfld %%st(3)\n\tfmul %%st(4)"
                 "\n\tfaddp\n\tfsqrt\n\tfstp %%st(3)\n\tfstp %%st(0)\n\tfstp %%st(0)\n\tfstps %[r]"
        :[r] "=m"(result):[x] "m"(vector.x),[y] "m"(vector.y),[z] "m"(vector.z)
        :"memory","st","st(1)","st(2)","st(3)","st(4)");
    return result;
}

class Driver {
    Port p;
    SourcePhysicsWorldRuntimeAbi &abi;
    const Owner &owner;
    const SourcePhysicsContactRecordObserver &observe;
    NativeTu15PhysicsFrameFp fp;
    Simd s;
    Flags flags;
    const u32 entry,frame;

    u32 word(u32 offset) const { return p.r32(frame+offset); }
    u32 byte(u32 offset) const { return p.r8(frame+offset); }
    u32 arg(unsigned index) const { return p.r32(entry+index*4u); }
    u32 x(unsigned reg) const { return abi.xmm[reg][0]; }
    void lane(unsigned reg,u32 value) { abi.xmm[reg]={value}; }
    void low(unsigned reg,u32 value) { abi.xmm[reg][0]=value; }
    void reg_byte(unsigned reg,u32 value) { abi.gp[reg]=(abi.gp[reg]&0xffffff00u)|(value&0xffu); }
    void reg_word(unsigned reg,u32 value) { abi.gp[reg]=(abi.gp[reg]&0xffff0000u)|(value&0xffffu); }
    void w(u32 site,u32 offset,u32 value) { p.w32(site,frame+offset,value); }
    void w8(u32 site,u32 offset,u32 value) { p.w8(site,frame+offset,value); }
    u32 actor() const { return abi.gp[5]; }

    void publish() {
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
        abi.x87_status=fp.status;
        abi.eflags=(abi.eflags&~0x8d5u)|flags.bits();
    }

    void sync() {
        const auto state=abi.floating_point.value();
        fp={state.x87_control_word,abi.x87_status,state.mxcsr};
        flags.load(abi.eflags);
    }

    void push(u32 site,u32 value) {
        const auto address=abi.gp[4]-4u;
        p.w32(site,address,value);
        abi.gp[4]=address;
    }

    u32 pop() {
        const auto value=p.r32(abi.gp[4]);
        abi.gp[4]+=4u;
        return value;
    }

    void invoke(u32 at,u32 target) {
        push(at,at+5u);
        publish();
        owner(p.memory,{at,target,false},abi,observe);
        sync();
        if(abi.gp[4]!=frame)
            throw std::runtime_error("2A3EA0 lost the actual child return stack");
    }

    void push_extended(u32 value) {
        if((value&0x7f800000u)==0x7f800000u)
            throw std::runtime_error("2A3EA0 reached the exceptional angle input");
        {
            detail::NativeTu15ArithmeticEnvironment env(fp);
            asm volatile("flds %1\n\tfstpt %0":"=m"(abi.x87[0]):"m"(value):"memory","st");
        }
        fp.status=(fp.status&~0x3a00u)|((fp.status-0x800u)&0x3800u);
        abi.x87_depth=1;
    }

    u32 pop_extended() {
        if(abi.x87_depth!=1) throw std::runtime_error("2A3EA0 lost its original acos result");
        u32 value;
        {
            detail::NativeTu15ArithmeticEnvironment env(fp);
            asm volatile("fldt %1\n\tfstps %0":"=m"(value):"m"(abi.x87[0]):"memory","st");
        }
        fp.status=(fp.status&~0x3800u)|((fp.status+0x800u)&0x3800u);
        abi.x87_depth=0;
        return value;
    }

    bool positive(u32 value) const { return s.above(value,0); }

    void resolve_property() {
        auto &g=abi.gp;
        g[4]=flags.minus(entry,0x580u,4);
        g[0]=p.r32(0x5020c8u);
        g[2]=p.r32(0x4ecbd0u);
        push(0x2a3eb1u,g[5]);
        push(0x2a3eb2u,g[6]);
        push(0x2a3eb3u,g[7]);
        g[7]=p.r32(g[0]+0x44u);
        g[0]=arg(1);
        g[5]=(g[3]&0xffffu)*0x888u;
        g[1]=(g[0]&0xffffu)*3u;
        g[0]=p.r32(g[2]+0x44u);
        g[0]=p.r32(g[0]+g[1]*4u+8u);
        g[1]=p.r32(g[0]);
        push(0x2a3eddu,g[1]);
        g[1]=g[3];
        g[5]=flags.plus(g[5],g[7],4);
        w(0x2a3ee2u,0x54u,g[0]);
        invoke(0x2a3ee6u,0x1ed700u);
        flags.minus(arg(8),0,2);
        g[7]=g[0];
        w(0x2a3ef6u,0x10u,g[7]);
        w8(0x2a3efau,0x0eu,0);
        w8(0x2a3effu,0x17u,0);
        if (flags.zf) {
            g[2]=p.r32(actor()+0x670u);
            p.w32(0x2a3f0cu,actor()+0x674u,g[2]);
        }
    }

    bool facing() {
        auto &g=abi.gp;
        const auto orientation=actor()+0x290u;
        g[1]=p.r32(orientation);
        g[2]=p.r32(orientation+4u);
        w(0x2a3f1du,0x1cu,g[1]);
        w(0x2a3f29u,0x20u,g[2]);
        g[0]=p.r32(orientation+8u);
        lane(4,0);
        lane(3,p.r32(0x46a0d0u));
        w(0x2a3f43u,0x24u,g[0]);
        p.w8(0x2a3f49u,actor()+0x6d1u,0);
        w(0x2a3f50u,0x24u,x(4));
        const auto distance=vehicle_candidates_tu15::compare_length(fp,{word(0x1cu),word(0x20u)},p.r32(0x46a0ecu));
        w(0x2a3f58u,0x28u,distance.rounded);
        if (!distance.comparison) {
            lane(0,s.div(x(3),word(0x28u)));
            lane(2,s.mul(x(0),word(0x1cu)));
            lane(1,s.mul(x(0),word(0x20u)));
            low(0,s.mul(x(0),x(4)));
            w(0x2a3f89u,0x1cu,x(2));
            w(0x2a3f8fu,0x20u,x(1));
            w(0x2a3f95u,0x24u,x(0));
        } else {
            lane(2,word(0x1cu));
            lane(1,word(0x20u));
        }
        g[6]=p.r32(0x474cd8u);
        lane(0,s.mul(p.r32(g[6]),x(2)));
        lane(2,s.mul(p.r32(g[6]+4u),x(1)));
        lane(1,p.r32(0x46a0dcu));
        low(0,s.add(x(0),x(2)));
        if (s.above(x(1),x(0))) {
            abi.xmm[0]=abi.xmm[1];
            w(0x2a3ff8u,0x18u,x(0));
        } else if (s.above(x(0),x(3))) {
            abi.xmm[0]=abi.xmm[3];
            w(0x2a3ff8u,0x18u,x(0));
        } else {
            // The source keeps the store of its tentative +1 bound even
            // when the smaller final cosine immediately replaces it.
            w(0x2a3ff0u,0x18u,x(3));
            w(0x2a3ff8u,0x18u,x(0));
        }
        push_extended(word(0x18u));
        invoke(0x2a4002u,0x32ccd8u);
        lane(0,p.r32(g[6]));
        lane(1,p.r32(g[6]+4u));
        low(1,s.mul(x(1),word(0x1cu)));
        low(0,s.mul(x(0),word(0x20u)));
        low(0,s.sub(x(0),x(1)));
        lane(1,0);
        const auto opposite=positive(x(0)^0x80000000u);
        w(0x2a4026u,0x18u,pop_extended());
        if (opposite) {
            lane(0,s.sub(0,word(0x18u)));
            w(0x2a4035u,0x18u,x(0));
        } else lane(0,word(0x18u));
        reg_byte(0,p.r8(actor()+0x5d0u));
        flags.logic(g[0]&0xffu,1);
        if(flags.zf) return false;
        reg_byte(0,p.r8(actor()+0x5d2u));
        flags.logic(g[0]&0xffu,1);
        if(!flags.zf) throw std::runtime_error("2A3EA0 blocked movement path");
        flags.logic(g[7],4);
        if(flags.zf) throw std::runtime_error("2A3EA0 lacks its actual inherited movement property");
        return true;
    }

    void publish_inactive_motion() {
        auto &g=abi.gp;
        g[7]=pop();
        g[6]=pop();
        p.w32(0x2a4877u,actor()+0x670u,x(1));
        p.w8(0x2a487fu,actor()+0x678u,0);
        p.w32(0x2a4886u,actor()+0x668u,x(0));
        g[5]=pop();
        g[4]=flags.plus(g[4],0x580u,4);
    }

    void find_movement_point() {
        auto &g=abi.gp;
        g[7]=arg(1);
        g[6]=frame+0x3cu;
        g[0]=g[7];
        invoke(0x2a4074u,0xbbe00u);
        g[0]=arg(2);
        const auto z=p.r32(g[0]+8u);
        g[1]=word(0x10u);
        const auto y=p.r32(g[0]+4u);
        lane(0,p.r32(g[1]+0x20u));
        const auto x_coord=p.r32(g[0]);
        g[2]=g[3];
        w(0x2a4095u,0x34u,x(0));
        w(0x2a40b1u,0x38u,length3(fp,{x_coord,y,z}));
        invoke(0x2a40b5u,0x275e90u);

        g[2]=frame+0x48u;
        push(0x2a40beu,g[2]);
        g[2]=word(0x38u);
        g[0]=frame+0x0eu;
        push(0x2a40c7u,g[0]);
        g[1]=frame+0x17u;
        push(0x2a40ccu,g[1]);
        push(0x2a40cdu,g[2]);
        g[0]=g[6];
        push(0x2a40d0u,g[0]);
        g[0]=g[7];
        g[7]=g[3];
        invoke(0x2a40d5u,0x201a20u);
        flags.logic(g[0]&0xffu,1);
        if(!flags.zf) throw std::runtime_error("2A3EA0 reached its waypoint-only return");
        reg_byte(0,byte(0xeu));
        flags.logic(g[0]&0xffu,1);
        if(!flags.zf) throw std::runtime_error("2A3EA0 reached its explicit waypoint-direction path");
        lane(1,0);
        reg_byte(0,p.r8(actor()+0x478u));
        flags.logic(g[0]&0xffu,1);
        lane(3,word(0x1cu));
        lane(2,s.sub(0,word(0x20u)));
        g[2]=frame+0x80u;
        w(0x2a4150u,0x58u,x(2));
        w(0x2a4156u,0x5cu,x(3));
        w(0x2a415cu,0x60u,x(1));
        w(0x2a4162u,0x28u,g[2]);
        if(flags.zf) throw std::runtime_error("2A3EA0 reached the route-cluster geometry path");
        w(0x2a4168u,0x28u,0);
    }

    void compare(u32 lhs,u32 rhs) {
        const auto order=s.order(lhs,rhs);
        const bool unordered=(lhs&0x7fffffffu)>0x7f800000u || (rhs&0x7fffffffu)>0x7f800000u;
        flags.load(unordered?0x45u:order[0]?0u:order[1]?0x40u:1u);
    }

    bool reuse_candidate() {
        constexpr u32 retained_heading_refresh=0x3f6e147bu; // TU1.5 46A9C8, .rdata.
        reg_byte(0,p.r8(actor()+0x40u));
        flags.logic(abi.gp[0]&0xffu,1);
        if(!flags.zf) return false;
        reg_byte(0,p.r8(actor()+0x678u));
        flags.logic(abi.gp[0]&0xffu,1);
        if(flags.zf) return false;

        lane(0,p.r32(actor()+0x664u));
        lane(4,p.r32(actor()+0x660u));
        low(0,s.mul(x(0),word(0x20u)));
        low(4,s.mul(x(4),x(3)));
        low(0,s.add(x(0),x(4)));
        compare(x(0),x(1));
        if(!flags.cf && !flags.zf) {
            compare(x(0),retained_heading_refresh);
            if(!flags.cf && !flags.zf) return false;

            lane(0,p.r32(actor()+0x664u));
            low(0,s.mul(x(0),x(3)));
            lane(3,p.r32(actor()+0x660u));
            low(3,s.mul(x(3),x(2)));
            low(0,s.add(x(0),x(3)));
            compare(x(0),x(1));
            const bool left=!flags.cf && !flags.zf;
            reg_byte(0,left?1u:flags.logic(0,1));
            reg_byte(1,p.r8(actor()+0x654u));
            flags.logic(abi.gp[1]&0xffu,1);
            const bool backing=!flags.zf;
            lane(0,p.r32(actor()+0x658u));
            compare(x(0),x(1));
            if(!flags.cf && !flags.zf) {
                flags.logic(abi.gp[0]&0xffu,1);
                if(left!=backing) return false;
            }
            compare(x(1),p.r32(actor()+0x658u));
            if(!flags.cf && !flags.zf) {
                flags.logic(abi.gp[0]&0xffu,1);
                if(left==backing) return false;
            }
        }

        reg_byte(0,p.r8(actor()+0x654u));
        flags.logic(abi.gp[0]&0xffu,1);
        lane(0,word(0x18u));
        low(0,s.sub(x(0),p.r32(actor()+0x668u)));
        if(flags.zf) {
            low(0,s.add(x(0),p.r32(actor()+0x658u)));
            p.w32(0x2a456eu,actor()+0x658u,x(0));
        } else {
            lane(2,p.r32(actor()+0x658u));
            low(2,s.sub(x(2),x(0)));
            p.w32(0x2a42cdu,actor()+0x658u,x(2));
        }
        lane(0,p.r32(0x46a488u));
        compare(x(0),p.r32(actor()+0x658u));
        if(flags.cf || flags.zf) {
            lane(2,p.r32(actor()+0x658u));
            lane(0,p.r32(0x46a2ecu));
            compare(x(2),x(0));
            if(flags.cf || flags.zf) abi.xmm[0]=abi.xmm[2];
        }
        p.w32(0x2a459fu,actor()+0x658u,x(0));
        return true;
    }

    u32 select_candidate() {
        auto &g=abi.gp;
        const auto waypoint=byte(0xeu);
        g[0]=word(0x34u);
        reg_byte(1,flags.minus(0,waypoint,1));
        push(0x2a42fau,g[0]);
        g[2]=frame+0x48u;
        g[0]=frame+0x58u;
        g[1]=flags.borrow(g[1],g[1],4);
        g[1]=flags.logic(g[1]&g[2],4);
        g[2]=word(0x10u);
        push(0x2a430bu,g[1]);
        push(0x2a430cu,g[0]);
        g[0]=arg(1);
        g[1]=frame+0x1cu;
        push(0x2a4318u,g[1]);
        push(0x2a4319u,g[2]);
        g[2]=arg(2);
        push(0x2a4321u,g[0]);
        push(0x2a4322u,g[3]);
        g[1]=frame+0x3cu;
        invoke(0x2a4327u,0x2a2aa0u);

        const auto old_slot=goal_tu15::sx16(p.r16(actor()+0x66cu));
        reg_word(0,u32(old_slot));
        flags.logic(g[0]&0xffffu,2);
        const auto count=goal_tu15::sx16(p.r16(vehicle_candidates_tu15::count));
        reg_word(2,u32(count));
        if(old_slot>=0 && old_slot<count) {
            g[0]=u32(old_slot)*5u;
            const auto score=vehicle_candidates_tu15::rows+8u+g[0]*8u;
            lane(0,s.mul(p.r32(score),p.r32(0x46a198u)));
            p.w32(0x2a435bu,score,x(0));
        }
        w(0x2a4364u,0x30u,0);
        w8(0x2a436cu,0x0fu,0);

        lane(1,0);
        g[7]=flags.logic(0xffffffffu,4);
        g[0]=flags.logic(0,4);
        flags.logic(g[2]&0xffffu,2);
        abi.xmm[0]=abi.xmm[1];
        if(count<=0 || count>8) throw std::runtime_error("2A3EA0 has no authored geometric candidate rows");
        g[1]=vehicle_candidates_tu15::rows+8u;
        for(std::int16_t index=0;index<count;++index) {
            lane(2,p.r32(g[1]));
            if(s.above(x(2),x(0))) {
                abi.xmm[0]=abi.xmm[2];
                g[7]=g[0];
            }
            g[0]=flags.increment(g[0],4,false);
            g[1]=flags.plus(g[1],vehicle_candidates_tu15::stride,4);
            flags.minus(g[0],g[2],2);
        }
        flags.minus(g[7],0xffffu,2);
        if(flags.zf) throw std::runtime_error("2A3EA0 reached its no-positive-score return");

        g[0]=u32(goal_tu15::sx16(p.r16(frame+0x30u)));
        g[6]=u32(goal_tu15::sx16(g[7]));
        g[2]=1;
        g[1]=g[6];
        g[2]=flags.shift(g[2],g[1]&31u,4);
        flags.logic(g[0]&g[2],4);
        if(!flags.zf) throw std::runtime_error("2A3EA0 reused a checked steering slot");
        g[2]=word(0x28u);
        g[0]=word(0x34u);
        push(0x2a43e8u,g[2]);
        push(0x2a43e9u,g[0]);
        g[1]=frame+0x58u;
        push(0x2a43eeu,g[1]);
        g[1]=word(0x10u);
        g[2]=frame+0x1cu;
        push(0x2a43f7u,g[2]);
        g[0]=frame+0x3cu;
        push(0x2a43fcu,g[0]);
        g[2]=g[6]*5u;
        const auto candidate=vehicle_candidates_tu15::rows+g[2]*8u;
        g[0]=candidate;
        push(0x2a4407u,g[1]);
        push(0x2a4408u,g[0]);
        g[0]=g[3];
        invoke(0x2a440bu,0x2a3170u);
        flags.logic(g[0]&0xffu,1);
        if(flags.zf) throw std::runtime_error("2A3EA0 reached the remaining-candidate steering scan");
        w8(0x2a443bu,0x0fu,1);
        return candidate;
    }

    void publish_candidate(u32 row) {
        auto &g=abi.gp;
        g[0]=flags.shift(u32(goal_tu15::sx16(g[7]))*5u,3,4);
        reg_byte(1,p.r8(row));
        p.w8(0x2a444fu,actor()+0x654u,g[1]);
        reg_byte(2,p.r8(row+1u));
        p.w8(0x2a445bu,actor()+0x655u,g[2]);
        g[1]=p.r32(row+4u);
        p.w32(0x2a4467u,actor()+0x658u,g[1]);
        g[2]=p.r32(row+8u);
        p.w32(0x2a4473u,actor()+0x65cu,g[2]);
        g[1]=p.r32(row+0xcu);
        p.w32(0x2a447fu,actor()+0x660u,g[1]);
        g[2]=p.r32(row+0x10u);
        p.w32(0x2a448bu,actor()+0x664u,g[2]);
        reg_word(0,p.r16(row+0x18u));
        p.w16(0x2a4498u,actor()+0x66cu,g[0]);
        reg_byte(0,byte(0x0fu));
        flags.logic(g[0]&0xffu,1);
        p.w8(0x2a44a5u,actor()+0x678u,1);
        if(flags.zf) throw std::runtime_error("2A3EA0 lost its accepted steering slot");
        lane(1,0);
    }

    bool reverse_steering() {
        auto &g=abi.gp;
        reg_byte(0,byte(0x0eu));
        flags.logic(g[0]&0xffu,1);
        abi.xmm[0]=abi.xmm[1];
        if(!flags.zf) throw std::runtime_error("2A3EA0 reached the waypoint-angle branch");
        g[7]=word(0x10u);
        reg_byte(0,p.r8(actor()+0x654u));
        flags.logic(g[0]&0xffu,1);
        lane(3,s.mul(p.r32(g[7]+0x30u),p.r32(0x46a164u)));
        lane(2,p.r32(g[7]+0x38u));
        return !flags.zf;
    }

    void steer(bool reverse_direction) {
        auto &g=abi.gp;
        const auto wanted=p.r32(actor()+0x658u);
        const auto negative_angle=s.above(x(1),wanted);
        low(0,reverse_direction?s.add(x(0),wanted):s.sub(x(0),wanted));
        low(0,s.mul(x(0),x(2)));
        w(reverse_direction?0x2a4547u:0x2a45bfu,0x10u,x(0));
        if(negative_angle) throw std::runtime_error("2A3EA0 reached its negative velocity angle");
        abi.xmm[2]=abi.xmm[1];
        low(2,s.sub(x(2),x(3)));
        if(s.above(x(2),x(0)) || s.above(x(0),x(3)))
            throw std::runtime_error("2A3EA0 reached the steering limit clamp");

        const auto angle=word(0x10u);
        g[6]=arg(3);
        g[0]=g[6];
        p.w32(0x2a4608u,g[6]+8u,x(1));
        const auto facing=vehicle_candidates_tu15::rotate_candidates(fp,{word(0x1cu),word(0x20u)},angle,false);
        p.w32(0x2a4621u,g[6],facing.x);
        p.w32(0x2a4631u,g[6]+4u,facing.y);
        invoke(0x2a4634u,0x30bd0u);
        lane(3,0);
        if((x(0)&0x7fffffffu)==0)
            throw std::runtime_error("2A3EA0 reached its zero steering direction");
        lane(0,p.r32(actor()+0x658u));
        if(!s.above_equal(x(0),x(3)))
            throw std::runtime_error("2A3EA0 reached its reverse movement scale");

        g[0]=arg(2);
        g[1]=p.r32(g[0]);
        g[2]=p.r32(g[0]+4u);
        g[0]=frame+0x28u;
        w(0x2a467eu,0x28u,g[1]);
        w(0x2a4682u,0x2cu,g[2]);
        invoke(0x2a4686u,0x2b3e0u);
        g[0]=word(0x38u);
        push(0x2a468fu,0);
        push(0x2a4691u,g[0]);
        reg_byte(0,flags.minus(0,byte(0x0eu),1));
        g[1]=frame+0x48u;
        g[2]=frame+0x1cu;
        g[0]=flags.borrow(g[0],g[0],4);
        g[0]=flags.logic(g[0]&g[1],4);
        push(0x2a46a4u,g[0]);
        push(0x2a46a5u,g[2]);
        push(0x2a46a6u,g[6]);
        g[0]=frame+0x28u;
        push(0x2a46abu,g[0]);
        g[0]=arg(1);
        g[1]=frame+0x3cu;
        push(0x2a46b7u,g[1]);
        g[1]=g[3];
        invoke(0x2a46bau,0x202130u);
        lane(5,p.r32(0x46a0dcu));
        lane(3,0);
    }

    void publish_motion() {
        auto &g=abi.gp;
        g[0]=word(0x54u);
        lane(2,s.mul(p.r32(g[0]+0x8cu),word(0x20u)));
        lane(1,s.mul(p.r32(g[0]+0x90u),word(0x24u)));
        low(2,s.add(x(2),x(1)));
        lane(1,s.mul(p.r32(g[0]+0x88u),word(0x1cu)));
        low(2,s.add(x(2),x(1)));
        lane(1,p.r32(g[7]+0x68u));
        if(!s.above(x(1),x(3))) throw std::runtime_error("2A3EA0 reached the alternate speed scale");
        lane(4,p.r32(g[7]+0x6cu));
        low(2,s.div(x(2),x(1)));
        abi.xmm[1]=abi.xmm[0];
        low(1,s.sub(x(1),x(2)));
        low(1,s.mul(x(1),x(4)));
        low(1,s.add(x(1),x(0)));
        if(s.above(x(1),x(3))) {
            lane(0,p.r32(g[7]+0x60u));
            if(s.above(x(0),x(3))) {
                low(1,s.sub(x(1),p.r32(actor()+0x674u)));
                abi.xmm[0]=abi.xmm[1];
                const auto cap=p.r32(g[7]+0x60u);
                if(s.above(x(0),cap)) lane(0,p.r32(g[7]+0x60u));
                lane(1,s.add(p.r32(actor()+0x674u),x(0)));
            }
        }
        const bool negative=s.above(x(3),x(1));
        abi.xmm[0]=negative?abi.xmm[3]:abi.xmm[1];
        g[0]=arg(5);
        p.w32(0x2a4784u,actor()+0x670u,x(0));
        p.w32(0x2a478cu,g[0],x(1));
        const auto property_limit=p.r32(g[7]+0x50u);
        if(s.above(x(1),property_limit)) {
            lane(1,p.r32(g[7]+0x50u));
        } else {
            low(3,s.sub(x(3),p.r32(g[7]+0x50u)));
            if(s.above(x(3),x(1))) abi.xmm[1]=abi.xmm[3];
        }
        lane(0,p.r32(0x46a0d0u));
        const bool upper_limit=s.above(x(1),x(0));
        p.w32(0x2a47b5u,g[0],x(1));
        if(!upper_limit && s.above(x(5),x(1))) abi.xmm[1]=abi.xmm[5];
        g[2]=arg(6);
        g[1]=arg(4);
        if(upper_limit) abi.xmm[1]=abi.xmm[0];
        lane(0,word(0x18u));
        // The +1 limit has its own source store sites and epilogue.
        p.w32(upper_limit?0x2a47d2u:0x2a480eu,g[0],x(1));
        g[7]=pop();
        p.w8(upper_limit?0x2a47d7u:0x2a4813u,g[2],0);
        p.w16(upper_limit?0x2a47dau:0x2a4816u,g[1],0);
        g[6]=pop();
        p.w32(upper_limit?0x2a47e0u:0x2a481cu,actor()+0x668u,x(0));
        g[5]=pop();
        g[4]=flags.plus(g[4],0x580u,4);
    }

public:
    Driver(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &state,
           const Owner &child,const SourcePhysicsContactRecordObserver &observer):
        p{memory,observer},abi(state),owner(child),observe(observer),
        fp{state.floating_point->x87_control_word,state.x87_status,state.floating_point->mxcsr},s(fp),
        entry(state.gp[4]),frame(entry-0x58cu) {
        flags.load(state.eflags);
    }
    u32 run() {
        try {
            resolve_property();
            if(facing()) {
                find_movement_point();
                if(!reuse_candidate()) publish_candidate(select_candidate());
                steer(reverse_steering());
                publish_motion();
            } else publish_inactive_motion();
            const auto resumed=p.r32(abi.gp[4]);
            abi.gp[4]+=0x24u;
            publish();
            return resumed;
        }catch(...) {
            publish();
            throw;
        }
    }
};

u32 run(SourcePhysicsContactMemory &memory,SourcePhysicsWorldRuntimeAbi &state,
                const Owner &child,const SourcePhysicsContactRecordObserver &observer) {
    if(!state.floating_point || state.x87_depth || state.active_mmx || !child)
        throw std::invalid_argument("Retained vehicle movement requires its real FP frame and child owners");
    return Driver(memory,state,child,observer).run();
}
#else
std::uint32_t run(SourcePhysicsContactMemory &,SourcePhysicsWorldRuntimeAbi &,
                  const SourcePhysicsWorldRuntimeServices::RequiredOwner &,
                  const SourcePhysicsContactRecordObserver &) {
    throw std::runtime_error("Exact native vehicle movement backend unavailable");
}
#endif
} // namespace halo2::vehicle_movement_tu15

namespace halo2::vehicle_movement_tu15 {
bool admits(const SourcePhysicsContactMemory &memory, const SourcePhysicsContactRuntimeCall &call,
                   const SourcePhysicsWorldRuntimeAbi &abi) {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
    if(call.tail || call.instruction!=0x1fbd03u || call.target!=0x2a3ea0u ||
       !abi.floating_point || abi.floating_point->x87_control_word!=0x27fu ||
       (abi.floating_point->mxcsr&0xffff1f80u)!=0x1f80u ||
       abi.x87_depth || abi.active_mmx || (abi.x87_status&0x8080u))
        return false;

    const auto &gp=abi.gp;
    const auto sp=gp[4], actor=gp[6], datum=gp[3], vehicle=gp[1];
    if(sp<0x60001000u || sp>0x600fffdcu || actor>0xfffff777u ||
       gp[0]!=actor+0x5ecu || gp[2]!=actor+0x5d6u || gp[7]!=actor+0x6d4u ||
       datum==0xffffffffu || vehicle==0xffffffffu)
        return false;
    const auto frame=memory.try_read_view(sp,0x24u);
    const auto row=memory.try_read_view(actor,0x888u);
    const auto pool=memory.try_read_view(0x5020c8u,4u);
    if(!frame || !row || !pool || !memory.try_write_extent(sp-0x1000u,0x1000u) ||
       !memory.try_write_extent(actor,0x888u))
        return false;
    const auto word=[](std::span<const std::byte> data,std::size_t offset=0) {
        std::uint32_t value;
        std::memcpy(&value,data.data()+offset,sizeof(value));
        return value;
    };
    if(word(*frame)!=0x1fbd08u || word(*frame,4u)!=vehicle ||
       word(*frame,8u)!=gp[0] || word(*frame,0x0cu)!=gp[7] ||
       word(*frame,0x10u)!=gp[2] || word(*frame,0x14u)!=actor+0x814u ||
       word(*frame,0x18u)!=actor+0x5d3u || word(*frame,0x1cu)!=actor+0x5d2u ||
       word(*frame,0x20u)!=gp[5])
        return false;
    const auto bytes=word(*pool);
    if(bytes>0xffffffbbu)
        return false;
    const auto rows=memory.try_read_view(bytes+0x44u,4u);
    if(!rows || std::uint64_t(word(*rows))+(datum&0xffffu)*0x888ull!=actor)
        return false;

    std::uint16_t salt,mode;
    std::memcpy(&salt,row->data(),2);
    std::memcpy(&mode,row->data()+0x270u,2);
    return salt==std::uint16_t(datum>>16u) && mode==5u &&
           word(*row,0x26cu)==vehicle &&
           (*row)[0x5d2u]==std::byte{} &&
           ((*row)[0x5d0u]==std::byte{} || (*row)[0x478u]!=std::byte{});
#else
    (void)memory;
    (void)call;
    (void)abi;
    return false;
#endif
}
} // namespace halo2::vehicle_movement_tu15
