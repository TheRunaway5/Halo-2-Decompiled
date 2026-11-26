#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_draw_visibility.hpp"

using namespace frame_misc_harness;
namespace visibility = halo2::native_particle_draw_visibility_detail;
using visibility::Resume;

namespace original {
void child(WorldState &state, std::uint32_t target) {
    log_call(entry_hash(state, target));
    state.gp[2] += 0x40u;
    state.gp[3] ^= 0x845f0000u;
    state.flags.plus(target, state.gp[1], 4);
    state.xmm[6] = _mm_set_ps(6.0f, 7.0f, 5.0f, 0.0f);
    if (policy == 11u || policy == 12u) throw std::runtime_error("Visibility query fault");
    state.gp[0] = policy % 4u == 1u ? 0u : 0x201u;
    state.pop();
    if (target == 0x167ef0u) state.gp[4] += 8u;
}
void run_167ef0(WorldState &state) { child(state, 0x167ef0u); }
void run_184690(WorldState &state) { child(state, 0x184690u); }
#include "visibility_reference.inc"
}

namespace {
Case author(std::mt19937 &random, unsigned index) {
    Case test;
    const auto variant = index % 24u;
    const auto stack = 0x20003080u + (index % 4u) * 4u;
    const auto particle = 0x10005000u;
    const auto view = 0x10007000u;
    const auto shape = 0x10009000u;
    random_abi(random, test, stack);
    auto &gp = test.abi.gp;
    const auto part = variant % 3u == 0u ? 0xffffffe1u : variant % 3u == 1u ? 1u : 31u;
    gp[0] = (gp[0] & ~255u) | (variant == 0u || variant == 4u || variant == 17u ? 1u : 0u);
    gp[2] = part;
    gp[5] = particle;
    gp[6] = view;
    gp[7] = variant == 20u ? stack : shape;
    const auto declaration = 0x1000b000u;
    auto &memory = test.memory;
    for (std::uint32_t offset = 0u; offset < 256u; offset += 4u)
        memory.put(stack - 64u + offset, std::uint32_t(random()));
    memory.put(stack + 0x84u, variant == 2u || variant == 17u ? 0x80u : 0u, 1);
    memory.put(stack + 0x14u, declaration);
    memory.put(gp[7] + 0x16u, variant == 5u || variant == 15u ? 0x200u : 0u, 2);
    memory.put(declaration + 0x16u, (variant % 2u == 0u ? 0x200u : 0u) | (random() & 0xffu), 2);
    const auto word = std::uint32_t(std::int32_t(part) >> 5u);
    memory.put(0x557034u + 4u * word, variant == 0u || variant == 1u ? 0u : 1u << (part & 31u));
    memory.put(0x556e34u + part, variant % 2u == 0u ? 0x7fu : 0x81u, 1);
    memory.put(0x556e18u, 0x1000d000u);
    std::mt19937_64 bits(random());
    memory.put(view + 0x2cu, special_float(bits));
    memory.put(stack + 0x18u, special_float(bits));
    memory.put(particle + 0xdu, std::uint32_t(random()), 1);
    if (variant == 15u) memory.drop(0x556e18u);
    if (variant == 16u) memory.drop(shape);
    if (variant == 17u || variant == 18u) memory.drop(particle);
    if (variant == 19u) memory.drop(view);
    if (variant == 21u) memory.drop(0x557034u);
    if (variant == 22u) memory.drop(declaration);
    if (variant == 23u) memory.drop(stack);
    test.policy = variant;
    return test;
}

struct Output {
    Result state;
    Resume continuation{Resume::retained};
};
Output run(Memory &memory, const Case &test, bool native) {
    Output outcome;
    auto &result = outcome.state;
    SourcePhysicsContactRecordObserver observe = [&](const auto &event) {
        result.stores.push_back(event);
        if ((test.policy == 4u && event.instruction == 0x1775cfu) ||
            (test.policy == 10u && event.instruction == 0x17764au))
            throw std::runtime_error("Visibility observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (native) {
        services.instruction = [](auto) { throw std::runtime_error("Visibility ran a literal callback"); };
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
        outcome.continuation = native ? visibility::bridge(state,original::replay_visibility,original::child)
                                      : original::retained_visibility(state);
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
    set_mode(mode,"HALO2_VERIFY_PARTICLE_DRAW_VISIBILITY");
    std::mt19937 random(0x177594u);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = author(random,index);
        auto a = test.memory.clone(), b = test.memory.clone();
        const auto n = run(a,test,true), r = run(b,test,false);
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
    const auto covered = visibility::coverage().load();
    if (covered != ((1u << visibility::branch_count)-1u) || !totals.faults || !totals.children) return 2;
    report(count,totals,covered,visibility::branch_count,mode);
}
