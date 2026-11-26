#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_draw_view.hpp"

using namespace frame_misc_harness;
namespace view = halo2::native_particle_draw_view_detail;
using view::Resume;
using view::Stage;

namespace original {
#include "view_refs.inc"
}

namespace {

Case author(std::mt19937 &random, unsigned index, Stage stage) {
    Case test;
    const auto variant = (index / 3u) % 18u;
    const auto stack = variant == 9u ? 0x20002000u : 0x20002100u - 4u * (index % 4u);
    const auto shape = 0x10002000u, particle = 0x10004000u;
    const auto table = 0x10006000u, rows = 0x10008000u;
    random_abi(random, test, stack);
    auto &gp = test.abi.gp;
    const auto row = index & 3u;
    gp[0] = variant == 0u ? 0xffffffffu : 0x47300000u | row;
    gp[5] = particle;
    gp[7] = shape;
    const auto bound_stack = stage == Stage::first && variant != 0u ? stack - 4u : stack;
    auto &memory = test.memory;
    memory.put(stack, 0x47327844u);
    if (variant != 9u) memory.put(stack - 4u, 0xb71037a2u);
    memory.put(stack + 0x14u, std::uint32_t(random()));
    memory.put(shape + 0x14u, index & 2u ? 0x8010u : (random() & 0xffffu), 2);
    memory.put(particle + 0x30u, 0x49600000u | row);
    std::mt19937_64 bits(random());
    memory.put(0x46a12cu, special_float(bits));
    test.abi.xmm[7][0] = special_float(bits);
    const auto arg = variant % 3u;
    memory.put(bound_stack + 0x7cu, arg, 1);
    memory.put(bound_stack + 0x80u, (index / 6u) & 1u, 1);
    memory.put(0x52d71cu, table);
    memory.put(table + 0x44u, rows);
    memory.put(rows + 0x34u * row + 2u, arg & 1u ? 0x42u : 0u, 1);
    if (variant == 11u) memory.drop(shape);
    if (variant == 12u) memory.drop(particle);
    if (variant == 13u) memory.drop(0x46a12cu);
    if (variant == 14u) memory.drop(table);
    if (variant == 15u) memory.drop(rows);
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
        if (test.policy == 17u && event.instruction != 0x177450u)
            throw std::runtime_error("Particle draw view observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (native) {
        services.instruction = [](auto) { throw std::runtime_error("Particle draw view ran a literal callback"); };
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
    try {
        constexpr std::array reference{original::native_particle_draw_view_reference_1772c0_prepare,
            original::native_particle_draw_view_reference_1772c0_first,
            original::native_particle_draw_view_reference_1772c0_next};
        const auto check = reference[static_cast<unsigned>(stage)];
        outcome.continuation = native ? view::bridge(state,stage,check) : check(state);
    } catch (const std::exception &error) { result.fault = error.what(); }
    result.abi = state.save();
    result.mxcsr = _mm_getcsr();
    return outcome;
}
}

int main(int argc, char **argv) {
    const auto count = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode,"HALO2_VERIFY_PARTICLE_DRAW_VIEW");
    std::mt19937 random(0x177376u);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto stage = static_cast<Stage>(index % 3u);
        const auto test = author(random,index,stage);
        auto a = test.memory.clone(), b = test.memory.clone();
        const auto n = run(a,test,true,stage), r = run(b,test,false,stage);
        if (n.continuation != r.continuation || n.state.fault != r.state.fault ||
            n.state.stores != r.state.stores || !a.same(b) ||
            !same_abi(n.state.abi,r.state.abi) || n.state.mxcsr != r.state.mxcsr ||
            (!census_mode && n.state.cursor != r.state.cursor)) {
            std::fprintf(stderr,"case %u %s: terminal %x/%x PC %x/%x; faults '%s'/'%s'\n",
                index,mode.c_str(),unsigned(n.continuation),unsigned(r.continuation),
                n.state.cursor,r.state.cursor,n.state.fault.c_str(),r.state.fault.c_str());
            return 1;
        }
        ++totals.compared;
        totals.stores += r.state.stores.size();
        totals.faults += !r.state.fault.empty();
    }
    const auto covered = view::coverage().load();
    if (covered != ((1u << view::branch_count)-1u) || !totals.faults) return 2;
    report(count,totals,covered,view::branch_count,mode);
}
