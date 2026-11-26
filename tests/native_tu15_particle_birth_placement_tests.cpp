#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_birth_placement.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_particle_birth_placement_detail;
namespace world = halo2::native_world_leaves_detail;

namespace {
constexpr std::uint32_t stack = 0x20004200u;
constexpr std::uint32_t particle = 0x10002000u, emitter = 0x10003000u, context = 0x10004000u;
constexpr std::uint32_t definition = 0x10005000u, system = 0x10006000u, origin = 0x10007000u;
constexpr std::uint32_t objects = 0x10008000u, object_data = 0x10009000u;

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
        if (accesses.size() == fault) throw std::runtime_error("Injected particle placement bank fault");
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
    test.abi.gp[world::ebp] = particle;
    test.abi.gp[world::ebx] = emitter;
    test.abi.gp[world::esi] = context;
    test.abi.gp[world::edi] = system;
    test.policy = index;
    auto &memory = test.memory;
    auto scalar_word = [&]() -> std::uint32_t {
        constexpr std::array special{0x7f800321u, 0x7fc00987u, 0xffc08b24u, 0x80000000u,
                                     0x7f800000u, 0xff800000u, 1u, 0x807fffffu};
        if (index % 5u == 1u) return special[random() % special.size()];
        return std::bit_cast<std::uint32_t>((int(random() % 4001u) - 2000) / 25.0f);
    };
    for (unsigned offset = 0; offset < 0x140u; offset += 4u)
        memory.put(stack + offset - 0x30u, scalar_word());
    for (const auto base : {emitter, particle, context, definition, system, origin, objects, object_data})
        for (unsigned offset = 0; offset < 0x300u; offset += 4u)
            memory.put(base + offset, scalar_word());

    memory.put(stack + 0x1cu, definition);
    memory.put(stack + 0xc4u, system);
    memory.put(stack + owner::particle_definition, definition);
    memory.put(stack + owner::origin, (index & 2u) ? 0x12980001u : 0xffffffffu);
    memory.put(definition + 0xcu, (index & 1u) != 0u, 2);
    memory.put(definition + 0x16u, (index & 4u) ? 0x7fu : 0x81u, 1);
    memory.put(definition + 0x78u, (index & 8u) ? 8u : 0u);
    memory.put(0x474d04u, index % 17u == 0u ? particle + owner::velocity : origin);
    memory.put(0x4f720cu, objects);
    memory.put(objects + 0x44u, object_data);
    memory.put(system + 0x18u, 0xd37a0001u);
    memory.put(owner::one, 0x3f800000u);
    memory.put(owner::ray_plane_z, 0xbf800000u);
    memory.put(owner::ray_offset, 0x3ba3d70au);
    memory.put(emitter + offsetof(owner::EmitterDatum, particle_count), index % 3u ? 0xffffu : 0x7fffu, 2);
    return test;
}

void run_176a40(WorldState &state) {
    constexpr auto target = owner::prepare_inputs;
    log_call(entry_hash(state, target));
    state.instruction_literal(target);
    auto &gp = state.gp;
    const auto inputs = r(state.memory, gp[world::esp] + 0x14u, 4) + owner::inputs;
    const auto changed = state.flags.logic(r(state.memory, inputs, 4) | 0x00010000u, 4);
    w(state.memory, state.observe, target, inputs, 4, changed);
    gp[world::eax] = changed;
    gp[world::ecx] = gp[world::edx];
    if (policy % 23u == 19u) throw std::runtime_error("Input preparation failed after a child write");
    state.pop();
    gp[world::esp] += 20u;
}

void run_252480(WorldState &state) {
    constexpr auto target = owner::evaluate_properties;
    log_call(entry_hash(state, target));
    state.instruction_literal(target);
    auto &gp = state.gp;
    const auto definition = r(state.memory, gp[world::esp] + 4u, 4);
    gp[world::eax] = definition;
    gp[world::ecx] = r(state.memory, definition + 4u, 4);
    w(state.memory, state.observe, target, gp[world::edi] + offsetof(owner::ParticleDatum, inverse_lifetime),
        4, gp[world::ecx]);
    if (policy % 23u == 20u) throw std::runtime_error("Property evaluation failed after a child write");
    state.pop();
    gp[world::esp] += 8u;
}

void run_145b70(WorldState &state) {
    constexpr auto target = owner::transform_vector;
    log_call(entry_hash(state, target));
    state.instruction_literal(target);
    auto &gp = state.gp;
    const auto source = gp[world::ecx];
    const auto destination = gp[world::eax];
    gp[world::ecx] = r(state.memory, gp[world::edx], 4);
    const auto first = r(state.memory, source, 4);
    const auto second = r(state.memory, source + 4u, 4);
    const auto third = r(state.memory, source + 8u, 4);
    w(state.memory, state.observe, target, destination, 4, third);
    if (policy % 23u == 21u && destination == particle + owner::velocity)
        throw std::runtime_error("Velocity transform failed after a child write");
    w(state.memory, state.observe, target, destination + 4u, 4, first);
    w(state.memory, state.observe, target, destination + 8u, 4, second);
    if (policy % 19u == 0u) state.xmm[4] = state.xmm[1];
    state.pop();
}

void dispatch(WorldState &state, std::uint32_t target, std::uint32_t site, bool tail) {
    if (target != owner::collision_query || site != 0x25103bu || tail)
        throw std::runtime_error("Unexpected particle placement dispatcher route");
    log_call(mix(entry_hash(state, target), site));
    state.instruction_literal(target);
    auto &gp = state.gp;
    const auto normal = r(state.memory, gp[world::esp] + 24u, 4);
    const auto position = r(state.memory, gp[world::esp] + 8u, 4);
    for (unsigned offset : {0x20u, 0x24u, 0x28u, 0x40u, 0x44u, 0x48u}) {
        const auto word = r(state.memory, position + offset % 12u, 4);
        w(state.memory, state.observe, target, normal + offset, 4, word);
    }
    gp[world::eax] = (gp[world::eax] & 0xffffff00u) | ((policy & 0x10u) ? 0x81u : 0u);
    if (policy % 23u == 22u) throw std::runtime_error("Collision query failed after publishing its result");
    state.pop();
    gp[world::esp] += 24u;
}
} // namespace

namespace original {
using ::dispatch;
using ::run_145b70;
using ::run_176a40;
using ::run_252480;
#include "placement.inc"
} // namespace original

namespace {
bool native_entry(WorldState &state) {
    return owner::bridge(state, original::native_particle_birth_placement_reference_250c60,
        [](auto &value, auto child, std::uint32_t site) {
            constexpr auto target = decltype(child)::value;
            if constexpr (target == owner::prepare_inputs) run_176a40(value);
            if constexpr (target == owner::evaluate_properties) run_252480(value);
            if constexpr (target == owner::transform_vector) run_145b70(value);
            if constexpr (target == owner::collision_query) dispatch(value, target, site, false);
        });
}

Result run(FaultBank &bank, const Case &fixture, bool native) {
    Result result;
    const SourcePhysicsContactRecordObserver stores = [&](const auto &store) { result.stores.push_back(store); };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t site) {
        if (native && site >= owner::first && site <= owner::last)
            throw std::runtime_error("Particle birth placement delivered a translated source callback");
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
        else original::native_particle_birth_placement_reference_250c60_live(state);
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
        std::fprintf(stderr, "Case %u access %u native '%s' source '%s', PC %x/%x stores %zu/%zu MXCSR %x/%x\n",
            case_no, at, mine.fault.c_str(), theirs.fault.c_str(), mine.cursor, theirs.cursor,
            mine.stores.size(), theirs.stores.size(), mine.mxcsr, theirs.mxcsr);
        for (unsigned i = 0; i < 8; ++i) {
            if (mine.abi.gp[i] != theirs.abi.gp[i])
                std::fprintf(stderr, "GP%u %x/%x\n", i, mine.abi.gp[i], theirs.abi.gp[i]);
            if (mine.abi.xmm[i] != theirs.abi.xmm[i])
                std::fprintf(stderr, "XMM%u low %x/%x\n", i, mine.abi.xmm[i][0], theirs.abi.xmm[i][0]);
        }
        throw std::runtime_error("Particle birth placement differential mismatch");
    }
    return unsigned(a.accesses.size());
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x250e67u);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = fixture(random, index);
        const auto accesses = compare(test, index, 0u, totals);
        if (index < 40u) {
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
