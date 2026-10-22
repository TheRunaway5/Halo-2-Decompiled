#include "halo2/physics_motion_step.hpp"
#include "physics/runtime/physics_step_internal.hpp"

namespace halo2 {
using namespace physics_step_detail;
namespace {
// Complete paired bodies1DC750/378AB0/321600/321760 authenticate these five
// read-only operands. Mutable body, force, step and scratch addresses pass
// through unchanged. This never supplies a constant value or a motion result.
class MotionImageMemory final : public M {
    M &underlying;
    struct Binding { std::uint32_t canonical, selected, size; };
    static constexpr std::array<Binding, 5> bindings{{
        {0x465030, 0x46a0d0, 4}, {0x465048, 0x46a0e8, 4},
        {0x465044, 0x46a0e4, 4}, {0x46502c, 0x46a0cc, 4},
        {0x41b760, 0x41f1d0, 16}}};
public:
    explicit MotionImageMemory(M &m) : underlying(m) {}
    void read(std::uint32_t p, std::span<std::byte> b) const override {
        for (const auto &binding : bindings)
            if (p >= binding.canonical && std::uint64_t(p) + b.size() <= std::uint64_t(binding.canonical) + binding.size) {
                underlying.read(binding.selected + p - binding.canonical, b); return;
            }
        underlying.read(p, b);
    }
    void write(std::uint32_t p, std::span<const std::byte> b) override {
        for (const auto &binding : bindings)
            if (std::uint64_t(p) < std::uint64_t(binding.canonical) + binding.size && std::uint64_t(p) + b.size() > binding.canonical)
                throw std::runtime_error("Write to immutable original motion operand");
        underlying.write(p, b);
    }
};
// Complete31F8B0, including the three intermediate aliased output stores.
void quaternion_product(M &m, std::uint32_t out, std::uint32_t a,
    std::uint32_t b, std::uint32_t esp, const O &o) {
    const auto frame = ((esp - 4u) & ~15u) - 12u;
    auto bv = v(m, b), av = v(m, a);
    auto result = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(av, av, 0xc9), _mm_shuffle_ps(bv, bv, 0xd2)),
        _mm_mul_ps(_mm_shuffle_ps(av, av, 0xd2), _mm_shuffle_ps(bv, bv, 0xc9)));
    vw(m, o, 0x31f8ea, out, result);
    bv = v(m, b);
    w(m, o, 0x31f8f4, frame + 8, 4, r(m, a + 12));
    result = _mm_add_ps(result, _mm_mul_ps(bv, _mm_set1_ps(f(r(m, frame + 8)))));
    vw(m, o, 0x31f90b, out, result);
    const auto bw = r(m, b + 12); av = v(m, a);
    w(m, o, 0x31f914, frame + 8, 4, bw);
    result = _mm_add_ps(result, _mm_mul_ps(av, _mm_set1_ps(f(r(m, frame + 8)))));
    vw(m, o, 0x31f92b, out, result);
    const auto product = mul(ext(f(r(m, a + 12))), ext(f(r(m, b + 12))));
    const auto dot = _mm_mul_ps(v(m, a), v(m, b));
    const auto yx = _mm_add_ss(_mm_shuffle_ps(dot, dot, 0x55), dot);
    const auto xyz = _mm_add_ss(_mm_shuffle_ps(dot, dot, 0xaa), yx);
    w(m, o, 0x31f957, frame + 8, 4, u(_mm_cvtss_f32(xyz)));
    w(m, o, 0x31f960, out + 12, 4, u(spill(sub(product, ext(f(r(m, frame + 8)))))));
}
}
void source_apply_physics_angular_impulse_1_1(M &m, std::uint32_t motion,
    std::uint32_t impulse, FP fp, const O &o) {
    FpScope scope(fp);
    const auto z = v(m, motion + 0xa0), x = v(m, motion + 0x80), y = v(m, motion + 0x90);
    const auto xylo = _mm_shuffle_ps(x, y, 0x44), xyhi = _mm_shuffle_ps(x, y, 0xee);
    const auto zlo = _mm_shuffle_ps(z, z, 0x44), zhi = _mm_shuffle_ps(z, z, 0xee);
    const auto iv = v(m, impulse);
    auto local = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(xylo, zlo, 0x88), _mm_shuffle_ps(iv, iv, 0)),
        _mm_mul_ps(_mm_shuffle_ps(xylo, zlo, 0xdd), _mm_shuffle_ps(iv, iv, 0x55)));
    local = _mm_add_ps(local, _mm_mul_ps(_mm_shuffle_ps(xyhi, zhi, 0x88), _mm_shuffle_ps(iv, iv, 0xaa)));
    local = _mm_mul_ps(v(m, motion + 0xe0), local);
    auto world = _mm_add_ps(
        _mm_mul_ps(v(m, motion + 0x80), _mm_shuffle_ps(local, local, 0)),
        _mm_mul_ps(v(m, motion + 0x90), _mm_shuffle_ps(local, local, 0x55)));
    world = _mm_add_ps(world, _mm_mul_ps(v(m, motion + 0xa0), _mm_shuffle_ps(local, local, 0xaa)));
    vw(m, o, 0x3215ea, motion + 0x50, _mm_add_ps(v(m, motion + 0x50), world));
}
namespace {
constexpr std::array<std::uint32_t, 13> force_sites{0x32177d, 0x32178a, 0x32179e, 0x3217bc, 0x3217e4, 0x3217fe, 0x32180d, 0x321812, 0x321829, 0x321842, 0x321851, 0x32187c, 0x321890};
constexpr std::array<std::uint32_t, 13> prepare_sites{0x3218c2, 0x3218cf, 0x3218e3, 0x321901, 0x321929, 0x321943, 0x321952, 0x321957, 0x32196e, 0x321989, 0x321998, 0x3219c2, 0x3219e6};
constexpr std::array<std::uint32_t, 15> pose_sites{0x321618, 0x32161f, 0x32162a, 0x321630, 0x32164b, 0x321674, 0x32167d, 0x32168c, 0x321695, 0x3216a3, 0x3216cb, 0x3216dc, 0x3216f8, 0x321748, 0x321750};
constexpr std::array<std::uint32_t, 15> readback_sites{0x321aea, 0x321af1, 0x321afc, 0x321b02, 0x321b1d, 0x321b46, 0x321b4f, 0x321b5e, 0x321b66, 0x321b74, 0x321b97, 0x321bb0, 0x321bcc, 0x321c19, 0x321c20};
void apply_motion_forces(M &m, std::uint32_t motion,
    std::uint32_t step, std::uint32_t gravity, std::uint32_t esp, FP fp, const std::array<std::uint32_t, 13> &sites, const O &o) {
    FpScope scope(fp); const auto frame = ((esp - 4u) & ~15u) - 0x30u;
    const auto dt = ext(f(r(m, step)));
    const auto force = v(m, motion + 0xf0);
    w(m, o, sites[0], frame + 12, 4, u(spill(mul(dt, ext(f(r(m, motion + 0x2c)))))));
    const auto scaled = _mm_mul_ps(force, _mm_set1_ps(f(r(m, frame + 12))));
    w(m, o, sites[1], frame + 12, 4, u(spill(dt)));
    vw(m, o, sites[2], motion + 0x40, _mm_add_ps(v(m, motion + 0x40), scaled));
    vw(m, o, sites[3], motion + 0x40, _mm_add_ps(v(m, motion + 0x40),
        _mm_mul_ps(v(m, gravity), _mm_set1_ps(f(r(m, frame + 12))))));
    auto damping = sub(ext(f(r(m, 0x465030))), mul(dt, ext(f(r(m, motion + 0x14)))));
    const auto zero = ext(f(r(m, 0x465048)));
    if (zero > damping) damping = zero;
    const auto linear = v(m, motion + 0x40);
    w(m, o, sites[4], frame + 12, 4, u(spill(damping)));
    vw(m, o, sites[5], motion + 0x40, _mm_mul_ps(linear, _mm_set1_ps(f(r(m, frame + 12)))));
    // FUCOMPP + TEST AH,44 + JNP: only ordered equality skips torque.
    const auto torque_w = ext(f(r(m, motion + 0x10c)));
    const bool torque_path = torque_w != zero;
    vw(m, o, sites[6], frame + 32, _mm_setzero_ps());
    vw(m, o, sites[7], motion + 0xf0, _mm_setzero_ps());
    if (torque_path) {
        const auto dt_word = r(m, step);
        const auto torque = v(m, motion + 0x100);
        w(m, o, sites[8], frame + 12, 4, dt_word);
        vw(m, o, sites[9], frame + 16, _mm_mul_ps(torque, _mm_set1_ps(f(dt_word))));
        source_apply_physics_angular_impulse_1_1(m, motion, frame + 16, fp, o);
        vw(m, o, sites[10], motion + 0x100, v(m, frame + 32));
    }
    damping = sub(ext(f(r(m, 0x465030))), mul(dt, ext(f(r(m, motion + 0x18)))));
    if (zero > damping) damping = zero;
    const auto angular = v(m, motion + 0x50);
    w(m, o, sites[11], frame + 12, 4, u(spill(damping)));
    vw(m, o, sites[12], motion + 0x50, _mm_mul_ps(angular, _mm_set1_ps(f(r(m, frame + 12)))));
}
void advance_motion_pose(M &m, std::uint32_t motion,
    std::uint32_t step, std::uint32_t esp, FP fp,
    const SourcePhysicsReciprocalSqrtEstimate &estimate,
    const std::array<std::uint32_t, 15> &sites, const O &o) {
    FpScope scope(fp); const auto frame = ((esp - 4u) & ~15u) - 0x24u - 8u;
    const auto linear = v(m, motion + 0x40), angular = v(m, motion + 0x50);
    vw(m, o, sites[0], motion + 0xc0, linear);
    vw(m, o, sites[1], motion + 0xd0, angular);
    vw(m, o, sites[2], motion + 0x60, v(m, motion + 0x70));
    w(m, o, sites[3], frame + 8, 4, r(m, step));
    vw(m, o, sites[4], motion + 0x70, _mm_add_ps(v(m, motion + 0x70),
        _mm_mul_ps(linear, _mm_set1_ps(f(r(m, frame + 8))))));
    const auto radius_dt = mul(ext(f(r(m, motion + 12))), ext(f(r(m, step))));
    auto squares = _mm_mul_ps(v(m, motion + 0x50), v(m, motion + 0x50));
    const auto xy = _mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares);
    auto length = _mm_add_ss(_mm_shuffle_ps(squares, squares, 0xaa), xy);
    vw(m, o, sites[5], frame + 12, length);
    length = _mm_sqrt_ss(length);
    w(m, o, sites[6], frame + 12, 4, u(_mm_cvtss_f32(length)));
    w(m, o, sites[7], frame + 8, 4, u(_mm_cvtss_f32(v(m, frame + 12))));
    const auto travel = mul(radius_dt, ext(f(r(m, frame + 8))));
    w(m, o, sites[8], frame + 8, 4, 0x3f000000);
    w(m, o, sites[9], motion + 0x6c, 4, u(spill(travel)));
    const auto half = _mm_mul_ps(v(m, motion + 0x50), _mm_set1_ps(f(r(m, frame + 8))));
    vw(m, o, sites[10], frame + 12, _mm_and_ps(half, v(m, 0x41b760)));
    quaternion_product(m, frame + 28, frame + 12, motion + 0x30, frame - 16u, o);
    w(m, o, sites[11], frame + 8, 4, r(m, step));
    vw(m, o, sites[12], motion + 0x30, _mm_add_ps(v(m, motion + 0x30),
        _mm_mul_ps(v(m, frame + 28), _mm_set1_ps(f(r(m, frame + 8))))));
    source_physics_normalize_quaternion_1_1(m, motion + 0x30, estimate, fp, o);
    source_physics_quaternion_matrix_1_1(m, motion + 0x80, motion + 0x30, fp, o);
    const auto center = v(m, motion + 0x20);
    auto offset = _mm_add_ps(
        _mm_mul_ps(v(m, motion + 0x80), _mm_shuffle_ps(center, center, 0)),
        _mm_mul_ps(v(m, motion + 0x90), _mm_shuffle_ps(center, center, 0x55)));
    offset = _mm_add_ps(offset, _mm_mul_ps(v(m, motion + 0xa0), _mm_shuffle_ps(center, center, 0xaa)));
    vw(m, o, sites[13], motion + 0xb0, _mm_sub_ps(v(m, motion + 0x70), offset));
    w(m, o, sites[14], motion + 0x7c, 4, 0x3f800000);
}
// Complete2E0D30: preserve scalar store/read order, including aliasing.
void transpose_solver_matrix(M &m, std::uint32_t out, std::uint32_t in, const O &o) {
    w(m, o, 0x2e0d36, out, 4, r(m, in));
    w(m, o, 0x2e0d3b, out + 20, 4, r(m, in + 20));
    w(m, o, 0x2e0d41, out + 40, 4, r(m, in + 40));
    w(m, o, 0x2e0d46, out + 12, 4, 0);
    w(m, o, 0x2e0d49, out + 28, 4, 0);
    w(m, o, 0x2e0d4c, out + 44, 4, 0);
    w(m, o, 0x2e0d52, out + 4, 4, r(m, in + 16));
    w(m, o, 0x2e0d58, out + 16, 4, r(m, in + 4));
    w(m, o, 0x2e0d5e, out + 8, 4, r(m, in + 32));
    w(m, o, 0x2e0d64, out + 32, 4, r(m, in + 8));
    w(m, o, 0x2e0d6a, out + 24, 4, r(m, in + 36));
    w(m, o, 0x2e0d70, out + 36, 4, r(m, in + 24));
}
__m128 rotate_solver_velocity(const M &m, std::uint32_t matrix, __m128 x) {
    const auto z = _mm_mul_ps(v(m, matrix + 32), _mm_shuffle_ps(x, x, 0xaa));
    const auto y = _mm_mul_ps(v(m, matrix + 16), _mm_shuffle_ps(x, x, 0x55));
    const auto xx = _mm_mul_ps(v(m, matrix), _mm_shuffle_ps(x, x, 0));
    return _mm_add_ps(_mm_add_ps(xx, y), z);
}
} // namespace
void source_apply_physics_motion_forces_1_1(M &m, std::uint32_t motion,
    std::uint32_t step, std::uint32_t gravity, std::uint32_t esp, FP fp, const O &o) {
    apply_motion_forces(m, motion, step, gravity, esp, fp, force_sites, o);
}
void source_integrate_physics_motion_1_1(M &m, std::uint32_t motion,
    std::uint32_t step, std::uint32_t esp, FP fp,
    const SourcePhysicsReciprocalSqrtEstimate &estimate, const O &o) {
    advance_motion_pose(m, motion, step, esp, fp, estimate, pose_sites, o);
}
std::uint32_t source_prepare_physics_solver_motion_1_1(M &m, std::uint32_t motion,
    std::uint32_t step, std::uint32_t gravity, std::uint32_t packet,
    std::uint32_t esp, FP fp, const O &o) {
    FpScope scope(fp);
    apply_motion_forces(m, motion, step, gravity, esp, fp, prepare_sites, o);
    transpose_solver_matrix(m, packet + 0x50, motion + 0x80, o);
    w(m, o, 0x3219ef, packet, 1, 2);
    vw(m, o, 0x3219f9, packet + 0x30, v(m, motion + 0xe0));
    vw(m, o, 0x321a01, packet + 0x10, v(m, motion + 0x40));
    vw(m, o, 0x321a09, packet + 0x40, v(m, motion + 0x70));
    vw(m, o, 0x321a40, packet + 0x20,
        rotate_solver_velocity(m, packet + 0x50, v(m, motion + 0x50)));
    w(m, o, 0x321a44, packet + 12, 1, 0);
    return packet + 0x80;
}
std::uint32_t source_readback_physics_solver_motion_1_1(M &m, std::uint32_t motion,
    std::uint32_t step, std::uint32_t scale_word, std::uint32_t packet,
    std::uint32_t esp, FP fp, const SourcePhysicsReciprocalSqrtEstimate &estimate,
    const O &o) {
    FpScope scope(fp); const auto scalar = ((esp - 4u) & ~15u) - 0x24u;
    w(m, o, 0x321a6c, scalar, 4, scale_word);
    const auto scale = _mm_set1_ps(f(r(m, scalar)));
    vw(m, o, 0x321a89, motion + 0x40, _mm_mul_ps(scale, v(m, packet + 0x40)));
    const auto averaged_angular = rotate_solver_velocity(m, motion + 0x80, v(m, packet + 0x50));
    vw(m, o, 0x321ae2, motion + 0x50, _mm_mul_ps(scale, averaged_angular));
    // Both complete bodies use the same physical pose temporaries. Their
    // register-save timing differs; each persistent store retains its own
    // source site. Pose advancement consumes the averaged solver velocities.
    advance_motion_pose(m, motion, step, esp, fp, estimate, readback_sites, o);
    vw(m, o, 0x321c2b, motion + 0x40, v(m, packet + 0x10));
    vw(m, o, 0x321c62, motion + 0x50,
        rotate_solver_velocity(m, motion + 0x80, v(m, packet + 0x20)));
    vw(m, o, 0x321c74, motion + 0x40, _mm_and_ps(v(m, motion + 0x40), v(m, 0x41b760)));
    vw(m, o, 0x321c87, motion + 0x50, _mm_and_ps(v(m, motion + 0x50), v(m, 0x41b760)));
    return packet + 0x80;
}
void source_apply_physics_motion_forces(M &m, std::uint32_t motion,
    std::uint32_t step, std::uint32_t gravity, std::uint32_t esp,
    SourcePhysicsContactImage image, FP fp, const O &o) {
    if (image == SourcePhysicsContactImage::MapPack11)
        source_apply_physics_motion_forces_1_1(m, motion, step, gravity, esp, fp, o);
    else if (image == SourcePhysicsContactImage::TitleUpdate15) {
        MotionImageMemory bound(m);
        source_apply_physics_motion_forces_1_1(bound, motion, step, gravity, esp, fp, o);
    } else throw std::runtime_error("Unverified physics motion executable image");
}
void source_integrate_physics_motion(M &m, std::uint32_t motion,
    std::uint32_t step, std::uint32_t esp, SourcePhysicsContactImage image,
    FP fp, const SourcePhysicsReciprocalSqrtEstimate &estimate, const O &o) {
    if (image == SourcePhysicsContactImage::MapPack11)
        source_integrate_physics_motion_1_1(m, motion, step, esp, fp, estimate, o);
    else if (image == SourcePhysicsContactImage::TitleUpdate15) {
        MotionImageMemory bound(m);
        source_integrate_physics_motion_1_1(bound, motion, step, esp, fp, estimate, o);
    } else throw std::runtime_error("Unverified physics motion executable image");
}
std::uint32_t source_prepare_physics_solver_motion(M &m, std::uint32_t motion,
    std::uint32_t step, std::uint32_t gravity, std::uint32_t packet,
    std::uint32_t esp, SourcePhysicsContactImage image, FP fp, const O &o) {
    if (image == SourcePhysicsContactImage::MapPack11)
        return source_prepare_physics_solver_motion_1_1(m, motion, step, gravity, packet, esp, fp, o);
    if (image == SourcePhysicsContactImage::TitleUpdate15) {
        MotionImageMemory bound(m);
        return source_prepare_physics_solver_motion_1_1(bound, motion, step, gravity, packet, esp, fp, o);
    }
    throw std::runtime_error("Unverified physics solver-motion executable image");
}
std::uint32_t source_readback_physics_solver_motion(M &m, std::uint32_t motion,
    std::uint32_t step, std::uint32_t scale, std::uint32_t packet, std::uint32_t esp,
    SourcePhysicsContactImage image, FP fp, const SourcePhysicsReciprocalSqrtEstimate &estimate,
    const O &o) {
    if (image == SourcePhysicsContactImage::MapPack11)
        return source_readback_physics_solver_motion_1_1(m, motion, step, scale, packet, esp, fp, estimate, o);
    if (image == SourcePhysicsContactImage::TitleUpdate15) {
        MotionImageMemory bound(m);
        return source_readback_physics_solver_motion_1_1(bound, motion, step, scale, packet, esp, fp, estimate, o);
    }
    throw std::runtime_error("Unverified physics solver-motion executable image");
}
} // namespace halo2
