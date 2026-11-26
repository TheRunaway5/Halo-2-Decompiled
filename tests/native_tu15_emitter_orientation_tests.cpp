#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_emitter_orientation.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_emitter_orientation_detail;
namespace world = halo2::native_world_leaves_detail;

namespace {
constexpr std::uint32_t frame = 0x20004080u, emitter = 0x10002000u, placement = 0x10003000u;
constexpr std::uint32_t definition = 0x10006000u, authored_offset = 0x10007000u;
constexpr std::uint32_t threshold_bits = 0x38d1b717u;

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
        if (accesses.size() == fault) throw std::runtime_error("Injected emitter placement bank fault");
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
    test.abi.gp[world::eax] = definition;
    test.policy = index % 16u;
    if (test.policy == 7u) test.abi.x87_depth = 0u;
    auto &memory = test.memory;
    for (unsigned offset = 0; offset < 0xd0u; offset += 4u)
        memory.put(frame - 0x90u + offset, random());
    memory.put(frame + owner::emitter_slot, emitter);
    memory.put(frame + owner::placement_slot, placement);
    memory.put(definition, random());
    memory.put(owner::angle_threshold, threshold_bits);
    memory.put(owner::zero, 0u);
    const std::array angles{u(0.0f), u(0.000025f), threshold_bits, u(0.125f), u(-0.25f), 0x7fc01357u, 0x7f800111u};
    memory.put(placement, angles[(index / 7u) % angles.size()]);
    memory.put(placement + 4u, angles[index % angles.size()]);
    std::mt19937_64 floats(random());
    for (unsigned offset = 0; offset < sizeof(halo2::ParticleEmitterState); offset += 4u)
        memory.put(emitter + offset, special_float(floats));
    for (unsigned axis = 0; axis < 3u; ++axis)
        memory.put(authored_offset + 4u*axis, special_float(floats));
    if (index % 5u == 0u) {
        memory.put(emitter + offsetof(halo2::ParticleEmitterState, position), 0x7fc02468u);
        memory.put(authored_offset, 0x7fc13579u);
    }
    return test;
}

void visit_child(WorldState &state, std::uint32_t target) {
    log_call(entry_hash(state, target));
    state.instruction_literal(target);
}

void run_2503f0(WorldState &state) {
    visit_child(state, 0x2503f0u);
    state.gp[world::edx] = state.flags.logic(r(state.memory, state.gp[world::eax], 4), 4);
    if (policy == 13u) throw std::runtime_error("Offset child stopped after an ABI change");
    state.gp[world::eax] = authored_offset;
    state.pop();
}

void run_145b70(WorldState &state) {
    visit_child(state, 0x145b70u);
    auto &gp = state.gp;
    for (unsigned axis = 0; axis < 3u; ++axis) {
        const auto bits = r(state.memory, gp[world::ecx] + 4u*axis, 4);
        w(state.memory, state.observe, 0x145b70u, gp[world::eax] + 4u*axis, 4, bits);
    }
    state.xmm[2] = _mm_set1_ps(0.75f);
    state.pop();
}

void run_1c180(WorldState &state) {
    visit_child(state, 0x1c180u);
    const auto bits = r(state.memory, state.gp[world::esp] + 4u, 4);
    if (policy == 14u) {
        state.gp[world::edx] = state.flags.logic(0x87654321u, 4);
        throw std::runtime_error("Angle child stopped after an ABI change");
    }
    if (policy != 7u) state.fp.push(ext(f(bits & 0x7fffffffu)));
    state.pop();
}

void dispatch(WorldState &state, std::uint32_t target, std::uint32_t site, bool tail) {
    visit_child(state, target);
    auto &gp = state.gp;
    if (tail) throw std::runtime_error("Unexpected tail dispatch in emitter placement");
    if (target == 0x1458a0u && site == 0x250923u) {
        const auto float_word = r(state.memory, gp[world::esp] + 4u, 4);
        w(state.memory, state.observe, target, gp[world::eax], 4, float_word);
        state.pop();
        gp[world::esp] += 12u;
    } else if (target == 0x1459b0u && site == 0x250937u) {
        const auto float_word = r(state.memory, gp[world::ecx], 4);
        w(state.memory, state.observe, target, gp[world::edx], 4, float_word);
        state.pop();
    } else {
        throw std::runtime_error("Unexpected orientation child or call site");
    }
}
} // namespace

namespace original {
using ::run_2503f0;
using ::run_145b70;
using ::run_1c180;
using ::dispatch;
#include "orientation.inc"
} // namespace original

namespace {
bool native_entry(WorldState &state) {
    return owner::bridge(state, original::native_emitter_orientation_reference_250780,
        [](auto &value, auto child, std::uint32_t site) {
            if constexpr (child.value == 0x2503f0u) run_2503f0(value);
            if constexpr (child.value == 0x145b70u) run_145b70(value);
            if constexpr (child.value == 0x1c180u) run_1c180(value);
            if constexpr (child.value == 0x1458a0u || child.value == 0x1459b0u) dispatch(value, child.value, site, false);
        });
}

Result run(FaultBank &bank, const Case &fixture, bool native) {
    Result result;
    const SourcePhysicsContactRecordObserver stores = [&](const auto &store) { result.stores.push_back(store); };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t site) {
        if (native && site >= owner::first && site <= owner::last)
            throw std::runtime_error("Emitter orientation delivered a translated source callback");
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
        if (native) result.admitted = native_entry(state);
        else original::native_emitter_orientation_reference_250780_live(state);
    } catch (const std::exception &error) {
        result.fault = error.what();
    }
    call_log = nullptr;
    result.abi = state.save();
    result.mxcsr = _mm_getcsr();
    asm volatile("fnclex\n\tfldcw %0" :: "m"(control));
    _mm_setcsr(initial);
    return result;
}

unsigned compare(const Case &test, unsigned case_no, unsigned at, Totals &totals) {
    FaultBank a{test.memory, at}, b{test.memory, at};
    const auto mine = run(a, test, true), theirs = run(b, test, false);
    ++totals.compared;
    totals.stores += theirs.stores.size();
    totals.children += theirs.calls.size();
    if (!theirs.fault.empty()) ++totals.faults;
    if ((!mine.admitted && mine.fault.empty()) || mine.fault != theirs.fault || mine.stores != theirs.stores ||
        a.accesses != b.accesses || !a.same(b) || mine.calls != theirs.calls ||
        (!census_mode && mine.cursor != theirs.cursor) || !same_abi(mine.abi, theirs.abi) ||
        mine.abi.fs_base != theirs.abi.fs_base || mine.mxcsr != theirs.mxcsr) {
        std::fprintf(stderr, "Case %u access %u native '%s' source '%s', PC %x/%x stores %zu/%zu calls %zu/%zu x87 %x/%x\n",
            case_no, at, mine.fault.c_str(), theirs.fault.c_str(), mine.cursor, theirs.cursor,
            mine.stores.size(), theirs.stores.size(), mine.calls.size(), theirs.calls.size(), mine.abi.x87_status, theirs.abi.x87_status);
        for (unsigned i = 0; i < 8; ++i)
            if (mine.abi.gp[i] != theirs.abi.gp[i])
                std::fprintf(stderr, "GP%u %x/%x\n", i, mine.abi.gp[i], theirs.abi.gp[i]);
        throw std::runtime_error("Particle emitter orientation differential mismatch");
    }
    return unsigned(a.accesses.size());
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x2508a5u);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = fixture(random, index);
        const auto accesses = compare(test, index, 0u, totals);
        if (index < 36u) {
            for (unsigned at = 1; at <= accesses; ++at) compare(test, index, at, totals);
        }
    }
    if (!census_mode) check_refusals(fixture(random, count), native_entry);
    report(count, totals, owner::summary().coverage.load(), owner::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
