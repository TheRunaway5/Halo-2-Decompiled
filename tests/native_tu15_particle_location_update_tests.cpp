#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_location_update.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace kernel = halo2::native_particle_location_update_detail;

namespace children {
void checked_return(WorldState &s, std::uint32_t target, std::uint32_t returned, unsigned arguments) {
    log_call(entry_hash(s, target));
    for (unsigned offset = 4u; offset <= arguments; offset += 4u)
        log_call(r(s.memory, s.gp[4] + offset, 4));
    if (s.pop() != returned) throw std::logic_error("Wrong particle location child return");
    s.gp[4] += arguments;
}

void run_178920(WorldState &s) {
    checked_return(s, 0x178920u, 0x17bb9au, 4u);
    s.set(1, s.flags.logic(0x4399d11cu, 4), 4, 0);
    s.xmm[7] = _mm_set_ss(0.75f);
}

void run_17ba50(WorldState &s) {
    const auto output = s.gp[7];
    checked_return(s, 0x17ba50u, 0x17bcd2u, 4u);
    w(s.memory, s.observe, 0x17ba50u, output, 4, 0xf1185228u);
    s.set(0, output, 4, 0);
}

void run_178110(WorldState &s) {
    checked_return(s, 0x178110u, 0x17bcf6u, 12u);
    s.xmm[7] = _mm_set_ss(-1.0f);
    s.set(1, s.flags.logic(0xf3e03114u, 4), 4, 0);
}

constexpr std::uint32_t prepare_method = 0x28011300u, first_method = 0x28013300u, second_method = 0x28015300u;
void dispatch(WorldState &s, std::uint32_t target, std::uint32_t at, bool tail = false) {
    if (tail) throw std::logic_error("Unexpected tail in particle location fixture");
    if (target == prepare_method && at == 0x13a6edu) checked_return(s, target, 0x13a6f0u, 4u);
    else if (target == first_method && at == 0x17bb7eu) {
        checked_return(s, target, 0x17bb81u, 0u);
        s.set(0, 0xf30e43cdu, 4, 0);
    } else if (target == second_method && at == 0x17bb87u) {
        checked_return(s, target, 0x17bb8au, 0u);
        s.set(0, 0x12f0598au, 4, 0);
    } else {
        throw std::logic_error("Unexpected particle location method");
    }
}
} // namespace children

#include "particle_location_update_reference.inc"

namespace {
constexpr std::uint32_t frame = 0x28002300u, effect = 0x28003300u, system_record = 0x28005300u;
constexpr std::uint32_t request = 0x28007300u, tag_table = 0x28009300u, system_tag = 0x2800a300u;
constexpr std::uint32_t definition = 0x2800b300u, vtable = 0x2800d300u, table_header = 0x28017300u;
constexpr std::uint32_t location_slots = 0x28019300u, scene = 0x2801b300u, objects = 0x2801d300u, planes = 0x2801f300u;
constexpr auto none = 0xffffffffu;
constexpr std::array<std::uint32_t, 10> float_words{0u, 0x3f800000u, 0xbf800000u, 0x80000000u, 0x7fc00412u,
                                                  0x7f800912u, 0x3f200000u, 0xffc00462u, 1u, 0x3fe00000u};
constexpr std::array<std::uint16_t, 5> slots{2u, 0u, 17u, 1u, 31u};
std::uint32_t handle(unsigned loc) { return 0x6f930000u | slots[loc]; }

Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, frame);
    test.policy = index;
    test.abi.fs_base = random();
    for (std::uint32_t address = frame - 0x90u; address <= frame + 0x18u; address += 4u)
        test.memory.put(address, random());
    test.memory.put(frame + 4u, effect);
    test.memory.put(frame + 8u, system_record);
    test.memory.put(frame + 12u, request);
    test.memory.put(frame + 16u, float_words[index % float_words.size()]);
    test.memory.put(frame + 20u, 0x61000010u);
    test.memory.put(request + 8u, 0u);
    test.memory.put(request + 0xeu, index % 4u, 2);
    test.memory.put(request + 0x12u, (index / 4u) % 4u, 2);
    test.memory.put(effect + 0x1cu, 0x40040001u);
    test.memory.put(effect + 0x20u, index % 5u ? 0u : 0xffffu);
    test.memory.put(effect + 0x104u, 0x654742a3u);
    test.memory.put(effect + 0x108u, 0x32c00503u);
    test.memory.put(system_record + 0xeu, 0u, 2);
    test.memory.put(system_record + 0x10u, 1u);
    test.memory.put(system_record + 0x30u, 0xacf001b2u);
    test.memory.put(system_record + 0x40u, none);
    test.memory.put(0x4f0414u, tag_table);
    test.memory.put(tag_table + 16u, 0x62736474u);
    test.memory.put(tag_table + 24u, system_tag);
    test.memory.put(tag_table + 32u, 0x5052544du);
    test.memory.put(system_tag + 0x18u, definition);
    test.memory.put(definition + 4u, 2u);
    test.memory.put(0x486048u, vtable);
    test.memory.put(vtable + 0x44u, children::second_method);
    test.memory.put(vtable + 0x48u, children::first_method);
    test.memory.put(vtable + 0x50u, children::prepare_method);
    test.memory.put(0x4f7208u, table_header);
    test.memory.put(table_header + 0x44u, location_slots);
    constexpr std::array table{0x17bcc2u, 0x17bbcbu, 0x17bc47u, 0x17bcfcu};
    for (unsigned i = 0u; i < table.size(); ++i) test.memory.put(kernel::location_modes + 4u * i, table[i]);
    const auto count = (index / 16u) % (slots.size() + 1u);
    test.memory.put(effect + 0x7cu, count ? handle(0u) : none);
    for (unsigned i = 0; i < slots.size(); ++i) {
        const auto address = location_slots + 0x3cu * slots[i];
        test.memory.put(address + 2u, i % 3u ? 0x8034u : 0u, 2);
        test.memory.put(address + 4u, i + 1u < count ? handle(i + 1u) : none);
        for (unsigned axis = 0u; axis < 3u; ++axis) {
            test.memory.put(address + 0x30u + 4u * axis, float_words[(index + axis + i) % float_words.size()]);
        }
    }
    test.memory.put(kernel::scene, scene);
    test.memory.put(scene + 0xa0u, objects);
    test.memory.put(scene + 0x68u, planes);
    constexpr std::array nodes{0xffu, 1u, 0x81u};
    test.memory.put(objects + 0x70u, nodes[(index / 5u) % nodes.size()], 1);
    test.memory.put(planes + 24u + 2u, index % 7u ? 1u : 0xffffu, 2);
    for (unsigned axis = 0u; axis < 4u; ++axis)
        test.memory.put(planes + 24u + 4u + axis * 4u, float_words[(index + 2u * axis) % float_words.size()]);
    return test;
}

bool bridge(WorldState &state) {
    return kernel::bridge(state, prepared::native_particle_location_update_reference_17bb10,
                          children::call_local,
                          [](WorldState &value, std::uint32_t target, std::uint32_t at) {
                              children::dispatch(value, target, at);
                          },
                          [](WorldState &value, std::uint32_t target) {
                              return children::canonical_branch(target, value.image);
                          });
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    fixture::use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x17bb10u);
    Totals totals;
    const auto source = original::native_particle_location_update_reference_17bb10_live;
    const auto watched = [](std::uint32_t site) { return site >= kernel::first && site <= kernel::last; };
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random, index);
        const auto accesses = fixture::compare_geometry(test, 0u, totals, bridge, source, watched);
        if (index < 96u)
            for (unsigned at = 1u; at <= accesses; ++at)
                fixture::compare_geometry(test, at, totals, bridge, source, watched);
    }
    for (auto wrong : {0x12804117u, 0x102f61u}) {
        auto test = make_case(random, 44u);
        test.memory.put(kernel::location_modes, wrong);
        fixture::compare_geometry(test, 0u, totals, bridge, source, watched);
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
