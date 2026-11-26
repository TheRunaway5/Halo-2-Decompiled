#include "native_tu15_frame_misc_harness.hpp"
#include "native_tu15_world_leaves_remap.hpp"
#include "physics/runtime/native_tu15_swept_support_world.hpp"

using namespace frame_misc_harness;
namespace owner = halo2::native_swept_support_world_detail;

namespace original {
struct ObservedWorldState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs{};
};
std::uint32_t world_observation_pc(std::uint32_t disc, std::uint32_t tu15, const ObservedWorldState &state) {
    return state.selected_observation_pcs && state.image == SourcePhysicsContactImage::TitleUpdate15 ? tu15 : disc;
}
std::uint32_t selected(std::uint32_t address, SourcePhysicsContactImage) {
    switch (address) {
    case 0x2e22ccu: return 0x2e5dccu;
    case 0x465048u: return 0x46a0e8u;
    default: throw std::runtime_error("Unexpected swept support operand");
    }
}
constexpr std::uint32_t shape = 0x10001000u, direction = 0x10003000u, output = 0x10005000u;
constexpr std::uint32_t child = 0x10007000u, table = 0x10009000u, support = 0x1234abcdu;
void dispatch(ObservedWorldState &state, std::uint32_t target, std::uint32_t site, bool tail) {
    if (site != 0x2e22c9u || tail || target != support) throw std::runtime_error("Unexpected shape support route");
    log_call(entry_hash(state, target));
    auto &gp = state.gp;
    const auto vertex = r(state.memory, gp[4] + 8u, 4);
    for (unsigned lane = 0; lane < 4u; ++lane)
        w(state.memory, state.observe, 0x401008u, vertex + lane * 4u, 4,
            r(state.memory, child + 4u + lane * 4u, 4));
    if (policy & 1u) gp[3] = direction + 0x40u;
    if (policy & 2u) gp[6] = shape + 0x40u;
    if (policy & 4u) gp[7] = output + 0x40u;
    gp[0] = 0x82310000u;
    gp[1] = state.flags.plus(0x899fffffu, 0x90000001u, 4);
    gp[2] ^= 0x3f00u;
    state.xmm[5] = _mm_set_ps(1, -2, 3, -4);
    state.pop();
    gp[4] += 8u;
}
#include "body.inc"
#include "reference.inc"
} // namespace original

namespace {
using original::ObservedWorldState;
using original::shape;
using original::direction;
using original::output;
using original::child;
using original::table;

struct Fixture : Case {
    std::uint32_t refused_write{};
    std::size_t refused_width{};
};

Fixture author(std::mt19937 &random, unsigned index) {
    Fixture test;
    auto &memory = test.memory;
    const auto stack = 0x200020d0u - 4u * (index % 4u);
    random_abi(random, test, stack);
    test.abi.gp[1] = shape;
    const auto scenario = index % 16u;
    test.policy = (index / 16u) % 8u;
    if (scenario < 4u || scenario >= 11u) test.policy = 0;
    for (unsigned offset = 0; offset < 0x100u; offset += 4u) {
        memory.put(stack - 0x80u + offset, static_cast<std::uint32_t>(random()));
        memory.put(shape + offset, static_cast<std::uint32_t>(random()));
        memory.put(direction + offset, static_cast<std::uint32_t>(random()));
        memory.put(output + offset, static_cast<std::uint32_t>(random()));
    }
    memory.put(stack, static_cast<std::uint32_t>(random()));
    memory.put(stack + 4u, direction);
    memory.put(stack + 8u, scenario == 7u ? shape + 0x10u : output);
    memory.put(shape + 0x30u, child);
    memory.put(child, table);
    memory.put(table + 0x30u, scenario == 8u ? 0u : original::support);
    memory.put(0x46a0e8u, 0);
    std::mt19937_64 bits(random());
    for (unsigned lane = 0; lane < 4u; ++lane) {
        const bool edge = scenario == 4u || scenario == 5u;
        const auto sweep = scenario == 1u ? u(-1.0f) : scenario == 2u ? 0u : scenario == 3u ? 0x7f800001u : u(1.0f);
        memory.put(shape + 0x10u + lane * 4u, edge ? special_float(bits) : sweep);
        memory.put(shape + 0x50u + lane * 4u, edge ? special_float(bits) : sweep);
        memory.put(direction + lane * 4u, edge ? special_float(bits) : u(1.0f));
        memory.put(direction + 0x40u + lane * 4u, edge ? special_float(bits) : 0u);
        memory.put(child + 4u + lane * 4u, lane == 3u ? special_float(bits) : u(float(lane + 1u)));
    }
    if (scenario == 9u) memory.drop(0x46a0e8u);
    if (scenario == 10u) memory.drop(table);
    if (scenario == 11u) memory.drop(direction);
    if (scenario == 12u) { test.refused_write = output; test.refused_width = 8; }
    if (scenario == 13u) { test.refused_write = output + 8u; test.refused_width = 8; }
    if (scenario == 14u) { test.refused_write = output + 12u; test.refused_width = 4; }
    if (scenario == 15u) memory.drop(shape);
    return test;
}

struct FaultMemory : Memory {
    explicit FaultMemory(const Fixture &test) : Memory(test.memory.clone()),
        address(test.refused_write), width(test.refused_width) {}
    std::uint32_t address;
    std::size_t width;
    unsigned attempts{};
    void write(std::uint32_t target, std::span<const std::byte> data) override {
        // The child writes the tag once before the parent's final tag store.
        if (target == address && data.size() == width && ++attempts == (width == 4u ? 2u : 1u))
            throw std::runtime_error("Authored sweep store failure");
        Memory::write(target, data);
    }
};

bool native(ObservedWorldState &state) {
    return owner::bridge(state, original::native_swept_support_reference_2e22b0, [](auto &child_state, auto target, auto site) {
        original::dispatch(child_state, target, site, false);
    });
}

Result execute(FaultMemory &memory, const Fixture &test, bool run_native) {
    Result result;
    SourcePhysicsContactRecordObserver observe = [&](const auto &event) { result.stores.push_back(event); };
    SourcePhysicsWorldRuntimeServices services, outer;
    if (run_native) {
        services.instruction = [](auto) { throw std::runtime_error("Unexpected translated swept support instruction"); };
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
    leaves_remap::wrap(services, outer);
    physics_step_detail::FpScope host(*test.abi.floating_point);
    ObservedWorldState state{memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(test.abi);
    state.selected_observation_pcs = !leaves_remap::enabled;
    policy = test.policy;
    call_log = &result.calls;
    try {
        if (run_native) result.admitted = native(state);
        else original::run_2e22b0(state);
    } catch (const std::exception &error) { result.fault = error.what(); }
    call_log = nullptr;
    result.abi = state.save();
    result.mxcsr = _mm_getcsr();
    return result;
}
} // namespace

int main(int argc, char **argv) {
    const auto count = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(leaves_remap::parse(mode), "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::mt19937 random(0x2e5db0u);
    Totals totals;
    unsigned coverage = 0;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = author(random, index);
        FaultMemory candidate(test), retained(test);
        const auto n = execute(candidate, test, true), r = execute(retained, test, false);
        if ((n.admitted || !n.fault.empty()) && n.fault == r.fault && candidate.same(retained) &&
            n.stores == r.stores && n.calls == r.calls && (census_mode || n.cursor == r.cursor) &&
            same_abi(n.abi, r.abi) && n.mxcsr == r.mxcsr) {
            ++totals.compared;
            totals.stores += r.stores.size();
            totals.children += r.calls.size();
            totals.faults += !r.fault.empty();
            if (r.fault.empty()) {
                const auto tag_store = leaves_remap::enabled ? 0x2e2314u : 0x2e5e14u;
                const bool advanced = std::any_of(r.stores.begin(), r.stores.end(), [&](const auto &store) {
                    return store.instruction == tag_store;
                });
                coverage |= advanced ? 1u : 2u;
            }
        } else {
            std::fprintf(stderr, "case %u %s: faults '%s'/'%s', cursors %x/%x, stores %zu/%zu, ABI %x/%x\n",
                index, mode.c_str(), n.fault.c_str(), r.fault.c_str(), n.cursor, r.cursor, n.stores.size(),
                r.stores.size(), n.abi.x87_status, r.abi.x87_status);
            return 1;
        }
    }
    if (coverage != 3u || !totals.faults || !totals.children) return 1;
    report(count, totals, coverage, 2, mode);
}
