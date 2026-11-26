#include "native_tu15_particle_birth_observer_fixture.hpp"
#include "rendering/effects/native_tu15_particle_location_entry.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_particle_location_entry_detail;
namespace world = halo2::native_world_leaves_detail;

namespace {
constexpr std::uint32_t stack = 0x200040b0u, location = 0x10002000u, transform = 0x10004000u;
constexpr std::uint32_t second_location = 0x10003000u, definition = 0x10006000u, observers = 0x10008000u;

struct Access {
    std::uint32_t address;
    std::size_t width;
    bool write;
    friend bool operator==(const Access &, const Access &) = default;
};

struct FaultBank : Memory {
    FaultBank(const Memory &base, unsigned fault) : Memory(base.clone()), fault(fault) {}
    unsigned fault;
    mutable std::vector<Access> accesses;

    void record(Access event) const {
        accesses.push_back(event);
        if (accesses.size() == fault) throw std::runtime_error("Injected particle entry bank fault");
    }
    void read(std::uint32_t address, std::span<std::byte> output) const override {
        record({address, output.size(), false});
        Memory::read(address, output);
    }
    void write(std::uint32_t address, std::span<const std::byte> input) override {
        record({address, input.size(), true});
        Memory::write(address, input);
    }
};

Case fixture(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, stack);
    test.abi.gp[world::eax] = (test.abi.gp[world::eax] & 0xffffff00u) | (index % 3u == 1u ? 0x80u : 0u);
    test.policy = index % 12u;
    auto &memory = test.memory;
    for (unsigned offset = 0; offset < 0x90u; offset += 4u)
        memory.put(stack - 0x40u + offset, random());
    memory.put(stack + 4u, location);
    memory.put(stack + 0x0cu, random());
    // An overlapping position copy still rereads each source word in order.
    const auto matrix = (index % 7u == 0u) ? location - 0x1cu : transform;
    memory.put(stack + 0x14u, matrix);
    for (unsigned offset = 0; offset < sizeof(owner::LocationDatum); offset += 4u) {
        memory.put(location + offset, random());
        memory.put(second_location + offset, random());
    }
    std::mt19937_64 floats(random());
    for (unsigned axis = 0; axis < 3; ++axis)
        memory.put(matrix + 0x28u + axis*4u, special_float(floats));
    memory.put(definition + offsetof(owner::SystemDefinition, emitter_count), index % 8u);
    memory.put(0x4f54f0u, observers);
    constexpr std::array counts{0xffffu, 0u, 1u, 4u, 0x8000u, 0x7fffu};
    memory.put(observers + 8u, counts[(index / 3u) % counts.size()], 2u);
    memory.put(owner::layout::globals::one, index % 6u == 0u ? special_float(floats) : u(1.0f));
    return test;
}

void run_178070(WorldState &state) {
    auto &gp = state.gp;
    log_call(mix(entry_hash(state, owner::resolver), 0x251c98u));
    state.instruction_literal(owner::resolver);
    // The resolver uses both input registers and owns the exact return slot.
    const auto emitter = gp[world::eax];
    const auto selector = gp[world::ecx];
    gp[world::edx] = state.flags.logic(emitter ^ selector, 4);
    if (policy == 6u) {
        gp[world::edi] = second_location;
        gp[world::ebx] &= 0xffffff00u;
        state.fp.push(3.5L);
    }
    if (policy == 9u) {
        gp[world::ebx] |= 0x80u;
        state.xmm[3] = _mm_set1_ps(2.0f);
    }
    if (policy == 11u) throw std::runtime_error("Resolver stopped after publishing partial state");
    w(state.memory, state.observe, owner::resolver, definition + 0x30u, 4, gp[world::edx] & 7u);
    gp[world::eax] = definition;
    state.pop();
}
} // namespace

namespace original {
using ::run_178070;
#include "entry.inc"
} // namespace original

namespace {
owner::Resume native_entry(WorldState &state) {
    return owner::bridge(state, original::native_particle_location_entry_reference_251c80, run_178070);
}

struct Execution {
    Result result;
    owner::Resume resume{owner::Resume::retained};
};

Execution run(FaultBank &bank, const Case &fixture, bool native, bool birth_observer) {
    Execution execution;
    auto &result = execution.result;
    const SourcePhysicsContactRecordObserver stores = [&](const auto &store) { result.stores.push_back(store); };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t site) {
        if (native && site >= owner::entry_site && site <= owner::forced_lod_site)
            throw std::runtime_error("Particle entry delivered its retained source callback");
        result.cursor = site;
    };
    if (native && !census_mode) {
        auto &cursor = services.instruction_cursor;
        cursor.position = &result.cursor;
        cursor.service_sites = halo2::native_census_detail::service_sites;
        cursor.callback_type = &services.instruction.target_type();
        cursor.executor = std::this_thread::get_id();
        cursor.literal_service_sites = true;
        cursor.observation_only_stores = !birth_observer;
    }

    std::uint16_t control;
    const auto initial = _mm_getcsr();
    asm volatile("fnstcw %0" : "=m"(control));
    BirthObserverFixture birth;
    const auto &observe = birth_observer ? birth.collector.observer() : stores;
    WorldState state{bank, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(fixture.abi);
    policy = fixture.policy;
    call_log = &result.calls;
    try {
        execution.resume = native ? native_entry(state) : original::native_particle_location_entry_reference_251c80_live(state);
    } catch (const std::exception &error) {
        result.fault = error.what();
    }
    call_log = nullptr;
    if (birth_observer && birth.presentation.colors_observed)
        throw std::runtime_error("Particle store altered the birth color receipt");
    result.abi = state.save();
    result.mxcsr = _mm_getcsr();
    asm volatile("fnclex\n\tfldcw %0" :: "m"(control));
    _mm_setcsr(initial);
    return execution;
}

unsigned compare(const Case &test, unsigned case_no, unsigned at, Totals &totals, bool birth_observer = false) {
    FaultBank a{test.memory, at}, b{test.memory, at};
    const auto native = run(a, test, true, birth_observer), original = run(b, test, false, birth_observer);
    const auto &mine = native.result;
    const auto &theirs = original.result;
    ++totals.compared;
    totals.stores += theirs.stores.size();
    totals.children += theirs.calls.size();
    if (!theirs.fault.empty()) ++totals.faults;
    if (native.resume != original.resume || mine.fault != theirs.fault || mine.stores != theirs.stores ||
        a.accesses != b.accesses || !a.same(b) || mine.calls != theirs.calls ||
        (!census_mode && mine.cursor != theirs.cursor) || !same_abi(mine.abi, theirs.abi) ||
        mine.abi.fs_base != theirs.abi.fs_base || mine.mxcsr != theirs.mxcsr) {
        std::fprintf(stderr, "Case %u access %u native '%s' source '%s' PC %x/%x resumes %u/%u stores %zu/%zu calls %zu/%zu\n",
            case_no, at, mine.fault.c_str(), theirs.fault.c_str(), mine.cursor, theirs.cursor,
            unsigned(native.resume), unsigned(original.resume), mine.stores.size(), theirs.stores.size(),
            mine.calls.size(), theirs.calls.size());
        for (unsigned i = 0; i < 8; ++i)
            if (mine.abi.gp[i] != theirs.abi.gp[i])
                std::fprintf(stderr, "GP%u %x/%x\n", i, mine.abi.gp[i], theirs.abi.gp[i]);
        throw std::runtime_error("Particle location entry differential mismatch");
    }
    return unsigned(a.accesses.size());
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x251c80u);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = fixture(random, index);
        const auto accesses = compare(test, index, 0u, totals);
        if (index < 42u) {
            for (unsigned at = 1; at <= accesses; ++at) compare(test, index, at, totals);
        }
        if (!census_mode && index < 48u) {
            const auto observed_accesses = compare(test, index, 0u, totals, true);
            if (observed_accesses != accesses)
                throw std::runtime_error("Birth collector changed the ordered preimage and write count");
            for (unsigned at = 1; at <= observed_accesses; ++at) compare(test, index, at, totals, true);
        }
    }
    if (!census_mode) {
        check_refusals(fixture(random, count), [](WorldState &state) {
            return native_entry(state) != owner::Resume::retained;
        });
    }
    if (!census_mode) check_birth_observer_refusals(fixture(random, count),
        [](WorldState &state) { return native_entry(state) != owner::Resume::retained; });
    report(count, totals, owner::summary().coverage.load(), owner::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
