#include "native_tu15_frame_misc_harness.hpp"
#include "native_tu15_particle_birth_observer_fixture.hpp"
#include "rendering/effects/native_tu15_emitter_emission.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_emitter_emission_detail;
namespace world = halo2::native_world_leaves_detail;

namespace {
constexpr std::uint32_t frame = 0x20004100u, emitter = 0x10002000u, other_emitter = 0x10003000u;
constexpr std::uint32_t definition = 0x10006000u, system = 0x10008000u, location = 0x10009000u;

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
        if (accesses.size() == fault) throw std::runtime_error("Injected emitter emission bank fault");
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
    random_abi(random, test, frame - 0x54u);
    test.abi.gp[world::ebp] = frame;
    test.policy = index % 16u;
    auto &memory = test.memory;
    for (unsigned offset = 0; offset < 0xd0u; offset += 4u)
        memory.put(frame - 0x90u + offset, random());
    memory.put(frame - 8u, definition);
    memory.put(frame + 8u, index % 7u == 0u ? 0x7fc02468u : u(0.5f));
    memory.put(frame + 0xcu, system);
    memory.put(frame + 0x10u, definition);
    memory.put(frame + 0x14u, location);
    memory.put(frame + owner::emitter_slot, emitter);
    memory.put(emitter + 4u, index % 3u == 0u ? 0xffffffffu : 0x12345678u);
    memory.put(other_emitter + 4u, 0xffffffffu);
    const std::array remainders{u(0.0f), u(0.3f), u(0.99995f), u(1.0f), u(1.25f), u(2.25f), u(3.75f), 0x7fc01357u};
    memory.put(emitter + owner::remainder, remainders[index % remainders.size()]);
    memory.put(other_emitter + owner::remainder, u(0.25f));
    memory.put(definition, index % 5u == 0u ? 0x7fc13579u : u(float(index % 4u) / 2.0f));
    memory.put(system, random());
    memory.put(location, random());
    memory.put(owner::one, u(1.0f));
    memory.put(owner::zero, 0u);
    memory.put(owner::epsilon, 0x38d1b717u);
    if (index == 35u) {
        memory.put(emitter + owner::remainder, u(1.0f - 0.0001f));
        test.abi.floating_point->mxcsr &= ~0x6000u;
    }
    return test;
}

void visit_child(WorldState &state, std::uint32_t target) {
    log_call(entry_hash(state, target));
    state.instruction_literal(target);
}

void run_176a20(WorldState &state) {
    visit_child(state, 0x176a20u);
    auto &gp = state.gp;
    gp[world::edx] = state.flags.logic(r(state.memory, gp[world::eax], 4), 4);
    if (policy == 15u) throw std::runtime_error("Accumulator predicate stopped after an ABI change");
    gp[world::eax] = 0xaa551100u | (policy % 2u);
    state.pop();
}

void run_250410(WorldState &state) {
    visit_child(state, 0x250410u);
    auto &gp = state.gp;
    gp[world::edx] = state.flags.logic(r(state.memory, gp[world::eax], 4), 4);
    if (policy == 12u) throw std::runtime_error("Emission-rate child stopped after an ABI change");
    state.xmm[0] = world::scalar(f(gp[world::edx]));
    state.pop();
}

void run_250440(WorldState &state) {
    visit_child(state, 0x250440u);
    auto &gp = state.gp;
    gp[world::edx] = r(state.memory, gp[world::eax], 4);
    gp[world::eax] = 0x99887700u | unsigned(policy % 3u != 0u);
    state.pop();
}

void run_178070(WorldState &state) {
    visit_child(state, 0x178070u);
    state.gp[world::edx] = r(state.memory, state.gp[world::ecx], 4);
    state.gp[world::eax] = definition;
    state.pop();
}

void run_250430(WorldState &state) {
    visit_child(state, 0x250430u);
    auto &gp = state.gp;
    gp[world::edx] = state.flags.logic(r(state.memory, gp[world::eax], 4), 4);
    if (policy == 13u) throw std::runtime_error("Interpolation predicate stopped after an ABI change");
    gp[world::eax] = 0x55443300u | unsigned(policy % 4u != 0u);
    state.pop();
}

void run_250c60(WorldState &state) {
    visit_child(state, 0x250c60u);
    auto &gp = state.gp;
    const auto stack = gp[world::esp];
    const auto fraction = r(state.memory, stack + 16u, 4);
    const auto scale = r(state.memory, stack + 24u, 4);
    gp[world::eax] = state.flags.logic(fraction ^ scale, 4);
    if (policy == 14u) throw std::runtime_error("Birth owner stopped after an ABI change");
    w(state.memory, state.observe, 0x250c60u, gp[world::ecx] + 4u, 4, gp[world::eax]);
    if (policy == 11u)
        w(state.memory, state.observe, 0x250c60u, gp[world::ebp] + owner::emitter_slot, 4, other_emitter);
    if (policy == 5u) {
        gp[world::edx] = 0x12345678u;
        state.fp.push(2.25L);
    }
    state.pop();
    gp[world::esp] += 24u;
}
} // namespace

namespace original {
using ::run_176a20;
using ::run_250410;
using ::run_250440;
using ::run_178070;
using ::run_250430;
using ::run_250c60;
#include "emission.inc"
} // namespace original

namespace {
bool native_entry(WorldState &state) {
    return owner::bridge(state, original::native_emitter_emission_reference_250780, [](auto &value, auto child) {
        if constexpr (child.value == 0x176a20u) run_176a20(value);
        if constexpr (child.value == 0x250410u) run_250410(value);
        if constexpr (child.value == 0x250440u) run_250440(value);
        if constexpr (child.value == 0x178070u) run_178070(value);
        if constexpr (child.value == 0x250430u) run_250430(value);
        if constexpr (child.value == 0x250c60u) run_250c60(value);
    });
}

Result run(FaultBank &bank, const Case &fixture, bool native, bool birth_observer) {
    Result result;
    const SourcePhysicsContactRecordObserver stores = [&](const auto &store) { result.stores.push_back(store); };
    const BirthObserverFixture birth;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t site) {
        if (native && site >= owner::first && site <= owner::last)
            throw std::runtime_error("Emitter emission delivered a translated source callback");
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
    const auto &observer = birth_observer ? birth.collector.observer() : stores;
    WorldState state{bank, observer, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(fixture.abi);
    policy = fixture.policy;
    call_log = &result.calls;
    try {
        if (native) result.admitted = native_entry(state);
        else original::native_emitter_emission_reference_250780_live(state);
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
    return result;
}

unsigned compare(const Case &test, unsigned case_no, unsigned at, Totals &totals, bool birth_observer = false) {
    FaultBank a{test.memory, at}, b{test.memory, at};
    const auto mine = run(a, test, true, birth_observer), theirs = run(b, test, false, birth_observer);
    ++totals.compared;
    totals.stores += theirs.stores.size();
    totals.children += theirs.calls.size();
    if (!theirs.fault.empty()) ++totals.faults;
    if ((!mine.admitted && mine.fault.empty()) || mine.fault != theirs.fault || mine.stores != theirs.stores ||
        a.accesses != b.accesses || !a.same(b) || mine.calls != theirs.calls ||
        (!census_mode && mine.cursor != theirs.cursor) || !same_abi(mine.abi, theirs.abi) ||
        mine.abi.fs_base != theirs.abi.fs_base || mine.mxcsr != theirs.mxcsr) {
        std::fprintf(stderr, "Case %u access %u native '%s' source '%s', PC %x/%x stores %zu/%zu calls %zu/%zu\n",
            case_no, at, mine.fault.c_str(), theirs.fault.c_str(), mine.cursor, theirs.cursor,
            mine.stores.size(), theirs.stores.size(), mine.calls.size(), theirs.calls.size());
        for (unsigned i = 0; i < 8; ++i)
            if (mine.abi.gp[i] != theirs.abi.gp[i])
                std::fprintf(stderr, "GP%u %x/%x\n", i, mine.abi.gp[i], theirs.abi.gp[i]);
        throw std::runtime_error("Particle emitter emission differential mismatch");
    }
    return unsigned(a.accesses.size());
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x25097bu);
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
    if (!census_mode) check_refusals(fixture(random, count), native_entry);
    if (!census_mode) check_birth_observer_refusals(fixture(random, count), native_entry);
    report(count, totals, owner::summary().coverage.load(), owner::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
