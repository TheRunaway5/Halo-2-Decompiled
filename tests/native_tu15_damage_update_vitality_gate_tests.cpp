#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_damage_update_vitality_gate.hpp"

using namespace frame_misc_harness;
namespace kernel = halo2::native_damage_update_vitality_gate_detail;

namespace original {
struct ObservedDamageState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t damage_observation_pc(std::uint32_t disc, std::uint32_t tu15, const ObservedDamageState &s) {
    return s.selected_observation_pcs && s.image == SourcePhysicsContactImage::TitleUpdate15 ? tu15 : disc;
}
std::uint32_t selected(std::uint32_t key, SourcePhysicsContactImage image) {
    if (image == SourcePhysicsContactImage::MapPack11) return key;
    if (key == 0x4edf18u) return 0x4f3218u;
    if (key == 0x465030u) return 0x46a0d0u;
    if (key == 0x465048u) return 0x46a0e8u;
    if (key == 0xd76cau) return 0xd861au;
    throw std::runtime_error("Unexpected damage section operand");
}

void run_159ef0(ObservedDamageState &state) {
    log_call(entry_hash(state,0x15b2d0u));
    state.gp[0] ^= 0x8f3187e9u;
    state.gp[1] ^= 0x81010000u;
    state.gp[2] ^= 0x24ab7910u;
    state.flags.plus(state.gp[2],state.gp[1],4);
    state.xmm[0] = _mm_set_ps(7.0f,6.0f,5.0f,4.0f);
    if (policy == 14u) throw std::runtime_error("Damage vitality scale child fault");
    state.pop();
    state.gp[4] += 4u;
}
#include "damage_vitality_gate_reference.inc"
}

namespace {
struct Input {
    Case caller;
    kernel::Entry start;
};

Input author(std::mt19937 &random, unsigned index) {
    const auto variant = index / 2u % 23u;
    Input input{{}, index & 1u ? kernel::Entry::clear : kernel::Entry::preserve};
    auto &test = input.caller;
    const auto stack = variant == 21u ? 0x20002004u : 0x20002080u;
    random_abi(random,test,stack);
    test.policy = variant;
    auto &memory = test.memory;
    constexpr auto object = 0x20004000u, options = 0x20006000u;
    test.abi.gp[5] = object;
    memory.put(stack-8u,0u);
    memory.put(stack+0x1cu,0xa5a50000u);
    memory.put(options+8u,variant == 1u ? 3u : 2u);
    memory.put(object+0xaau,variant == 2u ? 6u : 0u,1u);
    memory.put(object+0xe8u,variant == 4u ? 0u : (variant == 5u ? 0x7f810001u : 0x40000000u));
    memory.put(object+0x10au,variant == 6u ? 4u : (variant == 7u ? 0x10u : 0u),2u);
    memory.put(object+0x10bu,0x5cu,1u);
    memory.put(object+0x13cu,variant == 3u ? 0xffffffffu : 0x2000a000u);
    memory.put(0x4f3218u,options);
    memory.put(0x46a0d0u,variant == 8u ? 0x7fc10111u : 0x3f800000u);
    memory.put(0x46a0e8u,0u);
    if (variant == 10u) memory.drop(object);
    if (variant == 11u) memory.drop(0x46a0d0u);
    if (variant == 12u) memory.drop(stack);
    if (variant == 13u) memory.drop(0x4f3218u);
    if (variant == 21u) memory.drop(stack-8u);
    if (variant == 22u) memory.drop(options);
    return input;
}

struct Outcome {
    Result state;
    kernel::Next next{kernel::Next::retained};
};

Outcome execute_case(Memory &memory, const Input &input, bool native) {
    Outcome outcome;
    auto &result = outcome.state;
    const auto &test = input.caller;
    SourcePhysicsContactRecordObserver observe = [&](const auto &event) {
        result.stores.push_back(event);
        if ((test.policy == 16u && event.instruction == 0xd85cbu) ||
            (test.policy == 17u && event.instruction == 0xd8615u) ||
            (test.policy == 18u && event.instruction == 0xd861au) ||
            (test.policy == 19u && event.instruction == 0xd85f0u))
            throw std::runtime_error("Damage section observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (native) {
        services.instruction = [](auto) { throw std::runtime_error("Damage section ran a literal callback"); };
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
    original::ObservedDamageState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    state.selected_observation_pcs = true;
    state.load(test.abi);
    policy = test.policy;
    call_log = &result.calls;
    try {
        outcome.next = native ? kernel::bridge(state,input.start,original::native_damage_update_vitality_gate_reference_d73e0,
            original::run_159ef0) : original::native_damage_update_vitality_gate_reference_d73e0_live(state,input.start);
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
    set_mode(mode,"HALO2_VERIFY_DAMAGE_VITALITY_GATE");
    std::mt19937 random(0xd8a56u);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = author(random,index);
        auto a = test.caller.memory.clone(), b = test.caller.memory.clone();
        const auto native = execute_case(a,test,true), reference = execute_case(b,test,false);
        const auto &n = native.state, &r = reference.state;
        if (native.next != reference.next || n.fault != r.fault || n.stores != r.stores || n.calls != r.calls || !a.same(b) ||
            !same_abi(n.abi,r.abi) || n.mxcsr != r.mxcsr || (!census_mode && n.cursor != r.cursor)) {
            std::fprintf(stderr,"case %u %s: terminal %u/%u PC %x/%x; faults '%s'/'%s'\n",
                index,mode.c_str(),unsigned(native.next),unsigned(reference.next),n.cursor,r.cursor,n.fault.c_str(),r.fault.c_str());
            return 1;
        }
        ++totals.compared;
        totals.stores += r.stores.size();
        totals.children += r.calls.size();
        totals.faults += !r.fault.empty();
    }
    const auto covered = kernel::coverage().load();
    if (covered != ((1u << kernel::branch_count)-1u) || !totals.faults) return 2;
    report(count,totals,covered,kernel::branch_count,mode);
}
