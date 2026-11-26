#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_draw_submission.hpp"

using namespace frame_misc_harness;
namespace submit = halo2::native_particle_draw_submission_detail;
using submit::Stage;

namespace original {
constexpr std::uint32_t element = 0x10007000u;
void dispatch(WorldState &state, std::uint32_t target, std::uint32_t site, bool tail) {
    log_call(entry_hash(state,target) ^ site);
    if (tail) throw std::runtime_error("Unexpected draw tail");
    state.gp[0] = target == 0x178070u ? element : state.gp[0] ^ 0x812321u;
    state.flags.plus(state.gp[0],site,4);
    state.xmm[5] = _mm_set_ps(5.0f,3.0f,2.0f,1.0f);
    if ((policy == 10u && target == 0x1e4c0u) || (policy == 11u && target == 0x178070u) ||
        (policy == 12u && target == 0x40e20u))
        throw std::runtime_error("Draw submission child fault");
    state.pop();
    if (target == 0x1e4c0u) state.gp[4] += 8u;
    if (target == 0x40e20u) state.gp[4] += 32u;
}
void run_1e4c0(WorldState &state) { dispatch(state,0x1e4c0u,0x17776cu,false); }
#include "submission_refs.inc"
}

namespace {
Case author(std::mt19937 &random, unsigned index) {
    Case test;
    const auto variant = (index / 2u) % 18u;
    const auto stack = variant == 15u ? 0x20002000u : 0x20002080u + 4u * (index % 4u);
    random_abi(random,test,stack);
    auto &gp = test.abi.gp;
    gp[7] = variant % 4u ? 0x8004u : (variant % 8u ? 0x8000u : 0u);
    auto &memory = test.memory;
    for (unsigned offset : {0u,0x18u,0x28u,0x78u})
        memory.put(stack + offset,std::uint32_t(random()));
    if (variant != 15u) memory.put(stack - 36u,0x8124u);
    memory.put(original::element + 4u,std::uint32_t(random()));
    if (variant == 13u) memory.drop(original::element);
    if (variant == 14u) memory.drop(stack);
    test.policy = variant;
    return test;
}

struct Output {
    Result state;
    bool admitted{};
};
Output run(Memory &memory, const Case &test, bool native, Stage stage) {
    Output outcome;
    auto &result = outcome.state;
    SourcePhysicsContactRecordObserver observe = [&](const auto &event) {
        result.stores.push_back(event);
        if ((test.policy == 16u && event.instruction == 0x17775du) ||
            (test.policy == 17u && event.instruction == 0x177796u))
            throw std::runtime_error("Draw submission observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (native) {
        services.instruction = [](auto) { throw std::runtime_error("Draw submission ran a literal callback"); };
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
        const bool transform = stage == Stage::transform;
        const auto ref = transform ? original::native_particle_draw_submission_reference_1772c0_transform
                                   : original::native_particle_draw_submission_reference_1772c0_submit;
        if (native)
            outcome.admitted = submit::bridge(state,stage,ref,[](WorldState &child, auto target, auto site) {
                original::dispatch(child,target,site,false);
            });
        else {
            const auto original = transform ? original::native_particle_draw_submission_reference_1772c0_transform_live
                                            : original::native_particle_draw_submission_reference_1772c0_submit_live;
            original(state);
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
    set_mode(mode,"HALO2_VERIFY_PARTICLE_DRAW_SUBMISSION");
    std::mt19937 random(0x177759u);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = author(random,index);
        auto a = test.memory.clone(), b = test.memory.clone();
        const auto stage = index & 1u ? Stage::transform : Stage::submit;
        const auto n = run(a,test,true,stage), r = run(b,test,false,stage);
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
    const auto covered = submit::coverage().load();
    if (covered != ((1u << submit::branch_count)-1u) || !totals.faults || !totals.children) return 2;
    report(count,totals,covered,submit::branch_count,mode);
}
