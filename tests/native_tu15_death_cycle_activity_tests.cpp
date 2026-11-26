#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_death_cycle_activity.hpp"

#include <set>

namespace {
using namespace frame_misc_harness;
namespace native_activity = halo2::native_death_cycle_activity;
using namespace halo2::native_object_tick_detail::boundary;

constexpr std::uint32_t object_system = 0x10000000u, object_slots = 0x10001000u;
constexpr std::uint32_t object = 0x10002000u, tag = 0x10003000u, motion_records = 0x10004000u;
constexpr std::uint32_t direct_motion = 1u, reject_child = 2u;
constexpr std::uint32_t stage_begin = 0x103657u, stage_end = 0x1037e7u;
constexpr std::array modes{0x1037d3u, 0x103d68u, 0x103e32u, 0x103ed4u, 0x1040f0u, 0x103f1du, 0x104006u, 0x103ed4u};
constexpr std::array states{0x1037e7u, 0x10387cu, 0x103a6du, 0x103d0fu, 0x1038f5u, 0x103994u};

std::uint32_t canonical_branch(std::uint32_t address, SourcePhysicsContactImage) { return address; }

void run_102d50(WorldState &s) {
    const auto sp = s.gp[esp];
    const auto index = r(s.memory, sp+8u, 4);
    if (r(s.memory, sp, 4) != 0x10372bu || r(s.memory, sp+4u, 4) != 0x80240003u || index >= 4u)
        throw std::runtime_error("Incorrect activity CALL frame");
    log_call(mix(entry_hash(s, 0x102d50u), index));
    if (policy & reject_child) throw std::runtime_error("Activity child rejected entry");
    s.gp[eax] = (policy >> ((index & 3u)+3u)) & 0xffu;
    s.gp[ecx] ^= 0x2ef3b3a4u;
    s.gp[edx] ^= 0x434fc618u;
    s.flags.logic(low_byte(s.gp[eax]), 1);
    (void)s.pop();
    s.gp[esp] += 8u;
}

#include "death_activity_reference.inc"

bool native(WorldState &s) {
    if (!(policy & direct_motion)) {
        const auto next = native_activity::start(s, run_102d50);
        if (next == native_activity::ActivityExit::retained) return false;
        if (next == native_activity::ActivityExit::next_record) {
            s.instruction_literal(0x10412eu);
            return true;
        }
    }
    const auto next = native_activity::motion(s, canonical_branch);
    if (!next) return false;
    s.instruction_literal(next);
    return true;
}

Case fixture(std::mt19937 &random, unsigned index) {
    Case test;
    const auto sp = 0x20010400u+(index & 3u);
    random_abi(random, test, sp);
    test.policy = (random() & 0x7f8u) | ((index >> 3u) & direct_motion);
    auto &m = test.memory;
    for (auto at = sp-0x20u; at <= sp+0x70u; at += 4u) m.put(at, random());
    test.abi.gp[ebp] = object;
    test.abi.gp[edi] = tag;
    test.abi.gp[eax] = index % 3u;
    m.put(sp+0x16u, (index >> 4u) & 1u, 1);
    m.put(sp+0x24u, 3u*12u);
    m.put(sp+0x2cu, tag);
    m.put(sp+0x4cu, 0x80240003u);
    m.put(0x4ecbd0u, object_system);
    m.put(object_system+0x44u, object_slots);
    m.put(object_slots+3u*12u+8u, object);
    m.put(object+0x16cu, random() & 0xffu, 1);
    m.put(object+0x16eu, random() & 0x1ffu, 2);
    m.put(object+0x174u, random() & 0xffu, 1);
    m.put(object+0x17cu, (index % 4u)==1u ? 0xffffu : (index & 3u), 2);
    m.put(object+0x188u, random());
    m.put(tag+0x2c0u, index % 4u);
    m.put(tag+0x2c8u, (index % 7u)==1u ? 0u : 3u);
    m.put(tag+0x2ccu, motion_records);
    constexpr std::array mode_inputs{0u, 0u, 0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 0xffu};
    constexpr std::array state_inputs{0u, 1u, 2u, 3u, 4u, 5u, 6u, 0xffffu};
    for (unsigned i=0; i<3; ++i) {
        const auto data = object+12u*i;
        const auto motion = motion_records+64u*i;
        m.put(data+0x20cu, mode_inputs[index % mode_inputs.size()], 1);
        m.put(data+0x20eu, (index % 3u)==0u ? 4u : 0u, 2);
        m.put(data+0x210u, (index % 5u)==2u ? 0x80u : 0u, 1);
        m.put(motion+4u, i, 2);
        m.put(motion+6u, state_inputs[(index / mode_inputs.size()) % state_inputs.size()], 2);
    }
    for (unsigned i=0; i<modes.size(); ++i) m.put(0x10464cu+4u*i,modes[i]);
    for (unsigned i=0; i<states.size(); ++i) m.put(0x10466cu+4u*i,states[i]);
    return test;
}
}

int main() {
    constexpr unsigned cases = 1536;
    std::mt19937 random{0x103657u};
    Totals totals;
    std::set<std::uint32_t> reached, exits;
    for (unsigned index=0; index<cases; ++index) {
        auto test = fixture(random, index);
        test.full_observation = true;
        auto input = test.memory.clone();
        const auto baseline = execute(input, test, false, native, reference_activity);
        if (!baseline.fault.empty() || baseline.instructions.empty()) {
            std::fprintf(stderr, "Invalid original activity case %u at %x: %s\n", index,
                         baseline.cursor, baseline.fault.c_str());
            return 1;
        }
        exits.insert(baseline.cursor);
        for (auto pc : baseline.instructions)
            if (pc>=stage_begin && pc<stage_end) reached.insert(pc);
        const auto &store = baseline.stores[index % baseline.stores.size()];
        switch ((index / 3u) % 10u) {
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
            case 7: test.policy |= reject_child; break;
            case 8: {
                const auto table = (index & 1u) ? 0x10464cu : 0x10466cu;
                const auto invalid = (index & 1u) ? states[0] : modes[1];
                const unsigned count = (index & 1u) ? modes.size() : states.size();
                for (unsigned slot=0; slot<count; ++slot) test.memory.put(table+4u*slot, invalid);
                break;
            }
            case 9: test.memory.drop(0x10464cu); break;
        }
        if (!compare(index, test, native, reference_activity, totals, false, {}, true)) return 1;
    }
    report(cases, totals, reached.size(), exits.size(), "quiet-and-observed");
    for (auto pc : reached) std::fprintf(stderr, "%x ", pc);
    return !totals.faults || reached.size() != 108 || exits.size() != 13;
}
