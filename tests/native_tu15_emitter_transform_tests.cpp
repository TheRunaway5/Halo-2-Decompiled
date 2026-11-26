#include "native_tu15_particle_birth_observer_fixture.hpp"
#include "rendering/effects/native_tu15_emitter_transform.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_emitter_transform_detail;
namespace world = halo2::native_world_leaves_detail;

namespace {
constexpr std::uint32_t stack = 0x20004100u, emitter = 0x10002000u, matrix = 0x10005000u;
constexpr std::uint32_t second_emitter = 0x10003000u, definition = 0x10006000u, system = 0x10008000u;

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
        if (accesses.size() == fault) throw std::runtime_error("Injected emitter transform bank fault");
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
    test.abi.gp[world::ecx] = emitter;
    test.policy = index % 12u;
    auto &memory = test.memory;
    for (unsigned offset = 0; offset < 0xc0u; offset += 4u)
        memory.put(stack - 0x80u + offset, random());
    memory.put(stack + 8u, system);
    memory.put(stack + 12u, definition);
    const auto transform = index % 11u == 0u ? 0u : index % 7u == 0u ? emitter + 8u : matrix;
    memory.put(stack + 0x14u, transform);
    memory.put(system, random());
    memory.put(definition, random());
    memory.put(owner::one, u(1.0f));
    std::mt19937_64 floats(random());
    for (unsigned offset = 0; offset < sizeof(halo2::ParticleEmitterState); offset += 4u) {
        memory.put(emitter + offset, special_float(floats));
        memory.put(second_emitter + offset, special_float(floats));
    }
    for (unsigned offset = 0; offset < sizeof(halo2::ParticleEmitterFrame); offset += 4u)
        if (transform) memory.put(transform + offset, special_float(floats));
    // Explicit two-NaN case where the original destination payload wins.
    if (transform && index % 12u == 1u) {
        memory.put(transform, 0x7fc02468u);
        memory.put(transform + offsetof(halo2::ParticleEmitterFrame, position), 0x7fc01357u);
    }
    return test;
}

void visit_child(WorldState &state, std::uint32_t target) {
    log_call(entry_hash(state, target));
    state.instruction_literal(target);
}

void run_178070(WorldState &state) {
    visit_child(state, owner::resolver);
    auto &gp = state.gp;
    gp[world::edx] = state.flags.logic(r(state.memory, gp[world::ecx], 4), 4);
    if (policy == 8u) throw std::runtime_error("Resolver stopped after an ABI change");
    gp[world::eax] = definition;
    if (policy == 4u) state.fp.push(4.25L);
    state.pop();
}

void run_250400(WorldState &state) {
    visit_child(state, owner::orientation);
    auto &gp = state.gp;
    gp[world::edx] = state.flags.logic(r(state.memory, gp[world::eax], 4), 4);
    if (policy == 9u) throw std::runtime_error("Orientation child stopped after an ABI change");
    gp[world::eax] = definition;
    state.pop();
}

void run_176a30(WorldState &state) {
    visit_child(state, owner::scales_position);
    auto &gp = state.gp;
    gp[world::edx] = r(state.memory, gp[world::eax], 4);
    gp[world::eax] = 0x17171700u | (policy % 2u);
    gp[world::ebx] = state.flags.logic(0x21436587u, 4);
    if (policy == 10u) throw std::runtime_error("Scale child stopped after an ABI change");
    if (policy == 11u)
        w(state.memory, state.observe, owner::scales_position, gp[world::ebp] + owner::emitter_slot, 4, second_emitter);
    state.xmm[3] = _mm_set1_ps(1.0f);
    state.pop();
}
} // namespace

namespace original {
using ::run_178070;
using ::run_250400;
using ::run_176a30;
#include "transform.inc"
} // namespace original

namespace {
owner::Resume native_entry(WorldState &state) {
    return owner::bridge(state, original::native_emitter_transform_reference_250780, run_178070, run_250400, run_176a30);
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
        if (native && site >= owner::first && site <= owner::last)
            throw std::runtime_error("Emitter transform delivered a translated source callback");
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
        execution.resume = native ? native_entry(state) : original::native_emitter_transform_reference_250780_live(state);
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
            unsigned(native.resume), unsigned(original.resume), mine.stores.size(), theirs.stores.size(), mine.calls.size(), theirs.calls.size());
        for (unsigned i = 0; i < 8; ++i)
            if (mine.abi.gp[i] != theirs.abi.gp[i])
                std::fprintf(stderr, "GP%u %x/%x\n", i, mine.abi.gp[i], theirs.abi.gp[i]);
        throw std::runtime_error("Particle emitter transform differential mismatch");
    }
    return unsigned(a.accesses.size());
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x250780u);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = fixture(random, index);
        const auto accesses = compare(test, index, 0u, totals);
        if (index < 36u) {
            for (unsigned at = 1; at <= accesses; ++at) compare(test, index, at, totals);
        }
        if (!census_mode && index < 48u) {
            const auto observed_accesses = compare(test, index, 0u, totals, true);
            if (observed_accesses != accesses)
                throw std::runtime_error("Birth collector changed the ordered preimage and write count");
            for (unsigned at = 1; at <= observed_accesses; ++at) compare(test, index, at, totals, true);
        }
    }
    if (!census_mode) check_refusals(fixture(random, count), [](WorldState &state) {
        return native_entry(state) != owner::Resume::retained;
    });
    if (!census_mode) check_birth_observer_refusals(fixture(random, count),
        [](WorldState &state) { return native_entry(state) != owner::Resume::retained; });
    report(count, totals, owner::summary().coverage.load(), owner::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
