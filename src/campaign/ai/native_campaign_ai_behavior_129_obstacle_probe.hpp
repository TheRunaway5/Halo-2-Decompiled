#pragma once
#include "halo2/native_campaign_ai_route_traversal_tu15.hpp"
#include "native_campaign_ai_behavior_129_direction_publication.hpp"
#include "native_campaign_ai_goal_port_tu15.hpp"

#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
namespace halo2::behavior_129_direction {
using namespace goal_tu15;
constexpr u32 none = 0xffffffffu;

// 1CA560 traces a direction in the actor's local navigation. The World
// supplies the actual collision and floor queries at their original sites.
class ObstacleProbe {
    Port p_;
    SourcePhysicsWorldRuntimeAbi &abi_;
    NativeTu15PhysicsFrameFp &fp_;
    Simd s_{fp_};
    const NativeCampaignAiPerformServicesTu15 &services_;
    std::array<u32, 8> saved_;
    u32 frame_{}, actor_{}, direction_{}, request_{};
    std::array<u32,3> anchor_{};

    void push(u32 pc, u32 value) {
        abi_.gp[4] -= 4u;
        p_.w32(pc, abi_.gp[4], value);
    }
    void scalar(unsigned reg, u32 bits) { abi_.xmm[reg] = {bits, 0, 0, 0}; }
    void publish_fp() {
        abi_.floating_point = SourcePhysicsFactoryFloatingPoint{fp_.control_word, fp_.mxcsr};
        abi_.x87_status = fp_.status;
    }
    void call(u32 site, u32 entry) {
        push(site, site + 5u);
        publish_fp();
        const auto adopt_fp = [&] {
            if (abi_.floating_point)
                fp_ = {abi_.floating_point->x87_control_word, abi_.x87_status, abi_.floating_point->mxcsr};
        };
        try {
            if (entry == 0x2762a0u)
                native_campaign_ai_route_traversal_call_tu15(p_.memory, entry, abi_, services_, p_.observe);
            else if (entry == 0x1fb770u)
                native_campaign_ai_publish_probe_direction_tu15(p_.memory, abi_, fp_, p_.observe);
            else if (services_.behavior_direction_geometry)
                services_.behavior_direction_geometry(site, entry, abi_);
            else
                throw NativeCampaignAiPerformRequiredOwnerTu15(site, entry);
        } catch (...) {
            adopt_fp();
            throw;
        }
        adopt_fp();
        if (abi_.gp[4] != frame_) throw std::runtime_error("Native behavior129 direction child returned another stack");
    }
    u32 integer_scalar(i32 value) {
        detail::NativeTu15ArithmeticEnvironment environment(fp_);
        float scalar{};
        asm volatile("cvtsi2ss %1,%0" : "+x"(scalar) : "r"(value) : "memory");
        return std::bit_cast<u32>(scalar);
    }
    u32 stack_round(u32 bits, u32 address, u32 site) {
        u32 integer{};
        {
            detail::NativeTu15ArithmeticEnvironment environment(fp_);
            asm volatile("flds %1; fistpl %0" : "=m"(integer) : "m"(bits) : "memory", "st");
        }
        p_.w32(site, address, integer);
        return integer;
    }
    i32 truncate(u32 bits) {
        detail::NativeTu15ArithmeticEnvironment environment(fp_);
        i32 integer{};
        const auto value = std::bit_cast<float>(bits);
        asm volatile("cvttss2si %1,%0" : "=r"(integer) : "x"(value) : "memory");
        return integer;
    }
    bool known_sector(u32 sector) {
        if (!request_) return false;
        const auto prior_edi = abi_.gp[7];
        push(0x1ca80eu, 0x1ca813u);
        push(0x27c41bu, prior_edi);
        auto index = (sector & 0x1ffu) << 3u;
        u32 result;
        for (;;) {
            result = p_.r16(request_ + index * 2u + 0x120b6u);
            index = (index + 1u) & 0xfffu;
            if (result == 0xffffu || p_.r32(request_ + u32(sx16(result)) * 0x44u + 0xb8u) == sector)
                break;
        }
        abi_.gp[0] = result;
        abi_.gp[1] = index;
        abi_.gp[2] = request_;
        abi_.gp[7] = p_.r32(frame_ - 8u);
        abi_.gp[4] = frame_;
        return result != 0xffffu;
    }
    void trace_initial_sector(u32 sector) {
        auto &g = abi_.gp;
        push(0x1ca5fau, 0);
        push(0x1ca5fbu, 0x40a00000u);
        push(0x1ca600u, direction_);
        push(0x1ca601u, none);
        push(0x1ca603u, actor_ + 0x238u);
        g[1] = p_.r32(frame_ + 0x20u);
        push(0x1ca608u, g[1]);
        g[0] = sector;
        g[6] = frame_ + 0xb0u;
        call(0x1ca610u, 0x2762a0u);
        const auto x = p_.r32(frame_ + 0xb4u);
        const auto y = p_.r32(frame_ + 0xb8u);
        const auto z = p_.r32(frame_ + 0xbcu);
        const auto elapsed = p_.r32(frame_ + 0xc8u);
        p_.w32(0x1ca633u, frame_ + 0x10u, x);
        push(0x1ca63bu, frame_ + 0x54u);
        push(0x1ca63cu, none);
        p_.w32(0x1ca63eu, frame_ + 0x18u, z);
        g[1] = p_.r32(0x474d1cu);
        p_.w32(0x1ca648u, frame_ + 0x14u, y);
        g[0] = p_.r32(actor_ + 0x18u);
        push(0x1ca64fu, g[0]);
        push(0x1ca650u, g[1]);
        g[2] = frame_ + 0x10u;
        push(0x1ca655u, g[2]);
        push(0x1ca656u, 0x01808c2du);
        scalar(0, elapsed);
        p_.w32(0x1ca65bu, frame_ + 0x30u, elapsed);
        call(0x1ca661u, 0x16c620u);
        if (g[0] & 0xffu) {
            const auto hit_x = p_.r32(frame_ + 0x5cu);
            const auto hit_y = p_.r32(frame_ + 0x60u);
            const auto hit_z = p_.r32(frame_ + 0x64u);
            p_.w32(0x1ca676u, frame_ + 0x10u, hit_x);
            p_.w32(0x1ca67au, frame_ + 0x14u, hit_y);
            p_.w32(0x1ca67eu, frame_ + 0x18u, hit_z);
        }
    }
    bool segment_blocked(u32 up, int step) {
        auto &g = abi_.gp;
        const auto x = p_.r32(direction_);
        const auto y = p_.r32(direction_ + 4u);
        const auto z = p_.r32(direction_ + 8u);
        const auto pad = p_.r32(0x46a240u);
        const auto scalar_step = integer_scalar(step + 1);
        const auto distance = s_.mul(scalar_step, p_.r32(0x46a130u));
        const auto scaled_x = s_.mul(x, distance);
        const auto end_x = s_.add(scaled_x, p_.r32(frame_ + 0x10u));
        const auto scaled_y = s_.mul(y, distance);
        const auto end_y = s_.add(scaled_y, p_.r32(frame_ + 0x14u));
        const auto scaled_z = s_.mul(z, distance);
        const auto up_x = p_.r32(up);
        const auto end_z = s_.add(scaled_z, p_.r32(frame_ + 0x18u));
        const auto hit_x = s_.add(s_.mul(up_x, pad), end_x);
        const auto up_y = p_.r32(up + 4u);
        const auto hit_y = s_.add(s_.mul(up_y, pad), end_y);
        const auto up_z = p_.r32(up + 8u);
        g[0] = p_.r32(actor_ + 0x18u);
        g[2] = frame_ + 0x54u;
        push(0x1ca75au, g[2]);
        push(0x1ca75bu, none);
        push(0x1ca75du, g[0]);
        g[1] = frame_ + 0x34u;
        push(0x1ca762u, g[1]);
        const auto z_pad = s_.mul(up_z, pad);
        g[2] = frame_ + 0x24u;
        const auto hit_z = s_.add(z_pad, end_z);
        push(0x1ca76fu, g[2]);
        p_.w32(0x1ca770u, frame_ + 0x48u, hit_x);
        p_.w32(0x1ca776u, frame_ + 0x4cu, hit_y);
        p_.w32(0x1ca77cu, frame_ + 0x50u, hit_z);
        const auto dx = s_.sub(hit_x, anchor_[0]);
        const auto dy = s_.sub(hit_y, anchor_[1]);
        const auto dz = s_.sub(hit_z, anchor_[2]);
        push(0x1ca78eu, 0x01808c2du);
        p_.w32(0x1ca793u, frame_ + 0x34u, dx);
        p_.w32(0x1ca799u, frame_ + 0x38u, dy);
        p_.w32(0x1ca79fu, frame_ + 0x3cu, dz);
        scalar(0, dx); scalar(1, dy); scalar(2, dz);
        scalar(3, end_z); scalar(4, pad);
        call(0x1ca7a5u, 0x16c620u);
        return (g[0] & 0xffu) != 0;
    }
    void publish_direction(int step) {
        const auto time = p_.r32(0x51df54u);
        const auto ticks = integer_scalar(sx16(p_.r16(time + 2u)));
        p_.w32(0x1ca86bu, frame_ + 0x1cu, ticks);
        const auto rounded_ticks = stack_round(ticks, frame_ + 0x20u, 0x1ca875u);
        const auto seconds = integer_scalar(sx16(p_.r16(time + 2u)));
        const auto hold = s_.mul(seconds, p_.r32(0x449fd4u));
        p_.w32(0x1ca889u, frame_ + 0x1cu, hold);
        const auto rounded_hold = stack_round(hold, frame_ + 0x40u, 0x1ca893u);
        const auto elapsed = p_.r32(frame_ + 0x30u);
        const auto ticks_float = integer_scalar(std::bit_cast<i32>(rounded_ticks));
        const auto hold_float = integer_scalar(std::bit_cast<i32>(rounded_hold));
        auto &g = abi_.gp;
        g[6] = p_.r32(frame_ + 0x44u);
        g[0] = p_.r32(frame_ + 0xe8u);
        const auto duration = u32(truncate(s_.add(s_.mul(ticks_float, elapsed), hold_float)));
        const auto scalar_step = integer_scalar(step + 1);
        const auto distance = s_.mul(scalar_step, p_.r32(0x46a130u));
        g[1] = duration;
        push(0x1ca8d0u, duration);
        const auto advanced = s_.add(distance, elapsed);
        const auto travel = s_.add(advanced, p_.r32(0x46a0ccu));
        push(0x1ca8ddu, duration);
        g[2] = direction_;
        p_.w32(0x1ca8e0u, g[4], travel);
        call(0x1ca8e5u, 0x1fb770u);
    }

public:
    ObstacleProbe(SourcePhysicsContactMemory &memory, SourcePhysicsWorldRuntimeAbi &abi,
                  NativeTu15PhysicsFrameFp &fp, const NativeCampaignAiPerformServicesTu15 &services,
                  const SourcePhysicsContactRecordObserver &observe)
        : p_{memory, observe}, abi_(abi), fp_(fp), services_(services), saved_(abi.gp) {}

    void run() {
        auto &g = abi_.gp;
        const auto stack = g[4];
        g[4] -= 0xd4u;
        const auto rows = p_.r32(p_.r32(0x5020c8u) + 0x44u);
        const auto scenario = p_.r32(0x4ecc18u);
        const auto navigation_count = std::bit_cast<i32>(p_.r32(scenario + 0xc4u));
        push(0x1ca579u, saved_[3]);
        push(0x1ca57au, saved_[5]);
        direction_ = p_.r32(g[4] + 0xe4u);
        g[5] = direction_;
        push(0x1ca582u, saved_[6]);
        push(0x1ca583u, saved_[7]);
        frame_ = g[4];
        actor_ = rows + (p_.r32(frame_ + 0xe8u) & 0xffffu) * 0x888u;
        g[7] = actor_;
        g[3] = 0;
        p_.w32(0x1ca59du, frame_ + 0x20u, 0);
        if (navigation_count > 0) p_.w32(0x1ca5a9u, frame_ + 0x20u, p_.r32(scenario + 0xc8u));
        const auto origin = actor_ + 0x238u;
        const auto height = p_.r32(actor_ + 0x240u);
        p_.w16(0x1ca5bbu, frame_ + 0x78u, 0xffffu);
        p_.w32(0x1ca5c6u, frame_ + 0x10u, p_.r32(origin));
        p_.w32(0x1ca5cdu, frame_ + 0x14u, p_.r32(origin + 4u));
        p_.w32(0x1ca5d4u, frame_ + 0x18u, p_.r32(origin + 8u));
        const auto sector = p_.r32(actor_ + 0x28cu);
        p_.w32(0x1ca5e1u, frame_ + 0x1cu, height);
        p_.w32(0x1ca5eau, frame_ + 0x30u, 0);
        p_.w32(0x1ca5f0u, frame_ + 0x44u, origin);
        if (sector != none) trace_initial_sector(sector);

        auto up = p_.r32(0x474d10u);
        const auto up_x = p_.r32(up);
        const auto start_pad = p_.r32(0x46a16cu);
        for (unsigned axis = 0; axis < 3; ++axis) {
            const auto component = axis ? p_.r32(up + axis * 4u) : up_x;
            const auto lifted = s_.mul(component, start_pad);
            const auto anchored = s_.add(lifted, p_.r32(frame_ + 0x10u + axis * 4u));
            constexpr std::array sites{0x1ca69du, 0x1ca6b2u, 0x1ca6c7u};
            p_.w32(sites[axis], frame_ + 0x24u + axis * 4u, anchored);
            anchor_[axis] = anchored;
            scalar(7u - axis, anchored);
        }
        bool accepted = false;
        for (int step = 0; step < 4; ++step) {
            if (step) {
                anchor_[2] = p_.r32(frame_ + 0x2cu);
                anchor_[1] = p_.r32(frame_ + 0x28u);
                anchor_[0] = p_.r32(frame_ + 0x24u);
                up = p_.r32(0x474d10u);
            }
            if (segment_blocked(up, step)) break;
            g[0] = frame_ + 0x48u;
            push(0x1ca7b6u, g[0]);
            g[0] = p_.r32(0x474d10u);
            g[1] = frame_ + 0x54u;
            g[2] = frame_ + 0xd4u;
            call(0x1ca7c7u, 0x276e10u);
            const auto last_height = p_.r32(frame_ + 0x1cu);
            const auto maximum = s_.add(last_height, p_.r32(0x46a178u));
            auto floor = p_.r32(frame_ + 0x64u);
            g[6] = g[0];
            if (!s_.above(maximum, floor)) break;
            const auto start_z = p_.r32(frame_ + 0x18u);
            const auto maximum_drop = p_.r32(0x46a11cu);
            const auto drop = s_.sub(start_z, floor);
            if (!s_.above(maximum_drop, drop)) break;
            if (g[6] != none) {
                request_ = p_.r32(frame_ + 0xf0u);
                if (!known_sector(g[6])) {
                    g[2] = p_.r32(frame_ + 0x20u);
                    const auto node = p_.r32(g[2] + 4u) + g[6] * 8u;
                    const auto flags = p_.r16(node);
                    g[1] = node;
                    if (flags & 1u) {
                        bool blocked = false;
                        if (flags & 2u) {
                            call(0x1ca82fu, 0x202970u);
                            blocked = (g[0] & 0xffu) != 0;
                        }
                        if (!blocked) {
                            publish_direction(step);
                            accepted = true;
                            break;
                        }
                        floor = p_.r32(frame_ + 0x64u);
                    }
                }
            }
            g[3] = u32(step + 1);
            p_.w32(0x1ca843u, frame_ + 0x1cu, floor);
        }
        for (unsigned reg : {7u, 6u, 5u, 3u}) {
            g[reg] = p_.r32(g[4]);
            g[4] += 4u;
        }
        g[0] = (g[0] & 0xffffff00u) | u32(accepted);
        (void)p_.r32(stack);
        g[4] = stack + 16u;
        publish_fp();
    }
};
} // namespace halo2::behavior_129_direction
#endif
