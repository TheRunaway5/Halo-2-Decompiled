#include "halo2/native_campaign_ai_behavior_score_80_tu15.hpp"

#include <algorithm>
#include <bit>
#include <iostream>
#include <map>
#include <stdexcept>
#include <xmmintrin.h>

namespace {
using namespace halo2;
using Word = std::uint32_t;
constexpr auto image = SourcePhysicsContactImage::TitleUpdate15;
constexpr SourcePhysicsContactRuntimeCall root{0x1b0195u, 0x1c8590u, false};
constexpr SourcePhysicsFactoryFloatingPoint controls{0x027fu, 0x1f80u};
constexpr Word stack = 0x600efe84u, actor = 0xe3620000u, pool = 0x82fad850u, row = 0x82fad89cu;
constexpr Word none = 0xffffffffu, character = 0x98020007u, parent = 0x98030008u;
constexpr Word tags = 0x86001000u, tag = 0x86002000u, parent_tag = 0x86003000u, property = 0x86004000u;
constexpr Word prop_pool = 0x87001000u, prop = 0x87002000u, target = 0x87003000u;
constexpr Word object_pool = 0x88001000u, objects = 0x88002000u, origin = 0x88003000u, other = 0x88004000u;

void check(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}

struct Fault {};
struct MissingOwner {};

struct Access {
    char kind;
    Word pc, address;
    std::size_t bytes;
    bool operator==(const Access &) const = default;
};

struct Memory final : SourcePhysicsContactMemory {
    std::map<Word,std::byte> bytes;
    mutable std::vector<Access> accesses;
    std::vector<Word> sites;
    Word pc{};
    std::optional<std::size_t> fault;

    void touch(char kind, Word address, std::size_t size) const {
        accesses.push_back({kind, pc, address, size});
        if (fault == accesses.size() - 1u) throw Fault{};
    }
    void read(Word address, std::span<std::byte> out) const override {
        touch('r', address, out.size());
        for (std::size_t n = 0; n < out.size(); ++n) out[n] = bytes.at(address + Word(n));
    }
    void write(Word address, std::span<const std::byte> input) override {
        touch('w', address, input.size());
        for (std::size_t n = 0; n < input.size(); ++n) bytes.at(address + Word(n)) = input[n];
    }
    Word get(Word address, unsigned size = 4) const {
        Word value{};
        for (unsigned n = 0; n < size; ++n)
            value |= std::to_integer<Word>(bytes.at(address + n)) << (n * 8u);
        return value;
    }
    void put(Word address, Word value, unsigned size = 4) {
        for (unsigned n = 0; n < size; ++n) bytes[address + n] = std::byte(value >> (n * 8u));
    }
    void put_float(Word address, float value) { put(address, std::bit_cast<Word>(value)); }
};

Memory input() {
    Memory m;
    for (Word address = stack - 0x30u; address <= stack + 4u; address += 4u)
        m.put(address, 0x7b000000u | (address & 0xffffu));
    m.put(stack, 0x1b0197u);
    m.put(stack + 4u, actor);
    m.put(0x5020c8u, pool);
    m.put(pool + 0x44u, row);
    m.put(row + 0x224u, 0, 1);
    m.put(row + 0x338u, 0xf2700000u);
    m.put(row + 0x26cu, none);
    m.put(row + 0x18u, 0xe4f3027cu);
    m.put(row + 0x54u, character);
    m.put(0x4f0414u, tags);
    m.put(tags + 7u * 16u + 8u, tag);
    m.put(tag + 0xbcu, 0x80000000u);
    m.put(tag + 8u, parent);
    m.put(tags + 8u * 16u + 8u, parent_tag);
    m.put(parent_tag + 0xbcu, 1);
    m.put(parent_tag + 0xc0u, property);
    m.put_float(property + 4u, 2.0f);
    m.put_float(property + 0xcu, 8.0f);
    m.put(0x50eef0u, prop_pool);
    m.put(prop_pool + 0x44u, prop);
    m.put(prop + 0x24u, 1, 2);
    m.put_float(prop + 0x28u, 1.0f);
    m.put(prop + 0x20u, 0xabcd0002u);
    m.put(target + 0x3cu, 0xd12e0001u);
    m.put(0x4ecbd0u, object_pool);
    m.put(object_pool + 0x44u, objects);
    m.put(objects + 1u * 12u + 8u, origin);
    m.put(objects + 2u * 12u + 8u, other);
    m.put_float(origin + 0x88u, 1.0f);
    m.put_float(origin + 0x8cu, 2.0f);
    m.put_float(origin + 0x90u, 3.0f);
    m.put(other + 0x14u, 0x9abc0005u);
    m.put(other + 0x1fcu, 0x419bu, 2);
    return m;
}

SourcePhysicsWorldRuntimeAbi registers() {
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp = {root.target, 0x600efeccu, 0x600e0050u, actor, stack, 0x47baa0u, row, 0x48b6d0u};
    abi.eflags = 0x606u;
    abi.fs_base = 0x60200000u;
    abi.x87_depth = 1;
    abi.x87_status = 0x2800u;
    abi.x87[0] = 1.25L;
    abi.floating_point = controls;
    for (unsigned reg = 0; reg < 8; ++reg)
        abi.xmm[reg] = {0x3f800000u + reg, 0x7fc00000u + reg, 0x80000000u, reg};
    return abi;
}

struct Run {
    Memory memory = input();
    SourcePhysicsWorldRuntimeAbi abi = registers();
    std::vector<SourcePhysicsContactRuntimeCall> children;
    std::vector<SourcePhysicsContactRecordStore> stores;
    bool stop_missing{}, resolve_last{}, disposition = true;

    bool run(SourcePhysicsContactRuntimeCall call = root) {
        SourcePhysicsWorldRuntimeServices services;
        services.instruction = [&](Word pc) { memory.pc = pc; memory.sites.push_back(pc); };
        services.required_owner = [&](auto &called_memory, const auto &child, auto &regs, const auto &observe) {
            check(&called_memory == &memory && bool(observe) && !child.tail, "Child lost its real memory or observer");
            children.push_back(child);
            check(regs.gp[4] == (child.target == 0x1c83f0u ? stack - 28u : stack - 20u),
                  "Child received the wrong actual frame");
            switch (child.target) {
            case 0x1ea2e0u:
                check(child.instruction == 0x1c862du && memory.get(regs.gp[4]) == 0x1c8632u && regs.gp[0] == actor,
                      "Actor predicate received a different ABI");
                regs.gp[0] = std::uint32_t(disposition);
                break;
            case 0x2673a0u:
                check(child.instruction == 0x1c8647u && memory.get(regs.gp[4]) == 0x1c864cu && regs.gp[1] == prop,
                      "Property object query received a different ABI");
                regs.gp[0] = target;
                break;
            case 0x267450u:
                check(child.instruction == 0x1c8650u && memory.get(regs.gp[4]) == 0x1c8655u &&
                      regs.gp[1] == prop && regs.gp[3] == target, "Property object predicate received a different ABI");
                regs.gp[0] = 1;
                break;
            case 0x1c83f0u:
                check(child.instruction == 0x1c86edu && memory.get(regs.gp[4]) == 0x1c86f2u &&
                      memory.get(regs.gp[4] + 4u) == actor && memory.get(regs.gp[4] + 8u) == 0x9abc0005u &&
                      regs.gp[0] == 0x9abc0005u && regs.gp[1] == other && (regs.gp[3] & 0xffffu) == 0x419bu,
                      "Conditional unbound child did not receive the original call packet");
                if (stop_missing) throw MissingOwner{};
                regs.gp[0] = (regs.gp[0] & 0xffffff00u) | std::uint32_t(resolve_last);
                regs.gp[4] += 8u;
                break;
            default: throw std::runtime_error("Unexpected child call");
            }
            regs.gp[4] += 4u;
        };
        return native_campaign_ai_behavior_score_80_tu15_try_run(memory, image, call, abi, controls, services,
            [&](const SourcePhysicsContactRecordStore &store) {
                check(memory.pc == store.instruction, "Wrong source cursor at completed store");
                memory.touch('o', store.address, store.written.size());
                stores.push_back(store);
            });
    }
    bool saw(Word pc) const { return std::ranges::find(memory.sites, pc) != memory.sites.end(); }
};

void completed(const Run &test, Word score) {
    const auto before = registers();
    for (auto i : {3u, 5u, 6u, 7u}) check(test.abi.gp[i] == before.gp[i], "Callee-preserved GP differs");
    check((test.abi.gp[0] & 0xffffu) == score && test.abi.gp[1] == score && test.abi.gp[4] == stack + 8u &&
          test.memory.get(stack - 4u) == score && test.memory.sites.back() == 0x1c8707u,
          "Original score/cleared ECX/RET4 differs");
    check(test.abi.fs_base == before.fs_base && test.abi.x87 == before.x87 &&
          test.abi.x87_depth == before.x87_depth && test.abi.x87_status == before.x87_status &&
          test.abi.floating_point->x87_control_word == controls.x87_control_word,
          "Callee changed its non-SSE retained state");
}

void actor_and_character_exits() {
    for (auto [offset,value,branch] : std::array<std::array<Word,3>,5>{{
            {0x224u, 0x80u, 0x1c85bfu}, {0x338u, none, 0x1c85cfu}, {0x26cu, 0, 0x1c85dcu},
            {0x18u, none, 0x1c85e6u}, {0x54u, none, 0x1ecdb8u}}}) {
        Run test;
        test.memory.put(row + offset, value, offset == 0x224u ? 1 : 4);
        check(test.run() && test.saw(branch) && test.children.empty(), "Actor/character early exit changed");
        completed(test, 0);
        if (offset != 0x54u)
            check(test.abi.gp[0] == (row & 0xffff0000u), "MOV AX overwrote EAX high bits");
        check(test.abi.xmm == registers().xmm, "Actor/character path changed XMM registers");
    }

    Run inherited_none;
    inherited_none.memory.put(tag + 8u, none);
    check(inherited_none.run() && inherited_none.saw(0x1ecdb0u) && !inherited_none.saw(0x1ecdb1u),
          "Negative character count failed to follow the exact parent terminator");
    completed(inherited_none, 0);
}

void comparisons_and_actual_children() {
    Run masked_nan;
    masked_nan.memory.put(prop + 0x28u, 0x7fc01234u);
    const auto host = _mm_getcsr();
    check(masked_nan.run() && masked_nan.children.size() == 1 && masked_nan.saw(0x1c863bu) &&
          masked_nan.abi.floating_point->mxcsr == (controls.mxcsr | 1u) && _mm_getcsr() == host,
          "COMISS unordered/invalid status or host restoration differs");
    completed(masked_nan, 0);

    Run narrow;
    narrow.memory.put_float(property + 0xcu, 2.0f);
    check(narrow.run() && narrow.children.size() == 3 && narrow.saw(0x1c86c4u) && !narrow.saw(0x1c86c9u),
          "Squared property comparison skipped or used the wrong branch");
    completed(narrow, 0);
    check(narrow.abi.xmm[4] == std::array<Word,4>{std::bit_cast<Word>(14.0f),0,0,0},
          "Full scalar SSE register packet differs");

    Run missing;
    missing.stop_missing = true;
    bool stopped = false;
    try { missing.run(); } catch (const MissingOwner &) { stopped = true; }
    check(stopped && missing.children.size() == 4 && missing.memory.sites.back() == 0x1c86edu &&
          missing.abi.gp[4] == stack - 28u && missing.memory.get(stack - 4u) == 0,
          "Missing child supplied an outcome or lost its committed frame");

    Run composed;
    composed.resolve_last = true;
    check(composed.run() && composed.children.size() == 4 && composed.saw(0x1c86f6u),
          "Component child result did not reach actual score store");
    completed(composed, 3);
}

void guard_and_fault_prefixes() {
    Run wrong;
    wrong.abi.gp[7]++;
    const auto original = wrong.abi;
    check(!wrong.run() && wrong.memory.sites.empty() && wrong.memory.accesses.empty() && wrong.abi.gp == original.gp,
          "Unowned descriptor changed retained state");

    Run entire;
    check(entire.run(), "Component baseline refused");
    for (std::size_t stop = 0; stop < entire.memory.accesses.size(); ++stop) {
        Run fault;
        fault.memory.fault = stop;
        bool stopped = false;
        try { fault.run(); } catch (const Fault &) { stopped = true; }
        check(stopped && fault.memory.accesses.size() == stop + 1u && std::equal(fault.memory.accesses.begin(),
              fault.memory.accesses.end(), entire.memory.accesses.begin()), "Original memory/observer prefix differs");
        if (stop < 3u)
            check(fault.abi.gp[4] == (stop == 2u ? stack - 4u : stack), "PUSH fault published wrong ESP");
        check(fault.memory.sites.back() == entire.memory.accesses[stop].pc, "Fault lost the original PC");
    }

    Run old_return;
    old_return.memory.put(stack, 0x1b01a0u);
    old_return.memory.put(row + 0x224u, 1, 1);
    bool rejected = false;
    try { old_return.run(); } catch (const std::runtime_error &) { rejected = true; }
    check(rejected && old_return.abi.gp[4] == stack && old_return.memory.sites.back() == 0x1c8707u,
          "RET guard consumed a foreign continuation or refused before its actual read");
}
}

int main() try {
    actor_and_character_exits();
    comparisons_and_actual_children();
    guard_and_fault_prefixes();
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
