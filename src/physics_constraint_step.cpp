#include "halo2/physics_constraint_step.hpp"
#include "halo2/physics_jacobian.hpp"
#include "physics_step_internal.hpp"

namespace halo2 {
using namespace physics_step_detail;
void source_step_physics_constraints_1_1(M &m, const SourcePhysicsConstraintStepInput &in,
    std::uint32_t esp, FP fp, const SourcePhysicsConstraintStepServices &s, const O &o) {
    FpScope scope(fp); const auto frame = ((esp - 4u) & ~15u) - 0x40u;
    const auto required = [&](std::uint32_t site, std::uint32_t entry,
        std::optional<std::uint32_t> receiver, std::uint32_t entry_esp,
        std::vector<std::uint32_t> args) {
        if (!s.required_owner) throw std::runtime_error("Required original constraint solver/Jacobian/allocator owner unavailable");
        return s.required_owner(m, {site, entry, entry_esp, receiver, std::move(args)}, o);
    };
    w(m, o, 0x3226a2, frame + 0x14, 4, 0);
    w(m, o, 0x3226a6, frame + 0x28, 4, 0);
    auto arena = r(m, 0x48c058);
    w(m, o, 0x3226af, arena + 0x10, 1, 1);
    auto base = r(m, arena + 8);
    const auto tail_bytes = r(m, in.island + 0x18) * 4u + 8u;
    auto limit = base + r(m, r(m, 0x48c058) + 12);
    w(m, o, 0x3226c8, frame + 0x24, 4, tail_bytes);
    w(m, o, 0x3226cc, frame + 0x38, 4, base);
    // Each allocation branch preserves the original arena bank layout and
    // ordered allocator stores. Virtual allocation remains a real dependency.
    auto second = base;
    const auto lay_out = [&] {
        const auto first = base + r(m, in.island + 0x14) + 0x90u;
        w(m, o, 0x3226e1, frame + 0x1c, 4, first);
        second = first + r(m, in.island + 12);
        w(m, o, 0x3226e7, frame + 0x10, 4, base);
        w(m, o, 0x3226eb, frame + 0x18, 4, second);
    };
    lay_out();
    for (;;) {
        const auto tail = r(m, frame + 0x24), schema_bytes = r(m, in.island + 0x10);
        w(m, o, 0x3226f7, frame + 0x2c, 4, second);
        const auto schemas = second + tail;
        w(m, o, 0x3226fd, frame + 0x30, 4, schemas);
        const auto total_end = schemas + schema_bytes + 4u;
        if (total_end <= limit) break;
        const auto first = r(m, frame + 0x1c);
        auto allocator = r(m, 0x487638);
        auto available = r(m, allocator + 12);
        if (first >= limit) {
            const auto requested = total_end - r(m, frame + 0x38);
            const auto aligned = (requested + 15u) & ~15u;
            w(m, o, 0x3227fa, frame + 0x34, 4, requested);
            std::uint32_t allocated;
            if (si(aligned) > si(available))
                allocated = required(0x322803, r(m, r(m, allocator) + 0x24), allocator, frame - 8u, {aligned});
            else {
                allocated = r(m, allocator + 8);
                w(m, o, 0x322820, allocator + 8, 4, allocated + aligned);
                w(m, o, 0x322823, allocator + 12, 4, available - aligned);
            }
            w(m, o, si(aligned) > si(available) ? 0x322806 : 0x322826, frame + 0x14, 4, allocated);
            limit = allocated + requested; base = allocated; lay_out();
        } else if (r(m, frame + 0x18) < limit) {
            const auto requested = schema_bytes + tail + 4u;
            const auto aligned = (requested + 15u) & ~15u;
            std::uint32_t allocated;
            if (si(aligned) > si(available)) {
                allocated = required(0x3227ad, r(m, r(m, allocator) + 0x24), allocator, frame - 8u, {aligned});
                w(m, o, 0x3227b4, frame + 0x14, 4, allocated);
            } else {
                allocated = r(m, allocator + 8);
                w(m, o, 0x3227c9, allocator + 8, 4, allocated + aligned);
                w(m, o, 0x3227cf, allocator + 12, 4, available - aligned);
                w(m, o, 0x3227d6, frame + 0x14, 4, allocated);
            }
            base = r(m, frame + 0x10); limit = allocated + requested; second = allocated;
        } else {
            const auto extra = r(m, in.island + 8) * 2u - limit + r(m, in.island + 12) + first;
            const auto requested = schema_bytes + extra + tail + 4u;
            const auto aligned = (requested + 15u) & ~15u;
            w(m, o, 0x32274a, frame + 0x30, 4, requested);
            std::uint32_t allocated;
            if (si(aligned) > si(available))
                allocated = required(0x322753, r(m, r(m, allocator) + 0x24), allocator, frame - 8u, {aligned});
            else {
                allocated = r(m, allocator + 8);
                w(m, o, 0x32275b, frame + 0x34, 4, allocated);
                w(m, o, 0x322767, allocator + 8, 4, allocated + aligned);
                w(m, o, 0x32276e, allocator + 12, 4, available - aligned);
            }
            w(m, o, 0x32277b, frame + 0x18, 4, limit - r(m, in.island + 8));
            w(m, o, 0x322782, frame + 0x14, 4, allocated);
            w(m, o, 0x322786, frame + 0x28, 4, allocated);
            base = r(m, frame + 0x10); limit = allocated + requested; second = allocated + extra;
        }
    }
    base = r(m, frame + 0x10);
    w(m, o, 0x322840, base, 1, 2);
    for (const auto [site, offset] : std::array<std::pair<std::uint32_t, std::uint32_t>, 7>{{
        {0x322843, 0x30}, {0x322847, 0x10}, {0x32284b, 0x20}, {0x32284f, 0x40},
        {0x322853, 0x50}, {0x32285d, 0x60}, {0x322869, 0x70}}}) {
        if (offset == 0x70) {
            w(m, o, 0x322861, frame + 0x38, 4, in.bodies);
            w(m, o, 0x322865, frame + 0x34, 4, in.bodies + in.body_count * 4u);
        }
        vw(m, o, site, base + offset, _mm_setzero_ps());
    }
    auto packet = base + 0x80u;
    w(m, o, 0x32286d, frame + 0x3c, 4, packet);
    for (auto cursor = in.bodies; cursor < r(m, frame + 0x34); cursor += 4u) {
        const auto motion = r(m, r(m, cursor) + 0x3c);
        const auto offset = packet - r(m, frame + 0x10);
        if (r(m, motion + 8) != offset) w(m, o, 0x322885, motion + 8, 4, offset);
        const auto entry = r(m, r(m, motion) + 12);
        if (entry == 0x3218a0)
            packet = source_prepare_physics_solver_motion_1_1(m, motion, in.step, in.gravity, packet, frame - 16u, fp, o);
        else packet = required(0x322893, entry, motion, frame - 16u, {in.step, in.gravity, packet});
    }
    w(m, o, 0x3228ac, packet, 1, 3);
    w(m, o, 0x3228b2, frame + 0x24, 4, r(m, frame + 0x18));
    w(m, o, 0x3228bd, frame + 0x18, 4, in.constraints + in.constraint_count * 4u);
    w(m, o, 0x3228c5, frame + 0x1c, 4, r(m, frame + 0x1c));
    w(m, o, 0x3228cd, frame + 0x20, 4, r(m, frame + 0x30));
    for (auto cursor = in.constraints; cursor < r(m, frame + 0x18); cursor += 4u) {
        const auto constraint = r(m, cursor);
        const auto b = r(m, r(m, constraint + 0x10) + 0x3c);
        w(m, o, 0x3228de, frame + 0x38, 4, b);
        const auto a = r(m, r(m, constraint + 12) + 0x3c);
        base = r(m, frame + 0x10);
        w(m, o, 0x3228ed, in.jacobian_context, 4, base + r(m, a + 8));
        w(m, o, 0x3228fc, in.jacobian_context + 4, 4, base + r(m, r(m, frame + 0x38) + 8));
        if (r(m, frame + 0x1c) >= r(m, frame + 0x24)) {
            w(m, o, 0x322909, frame + 0x1c, 4, r(m, frame + 0x28));
            w(m, o, 0x32290d, frame + 0x24, 4, 0xffffffff);
        }
        const auto producer = r(m, r(m, constraint) + 0x24);
        const std::array<std::uint32_t, 2> arguments{in.jacobian_context, frame + 0x1c};
        if (s.jacobians && (producer == 0x31bbe0 || producer == 0x31bd00 || producer == 0x322380)) {
            //322921's actual stdcall return frame and nonvolatile registers.
            // Its callers discard EAX; all outputs reside in retained banks.
            w(m, {}, 0, frame - 12u, 4, 0x322924);
            source_build_physics_jacobians(m, producer, constraint, arguments,
                frame - 12u, SourcePhysicsContactImage::MapPack11, fp, *s.jacobians, o,
                {in.island, esp - 4u, cursor, limit});
        } else required(0x322921, producer, constraint, frame - 12u,
            {arguments.begin(), arguments.end()});
    }
    w(m, o, 0x322937, r(m, frame + 0x20), 4, 0x400);
    for (std::uint32_t i = 0; si(i) < si(r(m, in.island + 0x18)); ++i)
        w(m, o, 0x322954, r(m, frame + 0x2c) + i * 4u, 4, 0);
    std::uint32_t result;
    if (s.solver) {
        // Native call marshalling belongs to the caller ABI, outside the
        // solver's observed instruction interval. Preserve the actual source
        // return address and nonvolatile values used by its prologue.
        w(m, {}, 0, frame - 20u, 4, 0x322977);
        result = source_solve_physics_constraints(m, in.solver_configuration,
            r(m, frame + 0x30), r(m, frame + 0x10), r(m, frame + 0x2c),
            frame - 20u, SourcePhysicsContactImage::MapPack11, fp, *s.solver, o,
            {in.island, esp - 4u, in.solver_configuration, limit});
    } else result = required(0x322972, 0x322ed0, std::nullopt, frame - 20u,
        {in.solver_configuration, r(m, frame + 0x30), r(m, frame + 0x10), r(m, frame + 0x2c)});
    const auto profile = r(m, 0x487674);
    if (profile < r(m, 0x487678)) {
        w(m, o, 0x32298a, profile, 4, 0x41ba34);
        w(m, o, 0x322996, profile + 4, 4, u(spill(static_cast<long double>(si(r(m, in.island + 0x18))))));
        w(m, o, 0x322999, 0x487674, 4, profile + 8u);
    }
    if (result == 1) {
        const auto scale = r(m, in.solver_configuration + 0x20);
        packet = r(m, frame + 0x3c);
        w(m, o, 0x3229b4, frame + 0x38, 4, scale);
        for (auto cursor = in.bodies; cursor < r(m, frame + 0x34); cursor += 4u) {
            const auto motion = r(m, r(m, cursor) + 0x3c), entry = r(m, r(m, motion) + 0x10);
            if (entry == 0x321a60)
                packet = source_readback_physics_solver_motion_1_1(m, motion, in.step, scale, packet, frame - 16u, fp, s.reciprocal_sqrt, o);
            else packet = required(0x3229d1, entry, motion, frame - 16u, {in.step, scale, packet});
        }
    }
    const auto allocation = r(m, frame + 0x14);
    w(m, o, 0x3229e7, r(m, 0x48c058) + 0x10, 1, 0);
    if (allocation) {
        const auto bytes = (limit - allocation + 15u) & ~15u;
        const auto allocator = r(m, 0x487638);
        if (bytes && (allocation == r(m, allocator + 0x14) || !r(m, allocator + 0x14))) {
            required(0x322a0c, r(m, r(m, allocator) + 0x28), allocator, frame - 12u, {allocation, bytes});
            return;
        }
        const auto top = r(m, allocator + 8), available = r(m, allocator + 12);
        w(m, o, 0x322a20, allocator + 8, 4, top - bytes);
        w(m, o, 0x322a23, allocator + 12, 4, available + bytes);
    }
}
} // namespace halo2
