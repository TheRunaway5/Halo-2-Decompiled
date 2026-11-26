#pragma once

#include "physics/runtime/native_tu15_world_leaves.hpp"

namespace halo2::native_swept_support_world_detail {
namespace world = native_world_leaves_detail;
namespace source = physics_step_detail;
using world::eax;
using world::ebp;
using world::ebx;
using world::ecx;
using world::edi;
using world::esi;
using world::esp;

inline constexpr world::Function functions[]{{0x2e5db0u, 0x2e5e1du, 0x3b00u, 0x3b00u}};

struct VerifySummary {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if (calls) std::fprintf(stderr, "World swept support verify: %llu calls\n",
            static_cast<unsigned long long>(calls.load()));
    }
};

template<class State, class Dispatch>
void support(world::Port<State> &port, Dispatch &dispatch) {
    auto &state = port.s;
    auto &gp = port.gp;
    auto &x = state.xmm;

    // Query the wrapped shape through its virtual method. Its volatile
    // registers and even its changes to saved pointers remain observable.
    port.push(0x2e5db0u, gp[ebp]);
    gp[ebp] = gp[esp];
    gp[esp] = port.flags.logic(gp[esp] & 0xfffffff0u, 4);
    port.push(0x2e5db6u, gp[ecx]);
    port.push(0x2e5db7u, gp[ebx]);
    gp[ebx] = port.read(0x2e5db8u, gp[ebp] + 8u, 4);
    port.push(0x2e5dbbu, gp[esi]);
    port.push(0x2e5dbcu, gp[edi]);
    gp[edi] = port.read(0x2e5dbdu, gp[ebp] + 12u, 4);
    gp[esi] = gp[ecx];
    gp[ecx] = port.read(0x2e5dc2u, gp[esi] + 0x30u, 4);
    gp[eax] = port.read(0x2e5dc5u, gp[ecx], 4);
    port.push(0x2e5dc7u, gp[edi]);
    port.push(0x2e5dc8u, gp[ebx]);
    const auto target = port.read(0x2e5dc9u, gp[eax] + 0x30u, 4);
    port.push(0x2e5dc9u, 0x2e5dccu);
    world::call_dispatch(port, target, 0x2e22c9u, dispatch);

    // The dot product selects which end of the sweep contributes the support
    // point. The stack spill is read back before the x87 comparison.
    x[1] = port.read_vector(0x2e5dccu, gp[esi] + 0x10u);
    x[2] = port.read_vector(0x2e5dd0u, gp[ebx]);
    x[0] = native_tu15_leaves::ordered_mul_ps(x[1], x[2]);
    x[2] = world::ordered_add_ss(_mm_shuffle_ps(x[0], x[0], 0x55), x[0]);
    x[3] = world::ordered_add_ss(_mm_shuffle_ps(x[0], x[0], 0xaa), x[2]);
    gp[ecx] = gp[esp] + 12u;
    port.store(0x2e5df3u, gp[ecx], 4, source::u(_mm_cvtss_f32(x[3])));

    port.at(0x2e5df7u);
    state.require_x87();
    state.fp.push(source::ext(source::f(port.read(0x2e5df7u, gp[esp] + 12u, 4))));
    port.at(0x2e5dfbu);
    state.require_x87();
    state.fp.comparison(source::ext(source::f(port.read(0x2e5dfbu, 0x46a0e8u, 4))), 1);
    gp[eax] = (gp[eax] & 0xffff0000u) | state.fp.word();
    port.flags.logic((gp[eax] >> 8u) & 0x41u, 1);

    if (port.flags.zf) {
        // The vertex's fourth lane is a feature tag. Save it before the
        // vector publication and restore it only after both stores finish.
        x[0] = port.read_vector(0x2e5e08u, gp[edi]);
        port.at(0x2e5e0bu);
        state.require_x87();
        state.fp.push(source::ext(source::f(port.read(0x2e5e0bu, gp[edi] + 12u, 4))));
        x[0] = native_tu15_leaves::ordered_add_ps(x[0], x[1]);
        port.store_vector(0x2e5e11u, gp[edi], x[0]);
        port.at(0x2e5e14u);
        state.require_x87();
        port.store(0x2e5e14u, gp[edi] + 12u, 4, source::u(source::spill(state.fp.st(0))));
        state.fp.pop();
    }

    gp[edi] = port.pop(0x2e5e17u);
    gp[esi] = port.pop(0x2e5e18u);
    gp[ebx] = port.pop(0x2e5e19u);
    gp[esp] = gp[ebp];
    gp[ebp] = port.pop(0x2e5e1cu);
    port.ret(0x2e5e1du, 8u);
    port.publish(0x2e5e1du);
}

template<class State, class Reference, class Dispatch>
bool bridge(State &state, Reference reference, Dispatch dispatch) {
    static const bool reference_mode = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_SWEPT_SUPPORT_WORLD");
        return value && *value;
    }();
    if (reference_mode) return false;
    world::Publication publication;
    if (!world::admitted(state, functions, publication)) return false;
    static VerifySummary summary;
    world::execute(state, publication, summary.calls, reference,
        [&](world::Port<State> &port) { support(port, dispatch); });
    return true;
}
} // namespace halo2::native_swept_support_world_detail
