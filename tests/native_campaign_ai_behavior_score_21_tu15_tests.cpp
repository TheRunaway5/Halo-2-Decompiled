#include "halo2/native_campaign_ai_behavior_score_21_tu15.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
using namespace halo2;

constexpr SourcePhysicsContactRuntimeCall score_call{0x1b0195u, 0x1ca160u, false};
constexpr auto tu15 = SourcePhysicsContactImage::TitleUpdate15;
constexpr std::uint32_t stack = 0x600efeccu;
constexpr std::uint32_t pool = 0x601a0000u;
constexpr std::uint32_t rows = 0x60290000u;
constexpr std::uint32_t actor = 0xe3620000u;
constexpr std::uint32_t saved_ebx = 0x5746f3b2u;
constexpr std::uint32_t stack_preimage = 0x193674e2u;

void check(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}

struct Event {
    char kind;
    std::uint32_t address;
    std::size_t width{};
    bool operator==(const Event &) const = default;
};

struct Fault {};

struct Memory final : SourcePhysicsContactMemory {
    std::map<std::uint32_t, std::byte> bytes;
    mutable std::vector<Event> events;
    std::optional<std::size_t> stop_at;

    void event(Event value) const {
        events.push_back(value);
        if (stop_at == events.size() - 1u) throw Fault{};
    }

    std::uint32_t word(std::uint32_t at) const {
        std::uint32_t value{};
        for (unsigned i = 0; i < 4; ++i)
            value |= std::to_integer<std::uint32_t>(bytes.at(at + i)) << (i * 8u);
        return value;
    }

    void put(std::uint32_t at, std::uint32_t value, unsigned size = 4) {
        for (unsigned i = 0; i < size; ++i) bytes[at + i] = std::byte(value >> (i * 8u));
    }

    void read(std::uint32_t at, std::span<std::byte> out) const override {
        event({'r', at, out.size()});
        for (std::size_t i = 0; i < out.size(); ++i) out[i] = bytes.at(at + std::uint32_t(i));
    }

    void write(std::uint32_t at, std::span<const std::byte> input) override {
        event({'w', at, input.size()});
        for (std::size_t i = 0; i < input.size(); ++i) bytes.at(at + std::uint32_t(i)) = input[i];
    }
};

std::uint32_t row_byte(std::uint32_t datum) {
    return rows + (datum & 0xffffu) * 0x888u + 0x223u;
}

Memory seed(std::uint32_t datum, unsigned value) {
    Memory memory;
    memory.put(stack - 4u, stack_preimage);
    memory.put(stack, 0x1b0197u);
    memory.put(stack + 4u, datum);
    memory.put(0x5020c8u, pool);
    memory.put(pool + 0x44u, rows);
    memory.put(row_byte(datum), value, 1);
    return memory;
}

SourcePhysicsWorldRuntimeAbi initial() {
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp = {score_call.target, stack, 0x600e0015u, saved_ebx, stack,
              0x47ba8cu, 0x82fad89cu, 0x48b7c0u};
    abi.eflags = 0xed7u;
    abi.fs_base = 0x12340000u;
    abi.x87 = {1.25L, -1.0L};
    abi.x87_depth = 2;
    abi.x87_status = 0x2811u;
    abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x073fu, 0x5f80u};
    abi.xmm[0] = {0x80000000u, 0x7fc12345u, 0x1u, 0xffffffffu};
    abi.xmm[7] = {4u, 8u, 12u, 16u};
    return abi;
}

void verify_retained(const SourcePhysicsWorldRuntimeAbi &abi, const SourcePhysicsWorldRuntimeAbi &before) {
    check(abi.fs_base == before.fs_base && abi.xmm == before.xmm && abi.x87 == before.x87 &&
          abi.x87_depth == before.x87_depth && abi.x87_status == before.x87_status &&
          abi.floating_point && before.floating_point &&
          abi.floating_point->x87_control_word == before.floating_point->x87_control_word &&
          abi.floating_point->mxcsr == before.floating_point->mxcsr, "Retained FP/SIMD state differs");
    check(abi.active_mmx.has_value() == before.active_mmx.has_value(), "Retained MMX presence differs");
    if (before.active_mmx)
        check(abi.active_mmx->known_mask == before.active_mmx->known_mask &&
              abi.active_mmx->physical == before.active_mmx->physical, "Retained MMX state differs");
}

SourcePhysicsWorldRuntimeServices tracing(Memory &memory) {
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) { memory.event({'i', pc}); };
    return services;
}

std::vector<Event> expected(std::uint32_t datum, bool nonzero, bool observed) {
    std::vector<Event> events = {{'i', 0x1ca160u}, {'r', stack + 4u, 4},
        {'i', 0x1ca164u}, {'r', 0x5020c8u, 4}, {'i', 0x1ca16au}, {'r', pool + 0x44u, 4},
        {'i', 0x1ca16du}, {'i', 0x1ca173u}, {'i', 0x1ca179u}};
    if (observed) events.push_back({'r', stack - 4u, 4});
    events.push_back({'w', stack - 4u, 4});
    if (observed) events.push_back({'o', stack - 4u, 4});
    events.insert(events.end(), {{'i', 0x1ca17au}, {'r', row_byte(datum), 1}, {'i', 0x1ca181u},
        {'i', 0x1ca183u}, {'i', 0x1ca185u}, {'r', stack - 4u, 4}, {'i', 0x1ca186u}});
    if (nonzero) events.push_back({'i', 0x1ca188u});
    events.insert(events.end(), {{'i', 0x1ca18du}, {'r', stack, 4}});
    return events;
}

void complete(std::uint32_t datum, unsigned value, unsigned test_flags, bool observe_store) {
    auto memory = seed(datum, value);
    auto abi = initial();
    if (value == 0xffu) {
        abi.x87_depth = 0;
        abi.x87_status = 0;
        abi.active_mmx = SourcePhysicsMmxAlias{};
        abi.active_mmx->known_mask = 0x81u;
        abi.active_mmx->physical[7][9] = std::byte{0xb3};
    }
    const auto before = abi;
    const auto before_bytes = memory.bytes;
    SourcePhysicsContactRecordObserver observer;
    constexpr std::uint32_t replacement_ebx = 0x9a7845efu;
    if (observe_store) observer = [&](const SourcePhysicsContactRecordStore &store) {
        check(store.instruction == 0x1ca179u && store.address == stack - 4u &&
              store.before == std::vector<std::byte>{std::byte{0xe2}, std::byte{0x74}, std::byte{0x36}, std::byte{0x19}} &&
              store.written == std::vector<std::byte>{std::byte{0xb2}, std::byte{0xf3}, std::byte{0x46}, std::byte{0x57}},
              "PUSH store observer payload differs");
        check(abi.gp[4] == stack - 4u && memory.word(stack - 4u) == saved_ebx,
              "Store observer ran before PUSH publication");
        memory.event({'o', stack - 4u, 4});
        memory.put(stack - 4u, replacement_ebx);
    };
    check(native_campaign_ai_behavior_score_21_tu15_try_run(memory, tu15, score_call, abi,
          tracing(memory), observer), "Descriptor-21 call refused");
    const auto restored = observe_store ? replacement_ebx : saved_ebx;
    const std::array<std::uint32_t, 8> gp{value ? 3u : 0u, (datum & 0xffffu) * 0x888u,
        rows, restored, stack + 8u, before.gp[5], before.gp[6], before.gp[7]};
    check(abi.gp == gp && abi.eflags == ((before.eflags & ~0x8d5u) | test_flags),
          "Score, return stack, or integer flags differ");
    check(memory.events == expected(datum, value != 0, observe_store), "Ordered source accesses differ");
    check(memory.word(stack - 4u) == restored, "Popped stack storage differs");
    memory.put(stack - 4u, stack_preimage);
    check(memory.bytes == before_bytes, "Unrelated source memory changed");
    verify_retained(abi, before);
}

void ordered_faults() {
    const auto trace = expected(actor, true, true);
    for (std::size_t stop = 0; stop < trace.size(); ++stop) {
        auto memory = seed(actor, 0x80u);
        auto abi = initial();
        const auto before = abi;
        memory.stop_at = stop;
        const SourcePhysicsContactRecordObserver observer = [&](const auto &store) {
            memory.event({'o', store.address, store.written.size()});
        };
        bool stopped = false;
        try {
            native_campaign_ai_behavior_score_21_tu15_try_run(memory, tu15, score_call, abi,
                tracing(memory), observer);
        } catch (const Fault &) {
            stopped = true;
        }
        check(stopped && memory.events.size() == stop + 1u &&
              std::equal(memory.events.begin(), memory.events.end(), trace.begin()),
              "Fault moved or processed a later source instruction");
        const auto ecx = stop <= 1u ? before.gp[1] : (stop <= 6u ? actor : 0u);
        const auto edx = stop <= 3u ? before.gp[2] : (stop <= 5u ? pool : rows);
        const auto ebx = stop > 13u && stop <= 17u ? 0x5746f380u : saved_ebx;
        const auto esp = stop > 10u && stop <= 17u ? stack - 4u : stack;
        const auto eax = stop <= 14u ? score_call.target : (stop <= 19u ? 0u : 3u);
        const auto flags = stop <= 6u ? before.eflags :
            (before.eflags & ~0x8d5u) | (stop <= 15u ? 0x44u : 0x80u);
        check(abi.gp == std::array<std::uint32_t, 8>{eax, ecx, edx, ebx, esp,
              before.gp[5], before.gp[6], before.gp[7]} && abi.eflags == flags,
              "Partial fault ABI differs");
        check(memory.word(stack - 4u) == (stop > 10u ? saved_ebx : stack_preimage),
              "Fault committed or rolled back the wrong write");
        verify_retained(abi, before);
    }
}

void refuse_unowned() {
    for (unsigned kind = 0; kind < 6; ++kind) {
        auto memory = seed(actor, 0);
        auto call = score_call;
        auto image = tu15;
        auto abi = initial();
        if (kind == 0) image = SourcePhysicsContactImage::MapPack11;
        if (kind == 1) ++call.instruction;
        if (kind == 2) call.target = 0x1ca190u;
        if (kind == 3) call.tail = true;
        if (kind == 4) ++abi.gp[0];
        if (kind == 5) ++abi.gp[7];
        const auto before = abi;
        const auto before_bytes = memory.bytes;
        check(!native_campaign_ai_behavior_score_21_tu15_try_run(memory, image, call, abi, tracing(memory)) &&
              memory.events.empty() && memory.bytes == before_bytes && abi.gp == before.gp &&
              abi.eflags == before.eflags, "Unowned callback had effects");
        verify_retained(abi, before);
    }

    auto memory = seed(actor, 0);
    memory.put(stack, 0x1b01a0u);
    auto abi = initial();
    bool rejected = false;
    try {
        native_campaign_ai_behavior_score_21_tu15_try_run(memory, tu15, score_call, abi, tracing(memory));
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    check(rejected && memory.events == expected(actor, false, false) && abi.gp[4] == stack &&
          abi.gp[0] == 0 && memory.word(stack - 4u) == saved_ebx,
          "Foreign return was accepted or rejected at the wrong boundary");
}
}

int main() try {
    complete(actor, 0, 0x44u, false);
    complete(0x8e012345u, 1, 0, false);
    complete(0xe362ffffu, 0x80u, 0x80u, true);
    complete(0x125c0007u, 0xffu, 0x84u, false);
    ordered_faults();
    refuse_unowned();
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
