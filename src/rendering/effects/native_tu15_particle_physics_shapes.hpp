#pragma once

#include "rendering/effects/native_tu15_particle_system_update.hpp"

namespace halo2::native_particle_physics_shapes_detail {
namespace world = native_world_leaves_detail;
namespace system = native_particle_system_update_detail;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::edx;
using world::esi;
using world::esp;

inline constexpr std::uint32_t entry = 0x2c40d0u, last = 0x2c410au, motion = 0x2c2ad0u;
inline constexpr std::uint32_t shape_argument = 8u, shape_count = 0xcu, shape_data = 0x10u, shape_stride = 0x14u;
inline constexpr world::Function sites[]{{entry, last, 0u, 0u}};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if (calls)
            std::fprintf(stderr, "Particle physics shapes verify: %llu calls\n",
                         static_cast<unsigned long long>(calls.load()));
    }
};
inline VerifySummary &summary() { static VerifySummary value; return value; }

template<class State, class Direct>
void iterate(world::Port<State> &port, Direct &direct) {
    system::Step<State,Direct> step{port,direct};
    auto &gp = port.gp;
    auto &flags = port.flags;
    port.push(entry, gp[ebx]);
    port.push(0x2c40d1u, gp[esi]);
    gp[esi] = gp[eax];
    gp[eax] = port.read(0x2c40d4u, gp[edi] + shape_count, 4);
    gp[ebx] = flags.logic(0u, 4);
    flags.logic(gp[eax], 4);
    if (!flags.zf && flags.sf == flags.of) {
        port.push(0x2c40ddu, gp[ebp]);
        gp[ebp] = flags.logic(0u, 4);
        do {
            gp[eax] = port.read(0x2c40e0u, gp[edi] + shape_argument, 4);
            gp[ecx] = port.read(0x2c40e3u, gp[esp] + 0x18u, 4);
            gp[edx] = port.read(0x2c40e7u, gp[esp] + 0x14u, 4);
            port.push(0x2c40ebu, gp[eax]);
            gp[eax] = port.read(0x2c40ecu, gp[edi] + shape_data, 4);
            port.push(0x2c40efu, gp[ecx]);
            gp[ecx] = port.read(0x2c40f0u, gp[esp] + 0x18u, 4);
            port.push(0x2c40f4u, gp[edx]);
            gp[eax] = flags.plus(gp[eax], gp[ebp], 4);
            step.template call_local<motion>(0x2c40f7u, 0x2c40fcu);

            // The motion child can change the descriptor. Read its live count
            // again before comparing the signed loop index.
            gp[eax] = port.read(0x2c40fcu, gp[edi] + shape_count, 4);
            gp[ebx] = flags.increment(gp[ebx], 4, false);
            gp[ebp] = flags.plus(gp[ebp], shape_stride, 4);
            flags.minus(gp[ebx], gp[eax], 4);
        } while (flags.sf != flags.of);
        gp[ebp] = port.pop(0x2c4107u);
    }
    gp[esi] = port.pop(0x2c4108u);
    gp[ebx] = port.pop(0x2c4109u);
    port.ret(last, 12u);
    port.publish(last);
}

template<class State, class Ref, class Direct>
bool bridge(State &state, Ref reference, Direct direct) {
    const static bool force_reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_PARTICLE_PHYSICS_SHAPES");
        return value && *value;
    }();
    world::Publication publication;
    if (force_reference || !system::admit(state, sites, publication)) return false;
    world::execute(state, publication, summary().calls, reference, [&](auto &port) { iterate(port, direct); });
    return true;
}
} // namespace halo2::native_particle_physics_shapes_detail
