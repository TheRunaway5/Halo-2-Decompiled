#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_contrail_effect_launch_detail {

using namespace physics_world_runtime_detail;
using native_tu15_leaves::ordered_difference_ss;
using native_tu15_leaves::ordered_mul_ss;
using native_tu15_leaves::ordered_sum_ss;

enum class Begin { retained, tagged_lookup, direct_lookup, finished };
enum class Launch { retained, projection, allocation };
enum class Projection { retained, miss, finished };
enum class DefaultVector { retained, authored, finished };

template<class State>
class EffectFrame {
    State &s;
    std::uint32_t &position;

    void at(std::uint32_t pc) { position = pc; }
    std::uint32_t read(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        return r(s.memory, address, width);
    }
    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(f(read(pc, address)));
    }
    std::uint32_t local(std::uint32_t pc, unsigned offset) {
        return read(pc, s.gp[4] + offset);
    }
    __m128 local_scalar(std::uint32_t pc, unsigned offset) {
        return scalar(pc, s.gp[4] + offset);
    }
    void write(std::uint32_t pc, std::uint32_t address, std::uint32_t value) {
        at(pc);
        w(s.memory, s.observe, pc, address, 4, value);
    }
    void save(std::uint32_t pc, unsigned offset, std::uint32_t value) {
        write(pc, s.gp[4] + offset, value);
    }
    std::uint32_t bits(unsigned lane) const { return u(_mm_cvtss_f32(s.xmm[lane])); }
    void push(std::uint32_t pc, std::uint32_t value) {
        at(pc);
        s.push(pc, value);
    }
    std::uint32_t pop(std::uint32_t pc) {
        at(pc);
        return s.pop();
    }

    void return_from_frame(std::uint32_t pc) {
        auto &sp = s.gp[4];
        sp = s.flags.plus(sp, 0x8cu, 4);
        (void)pop(pc);
        sp += 40u;
    }

    void return_default_vector(std::uint32_t pc) {
        auto &sp = s.gp[4];
        sp = s.flags.plus(sp, 0xcu, 4);
        (void)pop(pc);
        sp += 8u;
    }

    void use_out_of_range_vector() {
        auto &g = s.gp;
        g[1] = read(0x219f6cu, 0x474d04u);
        g[2] = read(0x219f72u, g[1]);
        write(0x219f74u, g[7], g[2]);
        g[0] = read(0x219f76u, g[1] + 4u);
        write(0x219f79u, g[7] + 4u, g[0]);
        g[1] = read(0x219f7cu, g[1] + 8u);
        write(0x219f7fu, g[7] + 8u, g[1]);
        g[3] = pop(0x219f82u);
        return_default_vector(0x219f86u);
    }

    void use_disabled_vector() {
        auto &g = s.gp;
        g[0] = read(0x219f4fu, 0x474d04u);
        g[1] = read(0x219f54u, g[0]);
        write(0x219f56u, g[7], g[1]);
        g[2] = read(0x219f58u, g[0] + 4u);
        write(0x219f5bu, g[7] + 4u, g[2]);
        g[0] = read(0x219f5eu, g[0] + 8u);
        g[6] = pop(0x219f61u);
        write(0x219f62u, g[7] + 8u, g[0]);
        g[3] = pop(0x219f65u);
        return_default_vector(0x219f69u);
    }

    bool nonzero_or_unordered() {
        s.set(0, s.flags.bits() | 2u, 1, 8);
        s.flags.logic(s.reg(0, 1, 8) & 0x44u, 1);
        return s.flags.pf;
    }

    void finish_without_effect() {
        auto &g = s.gp;
        g[6] = pop(0x2198b7u);
        g[0] = g[1];
        g[5] = pop(0x2198bau);
        return_from_frame(0x2198c1u);
    }

    Begin prepare_lookup() {
        auto &g = s.gp;
        auto &x = s.xmm;
        auto &flags = s.flags;
        x[0] = local_scalar(0x21935au, 0xb8u);
        x[7] = scalar(0x219363u, g[5] + 4u);
        save(0x219368u, 0x20u, g[1]);
        g[1] = read(0x21936cu, g[5]);
        g[0] = flags.right(s.reg(1, 1), 3, 4, false);
        x[6] = x[0];
        x[6] = ordered_mul_ss(x[6], x[0]);
        x[1] = x[6];
        g[0] = flags.logic(g[0] & 1u, 4);
        flags.logic(s.reg(1, 1) & 0x10u, 1);
        x[1] = ordered_mul_ss(x[1], x[0]);
        save(0x219389u, 0x10u, bits(1));
        g[0] = flags.logic(flags.zf ? g[0] & 0xfffffffdu : g[0] | 2u, 4);
        push(0x219399u, g[3]);
        g[3] = local(0x21939au, 0x9cu);
        flags.logic(s.reg(3, 1) & 1u, 1);
        push(0x2193a4u, g[7]);
        g[7] = g[4] + 0x1cu;
        push(0x2193a9u, g[0]);
        if (flags.zf) {
            g[1] = local(0x2193c7u, 0xb4u);
            push(0x2193ceu, g[1]);
            g[1] = local(0x2193cfu, 0xb0u);
            return Begin::tagged_lookup;
        }
        g[0] = local(0x2193acu, 0xb4u);
        g[3] = flags.right(g[3], 1, 4, false);
        push(0x2193b5u, g[0]);
        g[0] = local(0x2193b6u, 0xb4u);
        s.set(3, flags.logic(s.reg(3, 1) & 1u, 1), 1);
        return Begin::direct_lookup;
    }

    void read_launch_coefficients() {
        auto &g = s.gp;
        auto &x = s.xmm;
        s.flags.logic(s.reg(3, 1), 1);
        g[1] = read(0x2193dfu, 0x52be68u);
        if (!s.flags.zf) {
            x[0] = scalar(0x2193e7u, 0x505e84u);
            x[1] = scalar(0x2193efu, g[1] + 4u);
            x[1] = ordered_sum_ss(x[1], scalar(0x2193f4u, g[5] + 0x20u));
            x[0] = ordered_sum_ss(x[0], x[7]);
            x[7] = scalar(0x2193fdu, 0x46a0d0u);
            save(0x219405u, 0x14u, bits(0));
            x[0] = scalar(0x21940bu, g[5] + 0x20u);
            x[0] = ordered_mul_ss(x[0], scalar(0x219410u, 0x46a11cu));
            x[0] = _mm_div_ss(x[0], x[1]);
            x[1] = scalar(0x21941cu, g[5] + 0x28u);
            x[2] = x[7];
            save(0x219424u, 0x10u, 2u);
        } else {
            x[0] = scalar(0x21942eu, 0x505e88u);
            x[1] = scalar(0x219436u, g[1] + 8u);
            x[1] = ordered_sum_ss(x[1], scalar(0x21943bu, g[5] + 0x20u));
            x[2] = scalar(0x219440u, 0x46a0d0u);
            x[0] = ordered_sum_ss(x[0], x[7]);
            save(0x21944cu, 0x14u, bits(0));
            x[0] = scalar(0x219452u, g[5] + 0x20u);
            x[0] = ordered_mul_ss(x[0], scalar(0x219457u, 0x46a11cu));
            x[0] = _mm_div_ss(x[0], x[1]);
            x[1] = scalar(0x219463u, g[5] + 0x24u);
            save(0x219468u, 0x10u, 1u);
            x[7] = x[2];
        }
        s.flags.logic(read(0x219473u, g[5], 1) & 0x20u, 1);
        x[5] = _mm_setzero_ps();
        x[2] = ordered_difference_ss(x[2], x[0]);
        x[1] = ordered_mul_ss(x[1], x[6]);
        if (!s.flags.zf) x[2] = x[5];
    }

    template<class Read64, class Spill>
    bool compute_launch_weight(Read64 read64, Spill spill) {
        auto &fp = s.fp;
        fp.push(ext(f(local(0x219487u, 0x14u))));
        s.xmm[4] = local_scalar(0x21948bu, 0xc4u);
        fp.st(0) = mul(fp.st(0), ext(f(local(0x219494u, 0x18u))));
        at(0x219498u);
        const auto rounded = spill(fp.st(0), fp.status);
        save(0x219498u, 0x14u, rounded);
        at(0x21949cu);
        fp.push(static_cast<long double>(std::bit_cast<double>(read64(s.memory, 0x46a0f0u))));
        at(0x2194a2u);
        s.unordered_compare_and_pop(fp.st(1));
        at(0x2194a4u);
        (void)fp.st(0);
        fp.pop();
        return nonzero_or_unordered();
    }

    void offset_launch_origin() {
        auto &g = s.gp;
        auto &x = s.xmm;
        auto &flags = s.flags;
        x[3] = x[4];
        x[3] = _mm_div_ss(x[3], local_scalar(0x2194afu, 0x14u));
        x[0] = x[3];
        x[0] = ordered_mul_ss(x[0], x[1]);
        at(0x2194bcu);
        flags.scalar_compare(x[5], x[0], false);
        if (!flags.cf && !flags.zf) {
            x[0] = x[5];
        } else {
            at(0x2194c6u);
            flags.scalar_compare(x[0], x[7], false);
            if (!flags.cf && !flags.zf) x[0] = x[7];
        }
        g[0] = local(0x2194ceu, 0xb4u);
        if (!flags.logic(g[0], 4)) return;

        struct Component { std::uint32_t offset, reference, origin, updated; };
        constexpr Component axes[]{
            {0u, 0x2194d9u, 0x2194e1u, 0x2194e5u},
            {4u, 0x2194e9u, 0x2194f2u, 0x2194f7u},
            {8u, 0x2194fcu, 0x219505u, 0x21950au},
        };
        for (const auto &axis : axes) {
            x[1] = scalar(axis.reference, g[0] + axis.offset);
            x[1] = ordered_mul_ss(x[1], x[3]);
            x[1] = ordered_sum_ss(x[1], scalar(axis.origin, g[6] + axis.offset));
            write(axis.updated, g[6] + axis.offset, bits(1));
        }
    }

    void use_zero_launch_weight() {
        auto &x = s.xmm;
        at(0x219511u);
        s.flags.scalar_compare(x[1], x[5], true);
        const bool use_one = nonzero_or_unordered();
        x[0] = use_one ? x[7] : x[5];
    }

    void project_launch_vector() {
        auto &g = s.gp;
        auto &x = s.xmm;
        x[1] = scalar(0x219520u, g[1]);
        x[1] = ordered_mul_ss(x[1], x[2]);
        x[2] = local_scalar(0x219528u, 0x20u);
        x[2] = ordered_difference_ss(x[2], scalar(0x21952eu, g[6] + 4u));
        x[1] = ordered_mul_ss(x[1], x[4]);
        x[1] = ordered_sum_ss(x[1], scalar(0x219537u, g[6] + 8u));
        write(0x21953cu, g[6] + 8u, bits(1));
        x[1] = local_scalar(0x219541u, 0x1cu);
        x[1] = ordered_difference_ss(x[1], scalar(0x219547u, g[6]));
        x[2] = ordered_mul_ss(x[2], x[0]);
        x[2] = ordered_sum_ss(x[2], scalar(0x21954fu, g[6] + 4u));
        write(0x219554u, g[6] + 4u, bits(2));
        x[2] = local_scalar(0x219559u, 0x24u);
        x[2] = ordered_difference_ss(x[2], scalar(0x21955fu, g[6] + 8u));
        x[1] = ordered_mul_ss(x[1], x[0]);
        x[1] = ordered_sum_ss(x[1], scalar(0x219568u, g[6]));
        x[2] = ordered_mul_ss(x[2], x[0]);
        x[2] = ordered_sum_ss(x[2], scalar(0x219570u, g[6] + 8u));
        write(0x219575u, g[6], bits(1));
        write(0x219579u, g[6] + 8u, bits(2));
    }

    Launch select_launch() {
        auto &g = s.gp;
        auto &flags = s.flags;
        g[0] = read(0x21957eu, g[5]);
        flags.logic(s.reg(0, 1) & 2u, 1);
        g[3] = 0x800005u;
        if (flags.zf) g[3] = local(0x21958au, 0x28u);
        if (flags.logic(s.reg(0, 1) & 4u, 1))
            g[3] = flags.logic(g[3] | 0x800002u, 4);
        if (!flags.logic(g[3], 4) || flags.logic(read(0x2195a0u, g[4] + 0xa0u, 1) & 4u, 1))
            return Launch::projection;
        save(0x2195aeu, 0x14u, 0u);
        return Launch::allocation;
    }

public:
    EffectFrame(State &state, std::uint32_t &cursor) : s(state), position(cursor) {}

    Begin open() {
        auto &g = s.gp;
        auto &x = s.xmm;
        x[0] = local_scalar(0x219330u, 0x28u);
        g[4] = s.flags.minus(g[4], 0x8cu, 4);
        push(0x21933cu, g[5]);
        g[5] = local(0x21933du, 0x98u);
        g[1] = s.flags.logic(0, 4);
        s.flags.scalar_compare(x[0], scalar(0x219346u, 0x46a0e8u), true);
        push(0x21934du, g[6]);
        g[6] = g[0];
        if (nonzero_or_unordered()) return prepare_lookup();
        finish_without_effect();
        return Begin::finished;
    }

    template<class Read64, class Spill>
    Launch prepare(Read64 read64, Spill spill) {
        read_launch_coefficients();
        if (compute_launch_weight(read64, spill)) offset_launch_origin();
        else use_zero_launch_weight();
        project_launch_vector();
        return select_launch();
    }

    void project_to_child() {
        auto &g = s.gp;
        auto &x = s.xmm;
        g[2] = local(0x219847u, 0xb0u);
        x[1] = ordered_mul_ss(x[1], x[4]);
        x[1] = ordered_sum_ss(x[1], scalar(0x219852u, g[2]));
        write(0x219856u, g[2], bits(1));

        x[0] = scalar(0x21985au, g[6] + 4u);
        x[0] = ordered_mul_ss(x[0], x[4]);
        x[0] = ordered_sum_ss(x[0], scalar(0x219863u, g[2] + 4u));
        write(0x219868u, g[2] + 4u, bits(0));

        x[0] = scalar(0x21986du, g[6] + 8u);
        x[0] = ordered_mul_ss(x[0], x[4]);
        x[0] = ordered_sum_ss(x[0], scalar(0x219876u, g[2] + 8u));
        g[6] = g[4] + 0x28u;
        write(0x21987fu, g[2] + 8u, bits(0));
    }

    Projection projection_result() {
        auto &g = s.gp;
        g[0] = local(0x219889u, 0x28u);
        s.flags.minus(g[0], 0xffffffffu, 4);
        if (s.flags.zf) return Projection::miss;

        g[1] = local(0x219896u, 0xa8u);
        g[7] = pop(0x21989du);
        write(0x21989eu, g[1], g[0]);
        g[0] = local(0x2198a0u, 0x28u);
        g[3] = pop(0x2198a4u);
        g[6] = pop(0x2198a5u);
        write(0x2198a6u, g[1] + 4u, g[0]);
        g[0] = local(0x2198a9u, 4u);
        g[5] = pop(0x2198adu);
        return_from_frame(0x2198b4u);
        return Projection::finished;
    }

    void return_launch_status() {
        auto &g = s.gp;
        g[0] = local(0x2197f8u, 0x10u);
        g[7] = pop(0x2197fcu);
        g[3] = pop(0x2197fdu);
        g[6] = pop(0x2197feu);
        g[5] = pop(0x2197ffu);
        return_from_frame(0x219806u);
    }

    void prepare_tagged_lookup() {
        auto &g = s.gp;
        auto &flags = s.flags;
        s.set(1, read(0x219e00u, g[1] + 4u, 2), 2);
        g[0] = flags.logic(0xffffffffu, 4);
        flags.minus(s.reg(1, 2), 0xffffffffu, 2);
        if (!flags.zf) {
            g[0] = read(0x219e0du, 0x4ecc18u);
            g[2] = std::uint32_t(std::bit_cast<std::int16_t>(std::uint16_t(g[1])));
            g[1] = read(0x219e15u, g[0] + 0xa0u);
            g[2] = flags.product(g[2], 0xb0u);
            g[0] = flags.logic(0, 4);
            s.set(0, read(0x219e23u, g[2] + g[1] + 0x76u, 2), 2);
        }
        g[2] = local(0x219e28u, 8u);
        g[1] = local(0x219e2cu, 4u);
        push(0x219e30u, g[2]);
        push(0x219e31u, g[1]);
    }

    void return_tagged_lookup() {
        s.set(0, s.flags.logic(0, 1), 1);
        (void)pop(0x219e39u);
        s.gp[4] += 8u;
    }

    DefaultVector admit_default_vector() {
        auto &g = s.gp;
        auto &flags = s.flags;
        g[4] = flags.minus(g[4], 0xcu, 4);
        flags.logic(s.reg(0, 2), 2);
        push(0x219e46u, g[3]);
        s.set(3, read(0x219e47u, g[4] + 0x18u, 1), 1);
        if (flags.sf != flags.of) {
            use_out_of_range_vector();
            return DefaultVector::finished;
        }
        flags.minus(s.reg(0, 2), read(0x219e51u, 0x506794u, 2), 2);
        if (flags.sf == flags.of) {
            use_out_of_range_vector();
            return DefaultVector::finished;
        }
        g[0] = std::uint32_t(std::bit_cast<std::int16_t>(std::uint16_t(g[0])));
        push(0x219e61u, g[6]);
        g[6] = flags.shift(g[0], 5u, 4);
        g[6] = flags.plus(g[6], 0x506798u, 4);
        flags.minus(read(0x219e6du, g[6], 1), 0u, 1);
        if (!flags.zf) return DefaultVector::authored;
        use_disabled_vector();
        return DefaultVector::finished;
    }

    void prepare_authored_vector() {
        auto &g = s.gp;
        auto &x = s.xmm;
        auto &flags = s.flags;
        g[1] = read(0x219e76u, 0x4ecc18u);
        g[0] = flags.product(g[0], 0x88u);
        g[2] = read(0x219e82u, g[1] + 0x88u);
        g[0] = read(0x219e88u, g[2] + g[0] + 0x50u);
        g[1] = read(0x219e8cu, 0x4f0414u);
        x[5] = _mm_setzero_ps();
        g[0] = flags.logic(g[0] & 65535u, 4);
        g[0] = flags.shift(g[0], 4u, 4);
        flags.logic(s.reg(3, 1) & 1u, 1);
        push(0x219ea0u, g[5]);
        g[5] = read(0x219ea1u, g[1] + g[0] + 8u);
        if (flags.zf) x[5] = scalar(0x219ea7u, g[5] + 0x10u);
        g[2] = read(0x219eacu, g[5] + 0x14u);
        x[3] = scalar(0x219eafu, g[5] + 0x10u);
        x[3] = ordered_mul_ss(x[3], scalar(0x219eb4u, g[6] + 0x10u));
        push(0x219eb9u, g[2]);
        g[2] = local(0x219ebau, 0x20u);
        g[0] = g[4] + 0x10u;
    }

    void finish_authored_vector() {
        auto &g = s.gp;
        auto &x = s.xmm;
        s.flags.logic(s.reg(3, 1) & 2u, 1);
        x[2] = scalar(0x219ecau, g[6] + 0x14u);
        x[1] = scalar(0x219ecfu, 0x46a0d0u);
        x[0] = x[1];
        x[0] = ordered_difference_ss(x[0], x[5]);
        x[2] = ordered_mul_ss(x[2], x[0]);
        x[2] = ordered_sum_ss(x[2], local_scalar(0x219ee2u, 0xcu));
        write(0x219ee8u, g[7], bits(2));

        x[2] = scalar(0x219eecu, g[6] + 0x18u);
        x[2] = ordered_mul_ss(x[2], x[0]);
        x[2] = ordered_sum_ss(x[2], local_scalar(0x219ef5u, 0x10u));
        write(0x219efbu, g[7] + 4u, bits(2));

        x[2] = scalar(0x219f00u, g[6] + 0x1cu);
        x[2] = ordered_mul_ss(x[2], x[0]);
        x[2] = ordered_sum_ss(x[2], local_scalar(0x219f09u, 0x14u));
        write(0x219f0fu, g[7] + 8u, bits(2));

        if (!s.flags.zf) {
            x[0] = x[1];
            x[0] = ordered_difference_ss(x[0], scalar(0x219f19u, g[5] + 0x18u));
            x[0] = ordered_mul_ss(x[0], scalar(0x219f1eu, g[7]));
            write(0x219f22u, g[7], bits(0));
            x[0] = x[1];
            x[0] = ordered_difference_ss(x[0], scalar(0x219f29u, g[5] + 0x18u));
            x[0] = ordered_mul_ss(x[0], scalar(0x219f2eu, g[7] + 4u));
            write(0x219f33u, g[7] + 4u, bits(0));
            x[1] = ordered_difference_ss(x[1], scalar(0x219f38u, g[5] + 0x18u));
            x[1] = ordered_mul_ss(x[1], x[2]);
            write(0x219f41u, g[7] + 8u, bits(1));
        }
        g[5] = pop(0x219f46u);
        g[6] = pop(0x219f47u);
        g[3] = pop(0x219f48u);
        return_default_vector(0x219f4cu);
    }

    void sample_authored_offset() {
        auto &g = s.gp;
        auto &x = s.xmm;
        auto &flags = s.flags;
        g[4] = flags.minus(g[4], 0xcu, 4);
        g[1] = read(0x21a2a3u, 0x474d04u);
        x[0] = scalar(0x21a2a9u, 0x46a178u);
        x[3] = ordered_mul_ss(x[3], scalar(0x21a2b1u, 0x46a174u));
        x[1] = scalar(0x21a2b9u, 0x46a128u);
        x[2] = scalar(0x21a2c1u, 0x46a170u);
        push(0x21a2c9u, g[5]);
        push(0x21a2cau, g[6]);
        push(0x21a2cbu, g[7]);

        g[7] = read(0x21a2ccu, g[1]);
        g[6] = g[0];
        write(0x21a2d0u, g[6], g[7]);
        g[7] = read(0x21a2d2u, g[1] + 4u);
        write(0x21a2d5u, g[6] + 4u, g[7]);
        g[1] = read(0x21a2d8u, g[1] + 8u);
        save(0x21a2dbu, 0xcu, bits(0));
        x[0] = scalar(0x21a2e1u, 0x46a16cu);
        save(0x21a2e9u, 0x10u, bits(0));
        x[0] = scalar(0x21a2efu, 0x46a168u);
        write(0x21a2f7u, g[6] + 8u, g[1]);
        g[7] = g[4] + 0xcu;
        save(0x21a2feu, 0x14u, bits(0));
        x[0] = local_scalar(0x21a304u, 0x1cu);
        g[6] = flags.logic(0u, 4);
        g[7] = flags.minus(g[7], g[2], 4);
        g[5] = 3u;

        do {
            x[4] = _mm_cvtsi32_ss(x[4], si(read(0x21a313u, 0x506b98u)));
            x[4] = ordered_mul_ss(x[4], scalar(0x21a31bu, g[7] + g[2]));
            x[4] = ordered_mul_ss(x[4], x[0]);
            x[4] = ordered_sum_ss(x[4], scalar(0x21a324u, g[2]));
            x[4] = ordered_mul_ss(x[4], x[1]);
            save(0x21a32cu, 0x1cu, bits(4));
            const auto magnitude = flags.logic(local(0x21a332u, 0x1cu) & 0x7fffffffu, 4);
            save(0x21a332u, 0x1cu, magnitude);
            x[4] = local_scalar(0x21a33au, 0x1cu);
            x[4] = ordered_sum_ss(x[4], x[2]);
            save(0x21a344u, 0x1cu, bits(4));

            g[1] = flags.logic(0u, 4);
            g[1] = read(0x21a34cu, g[4] + 0x1cu, 1);
            g[2] = flags.plus(g[2], 4u, 4);
            g[1] = flags.logic(g[1] & 63u, 4);
            g[1] = flags.plus(g[1], g[6], 4);
            g[1] = flags.shift(g[1] * 3u, 2u, 4);

            x[4] = scalar(0x21a361u, g[1] + 0x505e94u);
            x[4] = ordered_sum_ss(x[4], scalar(0x21a369u, g[0]));
            write(0x21a36du, g[0], bits(4));
            x[4] = scalar(0x21a371u, g[1] + 0x505e98u);
            x[4] = ordered_sum_ss(x[4], scalar(0x21a379u, g[0] + 4u));
            write(0x21a37eu, g[0] + 4u, bits(4));
            x[4] = scalar(0x21a383u, g[1] + 0x505e9cu);
            x[4] = ordered_sum_ss(x[4], scalar(0x21a38bu, g[0] + 8u));
            g[6] = flags.plus(g[6], 0x40u, 4);
            g[5] = flags.increment(g[5], 4, true);
            write(0x21a394u, g[0] + 8u, bits(4));
        } while (!flags.zf);

        x[0] = scalar(0x21a39fu, g[0]);
        x[0] = ordered_mul_ss(x[0], x[3]);
        write(0x21a3a7u, g[0], bits(0));
        x[0] = x[3];
        x[0] = ordered_mul_ss(x[0], scalar(0x21a3aeu, g[0] + 4u));
        g[7] = pop(0x21a3b3u);
        write(0x21a3b4u, g[0] + 4u, bits(0));
        x[0] = x[4];
        g[6] = pop(0x21a3bcu);
        x[0] = ordered_mul_ss(x[0], x[3]);
        write(0x21a3c1u, g[0] + 8u, bits(0));
        g[5] = pop(0x21a3c6u);
        g[4] = flags.plus(g[4], 0xcu, 4);
        (void)pop(0x21a3cau);
        g[4] += 4u;
    }
};

template<class State>
std::uint32_t *eligible_cursor(State &s) {
    const auto *cursor = s.immutable_cursor.cursor;
    if (s.image != SourcePhysicsContactImage::TitleUpdate15 || !cursor || !cursor->position ||
            (cursor->first_trace && *cursor->first_trace) || (cursor->second_trace && *cursor->second_trace) ||
            (s.observe && !cursor->observation_only_stores) ||
            !native_required_owner_policy_detail::preserves_instruction_policy(s.services))
        return nullptr;
    return cursor->position;
}

template<class State>
std::uint32_t *admitted(State &s) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_EFFECT_LAUNCH");
        return value && *value;
    }();
    return reference ? nullptr : eligible_cursor(s);
}

template<class State>
Begin open(State &s) {
    auto *position = admitted(s);
    return position ? EffectFrame(s, *position).open() : Begin::retained;
}

template<class State, class Read64, class Spill>
Launch prepare(State &s, Read64 read64, Spill spill) {
    auto *position = admitted(s);
    return position ? EffectFrame(s, *position).prepare(read64, spill) : Launch::retained;
}

template<class State>
std::uint32_t *projection_cursor(State &s) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_EFFECT_PROJECTION");
        return value && *value;
    }();
    return reference ? nullptr : eligible_cursor(s);
}

template<class State>
bool project_to_child(State &s) {
    auto *position = projection_cursor(s);
    if (!position) return false;
    EffectFrame(s, *position).project_to_child();
    return true;
}

template<class State>
Projection projection_result(State &s) {
    auto *position = projection_cursor(s);
    return position ? EffectFrame(s, *position).projection_result() : Projection::retained;
}

template<class State>
bool return_launch_status(State &s) {
    auto *position = projection_cursor(s);
    if (!position) return false;
    EffectFrame(s, *position).return_launch_status();
    return true;
}

template<class State>
std::uint32_t *tagged_lookup_cursor(State &s) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_EFFECT_TAGGED_LOOKUP");
        return value && *value;
    }();
    return reference ? nullptr : eligible_cursor(s);
}

template<class State>
bool prepare_tagged_lookup(State &s) {
    auto *position = tagged_lookup_cursor(s);
    if (!position) return false;
    EffectFrame(s, *position).prepare_tagged_lookup();
    return true;
}

template<class State>
bool return_tagged_lookup(State &s) {
    auto *position = tagged_lookup_cursor(s);
    if (!position) return false;
    EffectFrame(s, *position).return_tagged_lookup();
    return true;
}

template<class State>
DefaultVector admit_default_vector(State &s) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_EFFECT_DEFAULT_VECTOR");
        return value && *value;
    }();
    auto *position = reference ? nullptr : eligible_cursor(s);
    return position ? EffectFrame(s, *position).admit_default_vector() : DefaultVector::retained;
}

template<class State>
std::uint32_t *authored_vector_cursor(State &s) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_EFFECT_AUTHORED_VECTOR");
        return value && *value;
    }();
    return reference ? nullptr : eligible_cursor(s);
}

template<class State>
bool prepare_authored_vector(State &s) {
    auto *position = authored_vector_cursor(s);
    if (!position) return false;
    EffectFrame(s, *position).prepare_authored_vector();
    return true;
}

template<class State>
bool finish_authored_vector(State &s) {
    auto *position = authored_vector_cursor(s);
    if (!position) return false;
    EffectFrame(s, *position).finish_authored_vector();
    return true;
}

template<class State>
bool sample_authored_offset(State &s) {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_CONTRAIL_EFFECT_AUTHORED_OFFSET");
        return value && *value;
    }();
    auto *position = reference ? nullptr : eligible_cursor(s);
    if (!position) return false;
    EffectFrame(s, *position).sample_authored_offset();
    return true;
}

} // namespace halo2::native_contrail_effect_launch_detail
