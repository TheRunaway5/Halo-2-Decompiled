#include "native_tu15_frame_misc_harness.hpp"
#include "physics/vehicles/native_tu15_vehicle_x87_dispatch_331ecb.hpp"
#include "vehicle_x87_dispatch_accounting.inc"

using namespace frame_misc_harness;

namespace original {
using namespace halo2::physics_vehicle_motion_math_detail;

struct Tail {
    std::uint32_t target{}, pc{};
    bool tail{};
    bool operator==(const Tail &) const = default;
};

void dispatch(MathState &state, std::uint32_t target, std::uint32_t site, bool tail) {
    state.required(site, target, tail);
}

std::uint32_t sign_extend(std::uint32_t value, unsigned) {
    return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
}

#include "vehicle_x87_dispatch_owner.inc"
} // namespace original

namespace {
using original::MathState;
using original::Tail;
using Abi = SourcePhysicsContactRuntimeAbi;

bool identical(const Abi &a, const Abi &b) {
    return same_abi(a, b) && a.fs_base == b.fs_base && a.floating_point && b.floating_point &&
        a.floating_point->mxcsr == b.floating_point->mxcsr && !a.active_mmx && !b.active_mmx;
}

struct Packet {
    char access;
    std::uint32_t pc, address;
    std::size_t width;
    Abi abi;

    bool operator==(const Packet &other) const {
        return access == other.access && pc == other.pc && address == other.address &&
            width == other.width && identical(abi, other.abi);
    }
};

struct Provider : Memory {
    std::function<Packet(char, std::uint32_t, std::size_t)> snapshot;
    std::function<void()> start_trace;
    mutable std::vector<Packet> packets;
    unsigned stop, trace_at;
    bool change_fp;

    Provider(const Memory &backing, unsigned failure, unsigned trigger, bool fp)
        : Memory(backing.clone()), stop(failure), trace_at(trigger), change_fp(fp) {}

    void visit(char kind, std::uint32_t address, std::size_t width) const {
        packets.push_back(snapshot(kind, address, width));
        const unsigned n = packets.size();
        if (change_fp) {
            const auto control = std::uint16_t(0x037fu | ((n & 3u) << 10u));
            asm volatile("fldcw %0" :: "m"(control) : "memory");
            source_load_x87_exception_flags(std::uint16_t(n & 0x3fu));
            _mm_setcsr(0x1f80u | ((n & 3u) << 13u) | (n & 0x3fu));
        }
        if (n == stop) throw std::runtime_error("Injected vehicle math provider fault");
        if (n == trace_at) start_trace();
    }

    void read(std::uint32_t address, std::span<std::byte> out) const override {
        visit('r', address, out.size());
        Memory::read(address, out);
    }

    void write(std::uint32_t address, std::span<const std::byte> input) override {
        visit('w', address, input.size());
        Memory::write(address, input);
    }
};

enum class Policy {
    cursor, quiet, trace, arbitrary, alternate, unqualified_observer, shallow_stack,
    mapped, mapped_silent, mapped_trace, inexact_remap
};

struct Observation {
    std::vector<std::uint32_t> callbacks;
    std::vector<SourcePhysicsContactRecordStore> stores;
    Tail tail;
    std::optional<Abi> tail_abi;
    std::string error;
    Abi abi;
    std::uint32_t cursor{};
};

Observation run(Provider &memory, const Case &input, Policy policy, bool raw, bool observe) {
    Observation got;
    const bool silent = policy == Policy::mapped_silent;
    const bool mapped = silent || policy == Policy::mapped || policy == Policy::mapped_trace ||
        policy == Policy::inexact_remap;
    bool trace = policy == Policy::trace || policy == Policy::mapped_trace;
    SourcePhysicsContactRuntimeServices services, source;
    auto &published = mapped ? source : services;
    constexpr std::array sites{0x38846eu, 0x21f590u, 0x33b595u};
    const auto publish = [&](auto pc) {
        got.cursor = pc;
        if (!mapped || trace) got.callbacks.push_back(pc);
    };
    if (policy != Policy::quiet) {
        published.instruction = publish;
        if (policy != Policy::arbitrary) {
            published.instruction_cursor = {&got.cursor, &trace, nullptr, sites, std::this_thread::get_id(),
                &published.instruction.target_type(), true, policy != Policy::unqualified_observer};
        }
        if (silent) published.instruction = {};
    }
    using namespace native_vehicle_x87_dispatch_331ecb_detail;
    std::array<PhysicsInstructionRemapTu15::Binding, std::size(canonical_sites)> bindings{};
    if (mapped) {
        for (std::size_t i = 0; i < bindings.size(); ++i)
            bindings[i] = {canonical_sites[i], canonical_sites[i] + image_delta};
        if (policy == Policy::inexact_remap) ++bindings[15].value;
        services.instruction = PhysicsInstructionRemapTu15{&source, bindings};
    }
    services.required_owner = [&](auto &, const auto &call, auto &abi, const auto &) {
        got.tail = {call.target, call.instruction, call.tail};
        got.tail_abi = abi;
    };
    SourcePhysicsContactRecordObserver observer;
    if (observe) {
        observer = [&](const auto &event) {
            got.stores.push_back(event);
            memory.visit('o', event.address, event.written.size());
        };
    }
    FpScope scope(*input.abi.floating_point);
    MathState state{memory, observer, services, policy == Policy::alternate ?
        SourcePhysicsContactImage::MapPack11 : SourcePhysicsContactImage::TitleUpdate15};
    state.load(input.abi);
    if (policy == Policy::shallow_stack) state.fp.count = 1;
    memory.snapshot = [&](auto kind, auto address, auto width) {
        return Packet{kind, got.cursor, address, width, state.save()};
    };
    memory.start_trace = [&] {
        trace = true;
        if (silent) published.instruction = publish;
    };
    try {
        if (raw) original::dispatch_reference(state);
        else original::run_32e35b(state);
    } catch (const std::exception &error) {
        got.error = error.what();
    }
    got.abi = state.save();
    memory.snapshot = {};
    memory.start_trace = {};
    return got;
}

Case fixture(std::mt19937 &random, unsigned n) {
    constexpr auto stack = 0x62000ff0u, frame = 0x620003f0u, description = 0x63001000u;
    Case input;
    random_abi(random, input, stack);
    input.abi.gp[2] = description;
    input.abi.gp[5] = frame;
    input.abi.x87_depth = 2 + n % 7;
    constexpr std::array values{0.0L, -0.0L, 1.0L, -42.5L,
        std::numeric_limits<long double>::infinity(), -std::numeric_limits<long double>::infinity(),
        std::numeric_limits<long double>::quiet_NaN(), std::numeric_limits<long double>::denorm_min()};
    input.abi.x87[0] = values[n % values.size()];
    input.abi.x87[1] = values[(n / values.size()) % values.size()];
    for (unsigned local = 0x90u; local <= 0xa4u; ++local)
        input.memory.put(frame - local, std::uint8_t(random()), 1);
    input.memory.put(frame - 0xa4u, std::uint16_t(0x037fu | ((n & 3u) << 10u)), 2);
    for (unsigned i = 0; i < 272; ++i)
        input.memory.put(description - 112u + i, std::uint8_t(random()), 1);
    input.memory.put(description + 0xeu, n % 2 ? 5u : n % 4, 1);
    for (unsigned i = 0; i < 16; ++i) {
        input.memory.put(0x469bf4u + i, std::uint8_t(random()), 1);
        input.memory.put(0x46ecb4u + i, std::uint8_t(random()), 1);
    }
    return input;
}

struct Stats {
    unsigned cases{}, faults{}, providers{}, stores{}, tails{}, native_entries{}, traced_entries{};
};

unsigned compare(const Case &input, unsigned id, Stats &stats, bool forced, Policy policy,
                 bool observe, unsigned cut = 0, unsigned trace_at = 0) {
    Provider left(input.memory, cut, trace_at, id % 3u == 0u),
        right(input.memory, cut, trace_at, id % 3u == 0u);
    const auto ref = run(left, input, policy, true, observe);
    const auto got = run(right, input, policy, false, observe);
    const bool same_tail_abi = ref.tail_abi.has_value() == got.tail_abi.has_value() &&
        (!ref.tail_abi || identical(*ref.tail_abi, *got.tail_abi));
    if (ref.error != got.error || ref.cursor != got.cursor || ref.callbacks != got.callbacks ||
        !identical(ref.abi, got.abi) || left.packets != right.packets || ref.stores != got.stores ||
        !left.same(right) || ref.tail != got.tail || !same_tail_abi) {
        std::fprintf(stderr, "Vehicle math case %u fault at %u trace at %u policy %u pc %x/%x I/O %zu/%zu\n",
                     id, cut, trace_at, unsigned(policy), ref.cursor, got.cursor,
                     left.packets.size(), right.packets.size());
        throw std::runtime_error("Vehicle 331ECB differs from retained classification and provider behavior");
    }
    const bool should_fault = cut || policy == Policy::shallow_stack;
    if (got.error.empty() == should_fault || (!should_fault &&
        (!got.tail_abi || got.tail.pc != 0x32e3e5u || !got.tail.tail || got.abi.gp[4] != input.abi.gp[4]))) {
        throw std::runtime_error("Vehicle 331ECB lost its original fault or dynamic tail boundary");
    }
    ++stats.cases;
    stats.faults += should_fault;
    stats.providers += unsigned(got.tail_abi.has_value()) + unsigned(left.packets.size());
    stats.stores += unsigned(got.stores.size());
    stats.tails += unsigned(got.tail_abi.has_value());
    // Activating tracing during the final target read leaves no classifier
    // instructions to emit. The tail still dispatches with the new policy.
    const bool no_retained_suffix = !trace_at || trace_at == left.packets.size();
    stats.native_entries += !forced && (policy == Policy::cursor || policy == Policy::quiet ||
        policy == Policy::mapped || (policy == Policy::mapped_silent && !observe)) && no_retained_suffix;
    stats.traced_entries += bool(trace_at);
    return unsigned(left.packets.size());
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) return 2;
    const unsigned cases = std::stoul(argv[1]);
    const bool forced = std::string_view{argv[2]} == "reference";
    if (forced) setenv("HALO2_REFERENCE_VEHICLE_X87_DISPATCH_331ECB", "1", 1);
    Stats stats;
    std::mt19937 random(0x331ecbu);
    for (unsigned i = 0; i < cases; ++i) {
        auto input = fixture(random, i);
        compare(input, i, stats, forced, Policy::quiet, false);
        compare(input, i, stats, forced, Policy::cursor, true);
        compare(input, i, stats, forced, Policy::mapped, false);
        compare(input, i, stats, forced, Policy::mapped_silent, false);
    }
    for (unsigned i = 0; i < 12; ++i) {
        auto input = fixture(random, i);
        const auto packets = compare(input, i, stats, forced, Policy::cursor, true);
        for (unsigned nth = 1; nth <= packets; ++nth) {
            compare(input, i, stats, forced, Policy::cursor, true, nth);
            compare(input, i, stats, forced, Policy::cursor, true, 0, nth);
            compare(input, i, stats, forced, Policy::mapped, true, nth);
            compare(input, i, stats, forced, Policy::mapped, true, 0, nth);
        }
        const auto quiet_packets = compare(input, i, stats, forced, Policy::mapped_silent, false);
        for (unsigned nth = 1; nth <= quiet_packets; ++nth) {
            compare(input, i, stats, forced, Policy::mapped_silent, false, nth);
            compare(input, i, stats, forced, Policy::mapped_silent, false, 0, nth);
        }
        for (auto policy : {Policy::trace, Policy::arbitrary, Policy::alternate,
                           Policy::unqualified_observer, Policy::shallow_stack,
                           Policy::mapped_trace, Policy::inexact_remap})
            compare(input, i, stats, forced, policy, true);
    }
    std::printf("{\"cases\":%u,\"faults\":%u,\"providers\":%u,\"stores\":%u,"
                "\"tails\":%u,\"native_entries\":%u,\"trace_transitions\":%u}\n",
                stats.cases, stats.faults, stats.providers, stats.stores,
                stats.tails, stats.native_entries, stats.traced_entries);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
