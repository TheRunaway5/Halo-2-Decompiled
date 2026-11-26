#include "native_tu15_frame_misc_harness.hpp"
#include "physics/runtime/native_tu15_object_motion_update.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_object_motion_update_detail;

namespace {
constexpr std::uint32_t stack = 0x20002080u, headers = 0x10001000u, rows = 0x10003000u;
constexpr std::uint32_t object = 0x1002c000u, position = 0x1003d000u;
constexpr std::uint32_t forward = 0x10042000u, up = 0x1004e000u, service_state = 0x1005d000u;

void child(WorldState &s, std::uint32_t target, unsigned words) {
    const auto return_pc = r(s.memory, s.gp[4], 4);
    auto signature = entry_hash(s, target);
    for (unsigned i = 0; i <= words; ++i)
        signature = mix(signature, r(s.memory, s.gp[4] + 4u * i, 4));
    log_call(signature);
    std::mt19937_64 random(signature);
    for (unsigned index : {0u, 1u, 2u}) s.gp[index] = std::uint32_t(random());
    s.flags.load(std::uint32_t(random()) & 0x8d5u);
    s.xmm[random() % 8] = _mm_castsi128_ps(_mm_set1_epi32(int(random())));
    _mm_setcsr(_mm_getcsr() | (std::uint32_t(random()) & 63u));
    w(s.memory, s.observe, target, service_state, 4, std::uint32_t(random()));
    if (policy == 3u && target == 0xb7e50u && return_pc == 0xb956fu)
        throw std::runtime_error("Fixture entity rejected orientation update");
    s.gp[4] += 4u * (1u + words);
}

void required(WorldState &s, std::uint32_t site, std::uint32_t target, bool tail) {
    if (site != 0xb959cu || target != 0xbf050u || tail)
        throw std::runtime_error("Object motion changed the required route");
    child(s, target, 1u);
}
} // namespace

namespace original {
using namespace halo2::physics_world_runtime_detail;
void run_ba7e0(WorldState &s) { child(s, 0xba7e0u, 1u); }
void run_b7e50(WorldState &s) { child(s, 0xb7e50u, 2u); }
void run_1cd620(WorldState &s) { child(s, 0x1cd620u, 2u); }
void run_ba630(WorldState &s) { child(s, 0xba630u, 2u); }
#include "body.inc"
#include "reference.inc"
} // namespace original

namespace {
Case author(std::mt19937 &random, unsigned index) {
    Case test;
    auto &m = test.memory;
    random_abi(random, test, stack);
    const auto shape = (index % 32u);
    const std::uint32_t slot = (random() % 96u) | (shape & 1u ? 0x8000u : 0u);
    const auto handle = 0x80000000u | (std::uint32_t(random()) & 0x7fff0000u) | slot;
    for (unsigned i = 0; i < 32; ++i) m.put(stack - 0x40u + 4u * i, std::uint32_t(random()));
    m.put(stack, 0xb4207u);
    m.put(stack + 4u, handle);
    m.put(stack + 8u, shape == 0u || shape == 14u ? 0u : position);
    m.put(stack + 12u, shape == 0u || shape == 15u ? 0u : forward);
    m.put(stack + 16u, up);
    m.put(stack + 24u, shape & 2u, 1);
    m.put(stack + 28u, shape & 4u, 1);
    m.put(stack + 36u, shape & 8u, 1);
    m.put(0x4ecbd0u, headers);
    m.put(headers + 0x44u, rows);
    m.put(rows + 12u * slot + 8u, object);
    m.put(object + 4u, (std::uint32_t(random()) & ~0x100u) | (shape & 1u ? 0x100u : 0u));
    m.put(object + 0x14u, shape == 0u || shape == 2u || shape == 13u ? 0x80000003u : 0xffffffffu);
    m.put(object + 0xd4u, shape & 1u ? 0x84002014u : 0xffffffffu);
    m.put(0x46aa04u, std::bit_cast<std::uint32_t>(-10.0f));
    m.put(0x46a530u, std::bit_cast<std::uint32_t>(10.0f));
    m.put(service_state, 0x50000000u);
    for (unsigned axis = 0; axis < 3; ++axis) {
        const auto offset = 4u * axis;
        const bool rejected = shape >= 16u && shape < 25u && (shape - 16u) / 3u == axis;
        const auto value = rejected ? std::array{-11.0f, 11.0f, std::bit_cast<float>(0x7f800001u)}[(shape - 16u) % 3u]
                                    : float(int(random() % 21u) - 10);
        m.put(position + offset, std::bit_cast<std::uint32_t>(value));
        m.put(forward + offset, std::uint32_t(random()));
        m.put(up + offset, std::uint32_t(random()));
        m.put(object + 0x64u + offset, std::uint32_t(random()));
        m.put(object + 0x70u + offset, std::uint32_t(random()));
        m.put(object + 0x7cu + offset, std::uint32_t(random()));
    }
    switch (shape) {
    case 25u: m.drop(headers); break;
    case 26u: m.drop(rows); break;
    case 27u: m.drop(position); break;
    case 28u: m.drop(forward); break;
    case 29u: m.drop(up); break;
    case 30u: m.put(stack + 16u, object + 0x74u); break;
    case 31u: m.put(stack + 8u, stack + 0x14u); break;
    }
    return test;
}

bool native(WorldState &s) {
    return owner::bridge(s, original::native_frame_misc_reference_b9460, original::run_ba7e0,
                         original::run_b7e50, original::run_1cd620, original::run_ba630);
}
} // namespace

int main(int argc, char **argv) try {
    const auto count = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000u;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode, "HALO2_VERIFY_OBJECT_MOTION_UPDATE");
    SourcePhysicsContactRuntimeServices::RequiredOwner route = [](auto &memory, const auto &call, auto &abi, const auto &observe) {
        const SourcePhysicsWorldRuntimeServices services;
        WorldState child_state{memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
        child_state.load(abi);
        required(child_state, call.instruction, call.target, call.tail);
        abi = child_state.save();
    };
    std::mt19937 random(0xb9460u + (census_mode ? 1u : 0u) + (mode == "verify" ? 2u : 0u));
    Totals totals;
    for (unsigned index = 0; index < count; ++index)
        if (!compare(index, author(random, index), native, original::run_b9460, totals, false, route)) return 1;
    if (!census_mode) check_refusals(author(random, 0u), native);
    report(count, totals, owner::coverage().load(), unsigned(owner::branch_count), mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
