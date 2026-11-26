#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_death_cycle_opening.hpp"

#include <set>

namespace {
using namespace frame_misc_harness;
using namespace halo2::native_death_cycle_opening;

constexpr std::uint32_t object_system = 0x10000000u, object_slots = 0x10001000u;
constexpr std::uint32_t object = 0x10002000u, override_object = 0x10003000u;
constexpr std::uint32_t tag_slots = 0x10004000u, tag = 0x10005000u;
constexpr std::uint32_t datum = 0x80240003u, override_datum = 0x07010004u;
constexpr std::uint32_t continuation = 0x1031cbu;

void fixture_child(WorldState &s, std::uint32_t target, std::uint32_t resume, unsigned argument_bytes) {
    if (r(s.memory, s.gp[esp], 4) != resume)
        throw std::runtime_error("Original death-cycle return word differs");
    log_call(entry_hash(s, target));
    switch (target) {
        case 0x106760u:
            s.set(eax, policy & 1u, 1, 0);
            break;
        case 0xd39d0u:
            s.xmm[0] = _mm_castsi128_ps(_mm_set_epi32(0x11442233, 0x7fc00003, 0x12340000, int(policy << 16u)));
            break;
        case 0x107080u:
            s.gp[eax] = policy & 2u ? override_datum : 0xffffffffu;
            break;
        default:
            throw std::runtime_error("Unexpected death-cycle child");
    }
    (void)s.pop();
    s.gp[esp] += argument_bytes;
}

void run_106760(WorldState &s) { fixture_child(s, 0x106760u, 0x10313cu, 4); }
void run_d39d0(WorldState &s) { fixture_child(s, 0xd39d0u, 0x103162u, 0); }
void run_107080(WorldState &s) { fixture_child(s, 0x107080u, 0x1031a7u, 0); }

#include "death_cycle_opening_reference.inc"

bool native(WorldState &s) {
    if (!run(s, run_106760, run_d39d0, run_107080)) return false;
    s.instruction_literal(continuation);
    return true;
}

Case fixture(std::mt19937 &random, unsigned index) {
    Case test;
    const auto sp = 0x20010400u + (index & 3u);
    random_abi(random, test, sp);
    test.policy = index & 15u;
    test.full_observation = true;
    auto &memory = test.memory;
    for (auto address = sp - 0x80u; address < sp + 0x20u; address += 4)
        memory.put(address, random());
    memory.put(sp + 4, datum);
    memory.put(0x4ecbd0u, object_system);
    memory.put(object_system + 0x44u, object_slots);
    memory.put(object_slots + 12u * (datum & 0xffffu) + 8u, object);
    memory.put(object_slots + 12u * (override_datum & 0xffffu) + 8u, override_object);
    memory.put(object, 0x09000007u);
    memory.put(object + 0x12cu, index & 4u ? 1u : 0u);
    memory.put(object + 0x154u, random());
    memory.put(override_object + 0x13cu, index & 1u ? random() : 0xffffffffu);
    memory.put(0x4f0414u, tag_slots);
    memory.put(tag_slots + 16u * 7u + 8u, tag);
    memory.put(tag + 0x12cu, index & 8u ? 1u << 27u : 0u);
    memory.put(0x46a0d0u, random());
    return test;
}
}

int main() {
    constexpr unsigned cases = 768;
    std::mt19937 random{0x1030f0u};
    Totals totals;
    std::set<std::uint32_t> reached;
    for (unsigned index = 0; index < cases; ++index) {
        auto test = fixture(random, index);
        auto input = test.memory.clone();
        const auto baseline = execute(input, test, false, native, run_1030f0);
        if (!baseline.fault.empty() || baseline.calls.size() != 3 || baseline.cursor != continuation) {
            std::fprintf(stderr, "Invalid original sequence at case %u, PC %x: %s\n", index,
                         baseline.cursor, baseline.fault.c_str());
            return 1;
        }
        reached.insert(baseline.instructions.begin(), baseline.instructions.end());
        const auto &store = baseline.stores[(index / 16u) % baseline.stores.size()];
        switch ((index / 16u) % 8u) {
            case 0:
                break;
            case 1:
                test.full_observation = false;
                break;
            case 2:
                test.full_observation = false;
                test.activate_trace_at_store = store.instruction;
                break;
            case 3:
                test.reject_after_store_pc = store.instruction;
                break;
            case 4:
                test.memory.denied_write = store.address;
                break;
            case 5:
                test.reject_instruction = baseline.instructions[index % baseline.instructions.size()];
                break;
            case 6:
                test.memory.drop(object_slots);
                break;
            case 7:
                test.memory.drop(0x46a0d0u);
                break;
        }
        if (!compare(index, test, native, run_1030f0, totals, false, {}, true)) return 1;
    }
    reached.erase(continuation);
    report(cases, totals, reached.size(), 5, "quiet-and-observed");
    for (auto pc : reached) std::fprintf(stderr, "%x ", pc);
    return !totals.faults || reached.size() != 60;
}
