#pragma once

#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "physics/runtime/native_tu15_world_leaves.hpp"
#include "physics/runtime/physics_solver_internal.hpp"
#include "rendering/effects/native_tu15_particle_birth_observer.hpp"

namespace halo2::native_particle_observer_lens_detail {
namespace world = native_world_leaves_detail;
namespace math = physics_solver_detail;
using world::eax;
using world::ecx;
using world::edx;
using world::esi;
using world::esp;
using world::edi;
using world::ordered_add_ss;
using native_tu15_leaves::ordered_mul_ss;

inline constexpr std::uint32_t entry = 0x174bf0u, last = 0x1751a7u;
inline constexpr std::uint32_t forward = 0x20u, up = 0x2cu, horizontal_angle = 0x38u;
inline constexpr std::uint32_t aspect_ratio = 0x3cu, pitch_scale = 0x40u, yaw_scale = 0x44u;
inline constexpr std::uint32_t correction_angle = 0x4cu, projected_angle = 0x50u;
inline constexpr std::uint32_t lens_divisor = 0x55d6e4u, reference_axis = 0x474d10u;
inline constexpr world::Function owner_sites[]{{entry, last, 0u, 0u}};

struct Verification {
    std::atomic<std::uint64_t> calls{};
    ~Verification() {
        if (calls) std::fprintf(stderr, "Particle observer lens verify: %llu calls\n",
                                static_cast<unsigned long long>(calls.load()));
    }
};
inline Verification &verified() { static Verification state; return state; }

enum class Trig { sine, cosine, tangent };

template<class State, class Trigonometry, class Arctangent, class Spill>
struct Lens {
    world::Port<State> &port;
    Trigonometry trig;
    Arctangent atan;
    Spill spill;
    std::array<std::uint32_t, 8> &gp = port.gp;
    typename world::Port<State>::Flags &flags = port.flags;
    decltype(port.s.xmm) &x = port.s.xmm;
    decltype(port.s.fp) &fp = port.s.fp;

    std::uint32_t word(std::uint32_t pc, std::uint32_t address) {
        return port.read(pc, address, 4);
    }

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(math::f(word(pc, address)));
    }

    void store_word(std::uint32_t pc, std::uint32_t address, std::uint32_t value) {
        port.store(pc, address, 4, value);
    }

    void store_scalar(std::uint32_t pc, std::uint32_t address, __m128 value) {
        store_word(pc, address, math::u(_mm_cvtss_f32(value)));
    }

    long double extended(std::uint32_t pc, std::uint32_t address) {
        return math::ext(math::f(word(pc, address)));
    }

    void push_extended(std::uint32_t pc, long double value) {
        port.at(pc);
        fp.push(value);
        fp.status &= ~0x200u;
    }

    void duplicate_top(std::uint32_t pc) {
        port.at(pc);
        push_extended(pc, fp.st(0));
    }

    void push_float(std::uint32_t pc, std::uint32_t address) {
        push_extended(pc, extended(pc, address));
    }

    void push_double(std::uint32_t pc, std::uint32_t address) {
        port.at(pc);
        std::array<std::byte,8> bytes;
        port.s.memory.read(address, bytes);
        port.record(0, pc, address, 8, bytes.data());
        push_extended(pc, static_cast<long double>(std::bit_cast<double>(bytes)));
    }

    void store_extended(std::uint32_t pc, std::uint32_t address) {
        port.at(pc);
        store_word(pc, address, spill(fp.st(0), fp.status));
    }

    void spill_pop(std::uint32_t pc, std::uint32_t address) {
        store_extended(pc, address);
        fp.pop();
    }

    void discard_top(std::uint32_t pc) {
        port.at(pc);
        (void)fp.st(0);
        fp.status &= ~0x200u;
        fp.pop();
    }

    template<Trig Operation>
    void trig_top(std::uint32_t pc) {
        port.at(pc);
        std::uint16_t status;
        fp.st(0) = trig(Operation, fp.st(0), status);
        if constexpr (Operation == Trig::tangent)
            if (!(status & 0x400u)) fp.push(1.L);
        fp.status = (fp.status & ~0x600u) | (status & 0x600u);
    }

    void atan_pop(std::uint32_t pc) {
        port.at(pc);
        fp.st(1) = atan(fp.st(1), fp.st(0));
        fp.pop();
    }

    void multiply_top(std::uint32_t pc, std::uint32_t address) {
        port.at(pc);
        fp.st(0) = math::mul(fp.st(0), extended(pc, address));
    }

    void add_pop(std::uint32_t pc) {
        port.at(pc);
        fp.st(1) = math::add(fp.st(1), fp.st(0));
        fp.pop();
    }

    void prepare_angles() {
        gp[esp] = flags.minus(gp[esp], 0x50u, 4);
        push_float(0x174bf3u, gp[esi] + horizontal_angle);
        x[0] = scalar(0x174bf6u, gp[esi] + horizontal_angle);
        multiply_top(0x174bfbu, 0x46a0ccu);
        x[0] = _mm_div_ss(x[0], scalar(0x174c01u, lens_divisor));
        x[5] = scalar(0x174c09u, 0x46a0d0u);
        gp[eax] = gp[esi] + forward;
        trig_top<Trig::tangent>(0x174c14u);
        gp[ecx] = gp[eax];
        gp[edx] = word(0x174c18u, gp[ecx]);
        store_word(0x174c1au, gp[esp] + 0x14u, gp[edx]);
        gp[edx] = word(0x174c1eu, gp[ecx] + 4u);
        gp[ecx] = word(0x174c21u, gp[ecx] + 8u);
        store_scalar(0x174c24u, gp[esi] + projected_angle, x[0]);
        store_word(0x174c29u, gp[esp] + 0x1cu, gp[ecx]);
        gp[ecx] = word(0x174c2du, reference_axis);
        store_word(0x174c33u, gp[esp] + 0x18u, gp[edx]);
        discard_top(0x174c37u);
        port.at(0x174c39u);
        fp.st(0) = math::divide(fp.st(0), extended(0x174c39u, gp[esi] + aspect_ratio));
        push_double(0x174c3cu, 0x46a1d0u);
        atan_pop(0x174c42u);
        port.at(0x174c44u);
        fp.st(0) = math::add(fp.st(0), fp.st(0));
        store_extended(0x174c46u, gp[esi] + correction_angle);
        x[0] = scalar(0x174c49u, gp[ecx]);
        multiply_top(0x174c4du, 0x46a0ccu);
        x[2] = scalar(0x174c53u, gp[ecx] + 4u);
        x[7] = scalar(0x174c58u, gp[ecx] + 8u);
        x[0] = ordered_mul_ss(x[0], x[0]);
        trig_top<Trig::tangent>(0x174c61u);
        x[2] = ordered_mul_ss(x[2], x[2]);
        x[7] = ordered_mul_ss(x[7], x[7]);
        discard_top(0x174c6bu);
        duplicate_top(0x174c6du);
        multiply_top(0x174c6fu, gp[esi] + pitch_scale);
        push_extended(0x174c72u, 1.L);
        atan_pop(0x174c74u);
        port.at(0x174c76u);
        std::swap(fp.st(0), fp.st(1));
        fp.status &= ~0x200u;
        multiply_top(0x174c78u, gp[esi] + yaw_scale);
        push_extended(0x174c7bu, 1.L);
        atan_pop(0x174c7du);
        spill_pop(0x174c7fu, gp[esp] + 0xcu);
        duplicate_top(0x174c83u);
        trig_top<Trig::sine>(0x174c85u);
        spill_pop(0x174c87u, gp[esp] + 4u);
        x[1] = scalar(0x174c8bu, gp[esp] + 4u);
        trig_top<Trig::cosine>(0x174c91u);
    }

    // Project the lens-angle rotation onto the reference axis.
    void prepare_first_rotation() {
        x[3] = x[1];
        x[3] = ordered_mul_ss(x[3], scalar(0x174c96u, gp[ecx] + 4u));
        x[1] = ordered_mul_ss(x[1], scalar(0x174c9bu, gp[ecx] + 8u));
        store_scalar(0x174ca0u, gp[esp] + 0x24u, x[3]);
        x[6] = x[1];
        x[3] = x[5];
        x[3] = _mm_sub_ss(x[3], x[0]);
        x[1] = x[5];
        spill_pop(0x174cb3u, gp[esp]);
        push_float(0x174cb6u, gp[esp] + 4u);
        x[4] = scalar(0x174cbau, gp[esp]);
        multiply_top(0x174cbfu, gp[ecx]);
        x[3] = ordered_mul_ss(x[3], x[4]);
        x[3] = ordered_add_ss(x[3], x[0]);
        x[1] = _mm_sub_ss(x[1], x[4]);
        spill_pop(0x174ccdu, gp[esp] + 0x20u);
        x[0] = x[1];
        x[0] = ordered_mul_ss(x[0], scalar(0x174cd4u, gp[ecx]));
        x[0] = ordered_mul_ss(x[0], scalar(0x174cd8u, gp[ecx] + 4u));
        x[1] = ordered_mul_ss(x[1], scalar(0x174cddu, gp[ecx] + 8u));
        x[4] = x[0];
        x[4] = _mm_sub_ss(x[4], x[6]);
        store_scalar(0x174ce9u, gp[esp] + 0x38u, x[4]);
        x[0] = ordered_add_ss(x[0], x[6]);
        x[6] = scalar(0x174cf3u, gp[esp] + 0x24u);
        x[4] = x[5];
        x[4] = _mm_sub_ss(x[4], x[2]);
        x[4] = ordered_mul_ss(x[4], scalar(0x174d00u, gp[esp]));
        x[4] = ordered_add_ss(x[4], x[2]);
        x[2] = x[1];
        x[2] = ordered_mul_ss(x[2], scalar(0x174d0cu, gp[ecx]));
        x[1] = ordered_mul_ss(x[1], scalar(0x174d10u, gp[ecx] + 4u));
        store_scalar(0x174d15u, gp[esp] + 0x30u, x[0]);
        x[0] = x[2];
        x[2] = _mm_sub_ss(x[2], x[6]);
        x[0] = ordered_add_ss(x[0], x[6]);
        x[6] = x[5];
        x[6] = _mm_sub_ss(x[6], x[7]);
        x[6] = ordered_mul_ss(x[6], scalar(0x174d2du, gp[esp]));
        store_scalar(0x174d32u, gp[esp] + 0x34u, x[2]);
        x[6] = ordered_add_ss(x[6], x[7]);
        x[7] = scalar(0x174d3cu, gp[esp] + 0x20u);
        x[2] = x[1];
        gp[ecx] = gp[eax];
        flags.minus(gp[ecx], gp[eax], 4);
        x[2] = _mm_sub_ss(x[2], x[7]);
        x[1] = ordered_add_ss(x[1], x[7]);
        store_scalar(0x174d51u, gp[esp] + 0x48u, x[2]);
        store_scalar(0x174d57u, gp[esp] + 0x40u, x[1]);
        // The in-place rotation saves an original vector before overwriting it.
        gp[edx] = word(0x174d5fu, gp[ecx]);
        store_word(0x174d61u, gp[esp] + 0x20u, gp[edx]);
        gp[edx] = word(0x174d65u, gp[ecx] + 4u);
        gp[ecx] = word(0x174d68u, gp[ecx] + 8u);
        store_word(0x174d6bu, gp[esp] + 0x28u, gp[ecx]);
        store_word(0x174d6fu, gp[esp] + 0x24u, gp[edx]);
        gp[ecx] = gp[esp] + 0x20u;
    }

    // Apply the first rotation to the aiming frame and form its cross product.
    void rotate_aiming_frame() {
        x[1] = scalar(0x174d77u, gp[ecx]);
        x[1] = ordered_mul_ss(x[1], x[3]);
        x[7] = x[0];
        x[7] = ordered_mul_ss(x[7], scalar(0x174d82u, gp[ecx] + 8u));
        x[1] = ordered_add_ss(x[1], x[7]);
        x[7] = scalar(0x174d8bu, gp[esp] + 0x38u);
        x[7] = ordered_mul_ss(x[7], scalar(0x174d91u, gp[ecx] + 4u));
        x[1] = ordered_add_ss(x[1], x[7]);
        store_scalar(0x174d9au, gp[eax], x[1]);
        x[2] = ordered_mul_ss(x[2], scalar(0x174d9eu, gp[ecx] + 8u));
        x[1] = x[4];
        x[1] = ordered_mul_ss(x[1], scalar(0x174da6u, gp[ecx] + 4u));
        x[2] = ordered_add_ss(x[2], x[1]);
        x[1] = scalar(0x174dafu, gp[ecx]);
        x[1] = ordered_mul_ss(x[1], scalar(0x174db3u, gp[esp] + 0x30u));
        x[2] = ordered_add_ss(x[2], x[1]);
        store_scalar(0x174dbdu, gp[eax] + 4u, x[2]);
        x[7] = scalar(0x174dc2u, gp[ecx]);
        x[2] = scalar(0x174dc6u, gp[esp] + 0x40u);
        x[2] = ordered_mul_ss(x[2], scalar(0x174dccu, gp[ecx] + 4u));
        x[1] = x[6];
        x[1] = ordered_mul_ss(x[1], scalar(0x174dd4u, gp[ecx] + 8u));
        gp[ecx] = gp[esi] + up;
        x[2] = ordered_add_ss(x[2], x[1]);
        x[1] = scalar(0x174de0u, gp[esp] + 0x34u);
        gp[edx] = gp[ecx];
        flags.minus(gp[edx], gp[ecx], 4);
        x[7] = ordered_mul_ss(x[7], x[1]);
        x[2] = ordered_add_ss(x[2], x[7]);
        port.push(0x174df2u, gp[edi]);
        store_scalar(0x174df3u, gp[eax] + 8u, x[2]);
        // The in-place rotation saves an original vector before overwriting it.
        gp[edi] = word(0x174dfau, gp[edx]);
        store_word(0x174dfcu, gp[esp] + 0x24u, gp[edi]);
        gp[edi] = word(0x174e00u, gp[edx] + 4u);
        gp[edx] = word(0x174e03u, gp[edx] + 8u);
        store_word(0x174e06u, gp[esp] + 0x2cu, gp[edx]);
        store_word(0x174e0au, gp[esp] + 0x28u, gp[edi]);
        gp[edx] = gp[esp] + 0x24u;
        push_float(0x174e12u, gp[esp] + 0x10u);
        x[2] = scalar(0x174e16u, gp[edx] + 8u);
        trig_top<Trig::sine>(0x174e1bu);
        x[2] = ordered_mul_ss(x[2], x[0]);
        x[0] = scalar(0x174e21u, gp[edx] + 4u);
        x[0] = ordered_mul_ss(x[0], scalar(0x174e26u, gp[esp] + 0x3cu));
        x[2] = ordered_add_ss(x[2], x[0]);
        x[0] = scalar(0x174e30u, gp[edx]);
        x[0] = ordered_mul_ss(x[0], x[3]);
        x[2] = ordered_add_ss(x[2], x[0]);
        store_scalar(0x174e3cu, gp[ecx], x[2]);
        x[0] = scalar(0x174e40u, gp[edx]);
        x[0] = ordered_mul_ss(x[0], scalar(0x174e44u, gp[esp] + 0x34u));
        x[2] = scalar(0x174e4au, gp[edx] + 8u);
        x[2] = ordered_mul_ss(x[2], scalar(0x174e4fu, gp[esp] + 0x4cu));
        x[0] = ordered_add_ss(x[0], x[2]);
        x[2] = scalar(0x174e59u, gp[edx] + 4u);
        x[2] = ordered_mul_ss(x[2], x[4]);
        x[0] = ordered_add_ss(x[0], x[2]);
        store_scalar(0x174e66u, gp[ecx] + 4u, x[0]);
        x[0] = scalar(0x174e6bu, gp[edx]);
        x[0] = ordered_mul_ss(x[0], x[1]);
        x[1] = scalar(0x174e73u, gp[edx] + 4u);
        x[1] = ordered_mul_ss(x[1], scalar(0x174e78u, gp[esp] + 0x44u));
        x[0] = ordered_add_ss(x[0], x[1]);
        x[1] = scalar(0x174e82u, gp[edx] + 8u);
        x[1] = ordered_mul_ss(x[1], x[6]);
        x[0] = ordered_add_ss(x[0], x[1]);
        store_scalar(0x174e8fu, gp[ecx] + 8u, x[0]);
        x[0] = scalar(0x174e94u, gp[eax] + 4u);
        x[1] = scalar(0x174e99u, gp[ecx] + 4u);
        x[1] = ordered_mul_ss(x[1], scalar(0x174e9eu, gp[eax] + 8u));
        x[0] = ordered_mul_ss(x[0], scalar(0x174ea3u, gp[ecx] + 8u));
        x[2] = scalar(0x174ea8u, gp[eax]);
        x[2] = ordered_mul_ss(x[2], scalar(0x174eacu, gp[ecx] + 8u));
        x[3] = scalar(0x174eb1u, gp[eax] + 4u);
        x[3] = ordered_mul_ss(x[3], scalar(0x174eb6u, gp[ecx]));
        x[0] = _mm_sub_ss(x[0], x[1]);
        x[1] = scalar(0x174ebeu, gp[eax] + 8u);
        x[1] = ordered_mul_ss(x[1], scalar(0x174ec3u, gp[ecx]));
        x[1] = _mm_sub_ss(x[1], x[2]);
        x[2] = scalar(0x174ecbu, gp[ecx] + 4u);
        x[2] = ordered_mul_ss(x[2], scalar(0x174ed0u, gp[eax]));
        x[2] = _mm_sub_ss(x[2], x[3]);
    }

    // Build the second rotation around the cross product.
    void prepare_second_rotation() {
        x[4] = x[1];
        x[4] = ordered_mul_ss(x[4], x[1]);
        store_scalar(0x174edfu, gp[esp] + 0x28u, x[4]);
        x[4] = x[2];
        x[4] = ordered_mul_ss(x[4], x[2]);
        store_scalar(0x174eecu, gp[esp] + 0x2cu, x[4]);
        x[4] = x[0];
        x[3] = x[0];
        x[3] = ordered_mul_ss(x[3], x[0]);
        x[6] = x[2];
        spill_pop(0x174effu, gp[esp] + 8u);
        push_float(0x174f03u, gp[esp] + 0x10u);
        x[7] = scalar(0x174f07u, gp[esp] + 8u);
        trig_top<Trig::cosine>(0x174f0du);
        x[4] = ordered_mul_ss(x[4], x[7]);
        store_scalar(0x174f13u, gp[esp] + 0xcu, x[4]);
        x[4] = x[1];
        x[4] = ordered_mul_ss(x[4], x[7]);
        store_scalar(0x174f20u, gp[esp] + 0x10u, x[4]);
        x[6] = ordered_mul_ss(x[6], x[7]);
        x[4] = x[5];
        x[4] = _mm_sub_ss(x[4], x[3]);
        spill_pop(0x174f31u, gp[esp] + 4u);
        x[7] = scalar(0x174f35u, gp[esp] + 4u);
        x[4] = ordered_mul_ss(x[4], x[7]);
        x[4] = ordered_add_ss(x[4], x[3]);
        x[3] = x[5];
        x[3] = _mm_sub_ss(x[3], x[7]);
        store_scalar(0x174f4au, gp[esp] + 8u, x[3]);
        x[3] = ordered_mul_ss(x[3], x[1]);
        x[3] = ordered_mul_ss(x[3], x[0]);
        x[7] = x[3];
        x[7] = _mm_sub_ss(x[7], x[6]);
        x[3] = ordered_add_ss(x[3], x[6]);
        store_scalar(0x174f63u, gp[esp] + 0x34u, x[3]);
        x[3] = scalar(0x174f69u, gp[esp] + 0x28u);
        x[6] = x[5];
        x[6] = _mm_sub_ss(x[6], x[3]);
        x[6] = ordered_mul_ss(x[6], scalar(0x174f76u, gp[esp] + 4u));
        x[6] = ordered_add_ss(x[6], x[3]);
        x[3] = scalar(0x174f80u, gp[esp] + 8u);
        x[3] = ordered_mul_ss(x[3], x[2]);
        x[2] = x[3];
        x[2] = ordered_mul_ss(x[2], x[0]);
        x[0] = scalar(0x174f91u, gp[esp] + 0x10u);
        store_scalar(0x174f97u, gp[esp] + 0x3cu, x[7]);
        x[7] = x[2];
        x[2] = _mm_sub_ss(x[2], x[0]);
        x[7] = ordered_add_ss(x[7], x[0]);
        store_scalar(0x174fa8u, gp[esp] + 0x38u, x[2]);
        x[2] = scalar(0x174faeu, gp[esp] + 0x2cu);
        x[0] = x[5];
        x[0] = _mm_sub_ss(x[0], x[2]);
        x[0] = ordered_mul_ss(x[0], scalar(0x174fbbu, gp[esp] + 4u));
        x[3] = ordered_mul_ss(x[3], x[1]);
        x[0] = ordered_add_ss(x[0], x[2]);
        x[2] = scalar(0x174fc9u, gp[esp] + 0xcu);
        x[1] = x[3];
        gp[edx] = gp[eax];
        flags.minus(gp[edx], gp[eax], 4);
        x[3] = ordered_add_ss(x[3], x[2]);
        x[1] = _mm_sub_ss(x[1], x[2]);
        store_scalar(0x174fdeu, gp[esp] + 0x44u, x[3]);
        // The in-place rotation saves an original vector before overwriting it.
        gp[edi] = word(0x174fe6u, gp[edx]);
        store_word(0x174fe8u, gp[esp] + 0x24u, gp[edi]);
        gp[edi] = word(0x174fecu, gp[edx] + 4u);
        gp[edx] = word(0x174fefu, gp[edx] + 8u);
        store_word(0x174ff2u, gp[esp] + 0x2cu, gp[edx]);
        store_word(0x174ff6u, gp[esp] + 0x28u, gp[edi]);
        gp[edx] = gp[esp] + 0x24u;
    }

    // Apply the second rotation to the in-place forward and up vectors.
    void rotate_aiming_frame_again() {
        x[3] = scalar(0x174ffeu, gp[esp] + 0x3cu);
        x[3] = ordered_mul_ss(x[3], scalar(0x175004u, gp[edx] + 4u));
        x[2] = x[7];
        x[2] = ordered_mul_ss(x[2], scalar(0x17500cu, gp[edx] + 8u));
        x[2] = ordered_add_ss(x[2], x[3]);
        x[3] = x[4];
        x[3] = ordered_mul_ss(x[3], scalar(0x175018u, gp[edx]));
        x[2] = ordered_add_ss(x[2], x[3]);
        store_scalar(0x175020u, gp[eax], x[2]);
        x[2] = x[1];
        x[2] = ordered_mul_ss(x[2], scalar(0x175027u, gp[edx] + 8u));
        x[3] = x[6];
        x[3] = ordered_mul_ss(x[3], scalar(0x17502fu, gp[edx] + 4u));
        x[2] = ordered_add_ss(x[2], x[3]);
        x[3] = scalar(0x175038u, gp[esp] + 0x34u);
        x[3] = ordered_mul_ss(x[3], scalar(0x17503eu, gp[edx]));
        x[2] = ordered_add_ss(x[2], x[3]);
        store_scalar(0x175046u, gp[eax] + 4u, x[2]);
        x[2] = scalar(0x17504bu, gp[esp] + 0x44u);
        x[2] = ordered_mul_ss(x[2], scalar(0x175051u, gp[edx] + 4u));
        x[3] = x[0];
        x[3] = ordered_mul_ss(x[3], scalar(0x175059u, gp[edx] + 8u));
        x[2] = ordered_add_ss(x[2], x[3]);
        x[3] = scalar(0x175062u, gp[esp] + 0x38u);
        x[3] = ordered_mul_ss(x[3], scalar(0x175068u, gp[edx]));
        gp[edx] = gp[ecx];
        flags.minus(gp[edx], gp[ecx], 4);
        x[2] = ordered_add_ss(x[2], x[3]);
        store_scalar(0x175074u, gp[eax] + 8u, x[2]);
        // The in-place rotation saves an original vector before overwriting it.
        gp[edi] = word(0x17507bu, gp[edx]);
        store_word(0x17507du, gp[esp] + 0x24u, gp[edi]);
        gp[edi] = word(0x175081u, gp[edx] + 4u);
        gp[edx] = word(0x175084u, gp[edx] + 8u);
        store_word(0x175087u, gp[esp] + 0x2cu, gp[edx]);
        store_word(0x17508bu, gp[esp] + 0x28u, gp[edi]);
        gp[edx] = gp[esp] + 0x24u;
        x[4] = ordered_mul_ss(x[4], scalar(0x175093u, gp[edx]));
        x[2] = scalar(0x175097u, gp[edx] + 8u);
        x[2] = ordered_mul_ss(x[2], x[7]);
        x[4] = ordered_add_ss(x[4], x[2]);
        x[2] = scalar(0x1750a4u, gp[edx] + 4u);
        x[2] = ordered_mul_ss(x[2], scalar(0x1750a9u, gp[esp] + 0x3cu));
        x[4] = ordered_add_ss(x[4], x[2]);
        x[2] = scalar(0x1750b3u, gp[esp] + 0x34u);
        store_scalar(0x1750b9u, gp[ecx], x[4]);
        x[2] = ordered_mul_ss(x[2], scalar(0x1750bdu, gp[edx]));
        x[3] = scalar(0x1750c1u, gp[edx] + 8u);
        x[3] = ordered_mul_ss(x[3], x[1]);
        x[1] = scalar(0x1750cau, gp[edx] + 4u);
        x[2] = ordered_add_ss(x[2], x[3]);
        x[1] = ordered_mul_ss(x[1], x[6]);
        x[2] = ordered_add_ss(x[2], x[1]);
        x[1] = scalar(0x1750dbu, gp[esp] + 0x38u);
        store_scalar(0x1750e1u, gp[ecx] + 4u, x[2]);
        x[1] = ordered_mul_ss(x[1], scalar(0x1750e6u, gp[edx]));
        x[2] = scalar(0x1750eau, gp[edx] + 4u);
        x[2] = ordered_mul_ss(x[2], scalar(0x1750efu, gp[esp] + 0x44u));
        x[1] = ordered_add_ss(x[1], x[2]);
        x[2] = scalar(0x1750f9u, gp[edx] + 8u);
        x[2] = ordered_mul_ss(x[2], x[0]);
        x[1] = ordered_add_ss(x[1], x[2]);
        x[2] = scalar(0x175106u, gp[esp] + 0x20u);
        store_scalar(0x17510cu, gp[ecx] + 8u, x[1]);
    }

    // Project the original direction back onto the rotated frame and store the residual length.
    void finish_projection() {
        x[0] = scalar(0x175111u, gp[eax] + 4u);
        x[1] = scalar(0x175116u, gp[esp] + 0x1cu);
        x[3] = scalar(0x17511cu, gp[eax] + 8u);
        x[4] = scalar(0x175121u, gp[eax]);
        x[0] = ordered_mul_ss(x[0], x[1]);
        x[3] = ordered_mul_ss(x[3], x[2]);
        x[0] = ordered_add_ss(x[0], x[3]);
        x[3] = scalar(0x175131u, gp[esp] + 0x18u);
        x[4] = ordered_mul_ss(x[4], x[3]);
        x[0] = ordered_add_ss(x[0], x[4]);
        x[4] = scalar(0x17513fu, 0x46a0ecu);
        flags.scalar_compare(x[0], x[4], false);
        gp[edi] = port.pop(0x17514au);
        if (flags.cf || flags.zf) x[0] = x[4];
        x[5] = _mm_div_ss(x[5], x[0]);
        x[2] = ordered_mul_ss(x[2], x[5]);
        x[2] = _mm_sub_ss(x[2], scalar(0x175158u, gp[eax] + 8u));
        store_scalar(0x17515du, gp[esp] + 0x10u, x[2]);
        push_float(0x175163u, gp[esp] + 0x10u);
        multiply_top(0x175167u, gp[esp] + 0x10u);
        x[1] = ordered_mul_ss(x[1], x[5]);
        x[1] = _mm_sub_ss(x[1], scalar(0x17516fu, gp[eax] + 4u));
        store_scalar(0x175174u, gp[esp] + 0xcu, x[1]);
        push_float(0x17517au, gp[esp] + 0xcu);
        multiply_top(0x17517eu, gp[esp] + 0xcu);
        x[0] = x[5];
        x[0] = ordered_mul_ss(x[0], x[3]);
        x[0] = _mm_sub_ss(x[0], scalar(0x175189u, gp[eax]));
        add_pop(0x17518du);
        store_scalar(0x17518fu, gp[esp] + 8u, x[0]);
        push_float(0x175195u, gp[esp] + 8u);
        multiply_top(0x175199u, gp[esp] + 8u);
        add_pop(0x17519du);
        port.at(0x17519fu);
        fp.st(0) = math::root(fp.st(0));
        spill_pop(0x1751a1u, gp[esi] + 0x48u);
        gp[esp] = flags.plus(gp[esp], 0x50u, 4);
        port.ret(0x1751a7u, 0u);
        port.publish(0x1751a7u);
    }

    void run() {
        prepare_angles();
        prepare_first_rotation();
        rotate_aiming_frame();
        prepare_second_rotation();
        rotate_aiming_frame_again();
        finish_projection();
    }
};

template<class State, class Ref, class Trigonometry, class Arctangent, class Spill>
bool bridge(State &state, Ref reference, Trigonometry trig, Arctangent atan, Spill spill) {
    const static bool force_reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_OBSERVER_LENS");
        return value && *value;
    }();
    world::Publication publication;
    if (force_reference || state.active_mmx || (state.instruction_cursor && !state.live_instruction_cursor()) ||
        !native_required_owner_policy_detail::preserves_instruction_policy(state.services) ||
        !world::admitted(state, owner_sites, publication, native_particle_birth_observer_detail::qualified))
        return false;
    world::execute(state, publication, verified().calls, reference, [&](auto &port) {
        Lens<State,Trigonometry,Arctangent,Spill>{port, trig, atan, spill}.run();
    });
    return true;
}
} // namespace halo2::native_particle_observer_lens_detail
