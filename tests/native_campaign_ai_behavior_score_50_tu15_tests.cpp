#include "halo2/native_campaign_ai_behavior_score_50_tu15.hpp"

#include <bit>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
using namespace halo2;
using Word = std::uint32_t;
constexpr auto image = SourcePhysicsContactImage::TitleUpdate15;
constexpr SourcePhysicsContactRuntimeCall call{0x1b0128u, 0x1bfbf0u, false};
constexpr SourcePhysicsFactoryFloatingPoint controls{0x027fu, 0x1f80u};
constexpr Word stack = 0x600efe9cu, request = 0x600eff78u, actor = 0xe3620000u;
constexpr Word pool = 0x82fad850u, row = 0x82fad89cu, descriptor = 0x48af44u;
constexpr Word prop_id = 0xf2700001u, prop_pool = 0x85001000u, props = 0x85002000u;
constexpr Word property = 0x85003000u, limit = 0x85004000u, target = 0x85005000u;
constexpr Word game = 0x85006000u, clock = 0x85007000u, none = 0xffffffffu;

void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

struct ChildStop {};
struct WriteRefused {};

struct Access {
    char kind;
    Word pc, address;
    std::size_t width;
    bool operator==(const Access &) const = default;
};

struct Memory final : SourcePhysicsContactMemory {
    std::map<Word, std::byte> bytes;
    std::vector<Word> trace;
    mutable std::vector<Access> accesses;
    std::optional<Word> refuse_store;
    Word pc{};

    void read(Word address, std::span<std::byte> out) const override {
        accesses.push_back({'r', pc, address, out.size()});
        for (std::size_t i = 0; i < out.size(); ++i) out[i] = bytes.at(address + Word(i));
    }
    void write(Word address, std::span<const std::byte> in) override {
        accesses.push_back({'w', pc, address, in.size()});
        if (refuse_store == pc) throw WriteRefused{};
        for (std::size_t i = 0; i < in.size(); ++i) bytes.at(address + Word(i)) = in[i];
    }
    void put(Word address, Word value, unsigned width = 4) {
        for (unsigned i = 0; i < width; ++i) bytes[address + i] = std::byte(value >> (i * 8u));
    }
    void real(Word address, float value) { put(address, std::bit_cast<Word>(value)); }
    Word get(Word address) const {
        Word value{};
        for (unsigned i = 0; i < 4u; ++i) value |= std::to_integer<Word>(bytes.at(address + i)) << (i * 8u);
        return value;
    }
};

enum class Fixture { no_prop, through_first_child, no_property, weak_prop, wrong_target, selected, cooldown_equal, cooldown_passed };

struct Run {
    Memory memory;
    SourcePhysicsWorldRuntimeAbi abi{};
    std::vector<Word> children;
    std::vector<SourcePhysicsWorldRuntimeAbi> frames;
    std::vector<SourcePhysicsContactRecordStore> stores;
    Fixture fixture;
    bool difficulty_enabled;

    explicit Run(Fixture which) : fixture(which), difficulty_enabled(which == Fixture::cooldown_equal || which == Fixture::cooldown_passed) {
        abi.gp = {0x82faffffu, request, 0x600e0032u, actor, stack, 0x47c0acu, row, descriptor};
        abi.eflags = 0x607u;
        abi.fs_base = 0x60200000u;
        abi.floating_point = controls;
        abi.x87_depth = 1;
        abi.x87_status = 0x2800u;
        abi.x87[0] = 1.5L;
        for (unsigned i = 0; i < abi.xmm.size(); ++i)
            abi.xmm[i] = {0x3f800000u + i, 0x80000000u, i, 0x7fc12345u};
        for (Word address = stack - 24u; address <= stack + 16u; address += 4u)
            memory.put(address, 0xb3110000u | (address & 0xffffu));
        memory.put(stack, 0x1b012bu);
        memory.put(stack + 4u, actor);
        memory.put(stack + 8u, request);
        memory.put(0x5020c8u, pool);
        memory.put(pool + 0x44u, row);
        memory.put(0x47c1a4u, 0x6198ffffu);
        memory.put(row + 0x338u, which == Fixture::no_prop ? none : prop_id);
        memory.put(row + 0x1f4u, difficulty_enabled ? 100u : none);
        memory.put(0x50eef0u, prop_pool);
        memory.put(prop_pool + 0x44u, props);
        memory.put(props + 0x3cu + 0x27u, which == Fixture::weak_prop ? 0xffu : 2u, 1);
        memory.real(props + 0x3cu + 0x28u, 3.0f);
        memory.real(property + 0x34u, 4.0f);
        memory.real(limit + 0x18u, 2.0f);
        memory.put(0x4f3218u, game);
        memory.put(game + 8u, 1u);
        memory.put(game + 0x132u, 0xfffeu, 2);
        memory.put(target + 0x3cu, which == Fixture::wrong_target ? 42u : none);
        memory.put(0x51df54u, clock);
        memory.put(clock + 8u, which == Fixture::cooldown_passed ? 109u : 108u);
        memory.real(clock + 4u, 0.25f);
    }

    bool run(SourcePhysicsContactRuntimeCall actual = call, SourcePhysicsContactImage mode = image) {
        SourcePhysicsWorldRuntimeServices services;
        services.instruction = [&](Word pc) { memory.pc = pc; memory.trace.push_back(pc); };
        services.required_owner = [&](auto &bank, const auto &child, auto &state, const auto &observe) {
            check(&bank == &memory && bool(observe) && !child.tail && state.gp[3] == actor && state.gp[6] == row,
                "Descriptor-50 child lost live bank or retained actor state");
            children.push_back(child.target);
            frames.push_back(state);
            const auto sp = state.gp[4];
            check(sp == stack - (child.target == 0x1f1950u ? 24u : 20u), "Child received an unrelated stack");
            switch (child.target) {
            case 0x1ecfc0u:
                check(child.instruction == 0x1bfc26u && memory.get(sp) == 0x1bfc2bu && state.gp[1] == prop_id &&
                    state.gp[2] == row && state.gp[0] == 0x6198ffffu && state.gp[7] == descriptor,
                    "Initial child did not receive the actual original ABI");
                if (fixture == Fixture::through_first_child) throw ChildStop{};
                state.gp[0] = fixture == Fixture::no_property ? 0u : property;
                break;
            case 0x1ed240u:
                check(child.instruction == 0x1bfc2du && memory.get(sp) == 0x1bfc32u && state.gp[5] == state.gp[0],
                    "Second child lost the first result or return");
                state.gp[0] = difficulty_enabled ? limit : 0u;
                break;
            case 0x1f1950u:
                check(child.instruction == 0x1bfc55u && memory.get(sp) == 0x1bfc5au &&
                    memory.get(sp + 4u) == 0x15u && state.gp[1] == 0xfffffffeu,
                    "Difficulty child lost the configured signed parameter or RET 4 frame");
                state.xmm[0] = {0x3f800000u, 0x80000000u, 0x12345678u, 0x7fc12345u};
                break;
            case 0x2673a0u:
                check(child.instruction == 0x1bfc90u && memory.get(sp) == 0x1bfc95u && state.gp[1] == props + 0x3cu,
                    "Target child lost the live prop row");
                state.gp[0] = target;
                break;
            default: throw std::runtime_error("Unexpected descriptor-50 child");
            }
            state.gp[4] += child.target == 0x1f1950u ? 8u : 4u;
        };
        return native_campaign_ai_behavior_score_50_tu15_try_run(memory, mode, actual, abi, controls, services,
            [&](const auto &store) {
                check(memory.pc == store.instruction, "Completed write lost its original PC");
                stores.push_back(store);
            });
    }
};

void check_exits() {
    for (const auto fixture : {Fixture::no_prop, Fixture::no_property, Fixture::weak_prop, Fixture::wrong_target,
                              Fixture::selected, Fixture::cooldown_equal, Fixture::cooldown_passed}) {
        Run run(fixture);
        const auto before = run.abi;
        check(run.run() && run.abi.gp[4] == stack + 12u, "Original RET 8 was lost");
        const auto take = fixture == Fixture::selected || fixture == Fixture::cooldown_passed;
        const auto expected_children = fixture == Fixture::no_prop ? 0u :
            (fixture == Fixture::no_property || fixture == Fixture::weak_prop) ? 2u :
            run.difficulty_enabled ? 4u : 3u;
        check((run.abi.gp[0] & 0xffffu) == (take ? 16u : 0xffffu) && run.children.size() == expected_children &&
            run.memory.pc == (take ? 0x1bfcc7u : 0x1bfcd4u), "Score gates or child order differ");
        for (const unsigned reg : {3u, 5u, 6u, 7u})
            check(run.abi.gp[reg] == before.gp[reg], "Callee-saved register changed");
        check(run.abi.x87_depth == before.x87_depth && run.abi.x87_status == before.x87_status &&
            run.abi.x87[0] == before.x87[0] && run.abi.fs_base == before.fs_base, "Scalar score changed retained x87 or FS");
        if (run.difficulty_enabled)
            check(run.abi.xmm[0] == std::array<Word, 4>{0x40000000u, 0x80000000u, 0x12345678u, 0x7fc12345u},
                "Scalar product changed upper XMM lanes");
    }
}

void check_faults_and_scope() {
    Run boundary(Fixture::through_first_child);
    bool reached = false;
    try { (void)boundary.run(); } catch (const ChildStop &) { reached = true; }
    check(reached && boundary.abi.gp[4] == stack - 20u && boundary.stores.size() == 5u &&
        boundary.memory.pc == 0x1bfc26u && boundary.children == std::vector<Word>{0x1ecfc0u},
        "First child failure lost the actual source prefix");
    Run guard(Fixture::selected);
    guard.memory.refuse_store = 0x1bfc26u;
    bool refused = false;
    try { (void)guard.run(); } catch (const WriteRefused &) { refused = true; }
    check(refused && guard.abi.gp[4] == stack - 16u && guard.stores.size() == 4u && guard.children.empty(),
        "Guard refusal consumed the rejected return-word store");
    Run wrong(Fixture::selected);
    check(!wrong.run({call.instruction + 1u, call.target, false}) &&
        !wrong.run({call.instruction, call.target, true}) && !wrong.run(call, SourcePhysicsContactImage::MapPack11),
        "Unrelated caller was admitted");
    wrong.abi.gp[7] = descriptor + 4u;
    check(!wrong.run() && wrong.memory.trace.empty() && wrong.memory.accesses.empty(),
        "Unrelated descriptor caused an original effect");
}
}

int main() try {
    check_exits();
    check_faults_and_scope();
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
