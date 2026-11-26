#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_damage_update_return.hpp"

using namespace frame_misc_harness;
namespace entry = halo2::native_damage_update_return_detail;
using entry::Entry;
Entry current_entry{};

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
    if (key == 0xd7c3du) return 0xd8b8du;
    throw std::runtime_error("Unexpected map operand");
}
void run_da150(ObservedDamageState &state) {
    log_call(entry_hash(state,0xdb0a0u));
    state.gp[0] ^= 0x8f3187e9u;
    state.gp[1] ^= 0x81010000u;
    state.gp[2] ^= 0x24ab7910u;
    state.flags.plus(state.gp[2],state.gp[1],4);
    state.xmm[7] = _mm_set_ps(7.0f, 6.0f, 5.0f, 4.0f);
    if (policy == 5u) throw std::runtime_error("Damage notification child fault");
    state.pop();
    state.gp[4] += 4u;
}
#include "damage_update_refs.inc"
}

namespace {
Case author(std::mt19937 &random, unsigned index) {
    Case test;
    const auto variant = index / 3u % 13u;
    current_entry = static_cast<Entry>(index % 3u);
    const auto stack = variant == 12u ? 0xfffffffcu : variant <= 1u ? 0x20002ff8u : 0x20002080u;
    random_abi(random,test,stack);
    auto &memory = test.memory;
    for (unsigned offset = 0; offset <= 0x90u; offset += 4u) memory.put(stack+offset,random());
    if (variant == 1u) memory.drop(stack+8u);
    if (variant == 2u) memory.drop(stack);
    if (variant == 3u && current_entry == Entry::notify) {
        test.abi.gp[4] = 0x20002000u;
        memory.drop(0x20001000u);
    }
    test.policy=variant;
    return test;
}

Result run(Memory &memory, const Case &test, bool native) {
    Result result;
    SourcePhysicsContactRecordObserver observe = [&](const auto &event) {
        result.stores.push_back(event);
        if (test.policy>=6u && test.policy<=8u &&
            event.instruction==std::array{0xd8b6du,0xd8b87u,0xd8b88u}[test.policy-6u])
            throw std::runtime_error("Damage return observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (native) {
        services.instruction = [](auto) { throw std::runtime_error("Damage return ran a literal callback"); };
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
        if (native) result.admitted = entry::bridge(state,current_entry,original::native_damage_update_return_reference_d73e0,
            original::run_da150);
        else original::native_damage_update_return_reference_d73e0_live(state,current_entry);
    } catch (const std::exception &error) { result.fault = error.what(); }
    call_log = nullptr;
    result.abi = state.save();
    result.mxcsr = _mm_getcsr();
    return result;
}
}

int main(int argc, char **argv) {
    const auto count = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode,"HALO2_VERIFY_DAMAGE_UPDATE_RETURN");
    std::mt19937 random(0xd8330u);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = author(random,index);
        auto a = test.memory.clone(), b = test.memory.clone();
        const auto n = run(a,test,true), r = run(b,test,false);
        if (n.fault != r.fault ||
            n.calls != r.calls || n.stores != r.stores || !a.same(b) ||
            !same_abi(n.abi,r.abi) || n.mxcsr != r.mxcsr ||
            (!census_mode && n.cursor != r.cursor)) {
            std::fprintf(stderr,"case %u %s: entry %u PC %x/%x; faults '%s'/'%s'\n",
                index,mode.c_str(),unsigned(current_entry),
                n.cursor,r.cursor,n.fault.c_str(),r.fault.c_str());
            return 1;
        }
        ++totals.compared;
        totals.stores += r.stores.size();
        totals.children += r.calls.size();
        totals.faults += !r.fault.empty();
    }
    const auto covered = entry::coverage().load();
    if (covered != ((1u << static_cast<unsigned>(Entry::count))-1u) || !totals.faults) return 2;
    report(count,totals,covered,static_cast<unsigned>(Entry::count),mode);
}
