#include "halo2/native_campaign_ai_vehicle_movement_tu15.hpp"
#include "campaign/ai/native_campaign_ai_goal_port_tu15.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include <cstring>

namespace halo2::vehicle_steering_tu15::sector_frame {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
using goal_tu15::Port;
using goal_tu15::u32;
using physics_contact_runtime_detail::Flags;

inline void run(SourcePhysicsContactMemory &memory, SourcePhysicsWorldRuntimeAbi &abi,
                const SourcePhysicsContactRecordObserver &observe) {
    const Port p{memory,observe};
    auto &[ax,cx,dx,bx,sp,bp,si,di]=abi.gp;
    Flags flags;
    flags.load(abi.eflags);
    const auto push=[&](u32 pc,u32 value) {
        const auto at=sp-4u;
        p.w32(pc,at,value);
        sp=at;
    };
    const auto pop=[&](u32 &value) {
        value=p.r32(sp);
        sp+=4u;
    };
    const auto publish=[&] { abi.eflags=(abi.eflags & ~0x8d5u)|flags.bits(); };
    const auto finish=[&](unsigned saved) {
        if(saved>=4) pop(di);
        if(saved>=3) pop(si);
        if(saved>=2) pop(bx);
        pop(bp);
        (void)p.r32(sp);
        sp+=8u;
        publish();
    };
    const auto nonpositive=[&](u32 value) { flags.logic(value,4);return flags.zf || flags.sf; };
    const auto below_signed=[&](u32 left,u32 right) {
        flags.minus(left,right,4);
        return flags.sf!=flags.of;
    };
    const auto contained=[&](u32 bounds) {
        if(below_signed(bp,p.r32(bounds+4u))) return false;
        flags.minus(bp,p.r32(bounds+8u),4);
        return flags.zf || flags.sf!=flags.of;
    };
    try {
        cx=p.r32(0x4ecc18u);
        dx=p.r32(cx+0xc4u);
        ax=flags.logic(0xffffffffu,4);
        const bool absent=nonpositive(dx);
        push(0x218b81u,bp);
        bp=p.r32(sp+8u);
        if(absent) { finish(1);return; }
        dx=p.r32(cx+0xc8u);
        if(!flags.logic(dx,4)) { finish(1);return; }
        flags.logic(bp,4);
        if(flags.sf || !below_signed(bp,p.r32(dx))) { finish(1);return; }
        cx=p.r32(dx+4u);
        push(0x218badu,bx);
        if(!flags.logic(p.r8(cx+bp*8u)&4u,1)) { finish(2);return; }
        bx=p.r32(dx+0x30u);
        push(0x218bb7u,si);
        si=flags.logic(0,4);
        if(nonpositive(bx)) { finish(3);return; }
        push(0x218bbeu,di);
        di=p.r32(dx+0x34u);
        cx=flags.logic(0,4);
        for(;;) {
            cx=flags.plus(cx*0x1cu,di,4);
            dx=cx;
            if(flags.logic(p.r8(dx)&1u,1) && contained(dx)) break;
            si=flags.increment(si,4,false);
            cx=u32(goal_tu15::sx16(si));
            if(!below_signed(cx,bx)) { finish(4);return; }
        }
        bx=p.r32(dx+0xcu);
        si=flags.logic(0,4);
        if(nonpositive(bx)) { finish(4);return; }
        di=p.r32(dx+0x10u);
        cx=flags.logic(0,4);
        for(;;) {
            cx=flags.plus(flags.shift(cx,4,4),di,4);
            if(contained(cx)) break;
            si=flags.increment(si,4,false);
            cx=u32(goal_tu15::sx16(si));
            if(!below_signed(cx,bx)) { finish(4);return; }
        }
        cx=(cx&0xffff0000u)|p.r16(cx+0xcu);
        flags.logic(cx&0xffffu,2);
        if(!flags.sf) {
            si=p.r32(dx+0x14u);
            cx=u32(goal_tu15::sx16(cx));
            if(below_signed(cx,si)) {
                dx=p.r32(dx+0x18u);
                ax=(ax&0xffff0000u)|p.r16(dx+cx*4u);
            }
        }
        finish(4);
    } catch(...) {
        publish();
        throw;
    }
}
#endif
} // namespace halo2::vehicle_steering_tu15::sector_frame

namespace halo2::vehicle_steering_tu15 {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
using goal_tu15::Port;
using goal_tu15::Simd;
using goal_tu15::u32;
using physics_contact_runtime_detail::Flags;
using RequiredOwner = SourcePhysicsWorldRuntimeServices::RequiredOwner;

struct Vec3Words {
    u32 x, y, z;
};

inline u32 subtract_and_spill(NativeTu15PhysicsFrameFp &fp, u32 left, u32 right) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    u32 result;
    asm volatile("flds %1\n\tfsubs %2\n\tfstps %0"
        : "=m"(result) : "m"(left), "m"(right) : "memory", "st");
    return result;
}

inline std::uint64_t angle_steps(NativeTu15PhysicsFrameFp &fp, u32 angle, u32 scale) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    std::uint64_t result;
    asm volatile("flds %1\n\tfabs\n\tfmuls %2\n\tfstpl %0"
        : "=m"(result) : "m"(angle), "m"(scale) : "memory", "st");
    return result;
}

struct Length {
    u32 value;
    bool below_epsilon;
};

inline Length normalizer_length(NativeTu15PhysicsFrameFp &fp, Vec3Words components, u32 epsilon) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    u32 length;
    unsigned char below;
    asm volatile("flds %[x]\n\tfmuls %[x]\n\tflds %[z]\n\tfmuls %[z]\n\tfaddp"
                 "\n\tflds %[y]\n\tfmuls %[y]\n\tfaddp\n\tfsqrt\n\tfsts %[length]"
                 "\n\tfabs\n\tflds %[epsilon]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[below]"
        : [length] "=m"(length), [below] "=q"(below)
        : [x] "m"(components.x), [y] "m"(components.y), [z] "m"(components.z), [epsilon] "m"(epsilon)
        : "memory", "cc", "st", "st(1)");
    return {length, below != 0};
}

inline u32 endpoint_distance(NativeTu15PhysicsFrameFp &fp, Vec3Words a, Vec3Words b) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    u32 result;
    // Preserve the original association and extended subtraction through the square root.
    asm volatile("flds %[ax]\n\tfsubs %[bx]\n\tflds %[ay]\n\tfsubs %[by]\n\tflds %[az]\n\tfsubs %[bz]"
                 "\n\tfld %%st(0)\n\tfmul %%st(1)\n\tfld %%st(2)\n\tfmul %%st(3)\n\tfaddp"
                 "\n\tfld %%st(3)\n\tfmul %%st(4)\n\tfaddp\n\tfsqrt"
                 "\n\tfstp %%st(3)\n\tfstp %%st(0)\n\tfstp %%st(0)\n\tfstps %[out]"
        : [out] "=m"(result)
        : [ax] "m"(a.x), [ay] "m"(a.y), [az] "m"(a.z), [bx] "m"(b.x), [by] "m"(b.y), [bz] "m"(b.z)
        : "memory", "st", "st(1)", "st(2)", "st(3)", "st(4)");
    return result;
}

inline u32 rotation_component(NativeTu15PhysicsFrameFp &fp, u32 angle, bool cosine) {
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    u32 result;
    if (cosine) {
        asm volatile("flds %1\n\tfcos\n\tfstps %0"
            : "=m"(result) : "m"(angle) : "memory", "st");
    } else {
        asm volatile("flds %1\n\tfsin\n\tfstps %0"
            : "=m"(result) : "m"(angle) : "memory", "st");
    }
    return result;
}

struct SweepSites {
    u32 exclusions, maximum, direction, target, origin, navigation, call;
};

class SteeringTest {
    Port p;
    SourcePhysicsWorldRuntimeAbi &abi;
    const RequiredOwner &owner;
    const SourcePhysicsContactRecordObserver &observe;
    NativeTu15PhysicsFrameFp fp;
    Simd s;
    Flags flags;
    const u32 entry;
    const u32 frame;
    const u32 candidate;
    u32 actor{};

    void state() {
        abi.floating_point = SourcePhysicsFactoryFloatingPoint{fp.control_word, fp.mxcsr};
        abi.x87_status = fp.status;
        abi.eflags = (abi.eflags & ~0x8d5u) | flags.bits();
    }

    void restore_state() {
        const auto control = abi.floating_point.value();
        fp = {control.x87_control_word, abi.x87_status, control.mxcsr};
        flags.load(abi.eflags);
    }

    u32 word(u32 offset) const { return p.r32(frame + offset); }
    u32 byte(u32 offset) const { return p.r8(frame + offset); }
    u32 x(unsigned reg) const { return abi.xmm[reg][0]; }

    void lane(unsigned reg, u32 value) { abi.xmm[reg] = {value}; }
    void low(unsigned reg, u32 value) { abi.xmm[reg][0] = value; }
    void byte_register(unsigned reg, u32 value) { abi.gp[reg] = (abi.gp[reg] & ~0xffu) | (value & 0xffu); }

    void w(u32 site, u32 offset, u32 value) { p.w32(site, frame + offset, value); }
    void w8(u32 site, u32 offset, u32 value) { p.w8(site, frame + offset, value); }

    Vec3Words vector_at(u32 address) const {
        return {p.r32(address), p.r32(address + 4u), p.r32(address + 8u)};
    }

    void push(u32 site, u32 value) {
        const auto destination = abi.gp[4] - 4u;
        p.w32(site, destination, value);
        abi.gp[4] = destination;
    }

    u32 pop() {
        const auto value = p.r32(abi.gp[4]);
        abi.gp[4] += 4u;
        return value;
    }

    void call(u32 site, u32 target, u32 expected_sp) {
        push(site, site + 5u);
        state();
        if (target == 0x218b70u) {
            sector_frame::run(p.memory,abi,observe);
        } else owner(p.memory, {site, target, false}, abi, observe);
        restore_state();
        if (abi.gp[4] != expected_sp)
            throw std::runtime_error("2A3170 child lost its authenticated return frame");
    }

    void start() {
        auto &g = abi.gp;
        g[4] = flags.minus(entry, 0x114u, 4);
        g[1] = p.r32(entry + 0x14u);
        g[2] = p.r32(entry + 0x0cu);
        g[0] = (g[0] & 0xffffu) * 0x888u;
        push(0x2a318fu, g[3]);
        push(0x2a3190u, g[5]);
        push(0x2a3191u, g[6]);
        g[6] = p.r32(0x5020c8u);
        actor = flags.plus(g[0], p.r32(g[6] + 0x44u), 4);
        w(0x2a319bu, 0x6cu, actor);
        g[0] = p.r32(entry + 4u);
        flags.minus(p.r8(g[0]), 0, 1);
        push(0x2a31a9u, g[7]);
        w8(0x2a31aau, 0x13u, 1);
        if (!flags.zf) {
            lane(1,word(0x13cu));
            lane(0,p.r32(g[1]));
            low(0,s.mul(x(0),x(1)));
            low(0,s.add(x(0),p.r32(g[2])));
            w(0x2a31c6u,0x50u,x(0));
            lane(0,p.r32(g[1]+4u));
            low(0,s.mul(x(0),x(1)));
            low(0,s.add(x(0),p.r32(g[2]+4u)));
            w(0x2a31dau,0x54u,x(0));
        } else {
            lane(0, s.sub(0, word(0x13cu)));
            lane(1, p.r32(g[1]));
            low(1, s.add(s.mul(x(1), x(0)), p.r32(g[2])));
            w(0x2a31fau, 0x50u, x(1));
            lane(1, p.r32(g[1] + 4u));
            low(1, s.add(s.mul(x(1), x(0)), p.r32(g[2] + 4u)));
            w(0x2a320eu, 0x54u, x(1));
        }

        g[5] = actor;
        g[1] = word(0x12cu);
        g[2] = p.r32(actor + 0x28cu);
        lane(0, p.r32(g[1] + 0x14u));
        push(0x2a322au, g[2]);
        w(0x2a322bu, 0x4cu, x(0));
        call(0x2a3231u, 0x218b70u, frame);
        g[3] = word(0x134u);
        g[7] = frame + 0x94u;
        g[5] = flags.plus(actor, 0x27cu, 4);
        g[6] = g[0];
        call(0x2a324cu, 0x218a40u, frame);
        g[3] = word(0x138u);
        g[7] = frame + 0x18u;
        g[0] = g[6];
        call(0x2a325eu, 0x218a40u, frame);

        g[0] = candidate;
        flags.minus(p.r8(candidate), 0, 1);
        if (!flags.zf) {
            lane(1,word(0x13cu));
            lane(0,word(0x18u));
            low(0,s.mul(x(0),x(1)));
            low(0,s.add(x(0),p.r32(g[5])));
            w(0x2a3287u,0x94u,x(0));
            lane(0,word(0x1cu));
            low(0,s.mul(x(0),x(1)));
            low(0,s.add(x(0),p.r32(g[5]+4u)));
            w(0x2a329fu,0x98u,x(0));
        } else {
            lane(0, s.sub(0, word(0x13cu)));
            lane(1, s.add(s.mul(word(0x18u), x(0)), p.r32(g[5])));
            w(0x2a32c5u, 0x94u, x(1));
            lane(1, s.add(s.mul(word(0x1cu), x(0)), p.r32(g[5] + 4u)));
            w(0x2a32ddu, 0x98u, x(1));
        }
        lane(0, p.r32(0x46a0dcu));
        g[1] = actor;
        g[0] = p.r32(actor + 0x28cu);
        flags.minus(g[0], 0xffffffffu, 4);
        lane(1, s.mul(word(0x18u), x(0)));
        w(0x2a3305u, 0x58u, x(1));
        lane(1, s.mul(word(0x1cu), x(0)));
        w(0x2a3315u, 0x5cu, x(1));
        lane(1, s.mul(word(0x20u), x(0)));
        w(0x2a3325u, 0x60u, x(1));
        w8(0x2a332bu, 0x11u, 0);
        w8(0x2a3330u, 0x12u, 0);
        w(0x2a3335u, 0x34u, g[0]);
        if (flags.zf) throw std::runtime_error("2A3170 reached its absent-sector return");
    }

    u32 navigation(u32 base) {
        flags.logic(p.r32(base + 0xc4u), 4);
        return flags.zf || flags.sf ? 0 : p.r32(base + 0xc8u);
    }

    void sweep(SweepSites sites, u32 nav, u32 position, u32 sector, u32 direction,
               u32 distance, u32 result) {
        // Keep all six persistent stack writes and the caller's actual source return.
        push(sites.exclusions, 0);
        push(sites.maximum, distance);
        push(sites.direction, direction);
        push(sites.target, 0xffffffffu);
        push(sites.origin, position);
        push(sites.navigation, nav);
        abi.gp[0] = sector;
        abi.gp[6] = result;
        call(sites.call, 0x2762a0u, frame);
    }

    void entrance_sweeps() {
        auto &g = abi.gp;
        g[3] = p.r32(0x4ecc18u);
        g[1] = p.r32(g[3] + 0xc4u);
        const auto nav = navigation(g[3]);
        g[7] = word(0x4cu);
        g[2] = frame + 0x18u;
        sweep({0x2a335bu,0x2a335du,0x2a3362u,0x2a3363u,0x2a3365u,0x2a3366u,0x2a3372u},
              nav,g[5],word(0x34u),g[2],g[7],frame + 0xb8u);
        g[0] = word(0xc8u);
        g[1] = p.r32(g[3] + 0xc4u);
        w(0x2a3384u, 0x40u, g[0]);
        const auto again = navigation(g[3]);
        g[1] = frame + 0x58u;
        sweep({0x2a3394u,0x2a3396u,0x2a339bu,0x2a339cu,0x2a339eu,0x2a339fu,0x2a33abu},
              again,g[5],word(0x34u),g[1],g[7],frame + 0xdcu);
    }

    std::int16_t prepare_sampling() {
        auto &g = abi.gp;
        g[0] = candidate;
        flags.minus(p.r8(candidate), 0, 1);
        g[2] = word(0xecu);
        lane(7, word(0xf4u));
        lane(6, word(0xd0u));
        w(0x2a33d3u, 0x24u, g[2]);
        u32 direction_x, direction_y;
        if (!flags.zf) {
            lane(5,word(0x13cu));
            lane(3,0);
            abi.xmm[0]=abi.xmm[5];
            low(0,s.sub(x(0),x(6)));
            abi.xmm[2]=abi.xmm[3];
            low(2,s.sub(x(2),x(0)));
            lane(0,word(0x18u));
            direction_x=x(0);
            abi.xmm[1]=abi.xmm[0];
            low(1,s.mul(x(1),x(2)));
            w(0x2a3404u,0x2cu,x(1));
            lane(1,word(0x1cu));
            direction_y=x(1);
            abi.xmm[4]=abi.xmm[1];
            low(4,s.mul(x(4),x(2)));
            w(0x2a3417u,0x30u,x(4));
            abi.xmm[4]=abi.xmm[7];
            low(4,s.add(x(4),x(5)));
            abi.xmm[2]=abi.xmm[3];
            low(2,s.sub(x(2),x(4)));
            abi.xmm[4]=abi.xmm[0];
            low(4,s.mul(x(4),x(2)));
            w(0x2a3432u,0x44u,x(4));
            abi.xmm[4]=abi.xmm[1];
            low(4,s.mul(x(4),x(2)));
            w(0x2a343fu,0x48u,x(4));
            abi.xmm[2]=abi.xmm[3];
            low(2,s.sub(x(2),x(5)));
            abi.xmm[4]=abi.xmm[0];
            low(4,s.mul(x(4),x(2)));
            w(0x2a3453u,0x64u,x(4));
            abi.xmm[4]=abi.xmm[1];
            low(4,s.mul(x(4),x(2)));
            w(0x2a3460u,0x68u,x(4));
        } else {
            lane(2,word(0x13cu));
            lane(4,word(0x18u));
            lane(5,word(0x1cu));
            direction_x=x(4);
            direction_y=x(5);
            const auto front = s.add(x(6),x(2));
            w(0x2a348bu,0x2cu,s.mul(x(4),front));
            const auto forward_y = s.mul(x(5),front);
            const auto back = s.sub(x(2),x(7));
            w(0x2a34a6u,0x44u,s.mul(x(4),back));
            w(0x2a34b3u,0x48u,s.mul(x(5),back));
            w(0x2a34c0u,0x64u,s.mul(x(4),x(2)));
            const auto center_y=s.mul(x(5),x(2));
            w(0x2a34cdu,0x30u,forward_y);
            w(0x2a34d6u,0x68u,center_y);
        }

        const auto angle=p.r32(candidate+4u);
        const auto origin=p.r32(g[5]);
        w(0x2a34f3u,0x7cu,s.add(s.mul(direction_x,x(6)),origin));
        g[1]=g[5];
        g[2]=p.r32(g[1]);
        const auto first_y=s.add(s.mul(direction_y,x(6)),p.r32(g[5]+4u));
        lane(3,s.sub(0,x(7)));
        const auto doubled=angle_steps(fp,angle,p.r32(0x46a5f4u));
        w(0x2a3513u,0x70u,g[2]);
        g[2]=p.r32(g[1]+4u);
        g[1]=p.r32(g[1]+8u);
        const auto offset_x=s.add(s.mul(direction_x,x(3)),p.r32(g[5]));
        w(0x2a3526u,0x80u,first_y);
        lane(2,p.r32(g[5]+8u));
        w(0x2a3534u,0x88u,offset_x);
        lane(1,s.add(s.mul(direction_y,x(3)),p.r32(g[5]+4u)));
        g[4]=flags.minus(frame,8u,4);
        abi.xmm[0]=abi.xmm[2];
        physics_step_detail::w(p.memory, observe, 0x2a354cu, g[4],8u,doubled);
        w(0x2a354fu,0x74u,g[2]);
        w(0x2a3553u,0x78u,g[1]);
        w(0x2a355au,0x84u,x(2));
        w(0x2a3563u,0x8cu,x(1));
        w(0x2a356cu,0x90u,x(0));
        call(0x2a3575u,0x32b0c6u,frame-8u);
        g[4]=flags.plus(g[4],8u,4);
        call(0x2a357du,0x32cc0cu,frame);
        g[5]=1;
        flags.minus(g[0],g[5],2);
        w(0x2a358au,0x3cu,g[0]);
        return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(g[0]));
    }

    struct EndpointSites { u32 x, y, z; };
    void rotated_endpoint(u32 vector_x, u32 vector_y, u32 z,
                          u32 destination, EndpointSites sites, bool first) {
        const auto dx = word(vector_x);
        const auto dy = word(vector_y);
        lane(1,dy);
        lane(0,dx);
        if (first) {
            lane(5,dy);
            lane(4,dx);
        } else {
            lane(7,dy);
            lane(6,dx);
        }
        low(0,s.mul(x(0),x(3)));
        if (first) {
            low(1,s.mul(x(1),x(3)));
            low(4,s.mul(x(4),x(2)));
            low(5,s.mul(x(5),x(2)));
            low(0,s.sub(x(0),x(5)));
            lane(5,word(0x98u));
            low(1,s.add(x(1),x(4)));
            lane(4,word(0x94u));
        } else {
            low(7,s.mul(x(7),x(2)));
            low(0,s.sub(x(0),x(7)));
            low(1,s.mul(x(1),x(3)));
        }
        low(0,s.add(x(0),x(4)));
        if (!first) {
            low(6,s.mul(x(6),x(2)));
            low(1,s.add(x(1),x(6)));
        }
        w(sites.x,destination,x(0));
        lane(0,p.r32(z));
        low(1,s.add(x(1),x(5)));
        w(sites.y,destination+4u,x(1));
        w(sites.z,destination+8u,x(0));
    }

    void create_sample(std::int16_t step_limit) {
        auto &g = abi.gp;
        lane(0,0);
        if (s.above(0,p.r32(candidate+4u)))
            throw std::runtime_error("2A3170 reached its signed-angle sampling branch");
        g[0]=u32(std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(g[5])));
        const auto step_word=std::bit_cast<u32>(static_cast<float>(g[0]));
        lane(0,s.mul(step_word,p.r32(0x46a550u)));
        w(0x2a35d1u,0x14u,x(0));
        flags.minus(g[5],u32(std::uint16_t(step_limit)),2);
        if (flags.zf) {
            lane(0,p.r32(candidate+4u));
            w(0x2a35e3u,0x14u,x(0));
        }
        flags.minus(p.r8(candidate),0,1);
        if (flags.zf) {
            low(0,s.mul(x(0),p.r32(0x46a0dcu)));
            w(0x2a35f6u,0x14u,x(0));
        }
        byte_register(0,byte(0x12u));
        flags.logic(g[0]&0xffu,1);
        const auto sine=rotation_component(fp,word(0x14u),false);
        g[1]=word(0x130u);
        w(0x2a360fu,0x28u,sine);
        const auto cosine=rotation_component(fp,word(0x14u),true);
        lane(2,word(0x28u));
        w(0x2a361fu,0x38u,cosine);
        lane(3,word(0x38u));
        if (!flags.zf) throw std::runtime_error("2A3170 reached its obstructed start sample");
        const auto z=g[1]+8u;
        rotated_endpoint(0x2cu,0x30u,z,0xa0u,{0x2a366bu,0x2a367du,0x2a3686u},true);
        byte_register(2,byte(0x11u));
        flags.logic(g[2]&0xffu,1);
        rotated_endpoint(0x64u,0x68u,z,0xacu,{0x2a36d7u,0x2a36e9u,0x2a36f2u},false);
        if (!flags.zf) throw std::runtime_error("2A3170 reached its obstructed end sample");
        rotated_endpoint(0x44u,0x48u,z,0x58u,{0x2a372bu,0x2a373au,0x2a3740u},false);

        w(0x2a3751u,0x18u,subtract_and_spill(fp,word(0xacu),word(0x70u)));
        w(0x2a3760u,0x1cu,subtract_and_spill(fp,word(0xb0u),word(0x74u)));
        w(0x2a376fu,0x20u,subtract_and_spill(fp,word(0xb4u),word(0x78u)));
        const auto delta=vector_at(frame+0x18u);
        const auto length=normalizer_length(fp,delta,p.r32(0x46a0ecu));
        w(0x2a3791u,0x28u,length.value);
        if (length.below_epsilon) throw std::runtime_error("2A3170 reached the short steering segment path");
        lane(0,s.div(p.r32(0x46a0d0u),word(0x28u)));
        lane(1,s.mul(x(0),word(0x18u)));
        w(0x2a37bau,0x18u,x(1));
        lane(1,s.mul(word(0x1cu),x(0)));
        w(0x2a37cau,0x1cu,x(1));
        lane(1,s.mul(word(0x20u),x(0)));
        w(0x2a37dau,0x20u,x(1));
    }

    void sample_sweeps() {
        auto &g=abi.gp;
        flags.logic(g[0]&0xffu,1);
        g[3]=p.r32(0x4ecc18u);
        if (!flags.zf) throw std::runtime_error("2A3170 reached its alternate interrupted sweep");
        g[1]=p.r32(g[3]+0xc4u);
        const auto entrance=endpoint_distance(fp,vector_at(frame+0x7cu),vector_at(frame+0xa0u));
        w(0x2a3842u,0x38u,entrance);
        const auto nav=navigation(g[3]);
        g[1]=frame+0x7cu;
        g[2]=frame+0x18u;
        sweep({0x2a3852u,0x2a3854u,0x2a3859u,0x2a385au,0x2a3863u,0x2a3864u,0x2a3870u},
              nav,g[1],word(0x40u),g[2],word(0x38u),frame+0xb8u);
        flags.logic(g[0]&0xffu,1);
        w8(0x2a3877u,0x12u,g[0]);
        if (!flags.zf) throw std::runtime_error("2A3170 reached its first collision branch");
        g[2]=word(0xc8u);
        w(0x2a3884u,0x40u,g[2]);

        const auto center_nav=navigation(g[3]);
        g[1]=frame+0x70u;
        g[2]=frame+0x18u;
        sweep({0x2a38b2u,0x2a38b4u,0x2a38b9u,0x2a38bau,0x2a38c3u,0x2a38c4u,0x2a38d0u},
              center_nav,g[1],word(0x34u),g[2],word(0x28u),frame+0x100u);
        byte_register(3,g[0]);
        flags.logic(g[3]&0xffu,1);
        if (!flags.zf) throw std::runtime_error("2A3170 reached its center collision branch");
        g[2]=word(0x110u);
        w(0x2a38e2u,0x34u,g[2]);
        byte_register(0,byte(0x11u));
        flags.logic(g[0]&0xffu,1);
        if (!flags.zf) throw std::runtime_error("2A3170 reached its obstructed exit branch");

        g[1]=p.r32(0x4ecc18u);
        g[2]=p.r32(g[1]+0xc4u);
        const auto leaving=endpoint_distance(fp,vector_at(frame+0x88u),vector_at(frame+0x58u));
        w(0x2a393bu,0x38u,leaving);
        const auto exit_nav=navigation(g[1]);
        g[1]=frame+0x88u;
        g[2]=frame+0x18u;
        sweep({0x2a394bu,0x2a394du,0x2a3952u,0x2a3953u,0x2a395cu,0x2a395du,0x2a3969u},
              exit_nav,g[1],word(0x24u),g[2],word(0x38u),frame+0xdcu);
        flags.logic(g[0]&0xffu,1);
        w8(0x2a3970u,0x11u,g[0]);
        if (!flags.zf) throw std::runtime_error("2A3170 reached its exit collision branch");
        g[2]=word(0xecu);
        w(0x2a397du,0x24u,g[2]);
    }

    void accept_sample() {
        auto &g=abi.gp;
        flags.logic(g[3]&0xffu,1);
        if (!flags.zf) throw std::runtime_error("2A3170 reached its rejected center");
        byte_register(0,byte(0x12u));
        flags.logic(g[0]&0xffu,1);
        if (!flags.zf) throw std::runtime_error("2A3170 reached its rejected entrance");

        const auto center=vector_at(frame+0xacu);
        w(0x2a39c6u,0x70u,center.x);
        const auto front_x=word(0xa0u);
        w(0x2a39d1u,0x74u,center.y);
        const auto front_y=word(0xa4u);
        w(0x2a39dcu,0x78u,center.z);
        const auto front_z=word(0xa8u);
        w(0x2a39e7u,0x7cu,front_x);
        g[0]=word(0x58u);
        w(0x2a39efu,0x80u,front_y);
        g[1]=word(0x5cu);
        w(0x2a39fau,0x84u,front_z);
        g[2]=word(0x60u);
        g[5]=flags.increment(g[5],4,false);
        flags.minus(g[5],p.r16(frame+0x3cu),2);
        w(0x2a3a0bu,0x88u,g[0]);
        w(0x2a3a12u,0x8cu,g[1]);
        w(0x2a3a19u,0x90u,g[2]);
    }

public:
    SteeringTest(SourcePhysicsContactMemory &memory, SourcePhysicsWorldRuntimeAbi &state,
                 const RequiredOwner &child, const SourcePhysicsContactRecordObserver &observer):
        p{memory,observer}, abi(state), owner(child), observe(observer),
        fp{state.floating_point->x87_control_word,state.x87_status,state.floating_point->mxcsr}, s(fp),
        entry(state.gp[4]), frame(entry-0x124u), candidate(p.r32(entry+4u)) {
        flags.load(state.eflags);
    }

    u32 run() {
        try {
            start();
            entrance_sweeps();
            const auto samples=prepare_sampling();
            if (samples!=0 && samples!=1)
                throw std::runtime_error("2A3170 reached a different retained sample count");
            if (samples==1) {
                abi.gp[7]=candidate;
                create_sample(1);
                sample_sweeps();
                accept_sample();
            }
            auto &g=abi.gp;
            g[0]=word(0x140u);
            flags.logic(g[0],4);
            if (!flags.zf) throw std::runtime_error("2A3170 reached its optional route context");
            byte_register(0,byte(0x13u));
            g[7]=pop();
            g[6]=pop();
            g[5]=pop();
            g[3]=pop();
            g[4]=flags.plus(g[4],0x114u,4);
            const auto continuation=p.r32(g[4]);
            g[4]+=0x20u;
            state();
            return continuation;
        } catch (...) {
            state();
            throw;
        }
    }
};

std::uint32_t run(SourcePhysicsContactMemory &memory, SourcePhysicsWorldRuntimeAbi &abi,
                         const RequiredOwner &owner, const SourcePhysicsContactRecordObserver &observe) {
    if (!abi.floating_point || abi.x87_depth || abi.active_mmx || !owner)
        throw std::invalid_argument("Retained vehicle steering needs its actual frame and child owners");
    return SteeringTest(memory,abi,owner,observe).run();
}

bool admits(const SourcePhysicsContactMemory &memory, const SourcePhysicsContactRuntimeCall &call,
            const SourcePhysicsWorldRuntimeAbi &abi) {
    if(call.tail || call.instruction!=0x2a440bu || call.target!=0x2a3170u ||
       !abi.floating_point || abi.floating_point->x87_control_word!=0x27fu ||
       (abi.floating_point->mxcsr&0xffff1f80u)!=0x1f80u ||
       abi.x87_depth || abi.active_mmx || (abi.x87_status&0x8080u))
        return false;

    const auto &gp=abi.gp;
    const auto sp=gp[4];
    const auto slot=gp[6];
    if(sp<0x60000400u || sp>0x600fff80u || gp[0]!=gp[3] || slot>=8u || gp[7]!=slot)
        return false;
    const auto frame=memory.try_read_view(sp,0x20u);
    const auto row=memory.try_read_view(vehicle_candidates_tu15::rows+slot*vehicle_candidates_tu15::stride,8u);
    const auto count=memory.try_read_view(vehicle_candidates_tu15::count,2u);
    if(!frame || !row || !count || !memory.try_write_extent(sp-0x400u,0x400u))
        return false;
    const auto direction=(*row)[0];
    if(direction!=std::byte{} && (direction!=std::byte{1} || slot!=6u))
        return false;

    const auto word=[](std::span<const std::byte> data,std::size_t offset=0) {
        std::uint32_t value;
        std::memcpy(&value,data.data()+offset,sizeof(value));
        return value;
    };
    std::uint16_t candidate_count;
    std::memcpy(&candidate_count,count->data(),sizeof(candidate_count));
    if(candidate_count>8u || slot>=candidate_count || word(*frame)!=0x2a4410u ||
       word(*frame,4)!=vehicle_candidates_tu15::rows+slot*vehicle_candidates_tu15::stride ||
       word(*frame,0x0cu)!=sp+0x5cu || word(*frame,0x10u)!=sp+0x3cu ||
       word(*frame,0x14u)!=sp+0x78u || word(*frame,0x1cu)!=0)
        return false;

    const auto property=word(*frame,8u);
    if(property>0xffffffebu || gp[5]>0xfffffd6fu)
        return false;
    const auto maximum=memory.try_read_view(property+0x14u,4u);
    const auto sector=memory.try_read_view(gp[5]+0x28cu,4u);
    const auto scale=memory.try_read_view(0x46a5f4u,4u);
    if(!maximum || !sector || !scale || word(*sector)==0xffffffffu)
        return false;

    // The source tests the sample count after both entrance sweeps.
    // A zero turn skips the angular sweeps and keeps the entrance result.
    const auto angle=word(*row,4u);
    if(angle>=0x7f800000u)
        return false;
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,
                                abi.x87_status,abi.floating_point->mxcsr};
    const auto steps=angle_steps(fp,angle,word(*scale));
    return steps<=0x3ff0000000000000ull;
}
#else
bool admits(const SourcePhysicsContactMemory &,const SourcePhysicsContactRuntimeCall &,
            const SourcePhysicsWorldRuntimeAbi &) { return false; }
std::uint32_t run(SourcePhysicsContactMemory &,SourcePhysicsWorldRuntimeAbi &,
                  const SourcePhysicsWorldRuntimeServices::RequiredOwner &,
                  const SourcePhysicsContactRecordObserver &) {
    throw std::runtime_error("Exact native steering backend unavailable");
}
#endif
} // namespace halo2::vehicle_steering_tu15
