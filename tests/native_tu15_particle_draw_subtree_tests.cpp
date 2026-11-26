#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_draw_subtree.hpp"

using namespace frame_misc_harness;
namespace subtree = halo2::native_particle_draw_subtree_detail;

namespace original {
constexpr std::uint32_t rows = 0x10009000u;
constexpr std::uint32_t first_child = rows + 0x54u * 3u, second_child = rows + 0x54u * 4u;
constexpr std::uint32_t first_handle = 0xab300003u, second_handle = 0xcd300004u;
void run_1772c0(WorldState &state) {
    log_call(entry_hash(state, 0x1772c0u));
    const auto child = state.gp[6];
    state.gp[2] ^= 0x10010000u;
    state.flags.plus(state.gp[2], 0x1772c0u, 4);
    state.xmm[5] = _mm_set_ps(6.0f, -2.0f, 3.0f, 8.0f);
    if (policy == 11u) throw std::runtime_error("Descendant draw fault");
    if (policy == 6u && child == first_child)
        w(state.memory,state.observe,0x17775du,child + 0x38u,4,second_handle);
    if (policy == 12u) state.gp[6] = second_child;
    state.pop();
    state.gp[4] += 16u;
}
#include "subtree_refs.inc"
}

namespace {
Case author(std::mt19937 &random, unsigned index) {
    using namespace original;
    Case test;
    const auto variant = index % 18u;
    const auto stack = variant == 15u ? 0x20003000u : 0x20003080u + 4u * (index % 4u);
    const auto parent = 0x10003000u, array = 0x10007000u;
    random_abi(random,test,stack);
    test.abi.gp[5] = parent;
    auto &memory = test.memory;
    for (unsigned offset : {0u,4u,8u,0x70u,0x78u,0x7cu,0x80u})
        memory.put(stack + offset,std::uint32_t(random()));
    if (variant != 15u) memory.put(stack - 20u,0x343210u);
    memory.put(parent + 0x40u,variant % 4u == 0u && variant != 12u ? 0xffffffffu : first_handle);
    memory.put(parent + 0xcu,(random() & 0xfbffu) | (index & 1u ? 0x400u : 0u),2);
    memory.put(0x51df74u,array);
    memory.put(array + 0x44u,rows);
    memory.put(first_child + 0x38u,variant % 5u == 1u && variant != 6u ? second_handle : 0xffffffffu);
    memory.put(second_child + 0x38u,0xffffffffu);
    memory.put(first_child + 0xcu,std::uint32_t(random()),2);
    memory.put(second_child + 0xcu,std::uint32_t(random()),2);
    if (variant == 8u) memory.drop(parent);
    if (variant == 9u) memory.drop(0x51df74u);
    if (variant == 13u) memory.drop(array);
    if (variant == 14u) memory.drop(rows);
    if (variant == 16u) memory.drop(stack);
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
        if (test.policy == 17u && event.instruction != 0x17780eu)
            throw std::runtime_error("Descendant draw observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (native) {
        services.instruction = [](auto) { throw std::runtime_error("Descendant draw ran a literal callback"); };
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
            outcome.admitted = subtree::bridge(state, original::native_particle_draw_subtree_reference_1772c0,
                                            original::run_1772c0);
        else {
            original::retained_subtree(state);
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
    set_mode(mode,"HALO2_VERIFY_PARTICLE_DRAW_SUBTREE");
    std::mt19937 random(0x1777c4u);
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
    const auto covered = subtree::coverage().load();
    if (covered != ((1u << subtree::branch_count)-1u) || !totals.faults || !totals.children) return 2;
    report(count,totals,covered,subtree::branch_count,mode);
}
