#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_death_cycle_record_blend.hpp"

#include <set>

namespace {
using namespace frame_misc_harness;
using namespace halo2::native_object_tick_detail::boundary;
namespace blend = halo2::native_death_cycle_record_blend;

constexpr std::uint32_t resume = 0x1044f4u;
constexpr std::uint32_t record = 0x10000800u, definition = 0x10001000u, clock_base = 0x10001300u;
constexpr std::uint32_t object_table = 0x10001500u, objects = 0x10002000u;
constexpr std::uint32_t tag_table = 0x10003000u, tag = 0x10003500u;

void run_bc820(WorldState &s) {
    const auto sp = s.gp[esp];
    const auto ret = r(s.memory, sp, 4);
    if (ret != 0x104444u && ret != 0x1044ecu)
        throw std::runtime_error("Incorrect notification return address");
    if (r(s.memory, sp+8u, 4) != 0xffffffffu || r(s.memory, sp+12u, 4) != 0u ||
        r(s.memory, sp+16u, 4) != (ret == 0x104444u ? 0xffffffffu : 0u))
        throw std::runtime_error("Incorrect notification arguments");
    log_call(mix(entry_hash(s, 0xbc820u), ret));
    if (policy) throw std::runtime_error("Notification child fault");
    s.gp[eax] ^= 0x235af801u;
    s.gp[ecx] ^= 0x6254de13u;
    s.gp[edx] ^= 0x11a8bac2u;
    s.flags.logic(s.gp[eax], 4);
    (void)s.pop();
    s.gp[esp] += 16u;
}

#include "death_record_blend_reference.inc"

bool native(WorldState &s) {
    const auto child = [](auto &state, std::uint32_t target, std::uint32_t site) {
        if (target != 0xbc820u || r(state.memory, state.gp[esp], 4) != site+5u)
            throw std::runtime_error("Incorrect notification call site");
        run_bc820(state);
    };
    if (!blend::run(s, child)) return false;
    s.instruction_literal(resume);
    return true;
}

Case fixture(std::mt19937 &random, unsigned index) {
    constexpr std::array values{0u, 0x80000000u, 0x3e800000u, 0x3f800000u, 0x40800000u,
                                0xbf800000u, 1u, 0x00800000u, 0x7fc12345u, 0xff854321u, 0x7f800000u};
    constexpr std::array counters{0u, 1u, 0xfffeu, 0x8000u};
    Case test;
    const auto sp = 0x20010400u + (index&3u);
    random_abi(random, test, sp);
    test.policy = 0u;
    test.abi.gp[ebp] = definition;
    test.abi.gp[esi] = record;
    auto &m = test.memory;
    for (auto at = sp-0x20u; at<=sp+0x54u; at+=4u) m.put(at, random());
    m.put(sp+0x15u, index&1u, 1);
    m.put(sp+0x24u, 0x0cu);
    m.put(sp+0x28u, 0x10003b00u);
    m.put(sp+0x4cu, 0x80240003u);
    m.put(record+1u, index/2u%3u, 1);
    m.put(record+2u, counters[index/2u%counters.size()], 2);
    m.put(record+4u, index&1u ? 0u : 8u, 1);
    const auto value = [&](std::uint32_t forced) {
        return index&32u ? values[random()%values.size()] : forced;
    };
    m.put(record+0x10u, value(0x3f000000u));
    m.put(definition+0x18u, value(0x3ee00000u));
    m.put(definition+0xd4u, value(0x3f800000u));
    m.put(definition+0xd8u, value(0x3f800000u));
    m.put(0x51df54u, clock_base);
    m.put(clock_base+4u, value(0x3f800000u));
    m.put(0x46a0d0u, 0x3f800000u);
    m.put(0x46a0e8u, 0u);
    m.put(0x4ecbd0u, object_table);
    m.put(object_table+0x44u, objects);
    m.put(objects+0x0eu, index&2u ? 0x10u : 0u, 1);
    m.put(objects+0x14u, definition);
    m.put(definition, 0x00150005u);
    m.put(definition+4u, index&4u ? 1u : 0u, 1);
    m.put(definition+0x14u, index&8u ? 0xffffffffu : 0x80240004u);
    m.put(0x4f0414u, tag_table);
    m.put(tag_table+0x58u, tag);
    m.put(tag+0x38u, index&16u ? 0xffffffffu : 0x80240005u);
    return test;
}
}

int main() {
    constexpr unsigned cases = 1536;
    std::mt19937 random{0x104371u};
    Totals totals;
    std::set<std::uint32_t> reached;
    for (unsigned index=0; index<cases; ++index) {
        auto test = fixture(random, index);
        test.full_observation = true;
        auto input = test.memory.clone();
        const auto baseline = execute(input, test, false, native, reference_record_blend);
        if (!baseline.fault.empty() || baseline.cursor != resume) {
            std::fprintf(stderr, "Invalid record blend fixture %u at %x: %s\n", index,
                         baseline.cursor, baseline.fault.c_str());
            return 1;
        }
        for (auto pc : baseline.instructions)
            if (pc != resume) reached.insert(pc);
        const auto *store = baseline.stores.empty() ? nullptr : &baseline.stores[index%baseline.stores.size()];
        switch (index/13u%9u) {
            case 0: break;
            case 1: test.full_observation = false; break;
            case 2:
                test.full_observation = false;
                if (store) test.activate_trace_at_store = store->instruction;
                break;
            case 3: if (store) test.reject_after_store_pc = store->instruction; break;
            case 4: if (store) test.memory.denied_write = store->address; break;
            case 5: test.reject_instruction = baseline.instructions[index%baseline.instructions.size()]; break;
            case 6: test.memory.drop(clock_base+4u); break;
            case 7: test.memory.drop(tag+0x38u); break;
            case 8: test.policy = 1u; break;
        }
        if (!compare(index, test, native, reference_record_blend, totals, false, {}, true)) return 1;
    }
    report(cases, totals, reached.size(), 1, "quiet-and-observed");
    for (auto pc : reached) std::fprintf(stderr, "%x ", pc);
    return !totals.faults || !totals.children || reached.size() != 104;
}
