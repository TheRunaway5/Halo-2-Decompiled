#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_death_cycle_object_timer.hpp"

#include <set>

namespace {
using namespace frame_misc_harness;
namespace timer = halo2::native_death_cycle_object_timer;
using namespace halo2::native_object_tick_detail::boundary;

constexpr std::uint32_t object = 0x10002000u, tag = 0x10003000u, clock_base = 0x10004000u;
constexpr std::uint32_t datum = 0x80240003u, destination = 0x103436u;

void dispatch(WorldState &s, std::uint32_t target, std::uint32_t site, bool tail) {
    const auto sp = s.gp[esp];
    if (tail || r(s.memory, sp, 4) != site+5u)
        throw std::runtime_error("Incorrect original timer return site");
    log_call(mix(entry_hash(s, target), site));
    unsigned stack_bytes = 0;
    switch (target) {
        case 0xfc8d0u:
            if (site != 0x1032f9u || s.gp[edx] != datum)
                throw std::runtime_error("Incorrect parent notification input");
            if (policy & 1u) throw std::runtime_error("Parent notification fault");
            break;
        case 0x10a910u:
            if (site != 0x103313u || s.gp[ecx] != datum)
                throw std::runtime_error("Incorrect object notification input");
            if (policy & 2u) throw std::runtime_error("Object notification fault");
            break;
        case 0x108a70u:
            if (site != 0x1033afu || s.gp[ecx] != datum || r(s.memory, sp+4u, 4) != 0xffffffffu ||
                r(s.memory, sp+8u, 4) != 0 || r(s.memory, sp+12u, 4) != 0)
                throw std::runtime_error("Incorrect direction notification input");
            if (policy & 4u) throw std::runtime_error("Direction notification fault");
            stack_bytes = 12;
            break;
        default:
            throw std::runtime_error("Unknown object timer target");
    }
    s.gp[eax] ^= 0x235af801u;
    s.gp[ecx] ^= 0x6254de13u;
    s.gp[edx] ^= 0x11a8bac2u;
    s.flags.logic(s.gp[eax], 4);
    (void)s.pop();
    s.gp[esp] += stack_bytes;
}

#include "death_object_timer_reference.inc"

bool native(WorldState &s) {
    if (!timer::run(s, [](auto &state, auto target, auto site) { dispatch(state, target, site, false); }))
        return false;
    s.instruction_literal(destination);
    return true;
}

Case fixture(std::mt19937 &random, unsigned index) {
    constexpr std::array amounts{0u, 0x80000000u, 0x3f800000u, 0x40800000u, 0xbf800000u,
                                 1u, 0x00800000u, 0x7fc12345u, 0xff854321u, 0x7f800000u};
    constexpr std::array rates{0u, 0x3e800000u, 0x3f800000u, 0x40800000u, 0xbf800000u,
                               1u, 0x7fc54321u, 0xff823451u};
    Case test;
    const auto sp = 0x20010400u + (index & 3u);
    random_abi(random, test, sp);
    test.abi.gp[ebp] = object;
    test.policy = 0u;
    auto &m = test.memory;
    for (auto at = sp-0x20u; at<=sp+0x54u; at+=4u) m.put(at, random());
    m.put(sp+0x2cu, tag);
    m.put(sp+0x4cu, datum);
    m.put(object+4u, index & 1u ? 0x100u : 0u);
    m.put(object+0x14u, index & 2u ? 0xffffffffu : datum);
    m.put(object+0x16cu, index & 0x30u, 1);
    m.put(object+0x16eu, (index & 0x40u ? 0x80u : 0u) | (index & 0x80u ? 1u : 0u), 1);
    m.put(object+0x19eu, index % 5u ? index % 3u : 0x8000u, 2);
    m.put(object+0x18cu, amounts[random() % amounts.size()]);
    m.put(object+0x190u, amounts[random() % amounts.size()]);
    m.put(tag+0x12cu, index & 4u ? 0x400u : 0u);
    m.put(tag+0x248u, index & 8u ? 0xffffffffu : 0x09000004u);
    m.put(tag+0x250u, index & 8u ? 0x09000006u : 0xffffffffu);
    m.put(tag+0x2d8u, rates[random() % rates.size()]);
    m.put(tag+0x2dcu, rates[random() % rates.size()]);
    m.put(0x51df54u, clock_base);
    m.put(clock_base+4u, index & 1u ? 0x3f800000u : rates[random() % rates.size()]);
    m.put(0x46a0d0u, 0x3f800000u);
    return test;
}
}

int main() {
    constexpr unsigned cases = 1536;
    std::mt19937 random{0x1032a7u};
    Totals totals;
    std::set<std::uint32_t> reached;
    for (unsigned index=0; index<cases; ++index) {
        auto test = fixture(random, index);
        test.full_observation = true;
        auto input = test.memory.clone();
        const auto baseline = execute(input, test, false, native, reference_object_timer);
        if (!baseline.fault.empty() || baseline.cursor != destination) {
            std::fprintf(stderr, "Invalid object timer fixture %u at %x: %s\n", index,
                         baseline.cursor, baseline.fault.c_str());
            return 1;
        }
        for (auto pc : baseline.instructions)
            if (pc != destination) reached.insert(pc);
        const auto *store = baseline.stores.empty() ? nullptr : &baseline.stores[index % baseline.stores.size()];
        switch ((index / 13u) % 11u) {
            case 0: break;
            case 1: test.full_observation = false; break;
            case 2:
                test.full_observation = false;
                if (store) test.activate_trace_at_store = store->instruction;
                break;
            case 3: if (store) test.reject_after_store_pc = store->instruction; break;
            case 4: if (store) test.memory.denied_write = store->address; break;
            case 5: test.reject_instruction = baseline.instructions[index % baseline.instructions.size()]; break;
            case 6: test.memory.drop(clock_base); break;
            case 7: test.memory.drop(0x46a0d0u); break;
            case 8: test.policy = 1u; break;
            case 9: test.policy = 2u; break;
            case 10: test.policy = 4u; break;
        }
        if (!compare(index, test, native, reference_object_timer, totals, false, {}, true)) return 1;
    }
    report(cases, totals, reached.size(), 1, "quiet-and-observed");
    for (auto pc : reached) std::fprintf(stderr, "%x ", pc);
    return !totals.faults || !totals.children || reached.size() != 93;
}
