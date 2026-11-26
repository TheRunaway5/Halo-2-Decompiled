#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_draw_distance.hpp"

using namespace frame_misc_harness;
namespace distance = halo2::native_particle_draw_distance_detail;

namespace original {
void run_252ec0(WorldState &state) {
    log_call(entry_hash(state, 0x252ec0u));
    state.gp[3] ^= 0x10010000u;
    state.gp[1] ^= 0x80001000u;
    state.flags.plus(state.gp[3], 0x252ec0u, 4);
    state.xmm[5] = _mm_set_ps(6.0f, -2.0f, 4.0f, 8.0f);
    if (policy == 11u) throw std::runtime_error("Draw distance query fault");
    const std::array outputs{0x7fc00011u, 0xbf800000u, 0u, 0x3f800000u, 0x7f800000u};
    state.xmm[0] = _mm_set_ss(physics_step_detail::f(outputs[policy % outputs.size()]));
    state.pop();
}
#include "distance_refs.inc"
}

namespace {
Case author(std::mt19937 &random, unsigned index) {
    Case test;
    const auto variant = index % 21u;
    const auto stack = variant == 19u ? 0x20003000u : 0x20003080u + 4u * (index % 4u);
    const auto view = 0x10007000u;
    random_abi(random, test, stack);
    auto &gp = test.abi.gp;
    gp[0] = (gp[0] & ~255u) | (variant % 6u == 0u ? 0x80u : 0u);
    gp[6] = variant == 13u ? stack + 0x20u : view;
    auto &memory = test.memory;
    std::mt19937_64 bits(random());
    memory.put(stack + 0x14u, special_float(bits));
    for (unsigned axis = 0; axis < 3u; ++axis) {
        memory.put(gp[6] + 0x10u + 4u * axis, special_float(bits));
        memory.put(0x4c65c0u + 4u * axis, special_float(bits));
    }
    memory.put(0x4c65ecu, special_float(bits));
    memory.put(0x46a0e8u, variant == 8u ? 0x7fc01000u : 0u);
    memory.put(0x4c6854u, std::uint32_t(random()));
    if (variant == 14u) memory.drop(0x4c65c0u);
    if (variant == 15u) memory.drop(view);
    if (variant == 16u) memory.drop(0x46a0e8u);
    if (variant == 17u) memory.drop(0x4c6854u);
    if (variant == 18u) memory.drop(stack);
    if (variant == 20u) gp[6] = 0xfff00000u;
    test.policy = variant;
    return test;
}

struct Output {
    Result state;
    bool admitted{};
};
Output run(Memory &memory, const Case &test, bool native) {
    Output outcome;
    auto &result = outcome.state;
    SourcePhysicsContactRecordObserver observe = [&](const auto &event) {
        result.stores.push_back(event);
        if ((test.policy == 4u && event.instruction == 0x1776b3u) ||
            (test.policy == 10u && event.instruction == 0x1776d7u) ||
            (test.policy == 9u && event.instruction == 0x17765du))
            throw std::runtime_error("Draw distance observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (native) {
        services.instruction = [](auto) { throw std::runtime_error("Draw distance ran a literal callback"); };
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
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(test.abi);
    policy = test.policy;
    call_log = &result.calls;
    try {
        if (native)
            outcome.admitted = distance::bridge(state, original::native_particle_draw_distance_reference_1772c0,
                                                original::run_252ec0, original::native_single_spill);
        else {
            original::retained_distance(state);
            outcome.admitted = true;
        }
    } catch (const std::exception &error) { result.fault = error.what(); }
    call_log = nullptr;
    result.abi = state.save();
    result.mxcsr = _mm_getcsr();
    return outcome;
}
}

int main(int argc, char **argv) {
    const auto count = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode,"HALO2_VERIFY_PARTICLE_DRAW_DISTANCE");
    std::mt19937 random(0x17764eu);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = author(random,index);
        auto a = test.memory.clone(), b = test.memory.clone();
        const auto n = run(a,test,true), r = run(b,test,false);
        if ((!n.admitted && n.state.fault.empty()) || n.state.fault != r.state.fault ||
            n.state.calls != r.state.calls || n.state.stores != r.state.stores || !a.same(b) ||
            !same_abi(n.state.abi,r.state.abi) || n.state.mxcsr != r.state.mxcsr ||
            (!census_mode && n.state.cursor != r.state.cursor)) {
            std::fprintf(stderr,"case %u %s: PC %x/%x; faults '%s'/'%s'\n",
                index,mode.c_str(),
                n.state.cursor,r.state.cursor,n.state.fault.c_str(),r.state.fault.c_str());
            return 1;
        }
        ++totals.compared;
        totals.stores += r.state.stores.size();
        totals.children += r.state.calls.size();
        totals.faults += !r.state.fault.empty();
    }
    const auto covered = distance::coverage().load();
    if (covered != ((1u << distance::branch_count)-1u) || !totals.faults || !totals.children) return 2;
    report(count,totals,covered,distance::branch_count,mode);
}
