#include "halo2/native_campaign_ai_vehicle_movement_scale_tu15.hpp"

#include "halo2/native_campaign_ai_character_property_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include "physics/runtime/physics_contact_runtime_internal.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
using Word = std::uint32_t;
using Vector = std::array<Word, 4>;
using physics_contact_runtime_detail::Flags;

constexpr Word call_site = 0x2a46ba;
constexpr Word entry = 0x202130;
constexpr Word continuation = 0x2a46bf;
constexpr Word local_frame = 0x2c;
constexpr Word lowest_store = 0x44;
constexpr Word returned_bytes = 0x20;
constexpr Word extent_bytes = lowest_store + returned_bytes;

struct Refused {};

// 202130 and its admitted character-property child only write their frame.
// Keep a bounded shadow so dynamic branch admission is side-effect-free.
// The lease excludes write policy callbacks while this shadow is definitive.
class MovementFrame final : public SourcePhysicsContactMemory {
    const SourceRuntimeMemory &memory_;
    Word first_;
    std::array<std::byte, extent_bytes> shadow_;
    struct Store { Word address; std::array<std::byte, 4> value; };
    std::array<Store, 24> stores_{};
    std::size_t count_{};
    mutable unsigned reads_{};

public:
    MovementFrame(const SourceRuntimeMemory &memory, Word first, std::span<const std::byte> initial)
        : memory_(memory), first_(first) {
        std::copy(initial.begin(), initial.end(), shadow_.begin());
    }

    void read(Word address, std::span<std::byte> output) const override {
        // A different or cyclic property chain keeps the original owner.
        if (++reads_ > 256) throw Refused{};
        const auto end = std::uint64_t(address) + output.size();
        const auto frame_end = std::uint64_t(first_) + extent_bytes;
        if (address < frame_end && end > first_) {
            if (address < first_ || end > frame_end) throw Refused{};
            std::copy_n(shadow_.data() + (address - first_), output.size(), output.data());
            return;
        }
        const auto input = memory_.try_read_view(address, output.size());
        if (!input || input->size() != output.size()) throw Refused{};
        std::copy(input->begin(), input->end(), output.begin());
    }

    void write(Word address, std::span<const std::byte> input) override {
        if (input.size() != 4 || address < first_ || address - first_ > extent_bytes - 4 || count_ == stores_.size())
            throw Refused{};
        auto &store = stores_[count_++];
        store.address = address;
        std::copy(input.begin(), input.end(), store.value.begin());
        std::copy(input.begin(), input.end(), shadow_.begin() + (address - first_));
    }

    Word word(Word address, unsigned size = 4) const {
        std::array<std::byte, 4> bytes{};
        read(address, std::span(bytes).first(size));
        return std::bit_cast<Word>(bytes);
    }

    void word_store(Word address, Word value) {
        write(address, std::bit_cast<std::array<std::byte, 4>>(value));
    }

    void commit(SourceRuntimeMemory &memory) const {
        for (const auto &store : std::span(stores_).first(count_)) memory.write(store.address, store.value);
    }
};

// Explicit SSE operations preserve source rounding, denormals and NaN order.
#define SCALAR_OP(name, instruction)                                           \
    Word name(Word left, Word right) {                                          \
        auto a = std::bit_cast<float>(left);                                    \
        const auto b = std::bit_cast<float>(right);                             \
        asm volatile(instruction " %1,%0" : "+x"(a) : "x"(b));                  \
        return std::bit_cast<Word>(a);                                         \
    }
SCALAR_OP(add, "addss")
SCALAR_OP(subtract, "subss")
SCALAR_OP(multiply, "mulss")
SCALAR_OP(divide, "divss")
#undef SCALAR_OP

__m128 scalar(Word bits) { return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits))); }

bool above(Word left, Word right) {
    Flags flags;
    flags.scalar_compare(scalar(left), scalar(right), false);
    return !flags.cf && !flags.zf;
}

void load_x87(Word value) { asm volatile("flds %0" : : "m"(value) : "st"); }
void multiply_x87(Word value) { asm volatile("fmuls %0" : : "m"(value) : "st"); }
void subtract_x87(Word value) { asm volatile("fsubs %0" : : "m"(value) : "st"); }
Word store_x87_single() {
    Word result;
    asm volatile("fsts %0" : "=m"(result));
    return result;
}

// ST(0) is an already computed norm. Compare the loaded threshold against
// that live value before discarding both operands, as the original does.
bool threshold_above(Word threshold) {
    unsigned char greater;
    asm volatile("flds %1; fcomip %%st(1),%%st; fstp %%st(0); seta %0"
                 : "=q"(greater) : "m"(threshold) : "cc", "st", "st(1)");
    return greater != 0;
}

Word character_property(MovementFrame &frame, const SourcePhysicsWorldRuntimeAbi &abi,
                        Word base, Word actor, Word vehicle, Word object_table) {
    frame.word_store(base - 4, frame.word(vehicle));
    frame.word_store(base - 8, 0x20216e);
    auto child = abi;
    child.gp[4] = base - 8;
    child.gp[5] = object_table;
    child.gp[6] = vehicle;
    child.gp[7] = actor;
    native_campaign_ai_character_property_call_tu15(frame, 0x1ed700, child);
    // All skipped POP/RET loads belong to this checked private frame. The
    // child result is the genuine retained property row, including NONE.
    if (child.gp[4] != base || frame.word(base - 8) != 0x20216e || child.gp[0] == 0) throw Refused{};
    return child.gp[0];
}

void project_along_vehicle(MovementFrame &frame, Word base, Word vehicle, Word property,
                           Word direction, Word &vehicle_y, std::array<Vector, 8> &x) {
    const auto actor_direction = frame.word(base + 0x38);
    x[1] = {frame.word(actor_direction + 4)};
    x[2] = {frame.word(actor_direction)};
    x[1][0] = multiply(x[1][0], frame.word(direction + 4));
    x[2][0] = multiply(x[2][0], frame.word(direction));
    x[3] = {frame.word(property + 0x5c)};
    x[1][0] = add(x[1][0], x[2][0]);
    if (above(x[5][0], x[1][0])) x[1] = x[5];

    vehicle_y = frame.word(vehicle + 0x8c);
    const auto vehicle_x = frame.word(vehicle + 0x88);
    frame.word_store(base + 0x28, vehicle_y);
    load_x87(frame.word(base + 0x28));
    multiply_x87(frame.word(base + 0x28));
    frame.word_store(base + 0x24, vehicle_x);
    load_x87(frame.word(base + 0x24));
    x[2] = x[0];
    multiply_x87(frame.word(base + 0x24));
    x[2][0] = add(multiply(subtract(x[2][0], x[3][0]), x[1][0]), x[3][0]);
    asm volatile("faddp %%st,%%st(1)" : : : "st", "st(1)");
    frame.word_store(base + 0x1c, x[2][0]);
    x[2] = {frame.word(base + 0x28)};
    asm volatile("fsqrt" : : : "st");
    frame.word_store(base + 0x48, store_x87_single());
    asm volatile("fabs" : : : "st");
    if (threshold_above(frame.word(0x46a0ec))) {
        x[3] = {frame.word(base + 0x24)};
    } else {
        x[1] = x[0];
        x[1][0] = divide(x[1][0], frame.word(base + 0x48));
        x[3] = x[1];
        x[3][0] = multiply(x[3][0], frame.word(base + 0x24));
        x[2][0] = multiply(x[2][0], x[1][0]);
    }
    x[1] = {frame.word(direction)};
    x[1][0] = multiply(x[1][0], x[3][0]);
    x[3] = {frame.word(direction + 4)};
    x[3][0] = multiply(x[3][0], x[2][0]);
    x[2] = {frame.word(0x46a11c)};
    x[1][0] = multiply(add(add(x[1][0], x[3][0]), x[2][0]), frame.word(0x46a174));
    frame.word_store(base + 0x20, x[1][0]);
}

void scale_actor_displacement(MovementFrame &frame, Word base, Word actor, Word previous_point,
                              Word property, std::array<Vector, 8> &x) {
    load_x87(frame.word(actor + 0x510));
    subtract_x87(frame.word(previous_point));
    load_x87(frame.word(actor + 0x514));
    subtract_x87(frame.word(previous_point + 4));
    asm volatile("fld %%st(0); fmul %%st(1),%%st; fld %%st(2); fmul %%st(3),%%st; "
                 "faddp %%st,%%st(1); fsqrt; fstp %%st(2); fstp %%st(0)"
                 : : : "st", "st(1)", "st(2)", "st(3)");
    frame.word_store(base + 0x44, store_x87_single());
    if (threshold_above(frame.word(base + 0x3c))) {
        x[3] = {frame.word(property + 0x54)};
        x[1] = x[0];
        x[1][0] = subtract(x[1][0], x[3][0]);
        x[1][0] = divide(x[1][0], x[4][0]);
        x[1][0] = multiply(x[1][0], frame.word(base + 0x44));
        x[1][0] = add(x[1][0], x[3][0]);
        frame.word_store(base + 0x48, x[1][0]);
        x[6] = x[1];
    }
}

void clamp_and_scale(MovementFrame &frame, Word base, std::array<Vector, 8> &x) {
    x[0] = above(x[7][0], x[6][0]) ? x[6] : x[7];
    x[1] = {frame.word(base + 0x1c)};
    if (above(x[1][0], x[0][0])) x[0] = above(x[7][0], x[6][0]) ? x[6] : x[7];
    else x[0] = x[1];
    x[1] = {frame.word(base + 0x14)};
    if (above(x[0][0], x[1][0])) x[0] = x[1];
    x[0][0] = multiply(x[0][0], frame.word(base + 0x20));
    x[0][0] = multiply(x[0][0], frame.word(base + 0x18));
}

void movement_scale(MovementFrame &frame, SourcePhysicsWorldRuntimeAbi &abi) {
    const auto original_sp = abi.gp[4];
    const auto base = original_sp - local_frame;
    auto &x = abi.xmm;
    frame.word_store(base + 12, abi.gp[3]);
    frame.word_store(base + 8, abi.gp[5]);
    frame.word_store(base + 4, abi.gp[6]);
    const auto actor_pool = frame.word(frame.word(0x5020c8) + 0x44);
    frame.word_store(base, abi.gp[7]);
    const auto actor = actor_pool + (abi.gp[1] & 0xffffu) * 0x888;
    const auto object_table = frame.word(frame.word(0x4ecbd0) + 0x44);
    const auto vehicle = frame.word(object_table + (abi.gp[0] & 0xffffu) * 12 + 8);
    const auto property = character_property(frame, abi, base, actor, vehicle, object_table);

    x[1] = {frame.word(actor + 0x4b4)};
    x[5] = {};
    x[0] = {frame.word(0x46a0d0)};
    Flags compare;
    compare.scalar_compare(scalar(x[1][0]), scalar(x[5][0]), true);
    x[7] = x[0];
    frame.word_store(base + 0x1c, x[0][0]);
    frame.word_store(base + 0x10, x[7][0]);
    frame.word_store(base + 0x20, x[0][0]);
    frame.word_store(base + 0x14, x[0][0]);
    // TEST AH,44 after UCOMISS admits the actor scale on even parity.
    frame.word_store(base + 0x18, compare.zf == compare.pf ? x[1][0] : x[0][0]);
    const auto perpendicular = frame.word(base + 0x48, 1);
    const auto direction = frame.word(base + 0x3c);
    if (perpendicular) throw Refused{};
    Word vehicle_y{};
    project_along_vehicle(frame, base, vehicle, property, direction, vehicle_y, x);

    const auto optional_target = frame.word(base + 0x40);
    x[4] = {frame.word(property + 0x1c)};
    frame.word_store(base + 0x3c, x[4][0]);
    if (optional_target) throw Refused{};
    const auto alternate_look = frame.word(actor + 0x4ae, 1);
    x[6] = x[0];
    frame.word_store(base + 0x48, x[6][0]);
    if (alternate_look) throw Refused{};
    const auto previous_point = frame.word(base + 0x30);
    scale_actor_displacement(frame, base, actor, previous_point, property, x);
    const auto actor_action = frame.word(actor + 0x50c, 1);
    if (actor_action && frame.word(actor + 0x504, 2) == 1 && frame.word(actor + 0x5b0, 2) == 7)
        throw Refused{};

    // Only the continuation can observe the no-callback frame. Restore the
    // actual saved words and preserve all terminal SIMD lanes and ADD flags.
    abi.gp[7] = frame.word(base);
    abi.gp[6] = frame.word(base + 4);
    abi.gp[5] = frame.word(base + 8);
    abi.gp[3] = frame.word(base + 12);
    clamp_and_scale(frame, base, x);
    Flags flags;
    flags.plus(base + 16, 0x1c, 4);
    if (frame.word(original_sp) != continuation) throw Refused{};
    abi.gp[0] = (previous_point & ~0xffu) | actor_action;
    abi.gp[1] = optional_target;
    abi.gp[2] = vehicle_y;
    abi.gp[4] = original_sp + returned_bytes;
    abi.eflags = (abi.eflags & ~0x8d5u) | flags.bits();
}
} // namespace

bool native_campaign_ai_vehicle_movement_scale_tu15_try(SourceRuntimeMemory &memory,
    const SourcePhysicsContactRuntimeCall &call, SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) {
    const auto sp = abi.gp[4];
    if (call.instruction != call_site || call.target != entry || call.tail || observe ||
        !abi.floating_point || abi.active_mmx || abi.x87_depth || sp < lowest_store ||
        sp > 0xffffffffu - returned_bytes) return false;
    const auto control = *abi.floating_point;
    NativeTu15PhysicsFrameFp fp{control.x87_control_word, abi.x87_status, control.mxcsr};
    if ((fp.control_word & 0x3f) != 0x3f || (fp.status & 0x8080) ||
        (fp.mxcsr & 0x1f80) != 0x1f80 || (fp.mxcsr & 0xffff0000)) return false;

    const auto first = sp - lowest_store;
    const auto bytes = memory.try_write_extent(first, extent_bytes);
    if (!bytes || bytes->size() != extent_bytes) return false;
    const SourceRuntimeNativeWriteRange range{first, extent_bytes};
    const SourceRuntimeNativeWriteLease lease(memory, std::span(&range, 1));
    if (!lease) return false;
    auto output = abi;
    MovementFrame frame(memory, first, *bytes);
    try {
        if (frame.word(sp) != continuation) return false;
        detail::NativeTu15ArithmeticEnvironment arithmetic(fp);
        movement_scale(frame, output);
    } catch (const Refused &) {
        return false;
    }
    frame.commit(memory);
    output.floating_point = SourcePhysicsFactoryFloatingPoint{fp.control_word, fp.mxcsr};
    output.x87_status = fp.status;
    abi = output;
    return true;
}
} // namespace halo2
#else
namespace halo2 {
bool native_campaign_ai_vehicle_movement_scale_tu15_try(SourceRuntimeMemory &,
    const SourcePhysicsContactRuntimeCall &, SourcePhysicsWorldRuntimeAbi &,
    const SourcePhysicsContactRecordObserver &) { return false; }
} // namespace halo2
#endif
