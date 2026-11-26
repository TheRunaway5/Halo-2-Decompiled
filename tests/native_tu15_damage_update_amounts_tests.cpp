#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_damage_update_amounts.hpp"

using namespace frame_misc_harness;
namespace kernel = halo2::native_damage_update_amounts_detail;

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
    if (key == 0x518444u) return 0x51df54u;
    if (key == 0x46507cu) return 0x46a11cu;
    if (key == 0x465044u) return 0x46a0e4u;
    throw std::runtime_error("Unexpected damage section operand");
}
__m128 damage_mulss(__m128 lhs, __m128 rhs) {
    asm volatile("mulss %1,%0" : "+x"(lhs) : "x"(rhs));
    return lhs;
}
__m128 damage_addss(__m128 lhs, __m128 rhs) {
    asm volatile("addss %1,%0" : "+x"(lhs) : "x"(rhs));
    return lhs;
}
#include "damage_amounts_reference.inc"
}

namespace {
struct Input {
    Case caller;
    kernel::Entry start;
};

Input author(std::mt19937 &random, unsigned index) {
    const auto variant = index / 4u % 21u;
    Input input{{},static_cast<kernel::Entry>(index%4u)};
    auto &test = input.caller;
    const auto stack = variant == 20u ? 0x20002fe8u : 0x20002080u;
    random_abi(random,test,stack);
    test.policy = variant;
    auto &memory = test.memory;
    constexpr auto object = 0x20004000u, definition = 0x20006000u, settings = 0x20008000u;
    test.abi.gp[5] = object;
    test.abi.gp[3] = variant == 5u ? 0u : definition;
    memory.put(stack+0x13u,0x88u,1u);
    memory.put(stack+0x1cu,0x158d0000u);
    memory.put(object+0xf0u,variant == 0u ? 0x40000000u :
        variant == 6u ? 0x7fc11001u : variant == 7u ? 0x7f811001u : 0u);
    memory.put(object+0xe4u,variant == 2u ? 0u :
        variant == 3u ? 0x7fc12002u : 0x3f800000u);
    memory.put(object+0x10au,variant == 4u ? 0x14u : 0x10u,1u);
    memory.put(object+0x10bu,2u,1u);
    memory.put(definition+0x14u,0x3f000000u);
    memory.put(definition+0x68u,0x3f800000u);
    memory.put(0x51df54u,settings);
    memory.put(settings+2u,variant == 8u ? 0u : variant == 9u ? 0xfffdu : 3u,2u);
    memory.put(0x46a11cu,0x3f800000u);
    memory.put(0x46a0e4u,0x3f800000u);
    if (variant == 10u) memory.drop(object);
    if (variant == 11u) memory.drop(settings);
    if (variant == 12u) memory.drop(stack);
    if (variant == 13u) memory.drop(definition);
    if (variant == 14u) memory.drop(0x46a0e4u);
    if (variant == 15u) memory.drop(0x51df54u);
    if (variant == 20u) memory.drop(stack+0x1cu);
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
        if ((test.policy == 16u && event.instruction == 0xd8681u) ||
            (test.policy == 17u && event.instruction == 0xd86e4u) ||
            (test.policy == 18u && event.instruction == 0xd86d9u) ||
            (test.policy == 19u && event.instruction == 0xd869au))
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
    try {
        outcome.next = native ? kernel::bridge(state,input.start,original::native_damage_update_amounts_reference_d73e0,
            original::damage_mulss,original::damage_addss) : original::native_damage_update_amounts_reference_d73e0(state,input.start);
    } catch (const std::exception &error) { result.fault = error.what(); }
    result.abi = state.save();
    result.mxcsr = _mm_getcsr();
    return outcome;
}
}

int main(int argc, char **argv) {
    const auto count = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode,"HALO2_VERIFY_DAMAGE_UPDATE_AMOUNTS");
    std::mt19937 random(0xd8a56u);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = author(random,index);
        auto a = test.caller.memory.clone(), b = test.caller.memory.clone();
        const auto native = execute_case(a,test,true), reference = execute_case(b,test,false);
        const auto &n = native.state, &r = reference.state;
        if (native.next != reference.next || n.fault != r.fault || n.stores != r.stores || !a.same(b) ||
            !same_abi(n.abi,r.abi) || n.mxcsr != r.mxcsr || (!census_mode && n.cursor != r.cursor)) {
            std::fprintf(stderr,"case %u %s: terminal %u/%u PC %x/%x; faults '%s'/'%s'\n",
                index,mode.c_str(),unsigned(native.next),unsigned(reference.next),n.cursor,r.cursor,n.fault.c_str(),r.fault.c_str());
            return 1;
        }
        ++totals.compared;
        totals.stores += r.stores.size();
        totals.faults += !r.fault.empty();
    }
    const auto covered = kernel::coverage().load();
    if (covered != ((1u << kernel::branch_count)-1u) || !totals.faults) return 2;
    report(count,totals,covered,kernel::branch_count,mode);
}
