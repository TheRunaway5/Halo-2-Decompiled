#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_draw_routing.hpp"

using namespace frame_misc_harness;
namespace routing = halo2::native_particle_draw_routing_detail;
using routing::Resume;
using routing::Stage;

namespace original {
void dispatch(WorldState &state, std::uint32_t target, std::uint32_t site, bool tail) {
    log_call(entry_hash(state, target) ^ site);
    if (target != 0x3eb10u || site != 0x17756eu || tail)
        throw std::runtime_error("Unexpected particle routing dispatch");
    state.gp[0] = (state.gp[0] & 0xffffff00u) | (policy % 3u == 1u ? 0xabu : 0u);
    state.gp[3] ^= 0x11000000u;
    state.gp[1] ^= 0x0a100000u;
    state.flags.plus(state.gp[3], target, 4);
    state.xmm[6] = _mm_set_ps(6.0f, 8.0f, 2.0f, 1.0f);
    if (policy == 10u) throw std::runtime_error("Particle visibility query fault");
    state.pop();
}
#include "routing_refs.inc"
}

namespace {
Case author(std::mt19937 &random, unsigned index, Stage stage) {
    Case test;
    const auto variant = (index / 3u) % 18u;
    const auto stack = variant == 14u ? 0x20003000u : 0x20003080u + 4u * (index % 4u);
    const auto particle = 0x10004000u, view = 0x10007000u;
    random_abi(random, test, stack);
    test.abi.gp[5] = particle;
    test.abi.gp[6] = (variant == 9u && stage != Stage::visibility_input) ? stack - 0x20u : view;
    auto &memory = test.memory;
    memory.put(stack, std::uint32_t(random()));
    if (variant != 14u) memory.put(stack - 4u, std::uint32_t(random()));
    memory.put(stack + 0x12u, random() & 255u, 1);
    memory.put(particle + 0x20u, index & 2u ? 0x8004u : random() & 0xffffu, 2);
    const auto definition = (random() & 0xfbffu) | (variant % 3u == 2u ? 0x400u : 0u);
    memory.put(particle + 0xcu, definition, 2);
    memory.put(test.abi.gp[6] + 0xcu, index & 4u ? 0xffffffffu : 0x2487015u);
    memory.put(view + 0x30u, std::uint32_t(random()));
    memory.put(0x4c6854u, std::uint32_t(random()));
    if (variant == 11u) memory.drop(particle);
    if (variant == 12u) memory.drop(view);
    if (variant == 13u) memory.drop(0x4c6854u);
    if (variant == 16u) memory.drop(stack);
    test.policy = variant;
    return test;
}

struct Output {
    Result state;
    Resume continuation{Resume::retained};
};
Output run(Memory &memory, const Case &test, bool native, Stage stage) {
    Output outcome;
    auto &result = outcome.state;
    SourcePhysicsContactRecordObserver observe = [&](const auto &event) {
        result.stores.push_back(event);
        if (test.policy == 17u && event.instruction != 0x17756eu)
            throw std::runtime_error("Particle draw routing observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (native) {
        services.instruction = [](auto) { throw std::runtime_error("Particle draw routing ran a literal callback"); };
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
        constexpr std::array reference{original::native_particle_draw_routing_reference_1772c0_input_live,
            original::native_particle_draw_routing_reference_1772c0_skip_live,
            original::native_particle_draw_routing_reference_1772c0_advance_live};
        constexpr std::array replay{original::native_particle_draw_routing_reference_1772c0_input,
            original::native_particle_draw_routing_reference_1772c0_skip,
            original::native_particle_draw_routing_reference_1772c0_advance};
        const auto part = static_cast<unsigned>(stage);
        outcome.continuation = native ? routing::bridge(state, stage, replay[part],
            [](WorldState &child) { original::dispatch(child, 0x3eb10u, 0x17756eu, false); })
            : reference[part](state);
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
    set_mode(mode,"HALO2_VERIFY_PARTICLE_DRAW_ROUTING");
    std::mt19937 random(0x17756au);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto stage = static_cast<Stage>(index % 3u);
        const auto test = author(random,index,stage);
        auto a = test.memory.clone(), b = test.memory.clone();
        const auto n = run(a,test,true,stage), r = run(b,test,false,stage);
        if (n.continuation != r.continuation || n.state.fault != r.state.fault ||
            n.state.calls != r.state.calls || n.state.stores != r.state.stores || !a.same(b) ||
            !same_abi(n.state.abi,r.state.abi) || n.state.mxcsr != r.state.mxcsr ||
            (!census_mode && n.state.cursor != r.state.cursor)) {
            std::fprintf(stderr,"case %u %s: terminal %x/%x PC %x/%x; faults '%s'/'%s'\n",
                index,mode.c_str(),unsigned(n.continuation),unsigned(r.continuation),
                n.state.cursor,r.state.cursor,n.state.fault.c_str(),r.state.fault.c_str());
            return 1;
        }
        ++totals.compared;
        totals.stores += r.state.stores.size();
        totals.children += r.state.calls.size();
        totals.faults += !r.state.fault.empty();
    }
    const auto covered = routing::coverage().load();
    if (covered != ((1u << routing::branch_count)-1u) || !totals.faults) return 2;
    report(count,totals,covered,routing::branch_count,mode);
}
