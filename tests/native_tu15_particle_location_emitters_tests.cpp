#include "native_tu15_particle_birth_observer_fixture.hpp"
#include "rendering/effects/native_tu15_particle_location_emitters.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_particle_location_emitters_detail;
namespace world = halo2::native_world_leaves_detail;

namespace {
constexpr std::uint32_t stack = 0x200040a0u, array_header = 0x10000000u, rows = 0x10002000u;
constexpr std::uint32_t location = 0x10004000u, output = 0x10005000u, definition = 0x10007000u;
constexpr std::uint32_t emitter_defs = 0x10008000u, system = 0x1000a000u, other_header = 0x10001000u;
constexpr std::uint32_t none = 0xffffffffu, slots = 8u, emitter_size = sizeof(owner::EmitterDatum);

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
        if (accesses.size() == fault) throw std::runtime_error("Injected emitter tail bank fault");
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
    auto &memory = test.memory;
    random_abi(random, test, stack);
    test.abi.gp[world::edi] = location;
    test.abi.gp[world::ebp] = definition;
    test.policy = index % 16u;

    for (unsigned offset = 0; offset < 0x90u; offset += 4u)
        memory.put(stack - 0x30u + offset, random());
    memory.put(stack + 0x18u, index % 16u == 0u ? 0u : index % 16u == 1u ? none : 1u + index % 5u);
    const auto chain_count = index % 4u;
    memory.put(stack + 0x1cu, chain_count ? 0x10000u : none);
    memory.put(stack + 0x20u, definition);
    memory.put(stack + 0x28u, location);
    memory.put(stack + 0x2cu, random());
    memory.put(stack + 0x30u, system);
    memory.put(stack + 0x34u, index % 7u == 0u ? location : output);
    memory.put(stack + 0x38u, 0x12340000u | index);
    memory.put(location + 4u, chain_count ? 0x10000u : none);
    memory.put(location + 8u, chain_count ? 0x10000u + chain_count - 1u : none);
    memory.put(location + 0x1cu, random());
    memory.put(output + 0x1cu, random());
    memory.put(system + 0x50u, random());
    memory.put(definition + 0x34u, emitter_defs);
    memory.put(owner::layout::globals::emitters, array_header);
    memory.put(array_header + 0x44u, rows);
    memory.put(other_header + 0x44u, rows);
    for (unsigned slot = 0; slot < slots; ++slot) {
        const auto row = rows + slot * emitter_size;
        memory.put(row, 1u, 2u);
        memory.put(row + 2u, 0u, 2u);
        memory.put(row + 4u, none);
        memory.put(row + 8u, slot + 1u < chain_count ? 0x10000u + slot + 1u : none);
        memory.put(row + 12u, random());
    }
    return test;
}

void dispatch(WorldState &state, std::uint32_t target, std::uint32_t pc, bool tail) {
    if (target != owner::allocate_emitter || pc != 0x251e73u || tail)
        throw std::runtime_error("Unexpected emitter allocation route");
    auto &gp = state.gp;
    log_call(mix(entry_hash(state, target), pc));
    state.instruction_literal(target);
    const auto iteration = r(state.memory, gp[world::esp] + 0x18u, 4);
    gp[world::eax] = 0x10000u + 3u + (iteration % 5u);
    if (policy == 5u) gp[world::eax] = none;
    if (policy == 9u) gp[world::ebx] = other_header;
    if (policy == 10u) {
        gp[world::eax] = gp[world::edi] = 0xfffe0009u;
    }
    if (policy == 12u) {
        gp[world::edx] = state.flags.logic(0x76543210u, 4);
        throw std::runtime_error("Allocator stopped after publishing partial state");
    }
    if (gp[world::eax] != gp[world::edi]) {
        const auto row = rows + (gp[world::eax] & 0xffffu) * emitter_size;
        w(state.memory, state.observe, target, row, 2, 0x8f21u);
    }
    state.pop();
}

void run_250780(WorldState &state) {
    auto &gp = state.gp;
    log_call(mix(entry_hash(state, owner::update_emitter), 0x251f34u));
    state.instruction_literal(owner::update_emitter);
    const auto entry = gp[world::esp];
    const auto emitter = gp[world::ecx];
    const auto a = r(state.memory, entry + 4u, 4);
    const auto b = r(state.memory, entry + 12u, 4);
    const auto c = r(state.memory, entry + 24u, 4);
    gp[world::eax] = state.flags.logic(a ^ b ^ c, 4);
    gp[world::edx] = emitter;
    if (policy == 14u) {
        gp[world::ebx] += 0x30u;
        state.xmm[4] = _mm_set1_ps(1.0f);
    }
    if (policy == 13u) throw std::runtime_error("Emitter stopped after publishing partial state");
    w(state.memory, state.observe, owner::update_emitter, emitter + 0xcu, 4, gp[world::eax]);
    state.pop();
    gp[world::esp] += 24u;
}
} // namespace

namespace original {
using ::dispatch;
using ::run_250780;
#include "tail.inc"
} // namespace original

namespace {
bool run_native(WorldState &state) {
    return owner::bridge(state, original::native_particle_location_emitters_reference_251c80,
        [](auto &value, auto target, auto site) { dispatch(value, target, site, false); }, run_250780);
}

Result run(FaultBank &bank, const Case &fixture, bool native, bool birth_observer) {
    Result result;
    const SourcePhysicsContactRecordObserver stores = [&](const auto &store) { result.stores.push_back(store); };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t site) {
        if (native && site >= owner::first && site <= owner::last)
            throw std::runtime_error("Emitter tail delivered its retained source callback");
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
        if (native) result.admitted = run_native(state);
        else original::native_particle_location_emitters_reference_251c80_live(state);
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
        (!census_mode && mine.cursor != theirs.cursor) || !same_abi(mine.abi, theirs.abi) || mine.mxcsr != theirs.mxcsr) {
        std::fprintf(stderr, "Case %u access %u native '%s' source '%s' PC %x/%x EFLAGS %x/%x stores %zu/%zu calls %zu/%zu\n",
            case_no, at, mine.fault.c_str(), theirs.fault.c_str(), mine.cursor, theirs.cursor,
            mine.abi.eflags, theirs.abi.eflags, mine.stores.size(), theirs.stores.size(), mine.calls.size(), theirs.calls.size());
        for (unsigned i = 0; i < 8; ++i)
            if (mine.abi.gp[i] != theirs.abi.gp[i])
                std::fprintf(stderr, "GP%u %x/%x\n", i, mine.abi.gp[i], theirs.abi.gp[i]);
        throw std::runtime_error("Particle location emitter continuation differential mismatch");
    }
    return unsigned(a.accesses.size());
}

void require_owner_policy(Case test) {
    const SourcePhysicsContactRecordObserver observer;
    SourcePhysicsContactRuntimeServices services;
    services.required_owner = [](auto &, const auto &, auto &, const auto &) {
        throw std::runtime_error("Unqualified provider must be refused before native work");
    };
    WorldState state{test.memory, observer, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(test.abi);
    if (run_native(state)) throw std::runtime_error("Emitter tail admitted an unqualified required-owner policy");
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x251e30u);
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
    if (!census_mode) check_refusals(fixture(random, count), run_native);
    require_owner_policy(fixture(random, count));
    if (!census_mode) check_birth_observer_refusals(fixture(random, count), run_native);
    report(count, totals, owner::coverage().load(), owner::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
