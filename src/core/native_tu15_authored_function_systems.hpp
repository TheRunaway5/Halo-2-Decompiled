#pragma once

#include <cstdint>

namespace halo2::native_authored_function {
inline constexpr std::uint32_t completed = 0;
inline constexpr std::uint32_t retained_entry = 0xffffffffu;
inline constexpr std::uint32_t one_address = 0x46a0d0u;
inline constexpr std::uint32_t jump_table = 0x13e5f4u;
inline constexpr unsigned kind_count = 11;

enum class Target : std::uint32_t {
    first_parameter = 0x13deabu,
    second_parameter = 0x13debcu,
    transition_curve = 0x13dee5u,
    power = 0x13df3fu,
    periodic_curve = 0x13e035u,
    linear = 0x13e0edu,
    ramp = 0x13e146u,
    cubic = 0x13e2a2u,
    zero = 0x13e5e2u
};

enum class Access {
    blob,
    seed_result,
    count,
    kind,
    case_target,
    first_parameter,
    store_first,
    second_parameter_enabled,
    second_parameter,
    store_second,
    disabled_result,
    lower_compare,
    upper_compare,
    result
};
enum class Local : std::uint32_t {
    result = 0x0c,
    first = 0x20,
    second = 0x24,
    result_after_restoring = 0x04
};

// The common terminal uses the live result and saved stack frame.
template<class Memory, class Frame>
void clamp_result(Memory &memory, Frame &frame) {
    using enum Access;
    (void)memory.load32(lower_compare, frame.local(Local::result));
    if (frame.below_zero()) {
        frame.finish_at_zero();
        return;
    }
    (void)memory.load32(upper_compare, one_address);
    if (!frame.above_one())
        (void)memory.load32(result, frame.local(Local::result));
    frame.finish();
}

// The original entry reads the authored function reference and selects a
// case through the image's jump table. Trivial functions complete here.
// Curve, power and polynomial cases resume at the selected source label.
template<class Memory, class Frame>
std::uint32_t evaluate(Memory &memory, Frame &frame, std::uint32_t reference) {
    using enum Access;
    frame.reserve_frame();
    const auto function = memory.load32(blob, reference + 4u);
    frame.save_registers();
    memory.store32(seed_result, frame.local(Local::result), 0u);
    if (!function || !frame.has_data(memory.load32(count, reference))) {
        (void)memory.load32(result, frame.local(Local::result));
        frame.finish();
        return completed;
    }

    const auto function_kind = memory.load8(kind, function);
    if (function_kind >= kind_count) {
        (void)memory.load32(result, frame.local(Local::result));
        frame.finish();
        return completed;
    }
    const auto target = frame.resolve(memory.load32(case_target, jump_table + 4u * function_kind));
    switch (static_cast<Target>(target)) {
    case Target::first_parameter:
        memory.store32(store_first, frame.local(Local::result),
            memory.load32(first_parameter, frame.local(Local::first)));
        break;
    case Target::second_parameter:
        if (!(memory.load8(second_parameter_enabled, function + 1u) & 1u)) {
            frame.restore_disabled_arguments();
            memory.store32(disabled_result, frame.local(Local::result_after_restoring), 0u);
            frame.finish_disabled();
            return completed;
        }
        memory.store32(store_second, frame.local(Local::result),
            memory.load32(second_parameter, frame.local(Local::second)));
        break;
    case Target::zero:
        (void)memory.load32(result, frame.local(Local::result));
        frame.finish();
        return completed;
    case Target::transition_curve:
    case Target::power:
    case Target::periodic_curve:
    case Target::linear:
    case Target::ramp:
    case Target::cubic:
        return target;
    default: frame.unknown_target();
    }

    clamp_result(memory, frame);
    return completed;
}
} // namespace halo2::native_authored_function
