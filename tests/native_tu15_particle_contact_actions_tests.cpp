#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_contact_actions.hpp"
#include "rendering/effects/native_tu15_particle_property.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace kernel = halo2::native_particle_contact_actions_detail;

namespace children {
void notification(WorldState &s, std::uint32_t target, std::uint32_t resume, unsigned arguments) {
    log_call(entry_hash(s, target));
    for (unsigned offset = 4u; offset <= arguments; offset += 4u)
        log_call(r(s.memory, s.gp[4] + offset, 4));
    s.gp[0] ^= 0x100179u;
    s.gp[1] = s.flags.logic(0x7f14u, 4);
    s.xmm[3] = _mm_set_ss(-1.0f);
    if (s.pop() != resume) throw std::logic_error("Wrong particle notification return");
    s.gp[4] += arguments;
}

void run_250310(WorldState &s) { notification(s, 0x250310u, 0x2c32aeu, 4u); }
void run_250380(WorldState &s) { notification(s, 0x250380u, 0x2c32c4u, 8u); }
void run_2502b0(WorldState &s) { notification(s, 0x2502b0u, 0x2c32d9u, 4u); }
} // namespace children

namespace methods {
constexpr std::uint32_t describe = 0x28891000u, index = 0x28892000u, test_primary = 0x28893000u, test_secondary = 0x28894000u;

void dispatch(WorldState &s, std::uint32_t target, std::uint32_t at, bool tail = false) {
    const auto selection = (policy / 8u) % 8u;
    constexpr std::array groups{0u, 0u, kernel::sound_group, kernel::footstep_group, kernel::effect_group,
                               0x74736c61u, kernel::sound_group, kernel::footstep_group};
    if (tail) throw std::logic_error("Unexpected particle contact tail dispatch");
    log_call(entry_hash(s, target));
    std::uint32_t resume{};
    if (target == index && (at == 0x2c326bu || at == 0x2c3281u)) {
        resume = at == 0x2c326bu ? 0x2c326eu : 0x2c3284u;
        s.gp[0] = selection == (at == 0x2c326bu ? 0u : 1u) ? 0xffffffffu : 0x37960002u;
    } else if (target == describe && at == 0x2c328au) {
        resume = 0x2c328cu;
        s.gp[0] = groups[selection];
    } else if (target == test_primary && (at == 0x2c3313u || at == 0x2c3328u)) {
        resume = at == 0x2c3313u ? 0x2c3316u : 0x2c332bu;
        s.gp[0] = (selection & 1u) | 0x99f84100u;
    } else if (target == test_secondary && at == 0x2c333du) {
        resume = 0x2c3340u;
        s.gp[0] = (selection & 1u) | 0x4481f300u;
    } else {
        throw std::logic_error("Unexpected particle surface method");
    }
    s.gp[1] = s.flags.logic(0x4ffbccfeu, 4);
    if (target == test_primary || target == test_secondary) s.gp[7] |= 0xf010u;
    if (s.pop() != resume) throw std::logic_error("Wrong particle method return");
    log_call(entry_hash(s, target));
}
} // namespace methods

#include "particle_contact_actions_reference.inc"

namespace {
constexpr std::uint32_t frame = 0x28002300u, particle = 0x28004300u, system_record = 0x28006300u;
constexpr std::uint32_t system_tag = 0x28007300u, definition = 0x28008300u, tag_table = 0x28009300u;
constexpr std::uint32_t interface_record = 0x2800a300u, vtable = 0x2800b300u;
constexpr std::array normals{0u, 0x80000000u, 0x7fc00017u, 0x7f800019u, 0x3f800000u, 0x3f200000u};

Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, frame);
    test.policy = index;
    test.abi.fs_base = random();
    test.abi.gp[5] = particle;
    test.abi.eflags = (test.abi.eflags & ~0x40u) | (index % 8u < 4u ? 0x40u : 0u);
    for (std::uint32_t address = frame - 0x60u; address <= frame + 0x140u; address += 4u)
        test.memory.put(address, random());
    test.memory.put(particle + 2u, random(), 2);
    test.memory.put(frame + 0x13cu, system_record);
    test.memory.put(frame + kernel::interface_slot, interface_record);
    test.memory.put(frame + kernel::contact_kind, (index % 4u == 0u ? 4u : index % 4u));
    test.memory.put(frame + 0xe0u, normals[index % normals.size()]);
    test.memory.put(kernel::landing_normal, 0x3f200000u);
    test.memory.put(interface_record, vtable);
    test.memory.put(vtable, methods::describe);
    test.memory.put(vtable + 4u, methods::index);
    test.memory.put(vtable + 0x38u, methods::test_primary);
    test.memory.put(vtable + 0x3cu, methods::test_secondary);
    test.memory.put(system_record + 0xeu, 0u, 2);
    test.memory.put(system_record + 0x10u, 1u);
    test.memory.put(0x4f0414u, tag_table);
    test.memory.put(tag_table + 16u, 0x62736474u);
    test.memory.put(tag_table + 24u, system_tag);
    test.memory.put(system_tag + 0x18u, definition);
    test.memory.put(definition + 4u, 2u);
    test.memory.put(definition + 0x12u, index % 15u == 0u ? 1u : 0u, 2);
    return test;
}

bool bridge(WorldState &state) {
    return kernel::bridge_actions(state, prepared::native_particle_contact_actions_reference_2c2d90,
                                    children::action_local, [](WorldState &value, std::uint32_t target, std::uint32_t at) {
                                        methods::dispatch(value, target, at);
                                    });
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    fixture::use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x2c3248u);
    Totals totals;
    const auto source = original::native_particle_contact_actions_reference_2c2d90_live;
    const auto watched = [](std::uint32_t site) { return site >= kernel::first && site <= kernel::last; };
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random, index);
        const auto accesses = fixture::compare_geometry(test, 0u, totals, bridge, source, watched);
        if (index < 96u)
            for (unsigned at = 1u; at <= accesses; ++at)
                fixture::compare_geometry(test, at, totals, bridge, source, watched);
    }
    if (!census_mode) {
        const auto control = make_case(random, count);
        check_refusals(control, bridge);
        check_birth_observer_refusals(control, bridge);
    }
    report(count, totals, 0u, 0u, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
