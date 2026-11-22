#include "halo2/native_campaign_ai_behavior_score_17_tu15.hpp"

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
constexpr SourcePhysicsContactRuntimeCall root{0x1b0195u, 0x1b3600u, false};
constexpr SourcePhysicsFactoryFloatingPoint controls{0x027fu, 0x1f80u};
constexpr Word stack = 0x600efe84u, actor = 0xe3620000u, pool = 0x82fad850u, row = 0x82fad89cu;
constexpr Word object = 0xe4f3027cu, object_pool = 0x85001000u, objects = 0x85002000u, body = 0x85003000u;
constexpr Word property = 0x85004000u, prop_pool = 0x85005000u, prop = 0x85006000u, target = 0x85007000u;
constexpr Word clock = 0x85008000u, rng = 0x85009000u, none = 0xffffffffu;

void check(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}

struct Fault {};
struct ChildStop {};

struct Event {
    char kind;
    Word pc, address;
    std::size_t width;
    bool operator==(const Event &) const = default;
};

struct Memory final : SourcePhysicsContactMemory {
    std::map<Word, std::byte> bytes;
    std::vector<Word> sites;
    mutable std::vector<Event> events;
    Word pc{};
    std::optional<Event> fault;

    void touch(char kind, Word address, std::size_t width) const {
        events.push_back({kind, pc, address, width});
        if (fault == events.back()) throw Fault{};
    }
    void read(Word address, std::span<std::byte> output) const override {
        touch('r', address, output.size());
        for (std::size_t i = 0; i < output.size(); ++i) output[i] = bytes.at(address + Word(i));
    }
    void write(Word address, std::span<const std::byte> input) override {
        touch('w', address, input.size());
        for (std::size_t i = 0; i < input.size(); ++i) bytes.at(address + Word(i)) = input[i];
    }
    void put(Word address, Word value, unsigned width = 4) {
        for (unsigned i = 0; i < width; ++i) bytes[address + i] = std::byte(value >> (i * 8u));
    }
    void real(Word address, float value) { put(address, std::bit_cast<Word>(value)); }
    Word get(Word address, unsigned width = 4) const {
        Word value{};
        for (unsigned i = 0; i < width; ++i) value |= std::to_integer<Word>(bytes.at(address + i)) << (i * 8u);
        return value;
    }
};

Memory input() {
    Memory m;
    for (Word p = stack - 0x34u; p <= stack + 4u; p += 4u) m.put(p, 0x74000000u | (p & 0xffffu));
    m.put(stack, 0x1b0197u);
    m.put(stack + 4u, actor);
    m.put(0x5020c8u, pool);
    m.put(pool + 0x44u, row);
    m.put(row + 0x18u, object);
    m.put(row + 0x338u, 0xf2700000u);
    m.put(row + 0x229u, 0, 1);
    m.put(row + 0x1f8u, none);
    m.put(0x4ecbd0u, object_pool);
    m.put(object_pool + 0x44u, objects);
    m.put(objects + (object & 0xffffu) * 12u + 8u, body);
    m.put(body + 0x19u, 0, 1);
    m.real(property + 0x24u, 0.75f);
    m.real(property + 0x1cu, 1.0f);
    m.real(property + 0x20u, 5.0f);
    m.real(property + 0x34u, 1.0f);
    m.real(0x46a0e8u, 0.0f);
    m.put(0x51df54u, clock);
    m.put(clock + 8u, 100u);
    m.real(clock + 4u, 0.05f);
    m.put(0x50eef0u, prop_pool);
    m.put(prop_pool + 0x44u, prop);
    m.put(prop + 0x27u, 1, 1);
    m.real(prop + 0x28u, 2.0f);
    m.put(target + 0x64u, 1, 1);
    m.put(0x4f3cd8u, rng);
    return m;
}

SourcePhysicsWorldRuntimeAbi registers() {
    SourcePhysicsWorldRuntimeAbi a{};
    a.gp = {root.target, 0x600efeccu, 0x600e0011u, actor, stack, 0x47bb18u, row, 0x48a310u};
    a.eflags = 0x606u;
    a.fs_base = 0x60200000u;
    a.floating_point = controls;
    a.x87_depth = 1;
    a.x87_status = 0x2800u;
    a.x87[0] = 1.5L;
    for (unsigned n = 0; n < 8; ++n) a.xmm[n] = {0x3f800000u+n, 0x80000000u, n, 0x7fc12345u};
    return a;
}

void push_x87(SourcePhysicsWorldRuntimeAbi &a, long double x) {
    const auto top = ((a.x87_status >> 11u) + 7u) & 7u;
    for (auto n = a.x87_depth; n != 0u; --n) a.x87[n] = a.x87[n - 1u];
    a.x87[0] = x;
    ++a.x87_depth;
    a.x87_status = (a.x87_status & ~0x3a00u) | std::uint16_t(top << 11u);
}

struct Run {
    Memory memory = input();
    SourcePhysicsWorldRuntimeAbi abi = registers();
    std::vector<Word> children;
    std::vector<SourcePhysicsContactRecordStore> stores;
    bool property_missing{}, predicate{}, actor_filter{}, truncate_requested{};
    long double probability = 0.75L, random = 0.25L;
    std::optional<Word> child_to_stop;

    bool run(SourcePhysicsContactRuntimeCall call = root) {
        SourcePhysicsWorldRuntimeServices services;
        services.instruction = [&](Word pc) { memory.pc = pc; memory.sites.push_back(pc); };
        services.required_owner = [&](auto &child_memory, const auto &child, auto &a, const auto &observe) {
            check(&child_memory == &memory && bool(observe) && !child.tail, "Live child lost memory/observer authority");
            children.push_back(child.target);
            auto &sp = a.gp[4];
            const auto frame = [&](Word call_pc, Word return_pc, Word consumed_args) {
                check(child.instruction == call_pc && sp == stack - 24u - consumed_args &&
                      memory.get(sp) == return_pc, "Child received a nonoriginal call or stack word");
            };
            switch (child.target) {
            case 0x1ed240u:
                frame(0x1b363au, 0x1b363fu, 0);
                check(a.gp[0] == objects && a.gp[1] == (object & 0xffffu)*3u && a.gp[2] == object_pool &&
                      a.gp[3] == actor && a.gp[6] == row && a.gp[7] == body &&
                      memory.get(stack + 4u) == actor, "First child lost original actor/definition frame");
                a.gp[0] = property_missing ? 0u : property;
                break;
            case 0x113190u:
                frame(0x1b36b5u, 0x1b36bau, 4);
                check(a.gp[0] == object && memory.get(sp+4u) == object, "Object predicate lost its original argument");
                a.gp[0] = std::uint32_t(actor_filter);
                break;
            case 0x2673a0u:
                frame(0x1b36ddu, 0x1b36e2u, 0);
                check(a.gp[1] == prop && a.gp[6] == prop, "Prop lookup lost live row owner");
                a.gp[0] = target;
                break;
            case 0x1ea2e0u:
                frame(0x1b370bu, 0x1b3710u, 0);
                check(a.gp[0] == actor, "Actor predicate lost its original argument");
                a.gp[0] = std::uint32_t(predicate);
                break;
            case 0x1d2190u:
                frame(0x1b372du, 0x1b3732u, 4);
                check(a.gp[1] == memory.get(property + 0x24u) && memory.get(sp+4u) == a.gp[1],
                      "Probability child lost its original float word");
                push_x87(a, probability);
                break;
            case 0x25980u:
                frame(0x1b373cu, 0x1b3741u, 0);
                check(a.gp[1] == rng && memory.get(stack - 4u) ==
                      (truncate_requested ? 0x3f000000u : std::bit_cast<Word>(static_cast<float>(probability))),
                      "RNG child was called before the real rounded probability spill");
                push_x87(a, random);
                break;
            default: throw std::runtime_error("Unexpected descriptor-17 child");
            }
            if (child.target == child_to_stop) throw ChildStop{};
            sp += 4u + ((child.target == 0x113190u || child.target == 0x1d2190u) ? 4u : 0u);
        };
        return native_campaign_ai_behavior_score_17_tu15_try_run(memory, image, call, abi, controls, services,
            [&](const SourcePhysicsContactRecordStore &store) {
                check(memory.pc == store.instruction, "Completed store lost its original cursor");
                memory.touch('o', store.address, store.written.size());
                stores.push_back(store);
            });
    }
    bool saw(Word pc) const { return std::ranges::find(memory.sites, pc)!=memory.sites.end(); }
};

void completed(const Run &r, Word score, Word ret) {
    const auto before = registers();
    check((r.abi.gp[0] & 0xffffu) == score && r.memory.get(stack + 4u) == score &&
          r.abi.gp[4] == stack + 8u && r.memory.sites.back() == ret, "Actual score/argument/RET4 mismatch");
    for (const auto reg : {3u,5u,6u,7u}) check(r.abi.gp[reg] == before.gp[reg], "Callee-preserved GP mismatch");
    check(r.abi.fs_base == before.fs_base && r.abi.x87_depth == 1 && r.abi.x87[0] == before.x87[0] &&
          (r.abi.x87_status & 0x3800u) == 0x2800u, "Live FP/TLS stack was not preserved");
}

void verify_returns() {
    Run rejected;
    rejected.memory.put(body + 0x19u, 1, 1);
    check(rejected.run() && rejected.children == std::vector<Word>{0x1ed240u} && !rejected.saw(0x1b3653u),
          "Object gate moved before the actual property child");
    completed(rejected, 0, 0x1b375du);
    check(rejected.abi.gp[1] == registers().gp[1] && rejected.abi.xmm == registers().xmm,
          "Early return modified saved ECX or XMM");

    Run missing;
    missing.property_missing = true;
    check(missing.run() && missing.children.size()==1 && missing.saw(0x1b3655u), "Null property exit moved");
    completed(missing, 0, 0x1b375du);

    Run nan;
    nan.memory.put(property + 0x24u, 0x7fc01234u);
    const auto host_csr = _mm_getcsr();
    check(nan.run() && nan.children.size()==1 && nan.saw(0x1b366du) && !nan.saw(0x1b367au) &&
          nan.abi.floating_point->mxcsr == (controls.mxcsr | 1u) && _mm_getcsr() == host_csr,
          "COMISS invalid/unordered handling differs");
    completed(nan, 0, 0x1b375du);

    Run cooldown;
    cooldown.memory.put(row + 0x1f8u, 90u);
    check(cooldown.run() && cooldown.saw(0x1b369eu) && cooldown.children.size()==1 && !cooldown.saw(0x1b36b1u),
          "Actual signed clock conversion/cooldown rejected at the wrong boundary");
    completed(cooldown, 0, 0x1b375du);

    Run four;
    check(four.run() && four.children == std::vector<Word>{0x1ed240u,0x113190u,0x2673a0u,0x1ea2e0u},
          "Score-four path did not keep ordered required owners");
    completed(four, 4, 0x1b3726u);

    Run probability;
    probability.memory.put(target+0x64u, 0, 1);
    probability.abi.floating_point->x87_control_word = 0x067fu;
    probability.probability = 0.5L + 0x1p-25L;
    probability.truncate_requested = true;
    check(probability.run() && probability.children == std::vector<Word>{0x1ed240u,0x113190u,0x2673a0u,0x1d2190u,0x25980u} &&
          probability.memory.get(stack-4u)==0x3f000000u && probability.abi.gp[1]==0x3f000000u &&
          probability.saw(0x1b3745u) && (probability.abi.x87_status & 0x20u),
          "x87 control, signed pop, real spilled stack slot or RNG handoff differs");
    completed(probability, 3, 0x1b375du);

    Run equal;
    equal.predicate = true;
    equal.random = equal.probability;
    check(equal.run() && equal.children.size()==6 && equal.saw(0x1b3749u) && !equal.saw(0x1b374bu),
          "Equal probability took the strict-greater score branch");
    completed(equal, 0, 0x1b375du);
}

void verify_refusal_and_faults() {
    Run foreign;
    foreign.abi.gp[7]++;
    const auto before = foreign.abi;
    check(!foreign.run() && foreign.memory.events.empty() && foreign.memory.sites.empty() && foreign.abi.gp==before.gp,
          "Unqualified descriptor had effects");

    Run missing;
    missing.predicate = true;
    missing.child_to_stop = 0x1d2190u;
    bool stopped = false;
    try { missing.run(); } catch (const ChildStop &) { stopped = true; }
    check(stopped && missing.abi.gp[4]==stack-28u && missing.memory.sites.back()==0x1b372du &&
          missing.memory.get(missing.abi.gp[4])==0x1b3732u && missing.abi.x87_depth==2,
          "Unresolved child received synthetic success or lost its real retained packet");

    for (const char boundary : {'r','w','o'}) {
        Run fault;
        fault.predicate = true;
        fault.abi.floating_point->x87_control_word = 0x067fu;
        fault.probability = 0.5L + 0x1p-25L;
        fault.memory.fault = Event{boundary,0x1b3732u,stack-4u,4};
        stopped = false;
        try { fault.run(); } catch (const Fault &) { stopped = true; }
        const bool committed = boundary=='o';
        check(stopped && fault.memory.sites.back()==0x1b3732u &&
              fault.memory.get(stack-4u)==(committed ? 0x3f000000u : before.gp[1]) &&
              fault.abi.x87_depth==(committed ? 1u : 2u) && bool(fault.abi.x87_status & 0x20u)==committed,
              "Fault or observer moved the original FSTP commit boundary");
    }

    Run old_return;
    old_return.memory.put(stack,0x1b01a0u);
    old_return.property_missing = true;
    bool rejected = false;
    try { old_return.run(); } catch (const std::runtime_error &) { rejected = true; }
    check(rejected && old_return.abi.gp[4]==stack && old_return.memory.sites.back()==0x1b375du,
          "RET admitted a foreign continuation or stopped before its actual read");
}
}

int main() try {
    verify_returns();
    verify_refusal_and_faults();
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
