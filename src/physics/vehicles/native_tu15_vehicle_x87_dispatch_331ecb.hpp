#pragma once

#include "physics/runtime/physics_instruction_remap_tu15.hpp"
#include "physics/vehicles/physics_vehicle_motion_math_internal.hpp"

namespace halo2::native_vehicle_x87_dispatch_331ecb_detail {
using namespace physics_vehicle_motion_math_detail;

enum class Continuation {
    original, choose_control, alter_saved_control, load_control, classify_first,
    save_first_status, clear_error, swap_inputs, classify_second, restore_inputs,
    decode_high_status, keep_high_class, combine_classes, dispatch
};

inline bool reference_dispatch() {
    static const bool reference = [] {
        const auto *setting = std::getenv("HALO2_REFERENCE_VEHICLE_X87_DISPATCH_331ECB");
        return setting && *setting;
    }();
    return reference;
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline constexpr std::uint32_t image_delta = 0x3b70u;
inline constexpr std::uint32_t canonical_sites[]{
    0x32e35bu, 0x32e35fu, 0x32e361u, 0x32e368u, 0x32e36bu, 0x32e36eu,
    0x32e370u, 0x32e372u, 0x32e376u, 0x32e37du, 0x32e383u, 0x32e388u,
    0x32e38au, 0x32e390u, 0x32e391u, 0x32e397u, 0x32e39eu, 0x32e3a0u,
    0x32e3a6u, 0x32e3a8u, 0x32e3a9u, 0x32e3afu, 0x32e3b1u, 0x32e3b7u,
    0x32e3b9u, 0x32e3bbu, 0x32e3bdu, 0x32e3bfu, 0x32e3c1u, 0x32e3c2u,
    0x32e3c4u, 0x32e3c6u, 0x32e3c8u, 0x32e3cau, 0x32e3ccu, 0x32e3ceu,
    0x32e3cfu, 0x32e3d1u, 0x32e3d3u, 0x32e3d5u, 0x32e3d8u, 0x32e3deu,
    0x32e3e0u, 0x32e3e2u, 0x32e3e5u
};

inline bool authentic_sites(const PhysicsInstructionRemapTu15 &remap) {
    const auto *first = remap.find(canonical_sites[0]);
    if (!first || remap.bindings.size() - std::size_t(first - remap.bindings.data()) < std::size(canonical_sites))
        return false;
    return std::equal(std::begin(canonical_sites), std::end(canonical_sites), first,
        [](auto pc, const auto &row) { return row.key == pc && row.value == pc + image_delta; });
}

class Classifier {
    MathState &state_;
    const MathState::InstructionCursor *cursor_;
    std::uint32_t *position_;
    const SourcePhysicsContactRuntimeServices *remap_source_{};
    std::span<const PhysicsInstructionRemapTu15::Binding> remap_bindings_;

    void at(std::uint32_t pc) const {
        if (position_) *position_ = pc + (remap_source_ ? image_delta : 0u);
    }

    std::uint32_t read(std::uint32_t pc, std::uint32_t address, unsigned width) const {
        at(pc);
        return r(state_.memory, address, width);
    }

    void store(std::uint32_t pc, std::uint32_t address, unsigned width, std::uint32_t value) {
        at(pc);
        w(state_.memory, state_.observe, pc, address, width, value);
    }

    std::uint32_t local(unsigned offset) const { return state_.gp[5] - offset; }

    void swap_inputs(std::uint32_t pc) {
        at(pc);
        std::swap(state_.fp.st(0), state_.fp.st(1));
        state_.fp.status &= ~0x200u;
    }

    void classify_and_save(std::uint32_t classify_pc, std::uint32_t store_pc) {
        at(classify_pc);
        state_.classify();
        store(store_pc, local(0xa0u), 2, state_.fp.word());
    }

    // This is the status byte after SHL, SAR and ROL. It preserves the
    // rotated C3 bit and moves the C0/C1/C2 bits to the table index.
    static std::uint32_t classify_bits(std::uint32_t status) {
        return ((status << 1u) & 0xfeu) | ((status >> 6u) & 1u);
    }

    void read_class(unsigned status_shift, std::uint32_t pc) {
        const auto bits = classify_bits(state_.reg(1, 1, status_shift));
        state_.set(1, bits, 1, status_shift);
        const auto index = state_.flags.logic(bits & 0xfu, 1);
        state_.set(0, index, 1);
        state_.set(0, read(pc, state_.gp[3] + index, 1), 1);
    }

public:
    Classifier(MathState &state, const MathState::InstructionCursor *cursor, const PhysicsInstructionRemapTu15 *remap)
        : state_(state), cursor_(cursor), position_(cursor ? cursor->position : nullptr),
          remap_source_(remap ? remap->source : nullptr),
          remap_bindings_(remap ? remap->bindings : std::span<const PhysicsInstructionRemapTu15::Binding>{}) {}

    bool admitted() const {
        if (!state_.services.instruction) return !cursor_ && !state_.observe;
        const MathState::InstructionCursor *live;
        if (remap_source_) {
            // The composition owns an immutable site table. Recheck its
            // binding and the current source callback at every provider return.
            const auto *mapped = state_.services.instruction.target<PhysicsInstructionRemapTu15>();
            if (!mapped || mapped->source != remap_source_ || mapped->bindings.data() != remap_bindings_.data() ||
                mapped->bindings.size() != remap_bindings_.size())
                return false;
            // Draw providers can use the same composition without any
            // underlying instruction callback. Its remap already does nothing.
            if (!remap_source_->instruction) return !cursor_ && !state_.observe;
            live = mapped->live_cursor();
        } else {
            live = state_.literal_service_sites ? state_.live_instruction_cursor() : nullptr;
        }
        return live && live == cursor_ && live->position == position_ &&
            state_.has_literal_service_sites(*live) && (!state_.observe || live->observation_only_stores) &&
            !(live->first_trace && *live->first_trace) && !(live->second_trace && *live->second_trace);
    }

    Continuation run(std::uint32_t &target) {
        auto &gp = state_.gp;
        auto &flags = state_.flags;
        flags.minus(read(0x32e35bu, gp[2] + 0xeu, 1), 5, 1);
        if (!admitted()) return Continuation::choose_control;
        if (flags.zf) {
            const auto saved = read(0x32e361u, local(0xa4u), 2);
            state_.set(3, saved, 2);
            if (!admitted()) return Continuation::alter_saved_control;
            const auto high = flags.logic(((saved >> 8u) | 2u) & 0xfeu, 1);
            state_.set(3, (high << 8u) | 0x3fu, 2);
        } else {
            state_.set(3, 0x133fu, 2);
        }

        store(0x32e376u, local(0xa2u), 2, state_.reg(3, 2));
        if (!admitted()) return Continuation::load_control;
        load_control_word(static_cast<std::uint16_t>(read(0x32e37du, local(0xa2u), 2)));
        if (!admitted()) return Continuation::classify_first;

        gp[3] = 0x46ecb4u;
        at(0x32e388u);
        state_.classify();
        store(0x32e38au, local(0x94u), 4, gp[2]);
        if (!admitted()) return Continuation::save_first_status;
        store(0x32e391u, local(0xa0u), 2, state_.fp.word());
        if (!admitted()) return Continuation::clear_error;
        store(0x32e397u, local(0x90u), 1, 0);
        if (!admitted()) return Continuation::swap_inputs;

        swap_inputs(0x32e39eu);
        state_.set(1, read(0x32e3a0u, local(0x9fu), 1), 1);
        if (!admitted()) return Continuation::classify_second;
        classify_and_save(0x32e3a6u, 0x32e3a9u);
        if (!admitted()) return Continuation::restore_inputs;
        swap_inputs(0x32e3afu);
        state_.set(1, read(0x32e3b1u, local(0x9fu), 1), 1, 8);
        if (!admitted()) return Continuation::decode_high_status;

        read_class(8, 0x32e3c1u);
        if (!admitted()) return Continuation::keep_high_class;
        state_.set(0, state_.reg(0, 1), 1, 8);
        read_class(0, 0x32e3ceu);
        if (!admitted()) return Continuation::combine_classes;

        const auto code = (state_.reg(0, 1, 8) << 2u) | state_.reg(0, 1);
        gp[0] = static_cast<std::uint32_t>(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(code))));
        gp[1] = flags.logic(gp[1] & 0x404u, 4);
        gp[3] = flags.plus(gp[2] + gp[0], 0x10u, 4);
        target = read(0x32e3e5u, gp[3], 4);
        return Continuation::dispatch;
    }
};
#endif

inline Continuation classify(MathState &state, std::uint32_t &target) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if (reference_dispatch() || state.image != SourcePhysicsContactImage::TitleUpdate15 ||
        state.active_mmx || state.fp.count < 2)
        return Continuation::original;
    const auto *remap = state.services.instruction.target<PhysicsInstructionRemapTu15>();
    const auto *cursor = remap ? remap->live_cursor() : state.live_instruction_cursor();
    if (remap && (!remap->source || (remap->source->instruction && (!cursor || !authentic_sites(*remap)))))
        return Continuation::original;
    Classifier classifier{state, cursor, remap};
    if (classifier.admitted()) return classifier.run(target);
#else
    (void)state;
    (void)target;
#endif
    return Continuation::original;
}
} // namespace halo2::native_vehicle_x87_dispatch_331ecb_detail
