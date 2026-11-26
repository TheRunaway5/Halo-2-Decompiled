#include "native_tu15_particle_birth_observer_fixture.hpp"
#include "rendering/effects/native_tu15_particle_observer_lod.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_particle_observer_lod_detail;
namespace world = halo2::native_world_leaves_detail;

namespace original {
#include "scan.inc"
} // namespace original

namespace {
constexpr std::uint32_t definition = 0x10100000u, position = 0x10200000u, location = 0x10300000u;
constexpr std::uint32_t observers = 0x10400000u, observer_header = 0x10500000u, stack = 0x20000300u;
constexpr std::uint32_t data = 0x10600000u, policy_object = 0x10700000u;

struct Access {
    std::uint32_t address;
    std::size_t width;
    bool write;
    friend bool operator==(const Access &, const Access &) = default;
};

// Inject a failure on exactly one bank access, including store-preimage reads.
struct FaultBank : Memory {
    FaultBank(const Memory &base, unsigned at) : Memory(base.clone()), at(at) {}
    unsigned at;
    mutable std::vector<Access> accesses;

    void record(Access event) const {
        accesses.push_back(event);
        if (accesses.size() == at) throw std::runtime_error("Injected observer LOD bank fault");
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
    test.abi.gp[world::ebp] = definition;
    test.abi.gp[world::ecx] = position;
    test.abi.gp[world::edi] = location;
    test.abi.gp[world::ebx] = observers;
    test.abi.gp[world::edx] = index % 13u == 0u ? 0xfffffca8u : 0u;
    test.abi.gp[world::esi] = index % 17u == 0u ? 1u : 0u;
    test.abi.x87_depth %= 4u;
    test.abi.x87_status |= std::uint16_t(index & 0x3fu);
    test.abi.xmm[2] = {u(1.0f), 0u, 0u, 0u};
    test.abi.xmm[3] = {u(0.0f), 0u, 0u, 0u};

    constexpr float distances[]{2.0f, 8.0f, 12.5f, 21.0f, 26.5f, 30.0f, 35.0f, 11.25f};
    memory.put(position, u(distances[index % std::size(distances)]));
    memory.put(position + 4u, 0u);
    memory.put(position + 8u, 0u);
    memory.put(definition + 0x18u, u(10.0f));
    memory.put(definition + 0x1cu, u(5.0f));
    memory.put(definition + 0x20u, u(0.2f));
    memory.put(definition + 0x24u, u(30.0f));
    memory.put(definition + 0x28u, u(6.0f));
    memory.put(definition + 0x2cu, u(1.0f/6.0f));
    memory.put(data + 12u, index % 7u == 0u ? 0x4400u : 0x4000u, 2u);
    memory.put(location + 0x1cu, u(index % 10u == 0u ? 0.95f : 0.0f));
    memory.put(stack + 0x30u, data);
    memory.put(stack + 0x14u, 0u);
    memory.put(owner::observer_handle, index % 19u == 0u ? 0xffffu : 1u, 2u);
    const auto policy = (index / 7u) % 5u;
    memory.put(owner::lod_override, policy > 2u ? policy_object : 0u);
    memory.put(policy_object + 5u, policy == 4u ? 1u : 0u, 1u);
    memory.put(owner::local_observers, observer_header);
    const auto count = 1u + index % 4u;
    memory.put(observer_header + 8u, index % 31u == 0u ? 0xffffu : count, 2u);
    for (unsigned view = 0; view < count; ++view) {
        const auto observer = observers + owner::observer_stride * view;
        memory.put(observer, view == 0u ? 0u : u(float(view) * 0.75f));
        memory.put(observer + 4u, 0u);
        memory.put(observer + 8u, 0u);
        memory.put(observer + 0x50u, u(1.0f));
    }
    memory.put(0u, 0u);
    memory.put(4u, 0u);
    memory.put(8u, 0u);
    memory.put(0x50u, u(0.75f));

    if (index % 6u == 5u) {
        std::mt19937_64 floats(random());
        for (const auto address : {position, position + 4u, position + 8u, observers,
                                   observers + 4u, observers + 8u, observers + 0x50u,
                                   location + 0x1cu, definition + 0x1cu, definition + 0x28u}) {
            memory.put(address, special_float(floats));
        }
        test.abi.xmm[2][0] = special_float(floats);
        test.abi.xmm[3][0] = special_float(floats);
    }
    return test;
}

bool run_native(WorldState &state) {
    return owner::bridge(state, original::native_particle_observer_lod_reference_251c80);
}

Result run(FaultBank &bank, const Case &fixture, bool native, bool birth_observer) {
    Result result;
    const SourcePhysicsContactRecordObserver stores = [&](const auto &store) { result.stores.push_back(store); };
    SourcePhysicsWorldRuntimeServices services;
    if (native) {
        services.instruction = [](std::uint32_t) { throw std::runtime_error("Native scan delivered source callbacks"); };
        if (!census_mode) {
            auto &cursor = services.instruction_cursor;
            cursor.position = &result.cursor;
            cursor.service_sites = halo2::native_census_detail::service_sites;
            cursor.callback_type = &services.instruction.target_type();
            cursor.executor = std::this_thread::get_id();
            cursor.literal_service_sites = true;
            cursor.observation_only_stores = !birth_observer;
        }
    } else {
        services.instruction = [&](std::uint32_t site) { result.cursor = site; };
    }

    std::uint16_t control;
    const auto initial = _mm_getcsr();
    asm volatile("fnstcw %0" : "=m"(control));
    BirthObserverFixture birth;
    const auto &observe = birth_observer ? birth.collector.observer() : stores;
    WorldState state{bank, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(fixture.abi);
    try {
        if (native) result.admitted = run_native(state);
        else original::native_particle_observer_lod_reference_251c80(state);
    } catch (const std::exception &error) {
        result.fault = error.what();
    }
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
    if (!theirs.fault.empty()) ++totals.faults;
    if ((!mine.admitted && mine.fault.empty()) || mine.fault != theirs.fault || mine.stores != theirs.stores ||
        a.accesses != b.accesses || !a.same(b) || (!census_mode && mine.cursor != theirs.cursor) ||
        !same_abi(mine.abi, theirs.abi) || mine.mxcsr != theirs.mxcsr) {
        std::fprintf(stderr, "Case %u fault prefix %u native '%s' source '%s' PC %x/%x FP %x/%x MXCSR %x/%x stores %zu/%zu\n",
            case_no, at, mine.fault.c_str(), theirs.fault.c_str(), mine.cursor, theirs.cursor,
            mine.abi.x87_status, theirs.abi.x87_status, mine.mxcsr, theirs.mxcsr, mine.stores.size(), theirs.stores.size());
        std::fprintf(stderr, "SIMD0 %x/%x SIMD1 %x/%x SIMD4 %x/%x depth %u/%u flags %x/%x\n",
            mine.abi.xmm[0][0], theirs.abi.xmm[0][0], mine.abi.xmm[1][0], theirs.abi.xmm[1][0],
            mine.abi.xmm[4][0], theirs.abi.xmm[4][0], mine.abi.x87_depth, theirs.abi.x87_depth,
            mine.abi.eflags, theirs.abi.eflags);
        throw std::runtime_error("Particle observer LOD continuation differential mismatch");
    }
    return unsigned(a.accesses.size());
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x251d00u);
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
    if (!census_mode) check_birth_observer_refusals(fixture(random, count), run_native);
    report(count, totals, owner::coverage().load(), owner::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
