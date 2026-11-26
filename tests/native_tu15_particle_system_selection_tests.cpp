#include "native_tu15_particle_birth_observer_fixture.hpp"
#include "rendering/effects/native_tu15_particle_system_selection.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_particle_system_selection_detail;
namespace system_update = halo2::native_particle_system_update_detail;
namespace world = halo2::native_world_leaves_detail;

namespace {
using enum world::Register;
constexpr std::uint32_t frame = 0x20008800u;
constexpr std::uint32_t systems = 0x14002000u, system_records = 0x14003000u;
constexpr std::uint32_t particles = 0x14004000u, particle_records = 0x14005000u;
constexpr std::uint32_t interface = 0x14006000u, vtable = 0x14007000u, contacts = 0x14008000u;
constexpr std::uint32_t first_vector = 0x14009000u, second_vector = 0x1400a000u, third_vector = 0x1400b000u;
constexpr std::uint32_t child_target = 0x380150u, alternate_target = 0x380170u;

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
        if (accesses.size() == fault) throw std::runtime_error("Injected particle selection bank fault");
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

bool repeat_attachment(unsigned index) { return (index / 10u) % 2u != 0u; }
unsigned scenario(unsigned index) { return index % 10u; }

Case fixture(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, frame);
    const auto attachment = repeat_attachment(index) ? 2u : 1u;
    test.abi.gp[edi] = 0x273c0001u;
    test.policy = index;
    auto &memory = test.memory;
    for (unsigned offset = 0; offset < 0xa0u; offset += 4u) memory.put(frame + offset, random());
    for (unsigned axis = 0; axis < 3u; ++axis)
        for (const auto vector : {first_vector, second_vector, third_vector})
            memory.put(vector + 4u * axis, random());
    memory.put(owner::system_array, systems);
    memory.put(systems + 0x44u, system_records);
    const auto record = system_records + attachment * sizeof(owner::location::SystemDatum);
    memory.put(frame + owner::attachment_slot, 0x172a0002u);
    memory.put(record + owner::attachment_next, index % 7u ? 0x172a0003u : system_update::none);
    const auto shape = scenario(index);
    const auto particle_index = index % 3u + 1u, stride = (index & 0x10u) ? 0x60u : 0x40u;
    const auto salt = (index & 0x20u) ? 0xe5a3u : 0x5115u;
    const auto handle = shape == 0u ? system_update::none : (salt << 16u) | particle_index;
    memory.put(record + owner::attached_particle, handle);
    memory.put(record + owner::contact_index, shape == 4u ? system_update::none : shape == 9u ? 0xfffffffeu : 2u);
    memory.put(owner::location::globals::particles, particles);
    memory.put(particles + 0x24u, stride);
    memory.put(particles + 0x38u, shape == 1u ? ((index & 0x20u) ? 0x80000000u : particle_index) : 4u);
    memory.put(particles + 0x44u, particle_records);
    memory.put(particle_records + particle_index * stride, shape == 2u ? 0u : shape == 3u ? salt ^ 0x200u : salt, 2u);
    memory.put(frame + owner::interface_slot, interface);
    memory.put(frame + owner::contact_table, contacts);
    memory.put(interface, vtable);
    memory.put(vtable + owner::count_method, index & 0x4u ? alternate_target : child_target);
    const auto contact = shape == 6u ? owner::fixed_contact_type_a :
        shape == 7u ? owner::fixed_contact_type_b : 0x70000c1u;
    memory.put(contacts - 8u, contact);
    memory.put(contacts + 4u, contact);
    memory.put(contacts + 8u, contact);
    memory.put(0x474d10u, first_vector);
    memory.put(0x474d1cu, first_vector);
    memory.put(0x474d08u, index % 11u ? second_vector : frame + 0x44u);
    memory.put(0x474d14u, index % 13u ? second_vector : frame + 0x44u);
    memory.put(0x474d0cu, index % 7u ? third_vector : frame + 0x58u);
    return test;
}

void dispatch(WorldState &state, std::uint32_t target, std::uint32_t site, bool tail) {
    if (site != 0x177a07u || (target != child_target && target != alternate_target) || tail)
        throw std::runtime_error("Particle selection interface route changed");
    log_call(mix(entry_hash(state, target), site));
    state.instruction_literal(target);
    auto &gp = state.gp;
    gp[edx] = r(state.memory, gp[ecx], 4);
    const auto shape = scenario(policy);
    if (shape == 6u && (policy / 20u) % 4u == 1u) gp[edi] = 1u;
    gp[eax] = shape == 5u ? 2u : 3u;
    state.flags.logic(gp[eax], 4);
    if (policy > 50u && policy % 23u == 20u)
        throw std::runtime_error("Particle selection method fault after partial state");
    state.pop();
}
} // namespace

namespace original {
using ::dispatch;
#include "system_selection.inc"
} // namespace original

namespace {
owner::Selection native_select(WorldState &state, bool repeat) {
    auto route = [](WorldState &value, auto target, auto site) { dispatch(value, target, site, false); };
    return repeat ? owner::bridge<true>(state, original::native_particle_system_selection_repeat_reference_1778d0, route) :
        owner::bridge<false>(state, original::native_particle_system_selection_first_reference_1778d0, route);
}

bool use_collector{};
struct Execution {
    Result result;
    owner::Selection next{owner::Selection::retained};
};

Execution run(FaultBank &bank, const Case &fixture, bool native) {
    Execution output;
    auto &result = output.result;
    const SourcePhysicsContactRecordObserver stores = [&](const auto &store) { result.stores.push_back(store); };
    BirthObserverFixture observed;
    const auto &observer = use_collector ? observed.collector.observer() : stores;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t site) {
        if (native && site >= owner::repeat && site <= owner::last_basis)
            throw std::runtime_error("Native particle selection delivered a translated site");
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
    const bool repeat = repeat_attachment(policy);
    call_log = &result.calls;
    try {
        if (native) output.next = native_select(state, repeat);
        else output.next = repeat ? original::native_particle_system_selection_repeat_reference_1778d0_live(state) :
            original::native_particle_system_selection_first_reference_1778d0_live(state);
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
        throw std::runtime_error("Particle system selection differential mismatch");
    }
    if (!at && !theirs.fault.empty() && theirs.fault != "Particle selection method fault after partial state")
        throw std::runtime_error("Particle selection fixture has an undeclared fault");
    return unsigned(right.accesses.size());
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x177985u);
    Totals totals;
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = fixture(random, index);
        const auto accesses = compare(test, 0u, totals);
        if (index < 48u) {
            for (unsigned at = 1u; at <= accesses; ++at) compare(test, at, totals);
        }
    }
    if (!census_mode) {
        const auto probe = [](WorldState &state) { return native_select(state, true) != owner::Selection::retained; };
        check_refusals(fixture(random, count), probe);
        check_birth_observer_refusals(fixture(random, count), probe);
    }
    report(count, totals, owner::summary().coverage.load(), owner::branch_count, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
