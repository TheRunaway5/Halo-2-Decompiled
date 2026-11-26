#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_draw_matrix.hpp"

using namespace frame_misc_harness;
namespace matrix = halo2::native_particle_draw_matrix_detail;
using matrix::Stage;

namespace original {
#include "matrix_refs.inc"
}

namespace {
Case author(std::mt19937 &random, unsigned index) {
    Case test;
    const auto variant = (index / 2u) % 16u;
    const auto stack = 0x20002080u + 4u * (index % 4u);
    const auto view = 0x10005000u;
    random_abi(random,test,stack);
    test.abi.gp[6] = variant & 1u ? stack + 0x1cu : view;
    const auto from = test.abi.gp[6] + 0x10u;
    auto &memory = test.memory;
    std::mt19937_64 bits(random());
    const auto float_word = [&] { return index & 4u ? special_float(bits) : std::uint32_t(bits()); };
    for (unsigned offset = 0x2cu; offset <= 0x70u; offset += 4u)
        memory.put(stack + offset,float_word());
    for (unsigned offset = 0; offset <= 8u; offset += 4u)
        memory.put(from + offset,float_word());
    for (unsigned offset = 0; offset <= 0x20u; offset += 4u)
        memory.put(0x4c65c0u + offset,float_word());
    memory.put(0x4c6854u,std::uint32_t(random()));
    memory.put(test.abi.gp[6] + 0x30u,std::uint32_t(random()));
    if (variant == 10u) memory.drop(view);
    if (variant == 11u) memory.drop(stack);
    if (variant == 12u) memory.drop(0x4c65c0u);
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
        if ((test.policy == 14u && (event.instruction == 0x1773e3u || event.instruction == 0x1774b5u)) ||
            (test.policy == 15u && (event.instruction == 0x177443u || event.instruction == 0x177562u)))
            throw std::runtime_error("Draw matrix observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (native) {
        services.instruction = [](auto) { throw std::runtime_error("Draw matrix ran a literal callback"); };
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
        const auto ref = stage == Stage::basis ? original::native_particle_draw_matrix_reference_1772c0_basis
                                               : original::native_particle_draw_matrix_reference_1772c0_transform;
        if (native)
            outcome.admitted = matrix::bridge(state,stage,ref);
        else {
            ref(state);
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
    set_mode(mode,"HALO2_VERIFY_PARTICLE_DRAW_MATRIX");
    std::mt19937 random(0x17739fu);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = author(random,index);
        auto a = test.memory.clone(), b = test.memory.clone();
        const auto stage = index & 1u ? Stage::transform : Stage::basis;
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
    const auto covered = matrix::coverage().load();
    if (covered != ((1u << matrix::branch_count)-1u) || !totals.faults) return 2;
    report(count,totals,covered,matrix::branch_count,mode);
}
