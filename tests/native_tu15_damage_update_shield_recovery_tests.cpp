#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_damage_update_shield_recovery.hpp"

using namespace frame_misc_harness;
namespace kernel = halo2::native_damage_update_shield_recovery_detail;

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
    switch (key) {
    case 0x518444u:return 0x51df54u;
    case 0x46507cu:return 0x46a11cu;
    case 0x4652d0u:return 0x46a370u;
    case 0xd77c3u:return 0xd8713u;
    case 0xd7840u:return 0xd8790u;
    case 0xd784cu:return 0xd879cu;
    default:throw std::runtime_error("Unexpected recovery operand");
    }
}
__m128 damage_mulss(__m128 lhs, __m128 rhs) {
    asm volatile("mulss %1,%0" : "+x"(lhs) : "x"(rhs));
    return lhs;
}
void child(ObservedDamageState &state,std::uint32_t target) {
    log_call(entry_hash(state,target));
    state.gp[1] ^= 0x81010000u;
    state.gp[2] ^= 0x24ab7910u;
    state.flags.plus(state.gp[2],state.gp[1],4);
    if (target == 0x1509b0u) {
        state.gp[0] = policy == 2u ? 0xffffffffu : 0x20006000u;
        state.xmm[2] = _mm_set_ss(0.4f);
        state.xmm[3] = _mm_set_ss(0.2f);
    }
    if (policy == 14u && target == 0x13d1c8u) throw std::runtime_error("Recovery recipient fault");
    if (policy == 15u && target == 0xb1c30u) throw std::runtime_error("Recovery notification fault");
    state.pop();
    if (target != 0x1509b0u) state.gp[4] += 4u;
}
void run_14f5d0(ObservedDamageState &state) { child(state,0x1509b0u); }
void run_13be58(ObservedDamageState &state) { child(state,0x13d1c8u); }
void run_a87e0(ObservedDamageState &state) { child(state,0xb1c30u); }
#include "damage_shield_recovery_reference.inc"
}

namespace {
struct Input {
    Case caller;
    kernel::Entry start;
};

Input author(std::mt19937 &random, unsigned index) {
    const auto variant = index / 2u % 23u;
    Input input{{},index & 1u ? kernel::Entry::recover : kernel::Entry::evaluate};
    auto &test = input.caller;
    const auto stack = variant == 21u ? 0x20002004u : 0x20002080u;
    random_abi(random,test,stack);
    test.policy = variant;
    auto &memory = test.memory;
    constexpr auto object = 0x20004000u, definition = 0x20006000u, settings = 0x20008000u;
    test.abi.gp[5] = object;
    test.abi.gp[3] = variant == 5u ? 0u : definition;
    test.abi.gp[6] = variant == 6u ? 1u : 2u;
    test.abi.xmm[2][0] = 0x3f000000u;
    memory.put(stack-8u,0u);
    memory.put(stack+0x13u,0u,1u);
    memory.put(stack+0x20u,0xa5000000u);
    memory.put(stack+0x24u,0x15cd0000u);
    memory.put(object+0xf0u,(variant >= 3u && variant <= 7u) ? 0x3e800000u : 0x3f800000u);
    memory.put(object+0x104u,variant == 3u ? 2u : 0u,2u);
    memory.put(object+0x10bu,2u,1u);
    memory.put(definition+0x6cu,0x3f200000u);
    memory.put(0x51df54u,settings);
    memory.put(settings+2u,variant == 1u || variant == 2u ? 5u : variant == 8u ? 0u : 1u,2u);
    memory.put(0x46a11cu,0x3f800000u);
    memory.put(0x46a370u,variant == 9u ? 0x7fc0002au : 0x3f800000u);
    if (variant == 10u) memory.drop(object);
    if (variant == 11u) memory.drop(settings);
    if (variant == 12u) memory.drop(stack);
    if (variant == 13u) memory.drop(0x51df54u);
    if (variant == 21u) memory.drop(stack-8u);
    if (variant == 22u) memory.drop(0x46a370u);
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
        if ((test.policy == 16u && event.instruction == 0xd873du) ||
            (test.policy == 17u && event.instruction == 0xd8751u) ||
            (test.policy == 18u && event.instruction == 0xd8797u) ||
            (test.policy == 19u && event.instruction == 0xd879cu))
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
        outcome.next = native ? kernel::bridge(state,input.start,original::native_damage_update_shield_recovery_reference_d73e0,
            [](auto &value,auto target){original::child(value,target);},original::damage_mulss) : original::native_damage_update_shield_recovery_reference_d73e0_live(state,input.start);
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
    set_mode(mode,"HALO2_VERIFY_DAMAGE_UPDATE_SHIELD_RECOVERY");
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
