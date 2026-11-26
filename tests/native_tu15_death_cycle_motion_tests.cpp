#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_death_cycle_motion.hpp"

#include <set>

namespace {
using namespace frame_misc_harness;
namespace motion = halo2::native_death_cycle_motion;
using namespace halo2::native_object_tick_detail::boundary;

constexpr std::uint32_t datum = 0x80240003u, object = 0x10002000u, record = 0x10004000u;
constexpr std::uint32_t object_system = 0x10001000u, object_slots = object_system + 0x500u;
constexpr std::uint32_t destination = 0x1040f0u;

void dispatch(WorldState &s, std::uint32_t target, std::uint32_t site, bool tail) {
    const auto sp = s.gp[esp];
    if (target != 0x107610u || site != 0x10380bu || tail ||
        r(s.memory, sp, 4) != 0x103810u || r(s.memory, sp+4u, 4) != datum ||
        r(s.memory, sp+12u, 4) != 0u)
        throw std::runtime_error("Incorrect motion query CALL frame");
    log_call(entry_hash(s,target));
    if (policy & 4u) throw std::runtime_error("Motion query rejected entry");
    s.set(eax, policy & 1u, 1, 0);
    (void)s.pop();
    s.gp[esp] += 12u;
}

void run_108040(WorldState &s) {
    if (r(s.memory, s.gp[esp], 4) != 0x103877u || s.gp[eax] != datum)
        throw std::runtime_error("Incorrect motion application CALL frame");
    log_call(entry_hash(s, 0x108040u));
    if (policy & 8u) throw std::runtime_error("Motion application rejected entry");
    (void)s.pop();
}

#include "death_motion_reference.inc"

bool native(WorldState &s) {
    if (!motion::run(s, [](auto &state, auto target, auto site) { dispatch(state, target, site, false); },
                    run_108040)) return false;
    s.instruction_literal(destination);
    return true;
}

Case fixture(std::mt19937 &random, unsigned index) {
    Case test;
    const auto sp = 0x20010400u + (index & 3u);
    const auto slot_offset = index % 3u * 12u;
    const auto record_index = (index >> 5u) % 3u;
    random_abi(random, test, sp);
    test.abi.gp[ebp] = object;
    test.abi.gp[ebx] = slot_offset;
    test.abi.gp[esi] = record;
    test.policy = (index >> 1u) & 1u;
    auto &m = test.memory;
    for (auto at = sp - 0x30u; at <= sp + 0x60u; at += 4u) m.put(at, random());
    m.put(sp+0x14u, index & 1u, 1);
    m.put(sp+0x15u, (index >> 2u) & 1u, 1);
    m.put(sp+0x24u, 12u * 3u);
    m.put(sp+0x4cu, datum);
    m.put(0x4ecbd0u, object_system);
    m.put(object_system+0x44u, object_slots);
    m.put(object_slots+12u*3u+8u, object);
    m.put(record+8u, index & 0x10u ? 0xffffu : record_index, 2);
    m.put(object+0x210u+slot_offset, random());
    m.put(object+0x1a8u+0x34u*record_index, random() & 0xffu, 1);
    return test;
}
}

int main() {
    constexpr unsigned cases = 768;
    std::mt19937 random{0x1037e7u};
    Totals totals;
    std::set<std::uint32_t> reached;
    for (unsigned index = 0; index < cases; ++index) {
        auto test = fixture(random, index);
        test.full_observation = true;
        auto input = test.memory.clone();
        const auto baseline = execute(input, test, false, native, reference_motion);
        if (!baseline.fault.empty() || baseline.cursor != destination || baseline.stores.empty()) {
            std::fprintf(stderr, "Invalid motion fixture %u at %x: %s\n", index,baseline.cursor,baseline.fault.c_str());
            return 1;
        }
        for (auto pc : baseline.instructions)
            if (pc != destination) reached.insert(pc);
        const auto &store = baseline.stores[index % baseline.stores.size()];
        switch ((index / 12u) % 9u) {
            case 0: break;
            case 1: test.full_observation = false; break;
            case 2:
                test.full_observation = false;
                test.activate_trace_at_store = store.instruction;
                break;
            case 3: test.reject_after_store_pc = store.instruction; break;
            case 4: test.memory.denied_write = store.address; break;
            case 5: test.reject_instruction = baseline.instructions[index % baseline.instructions.size()]; break;
            case 6: test.memory.drop(object_slots); break;
            case 7: test.policy |= 4u; break;
            case 8: test.policy |= 8u; break;
        }
        if (!compare(index, test, native, reference_motion, totals, false, {}, true)) return 1;
    }
    report(cases, totals, reached.size(), 1, "quiet-and-observed");
    for (auto pc : reached) std::fprintf(stderr, "%x ", pc);
    return !totals.faults || reached.size() != 40 || !totals.children;
}
