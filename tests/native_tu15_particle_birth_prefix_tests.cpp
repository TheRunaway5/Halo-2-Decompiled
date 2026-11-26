#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_birth_prefix.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_particle_birth_prefix_detail;
namespace world = halo2::native_world_leaves_detail;

namespace {
constexpr std::uint32_t stack = 0x20004200u, frame = stack - 0xc0u;
constexpr std::uint32_t emitter = 0x10002000u, system = 0x10003000u, context = 0x10004000u;
constexpr std::uint32_t other_context = 0x10005000u, definition = 0x10006000u, rng = 0x10007000u;
constexpr std::uint32_t array = 0x10008000u, other_array = 0x10009000u, data = 0x1000a000u;
constexpr std::uint32_t effect_array = 0x1000b000u, effects = 0x1000c000u, handle = 0xd3310001u;
constexpr std::uint32_t particle = data + sizeof(owner::ParticleDatum);

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
        if (accesses.size() == fault) throw std::runtime_error("Injected particle birth bank fault");
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
    test.abi.gp[world::eax] = context;
    test.abi.gp[world::ecx] = emitter;
    test.policy = index % 16u;
    test.abi.x87_status |= std::uint16_t(index & 0x3fu);
    if (test.policy == 10u) test.abi.x87_depth = 8u;
    auto &memory = test.memory;
    for (unsigned offset = 0; offset < 0x110u; offset += 4u)
        memory.put(frame + offset - 8u, random());
    memory.put(stack + 4u, system);
    memory.put(system + 4u, 0x7fc01357u);
    memory.put(system + offsetof(owner::inputs::Emitter, effect), index % 4u == 0u ? 0xffffffffu : 0xd8570001u);
    memory.put(emitter + 4u, random());
    memory.put(owner::inputs::globals::effects, effect_array);
    memory.put(effect_array + 0x44u, effects);
    memory.put(effects + sizeof(owner::inputs::EffectDatum) + 2u, index % 7u == 0u ? 0x88u : 0x80u, 1u);
    memory.put(owner::location::globals::particles, array);
    memory.put(array + 0x44u, data);
    memory.put(other_array + 0x44u, data);
    memory.put(context + 0x44u, random());
    memory.put(context + 0x50u, index % 2u == 0u ? particle : 0u);
    memory.put(other_context + 0x44u, random());
    memory.put(other_context + 0x50u, index % 3u == 0u ? particle : 0u);
    for (unsigned offset = 0; offset < sizeof(owner::ParticleDatum); offset += 4u)
        memory.put(particle + offset, random());
    const auto seed_owner = index % 11u == 0u ? frame + 0xcu : rng;
    memory.put(owner::random_state, seed_owner);
    memory.put(seed_owner + 4u, index % 3u == 1u ? 0xffffffffu : index % 3u == 2u ? 0u : random());
    memory.put(owner::low_unsigned_correction, 0x4f800000u);
    memory.put(owner::inputs::globals::word_scale, index % 5u == 1u ? 0x7fc02468u : 0x37800080u);
    memory.put(owner::random_word_range, 0x477fff00u);
    return test;
}

void dispatch(WorldState &state, std::uint32_t target, std::uint32_t site, bool tail) {
    if (target != owner::allocate || site != 0x250cb0u || tail)
        throw std::runtime_error("Unexpected particle allocator route");
    log_call(mix(entry_hash(state, target), site));
    state.instruction_literal(target);
    auto &gp = state.gp;
    gp[world::eax] = r(state.memory, gp[world::edx] + 0x44u, 4);
    if (policy == 13u) throw std::runtime_error("Particle allocator stopped after an ABI change");
    gp[world::eax] = policy == 11u ? 0xffffffffu : handle;
    if (policy == 12u) gp[world::edx] = other_array;
    if (gp[world::eax] != 0xffffffffu) w(state.memory, state.observe, target, particle, 2, handle >> 16u);
    state.pop();
}

void run_178070(WorldState &state) {
    log_call(entry_hash(state, owner::resolve));
    state.instruction_literal(owner::resolve);
    auto &gp = state.gp;
    gp[world::edx] = state.flags.logic(r(state.memory, gp[world::ecx] + 4u, 4), 4);
    if (policy == 14u) throw std::runtime_error("Particle resolver stopped after an ABI change");
    gp[world::eax] = definition;
    if (policy == 15u) {
        gp[world::esi] = other_context;
        state.fp.push(1.25L);
    }
    state.pop();
}
} // namespace

namespace original {
using ::dispatch;
using ::run_178070;
#include "birth.inc"
} // namespace original

namespace {
owner::Resume native_entry(WorldState &state) {
    return owner::bridge(state, original::native_particle_birth_prefix_reference_250c60,
        [](auto &value, auto target, auto site) { dispatch(value, target, site, false); }, run_178070);
}

struct Execution {
    Result result;
    owner::Resume resume{owner::Resume::retained};
};

Execution run(FaultBank &bank, const Case &fixture, bool native) {
    Execution execution;
    auto &result = execution.result;
    const SourcePhysicsContactRecordObserver stores = [&](const auto &store) { result.stores.push_back(store); };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t site) {
        if (native && site >= owner::first && site <= owner::last)
            throw std::runtime_error("Particle birth prefix delivered a translated source callback");
        result.cursor = site;
    };
    if (native && !census_mode) {
        auto &cursor = services.instruction_cursor;
        cursor.position = &result.cursor;
        cursor.service_sites = halo2::native_census_detail::service_sites;
        cursor.callback_type = &services.instruction.target_type();
        cursor.executor = std::this_thread::get_id();
        cursor.literal_service_sites = true;
        cursor.observation_only_stores = true;
    }
    std::uint16_t control;
    const auto initial = _mm_getcsr();
    asm volatile("fnstcw %0" : "=m"(control));
    WorldState state{bank, stores, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(fixture.abi);
    policy = fixture.policy;
    call_log = &result.calls;
    try {
        execution.resume = native ? native_entry(state) : original::native_particle_birth_prefix_reference_250c60_live(state);
    } catch (const std::exception &error) {
        result.fault = error.what();
    }
    call_log = nullptr;
    result.abi = state.save();
    result.mxcsr = _mm_getcsr();
    asm volatile("fnclex\n\tfldcw %0" :: "m"(control));
    _mm_setcsr(initial);
    return execution;
}

unsigned compare(const Case &test, unsigned case_no, unsigned at, Totals &totals) {
    FaultBank a{test.memory, at}, b{test.memory, at};
    const auto native = run(a, test, true), original = run(b, test, false);
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
        std::fprintf(stderr, "Case %u access %u native '%s' source '%s', PC %x/%x resumes %u/%u stores %zu/%zu x87 %x/%x\n",
            case_no, at, mine.fault.c_str(), theirs.fault.c_str(), mine.cursor, theirs.cursor, unsigned(native.resume),
            unsigned(original.resume), mine.stores.size(), theirs.stores.size(), mine.abi.x87_status, theirs.abi.x87_status);
        for (unsigned i = 0; i < 8; ++i)
            if (mine.abi.gp[i] != theirs.abi.gp[i])
                std::fprintf(stderr, "GP%u %x/%x\n", i, mine.abi.gp[i], theirs.abi.gp[i]);
        throw std::runtime_error("Particle birth prefix differential mismatch");
    }
    return unsigned(a.accesses.size());
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x250c60u);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = fixture(random, index);
        const auto accesses = compare(test, index, 0u, totals);
        if (index < 36u) {
            for (unsigned at = 1; at <= accesses; ++at) compare(test, index, at, totals);
        }
    }
    if (!census_mode) check_refusals(fixture(random, count), [](WorldState &state) {
        return native_entry(state) != owner::Resume::retained;
    });
    report(count, totals, owner::summary().coverage.load(), owner::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
