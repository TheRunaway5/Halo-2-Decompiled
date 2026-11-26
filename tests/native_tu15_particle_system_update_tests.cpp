#include "native_tu15_particle_birth_observer_fixture.hpp"
#include "rendering/effects/native_tu15_particle_system_update.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_particle_system_update_detail;
namespace world = halo2::native_world_leaves_detail;

namespace {
using enum world::Register;
constexpr std::uint32_t stack = 0x20005800u, frame = stack - 0x70u;
constexpr std::uint32_t system = 0x12002000u, definition = 0x12003000u;
constexpr std::uint32_t array = 0x12004000u, locations = 0x12005000u;
constexpr std::uint32_t object = 0x12006000u, vtable = 0x12007000u, contact_defs = 0x12008000u;
constexpr std::uint32_t method = 0x2ab340u, location_size = sizeof(owner::location::LocationDatum);

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
        if (accesses.size() == fault) throw std::runtime_error("Injected particle system bank fault");
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

struct Fixture {
    Case input;
    bool drain_only;
};
Fixture fixture(std::mt19937 &random, unsigned index) {
    Fixture test{{}, index % 5u == 4u};
    auto &input = test.input;
    random_abi(random, input, test.drain_only ? frame : stack);
    input.abi.gp[ecx] = input.abi.gp[ebp] = system;
    input.policy = index;
    auto &memory = input.memory;
    for (auto address = frame - 0x20u; address < stack + 0x30u; address += 4u) memory.put(address, random());
    for (const auto base : {system, definition, locations})
        for (unsigned offset = 0; offset < 0x100u; offset += 4u) memory.put(base + offset, random());

    memory.put(stack, 0x1778a6u);
    memory.put(stack + 4u, index % 2u ? 0x7f800017u : 0x3c888889u);
    memory.put(system + owner::flags_offset, 1u | ((index & 4u) ? 0x20u : 0u) |
        ((index & 8u) ? 0x100u : 0u) | ((index & 16u) ? 0x200u : 0u), 2);
    const auto effect_index = index & 2u ? -32 : 98;
    const auto bit_index = unsigned(effect_index) & 31u;
    memory.put(system + 0x20u, std::uint16_t(effect_index), 2);
    memory.put(owner::active_effects + std::uint32_t(effect_index >> 5) * 4u, index & 16u ? 1u << bit_index : 0u);
    memory.put(owner::location_array, array);
    memory.put(array + 0x44u, locations);
    const auto location_count = index % 4u;
    memory.put(system + owner::first_location, location_count ? 0x81110001u : owner::none);
    for (unsigned n = 1u; n <= 3u; ++n)
        memory.put(locations + n * location_size + owner::next_location,
                   n >= location_count ? owner::none : 0x81110001u + n);
    memory.put(system + owner::first_attachment, index % 7u < 3u ? 0x427e0003u : owner::none);
    memory.put(definition + offsetof(owner::location::SystemDefinition, particle_tag), index & 2u ? 0x8223ffffu : 0u);
    memory.put(object, vtable);
    memory.put(vtable + 0x24u, method);
    return test;
}

void begin_child(WorldState &state, std::uint32_t target, std::uint32_t site = 0u) {
    log_call(mix(entry_hash(state, target), site));
    state.instruction_literal(target);
}

void return_child(WorldState &state, unsigned args, unsigned phase) {
    if (policy >= 35u && (policy >> 4u) % 17u == phase) {
        state.gp[edx] = state.flags.logic(state.gp[eax] ^ 0x5cc8u, 4);
        throw std::runtime_error("Particle system child stopped after changing state");
    }
    state.pop();
    state.gp[esp] += args;
}

void run_178290(WorldState &state) {
    begin_child(state, owner::remove_inactive);
    state.gp[eax] = r(state.memory, state.gp[ebx] + owner::flags_offset, 1);
    w(state.memory, state.observe, owner::remove_inactive,
      state.gp[ebx] + owner::flags_offset, 1, state.flags.logic(state.gp[eax] & ~0x20u, 1));
    return_child(state, 0u, 3u);
}

void run_178070(WorldState &state) {
    begin_child(state, owner::resolve_system);
    state.gp[edx] = r(state.memory, state.gp[ecx] + 0x20u, 2);
    state.gp[eax] = definition;
    return_child(state, 0u, 4u);
}

void run_251a60(WorldState &state) {
    begin_child(state, owner::visit_location);
    auto &gp = state.gp;
    gp[edx] = r(state.memory, gp[esp] + 8u, 4);
    gp[ecx] = r(state.memory, gp[esp] + 4u, 4);
    const auto position = gp[ebx] + offsetof(owner::location::LocationDatum, position);
    w(state.memory, state.observe, owner::visit_location, position, 4, gp[edx] ^ gp[ecx]);
    if (policy % 7u == 0u) state.fp.push(1.125L);
    return_child(state, 8u, 5u);
}

void run_13a6b0(WorldState &state) {
    begin_child(state, owner::resolve_interface);
    state.gp[eax] = object;
    state.flags.logic(state.gp[edx], 4);
    return_child(state, 0u, 6u);
}

void dispatch(WorldState &state, std::uint32_t target, std::uint32_t site, bool tail) {
    if (target != method || site != 0x17796cu || tail)
        throw std::runtime_error("Particle system interface method route changed");
    begin_child(state, target, site);
    state.gp[edx] = r(state.memory, state.gp[ecx], 4);
    state.gp[eax] = contact_defs;
    return_child(state, 0u, 7u);
}

void run_178240(WorldState &state) {
    begin_child(state, owner::drain);
    const auto address = state.gp[eax] + owner::flags_offset;
    auto low = r(state.memory, address, 1);
    constexpr std::array returns{0u, 2u, 1u, 0xffffffffu, 0u, 0u};
    const auto kind = (policy / 3u) % returns.size();
    low &= ~(1u | 8u);
    if (kind == 1u) low |= 8u;
    if (kind == 2u) low |= 1u;
    state.gp[eax] = returns[kind];
    w(state.memory, state.observe, owner::drain, address, 1, low);
    return_child(state, 0u, 8u);
}
} // namespace

namespace original {
using ::dispatch;
using ::run_13a6b0;
using ::run_178070;
using ::run_178240;
using ::run_178290;
using ::run_251a60;
#include "system_update.inc"
} // namespace original

namespace {
owner::Resume native_setup(WorldState &state) {
    return owner::bridge_entry(state, original::native_particle_system_entry_reference_1778d0,
        [](WorldState &value, auto child) {
            constexpr auto target = decltype(child)::value;
            if constexpr (target == owner::remove_inactive) run_178290(value);
            if constexpr (target == owner::resolve_system) run_178070(value);
            if constexpr (target == owner::visit_location) run_251a60(value);
            if constexpr (target == owner::resolve_interface) run_13a6b0(value);
        }, [](WorldState &value, auto target, auto site) { dispatch(value, target, site, false); });
}

bool native_drain(WorldState &state) {
    return owner::bridge_drain(state, original::native_particle_system_drain_reference_1778d0,
        [](WorldState &value, auto) { run_178240(value); });
}

bool use_collector{};
struct Execution {
    Result result;
    owner::Resume next{owner::Resume::retained};
};

Execution run(FaultBank &bank, const Fixture &fixture, bool native) {
    Execution output;
    auto &result = output.result;
    const SourcePhysicsContactRecordObserver stores = [&](const auto &store) { result.stores.push_back(store); };
    BirthObserverFixture observed;
    const auto &observer = use_collector ? observed.collector.observer() : stores;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t site) {
        if (native && ((site >= owner::first && site <= owner::entry_last) ||
                       (site >= owner::epilogue && site <= owner::last)))
            throw std::runtime_error("System update delivered a translated boundary callback");
        result.cursor = site;
    };
    if (native && !census_mode) {
        auto &cursor = services.instruction_cursor;
        cursor.position = &result.cursor;
        cursor.service_sites = halo2::native_census_detail::service_sites;
        cursor.callback_type = &services.instruction.target_type();
        cursor.executor = std::this_thread::get_id();
        cursor.literal_service_sites = true;
        cursor.observation_only_stores = !use_collector;
    }
    const auto csr = _mm_getcsr();
    std::uint16_t control;
    asm volatile("fnstcw %0" : "=m"(control));
    WorldState state{bank, observer, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(fixture.input.abi);
    policy = fixture.input.policy;
    call_log = &result.calls;
    try {
        output.next = fixture.drain_only ? owner::Resume::drain :
            native ? native_setup(state) : original::native_particle_system_entry_reference_1778d0_live(state);
        if (output.next == owner::Resume::drain) {
            if (native) {
                if (!native_drain(state)) throw std::runtime_error("Native particle system drain was not admitted");
            } else original::native_particle_system_drain_reference_1778d0_live(state);
        }
    } catch (const std::exception &error) {
        result.fault = error.what();
    }
    call_log = nullptr;
    result.abi = state.save();
    result.mxcsr = _mm_getcsr();
    asm volatile("fnclex\n\tfldcw %0" :: "m"(control));
    _mm_setcsr(csr);
    return output;
}

unsigned compare(const Fixture &test, unsigned at, Totals &totals) {
    FaultBank left{test.input.memory, at}, right{test.input.memory, at};
    const auto native = run(left, test, true), translated = run(right, test, false);
    const auto &mine = native.result;
    const auto &theirs = translated.result;
    ++totals.compared;
    totals.children += theirs.calls.size();
    totals.stores += theirs.stores.size();
    if (!theirs.fault.empty()) ++totals.faults;
    if (native.next != translated.next || mine.fault != theirs.fault || mine.stores != theirs.stores ||
        left.accesses != right.accesses || !left.same(right) || mine.calls != theirs.calls ||
        (!census_mode && mine.cursor != theirs.cursor) || !same_abi(mine.abi, theirs.abi) ||
        mine.abi.fs_base != theirs.abi.fs_base || mine.mxcsr != theirs.mxcsr) {
        std::fprintf(stderr, "Case %u access %u, native '%s' translated '%s', PC %x/%x, routes %u/%u\n",
            test.input.policy, at, mine.fault.c_str(), theirs.fault.c_str(), mine.cursor, theirs.cursor,
            unsigned(native.next), unsigned(translated.next));
        for (unsigned i = 0u; i < 8u; ++i)
            if (mine.abi.gp[i] != theirs.abi.gp[i]) std::fprintf(stderr, "GP%u %x/%x\n", i, mine.abi.gp[i], theirs.abi.gp[i]);
        throw std::runtime_error("Particle system update differential mismatch");
    }
    if (!at && !theirs.fault.empty() && theirs.fault != "Particle system child stopped after changing state")
        throw std::runtime_error("Particle system fixture has an undeclared fault");
    if (theirs.fault.empty() && translated.next == owner::Resume::drain && theirs.abi.gp[esp] != stack + 8u)
        throw std::runtime_error("Particle system drain did not consume its original argument");
    return unsigned(right.accesses.size());
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x1778d0u);
    Totals totals;
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = fixture(random, index);
        const auto accesses = compare(test, 0u, totals);
        if (index < 42u) {
            for (unsigned at = 1u; at <= accesses; ++at) compare(test, at, totals);
        }
    }
    if (!census_mode) {
        check_refusals(fixture(random, count).input, native_drain);
        check_birth_observer_refusals(fixture(random, count).input, native_drain);
    }
    report(count, totals, owner::summary().coverage.load(), owner::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
