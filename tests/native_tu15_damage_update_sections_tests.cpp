#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_damage_update_sections.hpp"

using namespace frame_misc_harness;
namespace kernel = halo2::native_damage_update_sections_detail;

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
    throw std::runtime_error("Unexpected damage section operand");
}
__m128 damage_mulss(__m128 lhs, __m128 rhs) {
    asm volatile("mulss %1,%0" : "+x"(lhs) : "x"(rhs));
    return lhs;
}
#include "damage_sections_reference.inc"
}

namespace {
struct Input {
    Case caller;
    kernel::Entry start;
};

Input author(std::mt19937 &random, unsigned index) {
    const auto variant = index / 2u % 23u;
    Input input{{}, index & 1u ? kernel::Entry::pending : kernel::Entry::depletion};
    auto &test = input.caller;
    const auto stack = variant == 21u ? 0x20002fdcu : 0x20002080u;
    random_abi(random,test,stack);
    test.policy = variant;
    auto &memory = test.memory;
    constexpr auto table = 0x20004000u, description = 0x20006000u, effects = 0x20008000u, settings = 0x2000a000u;
    memory.put(stack+0x13u,0u,1u);
    memory.put(stack+0x18u,description);
    memory.put(stack+0x1cu,variant == 20u ? 0xfffffffcu : 0x2000c000u);
    memory.put(stack+0x20u,0u);
    memory.put(stack+0x24u,variant & 2u ? 2u : 0u);
    memory.put(description+0xbcu,variant == 2u ? 0u : 3u);
    memory.put(description+0xc0u,effects);
    memory.put(0x51df54u,settings);
    memory.put(settings+4u,0x3f800000u);
    test.abi.gp[0] = table;
    test.abi.gp[3] = variant == 20u ? 0x7fffffffu : uint32_t(index/46u % 3u);
    test.abi.gp[6] = 0u;
    test.abi.gp[7] = variant % 5u == 1u ? 2u : 1u;
    test.abi.xmm[0][0] = variant == 8u ? 0x7fc10001u : 0x3f800000u;
    test.abi.xmm[1][0] = variant == 4u ? 0x7fc20001u : 0x3f800000u;
    for (unsigned slot = 0; slot != 3u; ++slot) {
        memory.put(table+slot*8u,variant ? (variant == 5u ? 1u : 30u) : 0u,1u);
        memory.put(table+slot*8u+4u,variant == 1u ? 7u : 0u,2u);
        memory.put(effects+slot*0x38u+0x2cu,variant == 3u ? 0x3f000000u :
            variant == 22u ? 0x7f810000u : variant == 9u ? 0x40080000u : 0x40000000u);
    }
    // FISTP of 2.125 toward +infinity produces 3 and sets C1 on the TU1.5 machine path.
    if (variant == 9u) {
        auto &control = test.abi.floating_point->x87_control_word;
        control = (control & ~0xc00u) | 0x800u;
    }
    if (variant == 7u) {
        test.abi.x87_depth = 8;
        for (unsigned lane = 0; lane != 8u; ++lane) test.abi.x87[lane] = static_cast<long double>(lane)+0.25L;
    }
    if (variant == 10u) memory.drop(table);
    if (variant == 11u) memory.drop(description);
    if (variant == 12u) memory.drop(effects);
    if (variant == 13u) memory.drop(settings);
    if (variant == 14u) memory.drop(0x51df54u);
    if (variant == 15u) memory.drop(stack);
    if (variant == 21u) memory.drop(stack+0x24u);
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
        if ((test.policy == 16u && event.instruction == 0xd8aa5u) ||
            (test.policy == 17u && event.instruction == 0xd8ab8u) ||
            (test.policy == 18u && event.instruction == 0xd8ad8u) ||
            (test.policy == 19u && event.instruction == 0xd84eau))
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
        outcome.next = native ? kernel::bridge(state,input.start,original::native_damage_update_sections_reference_d73e0,
            original::damage_mulss) : original::native_damage_update_sections_reference_d73e0(state,input.start);
    } catch (const std::exception &error) { result.fault = error.what(); }
    result.abi = state.save();
    result.mxcsr = _mm_getcsr();
    return outcome;
}
}

int main(int argc, char **argv) {
    const auto count = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode,"HALO2_VERIFY_DAMAGE_UPDATE_SECTIONS");
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
        if (test.start == kernel::Entry::depletion && test.caller.policy == 9u &&
            (!(n.abi.x87_status & 0x200u) || a.get(test.caller.abi.gp[0],1u) != 27u)) {
            std::fprintf(stderr,"case %u lost the TU1.5 rounded integer or C1\n",index);
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
