#pragma once

#include "physics/runtime/native_tu15_mopp_ray_dispatch.hpp"

namespace halo2::native_mopp_ray_scale_inputs_detail {

enum class Continuation { original, load_endpoint, read_final_component, spill_components };

inline bool reference_mode() noexcept {
    static const bool reference = [] {
        const auto *value = std::getenv("HALO2_REFERENCE_MOPP_RAY_SCALE_INPUTS");
        return (value && *value) || native_raw_x87_detail::scoped_access_reference();
    }();
    return reference;
}

inline const std::byte *ordinary_bytes(const SourcePhysicsContactMemory &memory,
                                       std::uint32_t address, std::size_t width) noexcept {
    if (const auto *page = memory.direct_page(address, width))
        return page->bytes + (address & 4095u);
    const auto view = memory.try_read_view(address, width);
    return view ? view->data() : nullptr;
}

template<class State>
Continuation read_scale_inputs(State &state) {
    using namespace native_mopp_ray_dispatch_detail;
    if constexpr (std::endian::native != std::endian::little) return Continuation::original;
    if (reference_mode() || state.observe || !admitted(state)) return Continuation::original;

    auto &gp = state.gp;
    const auto *components = ordinary_bytes(state.memory, gp[6] + 1u, 2u);
    if (!components) return Continuation::original;
    gp[1] = std::to_integer<std::uint32_t>(components[0]);
    gp[2] = std::to_integer<std::uint32_t>(components[1]);
    retire(state, 0x30b36eu);

    const auto *endpoint = ordinary_bytes(state.memory, gp[7], 16u);
    if (!endpoint) return Continuation::load_endpoint;
    std::memcpy(&state.xmm[0], endpoint, 16u);

    // This spill is between the second and third opcode reads. The program
    // bytes and endpoint can alias the stack, so all reads above have been
    // consumed and the last component is loaded only after the write.
    retire(state, 0x30b375u);
    const auto spill = gp[4] + 0x14u;
    const auto component = gp[1];
    if (!state.memory.try_callback_free_write(spill, std::as_bytes(std::span{&component, 1})))
        physics_step_detail::w(state.memory, state.observe, 0x30b375u, spill, 4, component);

    const auto *final_component = ordinary_bytes(state.memory, gp[6] + 3u, 1u);
    if (!final_component) return Continuation::read_final_component;
    gp[1] = std::to_integer<std::uint32_t>(*final_component);
    retire(state, 0x30b379u);
    return Continuation::spill_components;
}

} // namespace halo2::native_mopp_ray_scale_inputs_detail
