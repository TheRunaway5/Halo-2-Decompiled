#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_property.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_particle_property_detail;

namespace {
constexpr std::uint32_t property = 0x10002040u;
constexpr std::uint32_t moved_property = 0x10004040u;
constexpr std::uint32_t inputs = 0x10006040u;
constexpr std::uint32_t moved_inputs = 0x10008040u;
constexpr std::uint32_t definition = 0x1000a040u;
constexpr std::uint32_t child_result = 0x1000c040u;
constexpr std::uint32_t stack = 0x20002040u;

void curve_child(WorldState &s, std::uint32_t target, std::uint32_t site) {
    if (target != 0x13de70u || site != 0x250004u)
        throw std::runtime_error("Unexpected particle curve dispatch");
    const auto low = r(s.memory, s.gp[4] + 4u, 4);
    const auto high = r(s.memory, s.gp[4] + 8u, 4);
    const auto digest = mix(mix(entry_hash(s, target), low), high);
    log_call(digest);
    w(s.memory, s.observe, site, child_result, 4, std::uint32_t(digest));
    if (policy & 0x10u) throw std::runtime_error("Particle curve provider fault");
    constexpr std::uint32_t factors[]{0xbf800000u, 0x3e800000u, 0x3fc00000u, 0x7fc00001u,
                                       0xffc00000u, 0x7f800001u, 0x80000000u, 0x00000001u};
    const std::array lanes{factors[policy & 7u], std::uint32_t(digest), high, low};
    std::memcpy(&s.xmm[0], lanes.data(), 16);
    s.gp[0] = std::uint32_t(digest >> 32u);
    s.gp[2] = std::uint32_t(digest);
    s.gp[4] += 12u;
    if (policy & 8u) {
        s.gp[6] = moved_property;
        s.gp[7] = moved_property + 8u;
        s.gp[3] = moved_inputs;
    }
}
} // namespace

namespace original {
std::uint32_t selected(std::uint32_t operand, SourcePhysicsContactImage image) {
    if (image != SourcePhysicsContactImage::TitleUpdate15)
        throw std::runtime_error("Unexpected particle property image");
    return operand;
}
std::uint32_t native_single_spill(long double value, std::uint16_t &status) {
    const auto result = owner::x87::store_single(value);
    status = std::uint16_t((status & ~0x200u) | (result.rounded_up ? 0x200u : 0u));
    return std::uint32_t(result.bits);
}
void dispatch(WorldState &s, std::uint32_t target, std::uint32_t site, bool tail) {
    if (tail) throw std::runtime_error("Unexpected particle curve tail call");
    curve_child(s, target, site);
}
#include "body.inc"
#include "reference.inc"
} // namespace original

namespace {
Case author(std::mt19937 &random, unsigned index) {
    Case fixture;
    auto &memory = fixture.memory;
    random_abi(random, fixture, stack);
    fixture.abi.gp[3] = inputs;
    fixture.abi.gp[6] = property;
    fixture.policy = index % 16u;

    for (std::uint32_t slot = stack - 32u; slot <= stack + 16u; slot += 4u)
        memory.put(slot, std::uint32_t(random()));
    constexpr std::uint32_t modifier_indices[]{0xffffu, 0x8000u, 2u, 3u, 4u};
    constexpr std::uint32_t modes[]{0u, 1u, 2u, 3u, 0xffffu};
    for (const auto address : {property, moved_property}) {
        memory.put(address, 0xffffu, 2);
        memory.put(address + 2u, 0xfffeu, 2);
        memory.put(address + 4u, modes[(index / 5u) % 5u], 2);
        memory.put(address + 6u, modifier_indices[index % 5u], 2);
        memory.put(address + 12u, definition);
    }
    std::mt19937_64 floats(random());
    for (const auto address : {inputs, moved_inputs}) {
        for (std::uint32_t offset = 0; offset < 32u; offset += 4u)
            memory.put(address - 8u + offset, index % 4u ? special_float(floats) : std::uint32_t(random()));
    }
    memory.put(definition + 1u, index % 11u == 0 ? 0x80u : 0u, 1);
    // Distinct NaNs pass through the affine branch on some profiles.
    memory.put(definition + 4u, index % 7u == 0 ? 0xffc00000u : u(0.25f));
    memory.put(definition + 8u, index % 7u == 0 ? 0x7fc00001u : u(1.25f));
    memory.put(owner::logic::one, u(1.0f));
    memory.put(child_result, 0u);

    switch (index % 19u) {
    case 0: fixture.policy |= 0x10u; break;
    case 1: memory.drop(property); break;
    case 2: memory.drop(inputs); break;
    case 3: memory.drop(definition); break;
    case 4: memory.drop(owner::logic::one); break;
    case 5: fixture.abi.gp[3] = stack + 4u; break;
    default: break;
    }
    return fixture;
}

bool native(WorldState &s) {
    return owner::bridge(s, original::native_frame_misc_reference_24ffe0, curve_child);
}
} // namespace

int main(int argc, char **argv) try {
    const auto cases = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000u;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode, "HALO2_VERIFY_PARTICLE_PROPERTY");
    std::mt19937 random(0x24ffe0u + (census_mode ? 1u : 0u) + (mode == "verify" ? 2u : 0u));
    Totals totals;
    for (unsigned index = 0; index < cases; ++index) {
        if (!compare(index, author(random, index), native, original::run_24ffe0, totals)) return 1;
    }
    if (!census_mode) check_refusals(author(random, 6u), native);
    report(cases, totals, owner::coverage().load(), unsigned(owner::branch_count), mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
