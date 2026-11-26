#include "native_tu15_particle_system_geometry_fixture.hpp"
#include "rendering/effects/native_tu15_particle_property.hpp"
#include "rendering/effects/native_tu15_particle_velocity_special.hpp"

using namespace frame_misc_harness;
namespace fixture = frame_misc_harness::geometry_fixture;
namespace special = halo2::native_particle_velocity_special_detail;
namespace cone = halo2::native_particle_velocity_cone_detail;
namespace bounds = halo2::native_particle_velocity_bounds_detail;
namespace angles = halo2::native_particle_velocity_angles_detail;
namespace fp_math = halo2::physics_step_detail;

namespace children {
constexpr std::array vector_words{0u, 0x80000000u, 0x3f800000u, 0xc0000000u, 0x7fc00412u, 0x7f800015u,
                                  0xffc00462u, 0x7f800000u, 0x00000001u, 0x00800000u, 0xbf400000u};

void range(WorldState &s, std::uint32_t at) {
    // The dispatched child may consume seeded state and change a live x87 stack.
    s.instruction_literal(0x259b0u);
    s.push(0x259b0u, s.gp[1]);
    s.instruction_literal(0x259b1u);
    s.gp[0] = r(s.memory, s.gp[1], 4);
    s.gp[0] = s.flags.product(s.gp[0], 1664525u);
    s.gp[0] = s.flags.plus(s.gp[0], 1013904223u, 4);
    s.instruction_literal(0x259beu);
    w(s.memory, s.observe, 0x259beu, s.gp[1], 4, s.gp[0]);
    s.gp[0] = s.flags.right(s.gp[0], 16u, 4, false);
    s.flags.logic(s.gp[0], 4);
    s.instruction_literal(0x259c5u);
    w(s.memory, s.observe, 0x259c5u, s.gp[4], 4, s.gp[0]);
    s.instruction_literal(0x259c8u);
    s.fp.push(static_cast<long double>(fp_math::si(r(s.memory,s.gp[4],4))));
    s.instruction_literal(0x259d3u);
    s.fp.st(0) = fp_math::mul(s.fp.st(0), fp_math::ext(f(r(s.memory,angles::random_scale,4))));
    s.instruction_literal(0x259d9u);
    s.fp.push(fp_math::ext(f(r(s.memory,s.gp[4] + 12u,4))));
    s.instruction_literal(0x259ddu);
    s.fp.st(0) = fp_math::sub(s.fp.st(0), fp_math::ext(f(r(s.memory,s.gp[4] + 8u,4))));
    s.fp.st(1) = fp_math::mul(s.fp.st(1),s.fp.st(0));
    s.fp.pop();
    s.instruction_literal(0x259e3u);
    s.fp.st(0) = fp_math::add(s.fp.st(0), fp_math::ext(f(r(s.memory,s.gp[4] + 8u,4))));
    s.instruction_literal(0x259e7u);
    s.gp[1] = s.pop();
    s.instruction_literal(0x259e8u);
    if (s.pop() != at + 5u) throw std::logic_error("Wrong random range return address");
    s.gp[4] += 8u;
}

void dispatch(WorldState &s, std::uint32_t target, std::uint32_t at, bool tail = false) {
    if (tail) throw std::logic_error("Unexpected velocity property tail dispatch");
    log_call(entry_hash(s, target));
    if (target == 0x13de70u && at == 0x250004u) {
        constexpr std::array curve_words{0u, 0x3f400000u, 0x3f800000u, 0x40000000u, 0x7fc07131u, 0x7f807112u};
        s.xmm[0] = _mm_set_ss(f(curve_words[(policy / 3u) % curve_words.size()]));
        if (s.pop() != 0x250009u) throw std::logic_error("Wrong authored curve return address");
        s.gp[4] += 8u;
    } else if (target == 0x148de0u && at == 0x252879u) {
        const auto address = s.gp[0], rng = s.gp[2];
        const auto seed = r(s.memory, rng, 4);
        w(s.memory, s.observe, target, rng, 4, seed * 1664525u + 1013904223u);
        for (unsigned axis = 0; axis < 3; ++axis)
            w(s.memory, s.observe, target, address + 4u * axis, 4, vector_words[(policy + axis) % vector_words.size()]);
        s.xmm[1] = _mm_set_ss(0.5f);
        s.gp[1] = s.flags.logic(0x1976eeabu, 4);
        if (s.pop() != at + 5u) throw std::logic_error("Wrong random direction return address");
    } else if (target == 0x259b0u && (at == 0x252996u || at == 0x2529e0u ||
                                     at == 0x252acau || at == 0x252ae6u || at == 0x252c98u)) {
        range(s, at);
    } else throw std::logic_error("Unexpected particle velocity child");
    log_call(entry_hash(s, target));
}
} // namespace children

#include "particle_velocity_remaining_reference.inc"

namespace {
constexpr std::uint32_t frame = 0x25003300u, particle = 0x25004300u, input = 0x25006300u;
constexpr std::uint32_t authored = 0x25008300u, curve = 0x2500a300u, seed = 0x2500c300u;
constexpr std::uint32_t context = 0x2500e300u, effects = 0x25010300u, data = 0x25012300u;
constexpr std::uint32_t default_extent = 0x25014300u;
constexpr std::array destinations{0x2524d4u,0x252597u,0x252628u,0x252672u,0x2526eeu,
                                  0x2527acu,0x2528b0u,0x252a12u,0x252b19u,0x252bd8u,0xdeffaceu,0xcc271u};

Case make_case(std::mt19937 &random, unsigned index) {
    Case test;
    random_abi(random, test, frame);
    test.policy = index;
    test.abi.fs_base = random();
    test.abi.gp[0] = index % destinations.size() == 9u ? 0x40000009u : index % destinations.size();
    test.abi.gp[3] = input;
    test.abi.gp[5] = authored;
    test.abi.gp[7] = particle;
    auto &memory = test.memory;
    for (std::uint32_t address = frame - 0x60u; address <= frame + 0x38u; address += 4u)
        memory.put(address, random());
    memory.put(frame + 0x30u, context);
    memory.put(frame + 0x38u, seed + 4u);
    memory.put(input, 0x3f000000u);
    memory.put(input + 4u, 0x3f400000u);
    for (auto offset : {0x7cu, 0x8cu}) {
        memory.put(authored + offset, 0u, 2);
        memory.put(authored + offset + 2u, 1u, 2);
        memory.put(authored + offset + 6u, 0xffffu, 2);
        memory.put(authored + offset + 12u, curve);
    }
    memory.put(curve, 0u);
    memory.put(curve + 4u, 0x3e800000u);
    memory.put(curve + 8u, 0x3fc00000u);
    memory.put(angles::random_state, seed);
    memory.put(seed + 4u, random());
    memory.put(angles::unsigned_adjustment, 0x4f800000u);
    memory.put(angles::random_scale, 0x37800000u);
    memory.put(angles::angle_scale, index % 17u ? 0x40c90fdbu : 0x7f800000u);
    memory.put(angles::spread_scale, index % 19u ? 0x3f000000u : 0xffc04131u);
    memory.put(0x46a0d0u, 0x3f800000u);
    memory.put(0x46a0dcu, 0xbf800000u);
    memory.put(context + 0x18u, ((index / 6u) & 1u) ? 0x70110002u : 0xffffffffu);
    memory.put(bounds::effects, effects);
    memory.put(effects + bounds::array_offset, data);
    memory.put(bounds::fallback_bounds, default_extent);
    memory.put(default_extent, 0x3fa00000u);
    memory.put(default_extent + 4u, 0x40000000u);
    memory.put(data + 0x320u + 0x74u, 0x40200000u);
    memory.put(data + 0x320u + 0x78u, 0x3e800000u);
    for (unsigned offset = 0u; offset < 0x40u; offset += 4u)
        memory.put(particle + offset, random());
    for (unsigned i = 0u; i < destinations.size(); ++i)
        memory.put(special::table + 4u * i,destinations[i]);
    return test;
}

using Native = bool (*)(WorldState &);
using Literal = void (*)(WorldState &);

template<unsigned Mode, void (*Replay)(WorldState &)>
bool run_mode(WorldState &s) {
    auto dispatch = [](WorldState &value,std::uint32_t target,std::uint32_t at) { children::dispatch(value,target,at); };
    auto trig = [](long double value,bool cosine,std::uint16_t &status) {
        return cosine ? source::native_fcos(value,status) : source::native_fsin(value,status);
    };
    if constexpr (Mode <= 5u)
        return cone::bridge<Mode == 5u>(s,Replay,children::curve_call,dispatch,trig,children::native_single_spill);
    else if constexpr (Mode <= 7u)
        return bounds::bridge<Mode == 7u>(s,Replay,children::curve_call,dispatch,trig,children::native_single_spill);
    else
        return special::bridge<Mode == 9u>(s,Replay,children::curve_call,dispatch,trig,children::native_single_spill);
}

#define ROUTE(Number, Name) {run_mode<Number,source::native_particle_velocity_remaining_##Name##_reference_252480>, \
                             source::native_particle_velocity_remaining_##Name##_reference_252480_live}
constexpr std::pair<Native,Literal> routes[]{ROUTE(4,mode4),ROUTE(5,mode5),ROUTE(6,mode6),
                                             ROUTE(7,mode7),ROUTE(8,mode8),ROUTE(9,alternate)};
#undef ROUTE

std::uint32_t valid(std::uint32_t target) {
    if (std::find(destinations.begin(),destinations.begin() + 10,target) != destinations.begin() + 10)
        return target;
    throw std::runtime_error("Required original solver dispatch target unavailable");
}

bool choose_velocity_mode(WorldState &s) {
    std::uint32_t target;
    const auto available = special::select(s,target,source::native_particle_velocity_remaining_selection_reference_252480,
                                          source::canonical_branch);
    if (available) s.gp[0] = valid(target);
    return available;
}

void select_source(WorldState &s) {
    s.gp[0] = valid(source::native_particle_velocity_remaining_selection_reference_252480(s));
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected case count and mode");
    const auto count = unsigned(std::stoul(argv[1]));
    const std::string mode = argv[2];
    fixture::use_collector = mode == "collector";
    set_mode(mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x2526eeu);
    Totals totals;
    const auto motion_sites = [](std::uint32_t pc) { return pc >= cone::fourth && pc <= special::alternate_exit; };
    const auto selection_site = [](std::uint32_t pc) { return pc == special::selection; };
    for (unsigned index = 0u; index < count; ++index) {
        const auto test = make_case(random, index);
        const auto [native, original] = routes[index % std::size(routes)];
        const auto accesses = fixture::compare_geometry(test,0u,totals,native,original,motion_sites);
        fixture::compare_geometry(test,0u,totals,choose_velocity_mode,select_source,selection_site);
        if (index < 72u) {
            for (unsigned at = 1u; at <= accesses; ++at)
                fixture::compare_geometry(test,at,totals,native,original,motion_sites);
            fixture::compare_geometry(test,1u,totals,choose_velocity_mode,select_source,selection_site);
        }
    }
    if (!census_mode) {
        const auto control = make_case(random, count);
        for (const auto &pair : routes) {
            check_refusals(control,pair.first);
            check_birth_observer_refusals(control,pair.first);
        }
        check_refusals(control,choose_velocity_mode);
        check_birth_observer_refusals(control,choose_velocity_mode);
    }
    report(count, totals, 0u, 0u, mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
