#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_physics_shapes.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace shapes = halo2::native_particle_physics_shapes_detail;

namespace shape_child {
constexpr std::uint32_t routine = 0x2c2ad0u, returned = 0x2c40fcu;
constexpr std::uint32_t descriptor = 0x2a006300u, storage = 0x2a008300u, moved_storage = 0x2a00a300u;
constexpr std::uint32_t particle = 0x2a00c300u, stack = 0x2a002300u;

void run_2c2ad0(WorldState &s) {
    const auto call = entry_hash(s, routine);
    s.instruction_literal(routine);
    const auto shape = r(s.memory, s.gp[0], 2);
    s.gp[2] = shape;
    const auto first = r(s.memory, s.gp[4] + 4u, 4);
    const auto second = r(s.memory, s.gp[4] + 8u, 4);
    const auto third = r(s.memory, s.gp[4] + 12u, 4);
    log_call(mix(mix(mix(call,first),second),third));
    w(s.memory, s.observe, routine, s.gp[6] + 4u * s.gp[3], 4, first ^ second ^ third ^ s.gp[1] ^ shape);
    if (s.gp[3] == 0u && (policy & 3u) != 0u) {
        constexpr std::array live_counts{0u, 0u, 4u, 0xffffffffu};
        w(s.memory, s.observe, routine, s.gp[7] + shapes::shape_count, 4, live_counts[policy & 3u]);
        w(s.memory, s.observe, routine, s.gp[7] + shapes::shape_data, 4, moved_storage);
        w(s.memory, s.observe, routine, s.gp[7] + shapes::shape_argument, 4, third ^ 0x400u);
    }
    s.gp[0] = s.flags.logic(shape ^ first, 4);
    s.gp[1] = second;
    s.xmm[2] = _mm_set_ss(static_cast<float>(shape));
    if (s.pop() != returned) throw std::logic_error("Wrong original shape motion return address");
    s.gp[4] += 12u;
    log_call(entry_hash(s,routine));
}
} // namespace shape_child

#include "particle_physics_shapes_reference.inc"

namespace {
Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random,test,shape_child::stack);
    test.policy = index;
    test.abi.fs_base = random();
    test.abi.gp[0] = shape_child::particle;
    test.abi.gp[7] = shape_child::descriptor;
    auto &memory = test.memory;
    for (unsigned offset = 0; offset <= 0x98u; offset += 4u)
        memory.put(shape_child::stack - 0x50u + offset,random());
    constexpr std::array counts{1u, 2u, 3u, 4u, 0u, 0xffffffffu, 0x80000000u};
    memory.put(shape_child::stack,0x25075cu);
    memory.put(shape_child::descriptor + shapes::shape_count,counts[index % counts.size()]);
    memory.put(shape_child::descriptor + shapes::shape_argument,random());
    memory.put(shape_child::descriptor + shapes::shape_data,index % 17u ? shape_child::storage : shape_child::stack - 0x30u);
    for (unsigned offset = 0; offset < 5 * shapes::shape_stride; offset += 4u) {
        memory.put(shape_child::storage + offset,random());
        memory.put(shape_child::moved_storage + offset,random());
    }
    for (unsigned offset = 0; offset < 20u; offset += 4u)
        memory.put(shape_child::particle + offset,random());
    return test;
}

bool native(WorldState &s) {
    return shapes::bridge(s,shape_source::native_particle_physics_shapes_reference_2c40d0,
                          [](WorldState &state, auto) { shape_child::run_2c2ad0(state); });
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    fixture::use_collector = mode == "collector";
    set_mode(mode,"HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x2c40d0u);
    Totals totals;
    const auto sites = [](std::uint32_t pc) { return pc >= shapes::entry && pc <= shapes::last; };
    const auto original = shape_source::native_particle_physics_shapes_reference_2c40d0_live;
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random,index);
        const auto accesses = fixture::compare_geometry(test,0u,totals,native,original,sites);
        if (index < 70u)
            for (unsigned at = 1u; at <= accesses; ++at)
                fixture::compare_geometry(test,at,totals,native,original,sites);
    }
    if (!census_mode) {
        const auto control = make_case(random,count);
        check_refusals(control,native);
        check_birth_observer_refusals(control,native);
    }
    report(count,totals,0u,0u,mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr,"%s\n",error.what());
    return 1;
}
