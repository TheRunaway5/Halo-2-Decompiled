#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_death_cycle_expiration.hpp"

#include <set>

namespace {
using namespace frame_misc_harness;
namespace expiration = halo2::native_death_cycle_expiration;
using namespace halo2::native_object_tick_detail::boundary;

constexpr std::uint32_t object = 0x10002000u, tag = 0x10003000u, records = 0x10004000u;
constexpr std::uint32_t clock_base = 0x10005000u, table = 0x1046a4u;

std::uint32_t canonical_branch(std::uint32_t address, SourcePhysicsContactImage) { return address; }

#include "death_expiration_reference.inc"

bool native(WorldState &s) {
    constexpr std::array entries{expiration::Entry::finish_motion, expiration::Entry::begin_records,
                                 expiration::Entry::expire_record};
    const auto route = expiration::run(s, entries[policy], canonical_branch);
    if (!route) return false;
    s.instruction_literal(route);
    return true;
}

Case fixture(std::mt19937 &random, unsigned index) {
    constexpr std::array timer_values{0x3f800000u, 0x40000000u, 0x00000000u, 0x80000000u,
                                     0x00000001u, 0x00800000u, 0xbf800000u, 0x7fc12345u,
                                     0xff812345u, 0x7f800000u};
    constexpr std::array rates{0x3f000000u, 0x40800000u, 0xbf000000u, 0u,
                               0x00000001u, 0x7fc54321u, 0x7f854321u, 0x7f800000u};
    constexpr std::array handlers{0x104204u, 0x1042a6u, 0x10434cu, 0x104360u};
    Case test;
    const auto sp = 0x20010400u+(index & 3u);
    random_abi(random, test, sp);
    test.abi.xmm[1][0] = index % 4u == 0u ? timer_values[random() % timer_values.size()] : 0u;
    test.abi.gp[ebp] = object;
    test.abi.gp[edi] = tag;
    test.abi.gp[ebx] = 0u;
    test.policy = index % 3u;
    auto &m = test.memory;
    for (auto at = sp-0x10u; at <= sp+0x50u; at += 4u) m.put(at, random());
    m.put(sp+0x14u, (index >> 4u) & 1u, 1);
    m.put(sp+0x15u, (index >> 5u) & 1u, 1);
    m.put(sp+0x20u, (index / 3u) % 4u);
    m.put(sp+0x28u, index % 4u);
    m.put(sp+0x2cu, tag);
    m.put(sp+0x34u, object);
    m.put(object+0x20cu, (index >> 3u) & 1u, 1);
    m.put(tag+0x2c8u, (index & 1u) ? 1u : 5u);
    m.put(tag+0x2d0u, (index % 7u)==0u ? 0u : 4u);
    m.put(tag+0x2d4u, records);
    m.put(0x51df54u, clock_base);
    m.put(clock_base+4u, 0x3f800000u);
    for (unsigned i=0; i<4; ++i) {
        const auto app = object+0x1a4u+0x34u*i;
        const auto record = records+0xecu*i;
        m.put(app+1u, (index / 3u) % 7u, 1);
        m.put(app+2u, (index & 1u) ? 3u : 0x8000u, 2);
        m.put(app+0x14u, timer_values[random() % timer_values.size()]);
        m.put(app+0x18u, timer_values[random() % timer_values.size()]);
        m.put(record+0xccu, rates[random() % rates.size()]);
        m.put(record+0xd0u, rates[random() % rates.size()]);
    }
    for (unsigned i=0; i<handlers.size(); ++i) m.put(table+4u*i, handlers[i]);
    return test;
}
}

int main() {
    constexpr unsigned cases = 1536;
    std::mt19937 random{0x1040f0u};
    Totals totals;
    std::set<std::uint32_t> reached, exits;
    for (unsigned index=0; index<cases; ++index) {
        auto test = fixture(random, index);
        test.full_observation = true;
        auto input = test.memory.clone();
        const auto baseline = execute(input, test, false, native, reference_expiration);
        if (!baseline.fault.empty() || baseline.stores.empty()) {
            std::fprintf(stderr, "Invalid expiration case %u at %x: %s\n", index,
                         baseline.cursor, baseline.fault.c_str());
            return 1;
        }
        exits.insert(baseline.cursor);
        for (auto pc : baseline.instructions)
            if (pc>=0x1040f0u && pc<0x104204u) reached.insert(pc);
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
            case 6: test.memory.drop(clock_base); break;
            case 7: test.memory.drop(table); break;
            case 8:
                for (unsigned slot=0; slot<4; ++slot) test.memory.put(table+4u*slot,0x103753u);
                break;
            case 9: test.memory.drop(records); break;
        }
        if (!compare(index, test, native, reference_expiration, totals, false, {}, true)) return 1;
    }
    report(cases, totals, reached.size(), exits.size(), "quiet-and-observed");
    for (auto pc : reached) std::fprintf(stderr, "%x ", pc);
    return !totals.faults || reached.size() != 69 || exits.size() != 7;
}
