#pragma once

#include "physics/runtime/physics_instruction_remap_tu15.hpp"
#include "physics/vehicles/physics_vehicle_motion_internal.hpp"

namespace halo2::native_vehicle_wheel_stage_detail {
using namespace physics_vehicle_motion_detail;

inline constexpr std::uint32_t image_delta = 0x4900u;

// The wheel stages enter through the same canonical MotionState. Provider
// operations keep their distinct PC identities and can activate tracing.
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline bool maps_stage(const PhysicsInstructionRemapTu15 &remap, std::span<const std::uint32_t> sites) {
    const auto *first = remap.find(sites[0]);
    return first && remap.bindings.size() - std::size_t(first - remap.bindings.data()) >= std::size(sites) &&
        std::equal(std::begin(sites), std::end(sites), first,
            [](auto pc, const auto &row) { return row.key == pc && row.value == pc + image_delta; });
}

struct Resume {
    std::uint32_t pc;
};

class Provider {
    MotionState &state_;
    const MotionState::InstructionCursor *cursor_;
    std::uint32_t *position_;
    const SourcePhysicsContactRuntimeServices *remap_source_{};
    std::span<const PhysicsInstructionRemapTu15::Binding> remap_bindings_;
    bool provider_returned_{};

public:
    void at(std::uint32_t pc) {
        if (provider_returned_ && !admitted()) throw Resume{pc};
        provider_returned_ = false;
        if (position_) *position_ = pc + (remap_source_ ? image_delta : 0u);
    }

    std::uint32_t word(std::uint32_t pc, std::uint32_t address, unsigned width = 4) {
        at(pc);
        const auto data = r(state_.memory, address, width);
        provider_returned_ = true;
        return data;
    }

    __m128 scalar(std::uint32_t pc, std::uint32_t address) {
        return _mm_set_ss(f(word(pc, address)));
    }

    void store_word(std::uint32_t pc, std::uint32_t address, std::uint32_t value, unsigned width = 4) {
        at(pc);
        w(state_.memory, state_.observe, pc, address, width, value);
        provider_returned_ = true;
    }

    void store(std::uint32_t pc, std::uint32_t address, __m128 value) {
        store_word(pc, address, u(_mm_cvtss_f32(value)));
    }

    Provider(MotionState &state, const MotionState::InstructionCursor *cursor,
                const PhysicsInstructionRemapTu15 *remap)
        : state_(state), cursor_(cursor), position_(cursor ? cursor->position : nullptr),
          remap_source_(remap ? remap->source : nullptr),
          remap_bindings_(remap ? remap->bindings : std::span<const PhysicsInstructionRemapTu15::Binding>{}) {}

    bool admitted() const {
        if ((_mm_getcsr() & 0x1f80u) != 0x1f80u) return false;
        if (!state_.services.instruction) return !cursor_ && !state_.observe;
        const MotionState::InstructionCursor *live;
        if (remap_source_) {
            const auto *mapped = state_.services.instruction.target<PhysicsInstructionRemapTu15>();
            if (!mapped || mapped->source != remap_source_ || mapped->bindings.data() != remap_bindings_.data() ||
                mapped->bindings.size() != remap_bindings_.size())
                return false;
            // The composition's instruction adapter is silent when its
            // wrapped service has no callback, including after a provider.
            if (!remap_source_->instruction) return !cursor_ && !state_.observe;
            live = mapped->live_cursor();
        } else {
            live = state_.literal_service_sites ? state_.live_instruction_cursor() : nullptr;
        }
        return live && live == cursor_ && live->position == position_ &&
            state_.has_literal_service_sites(*live) && (!state_.observe || live->observation_only_stores) &&
            !(live->first_trace && *live->first_trace) && !(live->second_trace && *live->second_trace);
    }
};

template<class Kernel>
std::uint32_t run_stage_with_exit(MotionState &state, std::span<const std::uint32_t> sites, Kernel kernel) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || state.active_mmx)
        return 0;
    const auto *remap = state.services.instruction.target<PhysicsInstructionRemapTu15>();
    const auto *cursor = remap ? remap->live_cursor() : state.live_instruction_cursor();
    if (remap && (!remap->source || (remap->source->instruction && (!cursor || !maps_stage(*remap, sites)))))
        return 0;
    Provider provider{state, cursor, remap};
    if (!provider.admitted()) return 0;
    try { return kernel(state, provider); }
    catch (const Resume &resume) { return resume.pc; }
}

template<class Kernel>
std::uint32_t run_stage(MotionState &state, std::span<const std::uint32_t> sites,
                        std::uint32_t continuation, Kernel kernel) {
    return run_stage_with_exit(state, sites, [&](MotionState &live, Provider &provider) {
        kernel(live, provider);
        return continuation;
    });
}
#endif

template<bool Pop, class Store>
void spill_x87_float(MotionState &state, Store store) {
    auto &fp = state.fp;
    std::uint16_t status = fp.status;
    std::uint32_t result;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1"
        : "=m"(result), "=m"(status) : "m"(fp.st(0)) : "st", "memory");
#else
    result = u(spill(fp.st(0)));
#endif
    store(result);
    // Retire rounding direction and any source pop after the store.
    // The observer can fault before this guest instruction has finished.
    fp.status = std::uint16_t((fp.status & ~0x200u) | (status & 0x200u));
    if constexpr (Pop) fp.pop();
}

template<class Store>
void spill_x87_float_and_pop(MotionState &state, Store store) {
    spill_x87_float<true>(state, store);
}
} // namespace halo2::native_vehicle_wheel_stage_detail
