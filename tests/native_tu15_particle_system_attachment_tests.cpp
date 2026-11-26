#include "native_tu15_particle_birth_observer_fixture.hpp"
#include "rendering/effects/native_tu15_particle_system_attachment.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_particle_system_attachment_detail;
namespace system_update = halo2::native_particle_system_update_detail;
namespace world = halo2::native_world_leaves_detail;

namespace {
using enum world::Register;
constexpr std::uint32_t frame = 0x20007400u, parent = 0x13002000u, child = 0x13003000u;
constexpr std::uint32_t particle = 0x13004000u, definition = 0x13005000u, tags = 0x13006000u;
constexpr std::uint32_t model_table = 0x13007000u, sprite_table = 0x13008000u, child_interface = 0x13009000u;
constexpr std::uint32_t transform = 0x1300a000u, location_table = 0x1300b000u, locations = 0x1300c000u;
constexpr std::uint32_t method_base = 0x380000u, link = 0x73ab0003u;
bool native_path{};

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
        if (accesses.size() == fault) throw std::runtime_error("Injected particle attachment bank fault");
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
    random_abi(random, test, frame);
    test.abi.gp[ebp] = parent;
    test.abi.gp[esi] = child;
    test.abi.gp[ebx] = particle;
    test.abi.gp[edi] = link;
    test.policy = index;
    auto &memory = test.memory;
    auto scalar = [&]() -> std::uint32_t {
        constexpr std::array edge_words{0x7fc01224u, 0xffc02412u, 0x7f801357u, 0x7f800000u, 0x80000000u, 0x00123456u};
        return index % 5u == 1u ? edge_words[random() % edge_words.size()] :
            u(float(int(random() % 2001u) - 1000) * 0.00390625f);
    };
    for (auto address = frame - 0xa0u; address < frame + 0xa0u; address += 4u)
        memory.put(address, scalar());
    for (const auto base : {parent, child, particle, definition, transform, locations})
        for (unsigned offset = 0u; offset < 0x100u; offset += 4u) memory.put(base + offset, scalar());
    memory.put(frame + owner::attachment_slot, link);
    memory.put(frame + owner::next_slot, index & 1u ? link + 1u : system_update::none);
    memory.put(parent + owner::parent_field, transform);
    memory.put(parent + system_update::first_attachment, link);
    memory.put(child + system_update::flags_offset, (index & 4u) ? 0x20u : 0u, 2);
    memory.put(child + 0x20u, 0x21u, 2);
    memory.put(system_update::active_effects + 4u, index % 2u ? 0x2u : 0u);
    memory.put(child + system_update::first_attachment, system_update::none);
    memory.put(child + system_update::first_location, index & 0x10u ? 0x53670001u : system_update::none);
    memory.put(system_update::location_array, location_table);
    memory.put(location_table + 0x44u, locations);
    memory.put(locations + sizeof(owner::location::LocationDatum) + system_update::next_location, system_update::none);
    memory.put(owner::location::globals::one, 0x3f800000u);
    memory.put(definition + offsetof(owner::SystemDefinition, particle_tag), 0x2728ffffu);
    memory.put(owner::location::globals::tag_instances, tags + sizeof(owner::TagInstance));
    constexpr std::array groups{owner::location::model_group, owner::location::sprite_group, 0x11223344u};
    memory.put(tags, groups[(index >> 1u) % groups.size()]);
    memory.put(owner::location::globals::model_interface, model_table);
    memory.put(owner::location::globals::sprite_interface, sprite_table);
    for (const auto offset : {0x44u, 0x48u, 0x50u}) {
        memory.put(model_table + offset, method_base + offset);
        memory.put(sprite_table + offset, method_base + 0x100u + offset);
    }
    memory.put(child_interface, model_table);
    memory.put(model_table + 0x24u, method_base + 0x24u);
    return test;
}

void begin_child(WorldState &state, std::uint32_t target, std::uint32_t site = 0u) {
    log_call(mix(entry_hash(state, target), site));
    state.instruction_literal(target);
}

void return_child(WorldState &state, unsigned args, unsigned phase) {
    if (policy >= 64u && (policy >> 4u) % 19u == phase) {
        state.gp[edx] = state.flags.logic(state.gp[eax] ^ 0x3dc9u, 4);
        throw std::runtime_error("Particle attachment child stopped after changing state");
    }
    state.pop();
    state.gp[esp] += args;
}

void run_178070(WorldState &state) {
    begin_child(state, owner::resolve_definition);
    state.gp[edx] = r(state.memory, state.gp[ecx] + 0x20u, 2);
    state.gp[eax] = definition;
    return_child(state, 0u, 4u);
}

void run_178920(WorldState &state) {
    begin_child(state, owner::update_attributes);
    const auto word = r(state.memory, state.gp[esp] + 4u, 4);
    w(state.memory, state.observe, owner::update_attributes, state.gp[ecx] + 4u,
        4, state.gp[eax] ^ word ^ state.gp[edi]);
    return_child(state, 4u, 5u);
}

void run_178110(WorldState &state) {
    begin_child(state, owner::update_transform);
    const auto orientation = r(state.memory, state.gp[esp] + 4u, 4);
    const auto component = r(state.memory, orientation + 0xcu, 4);
    w(state.memory, state.observe, owner::update_transform, state.gp[ebx] + 0x10u, 4, component);
    return_child(state, 12u, 6u);
}

void run_177fa0(WorldState &state) {
    begin_child(state, owner::unlink_system);
    const auto address = r(state.memory, state.gp[esp] + 4u, 4);
    w(state.memory, state.observe, owner::unlink_system, address, 4, state.gp[ecx]);
    w(state.memory, state.observe, owner::unlink_system, state.gp[ebx], 4, system_update::none);
    return_child(state, 4u, 7u);
}

void run_177020(WorldState &state) {
    begin_child(state, owner::destroy_system);
    const auto handle = r(state.memory, state.gp[esp] + 4u, 4);
    w(state.memory, state.observe, owner::destroy_system, transform + 0x10u, 4, handle);
    return_child(state, 4u, 8u);
}

void run_178290(WorldState &state) {
    begin_child(state, system_update::remove_inactive);
    const auto flags = r(state.memory, state.gp[ebx] + system_update::flags_offset, 1);
    w(state.memory, state.observe, system_update::remove_inactive, state.gp[ebx] + system_update::flags_offset, 1, flags);
    return_child(state, 0u, 9u);
}

void run_13a6b0(WorldState &state) {
    begin_child(state, system_update::resolve_interface);
    state.gp[eax] = child_interface;
    return_child(state, 0u, 10u);
}

void run_251a60(WorldState &state) {
    begin_child(state, system_update::visit_location);
    state.gp[ecx] = r(state.memory, state.gp[esp] + 8u, 4);
    w(state.memory, state.observe, system_update::visit_location,
        state.gp[ebx] + offsetof(owner::location::LocationDatum, position), 4, state.gp[ecx]);
    return_child(state, 8u, 11u);
}

void run_178240(WorldState &state) {
    begin_child(state, system_update::drain);
    const auto address = state.gp[eax] + system_update::flags_offset;
    auto flags = r(state.memory, address, 1);
    bool alive = policy & 8u;
    flags &= ~9u;
    if (alive) flags |= 8u;
    state.gp[eax] = alive ? 1u : 0u;
    w(state.memory, state.observe, system_update::drain, address, 1, flags);
    return_child(state, 0u, 12u);
}

void dispatch(WorldState &state, std::uint32_t target, std::uint32_t site, bool tail) {
    if (tail) throw std::runtime_error("Unexpected particle system attachment tail route");
    begin_child(state, target, site);
    const auto interface = state.gp[ecx];
    if (site == 0x17796cu && target == method_base + 0x24u && interface == child_interface) {
        state.gp[eax] = transform;
        return_child(state, 0u, 13u);
        return;
    }
    const bool sprite = interface == owner::location::globals::sprite_interface;
    const auto base = method_base + (sprite ? 0x100u : 0u);
    unsigned args = 0u;
    if (site == 0x177d65u && target == base + 0x50u) {
        state.gp[eax] = r(state.memory, state.gp[esp] + 4u, 4);
        args = 4u;
        if (policy % 13u == 2u)
            state.gp[ebx] = sprite ? owner::location::globals::model_interface : owner::location::globals::sprite_interface;
    } else if (site == 0x177d6cu && target == base + 0x48u) {
        state.gp[eax] = 0x14d33780u | (policy & 7u);
    } else if (site == 0x177d77u && target == base + 0x44u) {
        state.gp[eax] = 0x1f7fcb00u ^ policy;
        if (policy % 17u == 2u) state.fp.push(0.8125L);
    } else throw std::runtime_error("Unexpected particle system attachment method");
    return_child(state, args, 14u);
}

void run_1778d0(WorldState &state);
} // namespace

namespace original {
using ::dispatch;
using ::run_13a6b0;
using ::run_177020;
using ::run_1778d0;
using ::run_177fa0;
using ::run_178070;
using ::run_178110;
using ::run_178240;
using ::run_178290;
using ::run_178920;
using ::run_251a60;
#include "system_attachment.inc"
} // namespace original

namespace {
void run_1778d0(WorldState &state) {
    // This recursive child uses the same previously proved system entry and
    // drain as Effect Frame. Its authored record has no nested attachment.
    log_call(entry_hash(state, owner::update_system));
    const auto next = native_path ? system_update::bridge_entry(state, original::native_particle_system_entry_reference_1778d0,
        [](WorldState &value, auto target) {
            if constexpr (decltype(target)::value == system_update::remove_inactive) run_178290(value);
            if constexpr (decltype(target)::value == system_update::resolve_system) run_178070(value);
            if constexpr (decltype(target)::value == system_update::resolve_interface) run_13a6b0(value);
            if constexpr (decltype(target)::value == system_update::visit_location) run_251a60(value);
        }, [](WorldState &value, auto target, auto site) { dispatch(value, target, site, false); }) :
        original::native_particle_system_entry_reference_1778d0_live(state);
    if (next != system_update::Resume::drain) throw std::runtime_error("Nested system escaped the authored drain route");
    if (native_path) {
        if (!system_update::bridge_drain(state, original::native_particle_system_drain_reference_1778d0,
            [](WorldState &value, auto) { run_178240(value); }))
            throw std::runtime_error("Native nested drain was refused");
    } else original::native_particle_system_drain_reference_1778d0_live(state);
}

bool native_prepare(WorldState &state) {
    return owner::bridge_prepare(state, original::native_particle_system_attachment_prepare_reference_1778d0,
        [](WorldState &value, auto target) {
            if constexpr (decltype(target)::value == owner::resolve_definition) run_178070(value);
            if constexpr (decltype(target)::value == owner::update_attributes) run_178920(value);
            if constexpr (decltype(target)::value == owner::update_transform) run_178110(value);
        }, [](WorldState &value, auto target, auto site) { dispatch(value, target, site, false); });
}

owner::Completion native_recurse(WorldState &state) {
    return owner::bridge_recurse(state, original::native_particle_system_attachment_recurse_reference_1778d0,
        [](WorldState &value, auto target) {
            if constexpr (decltype(target)::value == owner::update_system) run_1778d0(value);
            if constexpr (decltype(target)::value == owner::unlink_system) run_177fa0(value);
            if constexpr (decltype(target)::value == owner::destroy_system) run_177020(value);
        });
}

bool use_collector{};
struct Execution {
    Result result;
    owner::Completion next{owner::Completion::retained};
};

Execution run(FaultBank &bank, const Case &fixture, bool native) {
    Execution output;
    auto &result = output.result;
    const SourcePhysicsContactRecordObserver stores = [&](const auto &store) { result.stores.push_back(store); };
    BirthObserverFixture observed;
    const auto &observer = use_collector ? observed.collector.observer() : stores;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t site) {
        if (native && ((site >= owner::first && site <= owner::last) ||
                       (site >= system_update::first && site <= system_update::entry_last) ||
                       (site >= system_update::epilogue && site <= system_update::last)))
            throw std::runtime_error("Native particle attachment delivered a translated site");
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
    state.load(fixture.abi);
    policy = fixture.policy;
    native_path = native;
    call_log = &result.calls;
    try {
        if (policy % 5u != 3u) {
            if (native) {
                if (!native_prepare(state)) throw std::runtime_error("Native attachment preparation was refused");
            } else original::native_particle_system_attachment_prepare_reference_1778d0_live(state);
        }
        output.next = native ? native_recurse(state) : original::native_particle_system_attachment_recurse_reference_1778d0_live(state);
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

unsigned compare(const Case &test, unsigned at, Totals &totals) {
    FaultBank left{test.memory, at}, right{test.memory, at};
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
            test.policy, at, mine.fault.c_str(), theirs.fault.c_str(), mine.cursor, theirs.cursor,
            unsigned(native.next), unsigned(translated.next));
        for (unsigned i = 0u; i < 8u; ++i)
            if (mine.abi.gp[i] != theirs.abi.gp[i]) std::fprintf(stderr, "GP%u %x/%x\n", i, mine.abi.gp[i], theirs.abi.gp[i]);
        throw std::runtime_error("Particle system attachment differential mismatch");
    }
    if (!at && !theirs.fault.empty() && theirs.fault != "Access escaped the authored fixture at 0" &&
        theirs.fault != "Particle attachment child stopped after changing state") {
        std::fprintf(stderr, "Undeclared particle attachment fault in case %u: %s\n", test.policy, theirs.fault.c_str());
        throw std::runtime_error("Particle attachment fixture escaped its cases");
    }
    if (theirs.fault.empty() && theirs.abi.gp[esp] != frame)
        throw std::runtime_error("Particle attachment handoff changed the frame depth");
    return unsigned(right.accesses.size());
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x177cd9u);
    Totals totals;
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = fixture(random, index);
        const auto accesses = compare(test, 0u, totals);
        if (index < 50u) {
            for (unsigned at = 1u; at <= accesses; ++at) compare(test, at, totals);
        }
    }
    if (!census_mode) {
        check_refusals(fixture(random, count), native_prepare);
        check_birth_observer_refusals(fixture(random, count), native_prepare);
    }
    report(count, totals, owner::summary().coverage.load(), owner::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
