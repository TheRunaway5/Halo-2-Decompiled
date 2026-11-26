#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_draw_position.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_particle_draw_position_detail;

namespace original {
#include "position_refs.inc"
}

namespace {
Case author(std::mt19937 &random, unsigned index) {
    Case test;
    const auto variant = (index / 2u) % 20u;
    const auto stack = 0x20002080u - 4u * (index % 4u);
    random_abi(random, test, stack);
    test.abi.gp[6] = variant == 5u ? stack + 0x20u : 0x10003000u;
    test.abi.gp[3] = (test.abi.gp[3] & 0xffffff00u) | (index & 4u ? 0x96u : 0u);
    auto &memory = test.memory;
    std::mt19937_64 bits(random());
    for (unsigned lane = 0; lane < 4u; ++lane) {
        memory.put(test.abi.gp[6] + 0x10u + lane * 4u, special_float(bits));
        memory.put(stack + 0x2cu + lane * 4u, special_float(bits));
        test.abi.xmm[lane][0] = special_float(bits);
    }
    test.abi.xmm[4][0] = special_float(bits);
    test.abi.xmm[7][0] = special_float(bits);
    for (unsigned axis = 0; axis < 3u; ++axis)
        memory.put(0x491e44u + axis * 4u, special_float(bits));
    const auto flags = (std::uint32_t(random()) & ~0x8000u) | (index & 4u ? 0x8000u : 0u);
    memory.put(stack + 0x20u, flags);
    if (variant == 7u) memory.drop(test.abi.gp[6]);
    if (variant == 8u) memory.drop(stack);
    if (variant == 9u) memory.drop(0x491e44u);
    test.policy = variant;
    return test;
}

struct Output {
    Result state;
    std::uint32_t continuation{};
};

Output run(Memory &memory, const Case &test, bool native, owner::Stage stage) {
    Output outcome;
    auto &result = outcome.state;
    SourcePhysicsContactRecordObserver observe = [&](const auto &event) {
        result.stores.push_back(event);
        if (test.policy == 10u && result.stores.size() == 2u)
            throw std::runtime_error("Draw position observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (native) {
        services.instruction = [](auto) { throw std::runtime_error("Draw position ran a literal callback"); };
        if (!census_mode) {
            auto &cursor = services.instruction_cursor;
            cursor.position = &result.cursor;
            cursor.service_sites = halo2::native_census_detail::service_sites;
            cursor.callback_type = &services.instruction.target_type();
            cursor.executor = std::this_thread::get_id();
            cursor.literal_service_sites = true;
            cursor.observation_only_stores = true;
        }
    } else services.instruction = [&](auto pc) { result.cursor = pc; };
    physics_step_detail::FpScope host(*test.abi.floating_point);
    WorldState state{memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(test.abi);
    try {
        const auto reference = stage == owner::Stage::copy ? original::native_particle_draw_position_reference_1772c0_copy
                                                           : original::native_particle_draw_position_reference_1772c0_adjust;
        outcome.continuation = native ? owner::bridge(state, stage, reference) : reference(state);
    } catch (const std::exception &error) { result.fault = error.what(); }
    result.abi = state.save();
    result.mxcsr = _mm_getcsr();
    return outcome;
}
} // namespace

int main(int argc, char **argv) {
    const auto count = argc > 1 ? unsigned(std::stoul(argv[1])) : 8000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode, "HALO2_VERIFY_PARTICLE_DRAW_POSITION");
    std::mt19937 random(0x1776dau);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = author(random, index);
        auto a = test.memory.clone(), b = test.memory.clone();
        const auto stage = index & 1u ? owner::Stage::copy : owner::Stage::adjust;
        const auto native = run(a, test, true, stage), retained = run(b, test, false, stage);
        const auto &n = native.state, &r = retained.state;
        if (native.continuation != retained.continuation || n.fault != r.fault ||
            !same_abi(n.abi, r.abi) || n.mxcsr != r.mxcsr || n.stores != r.stores ||
            (!census_mode && n.cursor != r.cursor) || !a.same(b)) {
            std::fprintf(stderr, "case %u %s: terminal %x/%x, PC %x/%x, faults '%s'/'%s'\n",
                index, mode.c_str(), native.continuation, retained.continuation, n.cursor, r.cursor,
                n.fault.c_str(), r.fault.c_str());
            return 1;
        }
        ++totals.compared;
        totals.stores += r.stores.size();
        totals.faults += !r.fault.empty();
    }
    const auto covered = owner::coverage().load();
    if (covered != ((1u << owner::branch_count) - 1u) || !totals.faults) return 1;
    report(count, totals, covered, owner::branch_count, mode);
}
