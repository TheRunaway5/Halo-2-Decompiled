#pragma once

#include "physics/runtime/native_tu15_portal_predicate_leaves.hpp"
#include "rendering/effects/native_tu15_contrail_vertex_emission.hpp"

namespace halo2::native_contrail_camera_facing_detail {

using namespace physics_world_runtime_detail;
using native_tu15_leaves::ordered_add_ss;
using native_tu15_leaves::ordered_mul_ss;

template<class Spill>
class CameraFacing {
    WorldState &s;
    Spill spill;

    std::uint32_t read(std::uint32_t address, unsigned width = 4) const {
        return r(s.memory, address, width);
    }
    __m128 scalar(std::uint32_t address) const { return _mm_set_ss(f(read(address))); }
    __m128 local(unsigned offset) const { return scalar(s.gp[4] + offset); }
    long double extended(std::uint32_t address) const { return ext(f(read(address))); }
    long double local_extended(unsigned offset) const { return extended(s.gp[4] + offset); }
    void store(std::uint32_t pc, unsigned offset, std::uint32_t value) {
        w(s.memory, s.observe, pc, s.gp[4] + offset, 4, value);
    }
    void store_lane(std::uint32_t pc, unsigned offset, unsigned lane) {
        store(pc, offset, u(_mm_cvtss_f32(s.xmm[lane])));
    }
    void spill_top(std::uint32_t pc, unsigned offset) {
        store(pc, offset, spill(s.fp.st(0), s.fp.status));
    }

    void use_previous_point() {
        auto &g = s.gp;
        auto &x = s.xmm[0];
        const auto point = g[5], previous = g[3];
        x = _mm_sub_ss(x, scalar(previous + 0x1cu));
        store_lane(0x432ceu, 0x64u, 0);
        x = scalar(0x4c65c4u);
        x = _mm_sub_ss(x, scalar(previous + 0x20u));
        store_lane(0x432e1u, 0x68u, 0);
        x = scalar(0x4c65c8u);
        x = _mm_sub_ss(x, scalar(previous + 0x24u));
        store_lane(0x432f4u, 0x6cu, 0);

        x = scalar(point + 0x1cu);
        x = _mm_sub_ss(x, scalar(previous + 0x1cu));
        g[1] = point + 0x1cu;
        store_lane(0x43307u, 0x40u, 0);
        x = scalar(point + 0x20u);
        x = _mm_sub_ss(x, scalar(previous + 0x20u));
        store_lane(0x43317u, 0x44u, 0);
        x = scalar(point + 0x24u);
        x = _mm_sub_ss(x, scalar(previous + 0x24u));
    }

    void use_next_point() {
        auto &g = s.gp;
        auto &flags = s.flags;
        auto &x = s.xmm[0];
        const auto point = g[5];
        x = _mm_sub_ss(x, scalar(point + 0x1cu));
        g[0] = read(point + 0x34u);
        g[1] = read(0x4f7210u);
        g[6] = read(g[1] + 0x44u);
        g[0] = flags.logic(g[0] & 0xffffu, 4);
        store_lane(0x4333fu, 0x64u, 0);
        g[0] = flags.product(g[0], 0x38u);
        x = scalar(0x4c65c4u);
        x = _mm_sub_ss(x, scalar(point + 0x20u));
        g[1] = point + 0x1cu;
        store_lane(0x43358u, 0x68u, 0);
        x = scalar(0x4c65c8u);
        x = _mm_sub_ss(x, scalar(point + 0x24u));
        store_lane(0x4336bu, 0x6cu, 0);

        x = scalar(g[0] + g[6] + 0x1cu);
        x = _mm_sub_ss(x, scalar(g[1]));
        g[0] = flags.plus(g[0], g[6], 4);
        store_lane(0x4337du, 0x40u, 0);
        x = scalar(g[0] + 0x20u);
        x = _mm_sub_ss(x, scalar(point + 0x20u));
        store_lane(0x4338du, 0x44u, 0);
        x = scalar(g[0] + 0x24u);
        x = _mm_sub_ss(x, scalar(point + 0x24u));
    }

    void prepare_directions() {
        s.flags.logic(s.gp[3], 4);
        s.xmm[0] = scalar(0x4c65c0u);
        if (s.flags.zf) use_next_point();
        else use_previous_point();
        store_lane(0x4339du, 0x48u, 0);
    }

    void component(unsigned a, unsigned b, unsigned c, unsigned d, std::uint32_t pc, unsigned out, bool pop) {
        auto &fp = s.fp;
        fp.push(local_extended(a));
        fp.st(0) = mul(fp.st(0), local_extended(b));
        fp.push(local_extended(c));
        fp.st(0) = mul(fp.st(0), local_extended(d));
        fp.st(1) = sub(fp.st(1), fp.st(0));
        fp.pop();
        spill_top(pc, out);
        if (pop) fp.pop();
    }

    bool normalizable(unsigned first, std::uint32_t pc) {
        auto &fp = s.fp;
        // The final cross product component is still extended on the stack.
        // Its authored square multiplies it by the rounded copy.
        fp.st(0) = mul(fp.st(0), local_extended(first + 8u));
        for (const auto offset : {first + 4u, first}) {
            fp.push(local_extended(offset));
            fp.st(0) = mul(fp.st(0), local_extended(offset));
            fp.st(1) = add(fp.st(1), fp.st(0));
            fp.pop();
        }
        fp.st(0) = root(fp.st(0));
        spill_top(pc, 0x10u);
        fp.st(0) = std::fabs(fp.st(0));
        fp.push(extended(0x46a0ecu));
        s.compare_and_pop(fp.st(1));
        (void)fp.st(0);
        fp.pop();
        return s.flags.cf || s.flags.zf;
    }

    void camera_side() {
        auto &x = s.xmm;
        component(0x48u, 0x68u, 0x44u, 0x6cu, 0x433b5u, 0x20u, true);
        component(0x40u, 0x6cu, 0x48u, 0x64u, 0x433cbu, 0x24u, true);
        component(0x44u, 0x64u, 0x40u, 0x68u, 0x433e1u, 0x28u, false);
        if (normalizable(0x20u, 0x433ffu)) {
            x[2] = x[7];
            x[2] = _mm_div_ss(x[2], local(0x10u));
            x[3] = x[2];
            x[3] = ordered_mul_ss(x[3], local(0x20u));
            x[5] = x[2];
            x[5] = ordered_mul_ss(x[5], local(0x24u));
            x[2] = ordered_mul_ss(x[2], local(0x28u));
            store_lane(0x43432u, 0x20u, 3);
            store_lane(0x43438u, 0x24u, 5);
            store_lane(0x4343eu, 0x28u, 2);
        } else {
            x[2] = local(0x28u);
            x[5] = local(0x24u);
            x[3] = local(0x20u);
        }
    }

    bool offset_pair() {
        auto &g = s.gp;
        auto &x = s.xmm;
        x[0] = scalar(g[1]);
        s.set(0, read(g[2] + 0x10u, 1), 1);
        s.flags.logic(s.reg(0, 1), 1);
        x[1] = scalar(g[5] + 0x20u);
        x[3] = ordered_mul_ss(x[3], x[4]);
        x[0] = _mm_sub_ss(x[0], x[3]);
        x[3] = ordered_add_ss(x[3], scalar(g[1]));
        x[5] = ordered_mul_ss(x[5], x[4]);
        x[2] = ordered_mul_ss(x[2], x[4]);
        x[4] = x[2];
        x[2] = scalar(g[5] + 0x24u);
        x[6] = x[3];
        x[3] = scalar(g[5] + 0x24u);
        x[1] = _mm_sub_ss(x[1], x[5]);
        x[5] = ordered_add_ss(x[5], scalar(g[5] + 0x20u));
        x[2] = _mm_sub_ss(x[2], x[4]);
        x[3] = ordered_add_ss(x[3], x[4]);
        return !s.flags.zf;
    }

    void face_normal() {
        auto &x = s.xmm;
        component(0x28u, 0x44u, 0x24u, 0x48u, 0x434b3u, 0x14u, true);
        component(0x20u, 0x48u, 0x28u, 0x40u, 0x434c9u, 0x18u, true);
        component(0x24u, 0x40u, 0x20u, 0x44u, 0x434dfu, 0x1cu, false);
        if (!normalizable(0x14u, 0x434fdu)) return;
        x[4] = x[7];
        x[4] = _mm_div_ss(x[4], local(0x10u));
        x[7] = x[4];
        x[7] = ordered_mul_ss(x[7], local(0x14u));
        store_lane(0x43521u, 0x14u, 7);
        x[7] = x[4];
        x[7] = ordered_mul_ss(x[7], local(0x18u));
        x[4] = ordered_mul_ss(x[4], local(0x1cu));
        store_lane(0x43536u, 0x18u, 7);
        store_lane(0x4353cu, 0x1cu, 4);
    }

public:
    CameraFacing(WorldState &state, Spill round) : s(state), spill(round) {}

    void run() {
        prepare_directions();
        camera_side();
        if (offset_pair()) face_normal();
    }
};

template<class Spill>
bool bridge(WorldState &s, Spill spill) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_CAMERA_FACING");
        return value && *value;
    }();
    if (reference || s.image != SourcePhysicsContactImage::TitleUpdate15 || s.services.instruction || s.observe ||
            !native_contrail_vertex_emission_detail::ScopedDrawServices::identifies(s)) return false;
    CameraFacing{s, spill}.run();
    return true;
}

} // namespace halo2::native_contrail_camera_facing_detail
