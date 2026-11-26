#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_animation_fade_start.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_animation_fade_start_detail;

namespace original {
std::uint32_t selected(std::uint32_t operand, SourcePhysicsContactImage image) {
    if (image != SourcePhysicsContactImage::TitleUpdate15)
        throw std::runtime_error("Unexpected image for animation fade start");
    return operand;
}
#include "body.inc"
#include "reference.inc"
} // namespace original

namespace {
constexpr std::uint32_t fade = 0x10002000u;
constexpr std::uint32_t time = 0x10006000u;
constexpr std::uint32_t stack = 0x20002040u;

Case author(std::mt19937 &random, unsigned index) {
    Case fixture;
    random_abi(random, fixture, stack);
    auto &m = fixture.memory;
    fixture.abi.gp[1] = fade;
    fixture.abi.gp[3] = (fixture.abi.gp[3] & 0xffffff00u) | 1u;
    m.put(stack - 4u, std::uint32_t(random()));
    m.put(stack, std::uint32_t(random()));
    m.put(stack + 4u, u(0.5f));
    m.put(owner::logic::clock, time);
    m.put(owner::logic::one, u(1.0f));
    m.put(time + 2u, 30u, 2);
    m.put(fade, 0x01001107u);
    std::mt19937_64 floats(random());

    switch (index % 16u) {
    case 0: m.put(stack + 4u, 0u); break;
    case 1: m.put(fade + 1u, 14u, 1); break;
    case 2: break; // Rescale an active counter.
    case 3: m.put(fade + 1u, 0u, 1); break;
    case 4:
        m.put(fade + 1u, 0u, 1);
        fixture.abi.gp[3] &= 0xffffff00u;
        break;
    case 5: m.put(stack + 4u, u(23.5f)); break;
    case 6:
        m.put(fade + 1u, 0xfeu, 1);
        m.put(fade, 0xf8u, 1);
        break;
    case 7:
        m.put(stack + 4u, special_float(floats));
        m.put(time + 2u, std::uint32_t(random()), 2);
        break;
    case 8:
        m.put(owner::logic::one, special_float(floats));
        m.put(fade, std::uint32_t(random()) | 0x0100u);
        break;
    case 9: fixture.abi.gp[1] = stack - 4u; break;
    case 10: fixture.abi.gp[1] = stack + 3u; break;
    case 11:
        m.put(stack + 4u, u(0.125f));
        m.put(fade, std::uint32_t(random()));
        break;
    case 12: m.drop(fade); break;
    case 13: m.drop(owner::logic::clock); break;
    case 14: m.drop(owner::logic::one); break;
    case 15: m.drop(stack); break;
    }
    return fixture;
}

bool native(WorldState &s) {
    return owner::bridge(s, original::native_frame_misc_reference_1e14f0);
}
} // namespace

int main(int argc, char **argv) try {
    const unsigned cases = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode, "HALO2_VERIFY_ANIMATION_FADE_START");
    std::mt19937 random(0x1e14f0u + (census_mode ? 1u : 0u) + (mode == "verify" ? 2u : 0u));
    Totals totals;
    for (unsigned index = 0; index < cases; ++index) {
        if (!compare(index, author(random, index), native, original::run_1e14f0, totals)) return 1;
    }
    if (!census_mode) check_refusals(author(random, 1u), native);
    report(cases, totals, owner::coverage().load(), unsigned(owner::branch_count), mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
