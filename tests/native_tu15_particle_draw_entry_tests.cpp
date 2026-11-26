#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_particle_draw_entry.hpp"

using namespace frame_misc_harness;
namespace entry = halo2::native_particle_draw_entry_detail;

namespace original {
constexpr std::uint32_t array = 0x10001000u, slots = 0x10003000u, tags = 0x10005000u;
constexpr std::uint32_t element = 0x10007000u, accessor = 0x10009000u, table = 0x1000b000u;
constexpr std::uint32_t selected = 0x1000d000u, description = 0x1000f000u;
constexpr std::uint32_t group = 0x10011000u, datum = 0x10013000u, data = 0x10015000u;
constexpr std::uint32_t method = 0x22ace0u;

void dispatch(WorldState &state, std::uint32_t target, std::uint32_t pc, bool tail) {
    log_call(entry_hash(state, target) ^ pc);
    if (tail) throw std::runtime_error("Unexpected draw tail");
    switch (target) {
    case 0x178070u:
        state.gp[0] = element;
        state.gp[1] ^= 0x10000u;
        if (policy & 16u) state.gp[5] += 1u;
        break;
    case 0x13a6b0u:
        state.gp[0] = (pc == 0x1772e7u && (policy & 1u)) ? 0u : accessor;
        state.gp[2] ^= 0x20000u;
        break;
    case method:
        state.gp[0] = selected;
        state.gp[2] ^= 0xc31bc000u;
        state.xmm[7] = _mm_set_ps(6, -4, 2, -1);
        if (policy & 32u) throw std::runtime_error("Draw virtual child fault");
        break;
    default: throw std::runtime_error("Unexpected draw dispatch target");
    }
    state.flags.plus(pc, target, 4);
    state.pop();
}
#include "body.inc"
#include "reference.inc"
} // namespace original

namespace {
using namespace original;

Case author(std::mt19937 &random, unsigned index) {
    Case test;
    const auto stack = 0x20002100u - 4u * (index % 4u);
    random_abi(random, test, stack);
    auto &memory = test.memory;
    const auto mode = index % 20u;
    test.policy = (mode == 0u ? 1u : 0u) | (index & 16u) | (mode == 9u ? 32u : 0u);
    const auto tag = mode == 3u ? 0xffffu : 7u;
    const auto tag_group = mode == 2u ? 0x5052544du : mode == 4u ? 0x0u : 0x70727433u;
    memory.put(0x51df74u, array);
    memory.put(array + 0x44u, slots);
    memory.put(0x4f0414u, tags);
    for (unsigned offset = 0; offset < 0xf0u; offset += 4u)
        memory.put(stack - 0xb0u + offset, static_cast<std::uint32_t>(random()));
    memory.put(stack + 4u, 2u | (std::uint32_t(random()) << 16u));
    memory.put(element + 4u, tag | (std::uint32_t(random()) << 16u));
    memory.put(element + 0x16u, mode == 5u ? 0x20u : 0u, 1);
    memory.put(accessor, table);
    memory.put(table + 0x28u, method);
    memory.put(tags + 16u * std::uint32_t(std::int32_t(std::int16_t(tag))), tag_group);
    memory.put((tags + 16u * tag) + 8u, description);
    memory.put(description + 0x24u, selected);
    memory.put(selected, 15u | (std::uint32_t(random()) << 16u));
    memory.put(tags + 15u * 16u + 8u, group);
    memory.put(group + 0x5cu, datum);
    memory.put(datum + 4u, data);
    memory.put(data + 2u, static_cast<std::uint32_t>(random()));
    if (mode == 10u) memory.drop(array);
    if (mode == 11u) memory.drop(element);
    if (mode == 12u) memory.drop(table);
    if (mode == 13u) memory.drop(group);
    if (mode == 14u) memory.drop(datum);
    if (mode == 15u) memory.drop(data);
    if (mode == 16u) memory.drop(0x4f0414u);
    if (mode == 17u) memory.drop(stack);
    if (mode == 18u) memory.put(table + 0x28u, 0u);
    if (mode == 19u) test.policy |= 256u;
    return test;
}

struct DrawResult {
    Result state;
    std::uint32_t continuation{};
};

DrawResult execute_draw(Memory &memory, const Case &test, bool native) {
    DrawResult outcome;
    auto &result = outcome.state;
    SourcePhysicsContactRecordObserver observe = [&](const auto &event) {
        result.stores.push_back(event);
        if ((test.policy & 256u) && event.instruction == 0x17736cu)
            throw std::runtime_error("Draw late store observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (native) {
        services.instruction = [](auto) { throw std::runtime_error("Draw entry ran a literal callback"); };
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
    WorldState state{memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(test.abi);
    policy = test.policy;
    call_log = &result.calls;
    try {
        outcome.continuation = native
            ? entry::bridge(state, native_particle_draw_reference_1772c0, [](auto &child, auto target, auto site) {
                  original::dispatch(child, target, site, false);
              })
            : retained_particle_draw_entry(state);
    } catch (const std::exception &error) { result.fault = error.what(); }
    call_log = nullptr;
    result.mxcsr = _mm_getcsr();
    result.abi = state.save();
    return outcome;
}
} // namespace

int main(int argc, char **argv) {
    const auto count = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode, "HALO2_VERIFY_PARTICLE_DRAW_ENTRY");
    std::mt19937 random(0x1772c0u);
    Totals totals;
    for (unsigned index = 0; index < count; ++index) {
        const auto test = author(random, index);
        auto a = test.memory.clone(), b = test.memory.clone();
        const auto native = execute_draw(a, test, true), retained = execute_draw(b, test, false);
        const auto &n = native.state;
        const auto &r = retained.state;
        if (native.continuation != retained.continuation ||
            n.fault != r.fault || n.calls != r.calls || n.stores != r.stores || !a.same(b) ||
            !same_abi(n.abi, r.abi) || n.mxcsr != r.mxcsr || (!census_mode && n.cursor != r.cursor)) {
            std::fprintf(stderr, "case %u %s: terminal %x/%x, PC %x/%x, %zu/%zu stores, faults '%s'/'%s'\n",
                index, mode.c_str(), native.continuation, retained.continuation, n.cursor, r.cursor,
                n.stores.size(), r.stores.size(), n.fault.c_str(), r.fault.c_str());
            return 1;
        }
        ++totals.compared;
        totals.stores += r.stores.size();
        totals.children += r.calls.size();
        totals.faults += !r.fault.empty();
    }
    const auto covered = entry::coverage().load();
    if (covered != ((1u << entry::branch_count) - 1u) || !totals.faults || !totals.children) return 1;
    report(count, totals, covered, entry::branch_count, mode);
}
