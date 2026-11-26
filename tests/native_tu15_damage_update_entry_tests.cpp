#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_damage_update_entry.hpp"

using namespace frame_misc_harness;
namespace entry = halo2::native_damage_update_entry_detail;
using entry::Resume;

namespace original {
struct ObservedDamageState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t damage_observation_pc(std::uint32_t disc, std::uint32_t tu15, const ObservedDamageState &state) {
    return state.selected_observation_pcs && state.image==SourcePhysicsContactImage::TitleUpdate15 ? tu15 : disc;
}
std::uint32_t selected(std::uint32_t key, SourcePhysicsContactImage image) {
    if (image == SourcePhysicsContactImage::MapPack11) return key;
    switch (key) {
    case 0x4e78d0u:return 0x4ecbd0u;
    case 0x4edf18u:return 0x4f3218u;
    case 0x4fcdbcu:return 0x5020bcu;
    case 0xd740du:return 0xd835du;
    case 0xd7160u:return 0xd80a0u;
    default:throw std::runtime_error("Unexpected map operand");
    }
}
constexpr std::uint32_t descriptor = 0x10009000u;
void run_d7160(ObservedDamageState &state) {
    log_call(entry_hash(state,0xd80a0u));
    state.gp[0] = policy % 6u == 1u ? 0u : descriptor;
    state.gp[1] ^= 0x81010000u;
    state.gp[2] ^= 0x24ab7910u;
    state.flags.plus(state.gp[2],state.gp[1],4);
    state.xmm[7] = _mm_set_ps(7.0f, 6.0f, 5.0f, 4.0f);
    if (policy == 11u) throw std::runtime_error("Damage descriptor child fault");
    state.pop();
}
#include "damage_update_refs.inc"
}

namespace {
Case author(std::mt19937 &random, unsigned index) {
    Case test;
    const auto variant = index % 21u;
    const auto stack = variant == 12u ? 0x20002040u : 0x20002080u + 4u * (index % 4u);
    const auto header = 0x10001000u, object_rows = 0x10003000u, object = 0x10005000u, options = 0x10007000u;
    random_abi(random,test,stack);
    auto &memory = test.memory;
    const auto index_word = index & 15u;
    memory.put(stack+4u, 0xafc80000u | index_word);
    if (variant != 12u) memory.put(stack-0x90u,0x4a4820u);
    memory.put(0x4ecbd0u,header);
    memory.put(header+0x44u,object_rows);
    memory.put(object_rows+12u*index_word+8u,object);
    memory.put(object+0xaau, variant % 3u ? 0u : 16u, 1);
    memory.put(object+0x13cu, variant % 4u == 1u ? 0xffffffffu : 9u);
    memory.put(object+0x104u, random() & 0xffffu, 2);
    memory.put(object+0x120u, variant % 5u == 2u ? 0u : (index & 4u ? 0x8012u : 0x60u), 2);
    memory.put(object+0x122u, index & 4u ? 0xffbcu : 0x124u, 2);
    memory.put(0x4f3218u,options);
    memory.put(options+8u,variant % 5u ? 1u : 2u);
    memory.put(0x5020bcu,variant % 7u ? 1u : 0u,1);
    memory.put(original::descriptor+0xbcu,variant % 5u == 1u ? 0x80000001u : 8u);
    if (variant == 13u) memory.drop(header);
    if (variant == 14u) memory.drop(object_rows);
    if (variant == 15u) memory.drop(object);
    if (variant == 16u) memory.drop(options);
    if (variant == 17u) memory.drop(original::descriptor);
    if (variant == 18u) memory.drop(0x5020bcu);
    test.policy=variant;
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
        if ((test.policy == 19u && event.instruction == 0xd836cu) ||
            (test.policy == 20u && event.instruction == 0xd83e4u))
            throw std::runtime_error("Damage entry observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (native) {
        services.instruction = [](auto) { throw std::runtime_error("Damage update ran a literal callback"); };
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
        outcome.continuation = native ? entry::bridge(state,original::native_damage_update_reference_d73e0,
            original::run_d7160,[](auto key) { return original::selected(key,SourcePhysicsContactImage::TitleUpdate15); })
            : original::native_damage_update_reference_d73e0_live(state);
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
    set_mode(mode,"HALO2_VERIFY_DAMAGE_UPDATE_ENTRY");
    std::mt19937 random(0xd8330u);
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
    const auto covered = entry::coverage().load();
    if (covered != ((1u << entry::branch_count)-1u) || !totals.faults) return 2;
    report(count,totals,covered,entry::branch_count,mode);
}
