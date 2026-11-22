#include "halo2/native_campaign_animation_distance_tu15.hpp"

#include <bit>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
using namespace halo2;
using Word = std::uint32_t;
constexpr auto image = SourcePhysicsContactImage::TitleUpdate15;
constexpr SourcePhysicsContactRuntimeCall call{0x11231bu, 0x1e2f60u, false};
constexpr SourcePhysicsFactoryFloatingPoint controls{0x027fu, 0x1f80u};
constexpr Word stack = 0x600efe90u, graph = 0x912ac230u, animation = 0x2fffffu;
constexpr Word rows = 0x91002000u, descriptor = rows + 47u * 0x6cu;
constexpr Word data = 0x91006000u, header = descriptor + 0x3cu, markers = 0x91008000u;
constexpr Word prefix = 0x600eff24u, total = 0x600eff18u;
constexpr Word local_prefix = stack - 0x10u;

void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

struct RefusedStore {};

struct Access {
    char kind;
    Word pc, address;
    std::size_t width;
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
        if (refuse_store == pc) throw RefusedStore{};
        for (std::size_t i = 0; i < in.size(); ++i) bytes.at(address + Word(i)) = in[i];
    }
    void put(Word address, Word value, unsigned width = 4) {
        for (unsigned i = 0; i < width; ++i) bytes[address + i] = std::byte(value >> (i * 8u));
    }
    Word get(Word address) const {
        Word value{};
        for (unsigned i = 0; i < 4u; ++i) value |= std::to_integer<Word>(bytes.at(address + i)) << (i * 8u);
        return value;
    }
    void real(Word address, float value) { put(address, std::bit_cast<Word>(value)); }
};

enum class Fixture { middle, first, absent, empty_header, other_mode, negative_frames,
    null_outputs, aliased_outputs, aliased_local, rounded, negative_zero, fault };

struct Run {
    using Child = SourcePhysicsWorldRuntimeServices::RequiredOwner;
    Memory memory;
    SourcePhysicsWorldRuntimeAbi abi{};
    std::vector<Word> targets;
    std::vector<SourcePhysicsWorldRuntimeAbi> children;
    std::vector<SourcePhysicsContactRecordStore> stores;
    Word prefix_address = prefix, total_address = total;

    explicit Run(Fixture fixture) {
        abi.gp = {graph, 0x900005c0u, prefix, 0x12u, stack, animation, 0x31u, 0x82e05b64u};
        abi.eflags = 0x207u;
        abi.fs_base = 0x60200000u;
        abi.floating_point = controls;
        for (unsigned i = 0; i < abi.xmm.size(); ++i)
            abi.xmm[i] = {0x3f800000u + i, 0x80000000u, i, 0x7fc12345u};
        for (Word address = stack - 128u; address <= stack + 0x98u; address += 4u)
            memory.put(address, 0xb3110000u | (address & 0xffffu));
        if (fixture == Fixture::null_outputs) prefix_address = total_address = 0;
        if (fixture == Fixture::aliased_outputs) total_address = prefix;
        if (fixture == Fixture::aliased_local) total_address = local_prefix;
        memory.put(stack, 0x112320u);
        memory.put(stack + 4u, animation);
        memory.put(stack + 8u, prefix_address);
        memory.put(stack + 12u, total_address);
        memory.put(0x51df20u, 0u, 1);
        memory.put(graph + 0x30u, rows);
        for (Word offset = 0; offset < 0x6cu; offset += 4u) memory.put(descriptor + offset, 0u);
        memory.put(descriptor + 0x11u, fixture == Fixture::other_mode ? 4u : 2u, 1);
        memory.put(descriptor + 0x13u, 9u, 1);
        memory.put(descriptor + 0x14u, fixture == Fixture::negative_frames ? 0xffffu : 3u, 2);
        memory.put(descriptor + 0x38u, data);
        memory.put(descriptor + 0x4cu, 3u);
        memory.put(descriptor + 0x50u, markers);
        memory.put(header, 0xfcu, 1);
        memory.put(header + 1u, 0xfdu, 1);
        memory.put(header + 2u, fixture == Fixture::empty_header ? 0u : 1u, 2);
        memory.put(header + 6u, 0xfffbu, 2);
        memory.put(header + 0xcu, 16u);
        memory.put(markers, 0x00020001u);
        memory.put(markers + 4u, fixture == Fixture::absent ? 1u : fixture == Fixture::first ? 0u : 0x10000u);
        memory.put(markers + 8u, fixture == Fixture::absent ? 2u : 0x20000u);
        memory.real(data + 4u, 1.5f);
        memory.real(data + 8u, -0.25f);
        memory.real(data + 12u, 2.0f);
        if (fixture == Fixture::rounded) {
            abi.floating_point->mxcsr = 0x5f80u;
            memory.real(data + 4u, 1.0f);
            memory.real(data + 8u, 0x1p-25f);
        }
        if (fixture == Fixture::negative_zero) {
            abi.floating_point->mxcsr = 0x3f80u;
            for (Word offset : {4u, 8u, 12u}) memory.put(data + offset, 0x80000000u);
        }
        if (fixture == Fixture::fault) memory.refuse_store = 0x1e3048u;
    }

    bool run(SourcePhysicsContactRuntimeCall actual = call, SourcePhysicsContactImage mode = image,
             const Child &external_child = {}) {
        SourcePhysicsWorldRuntimeServices services;
        services.instruction = [&](Word pc) { memory.pc = pc; memory.trace.push_back(pc); };
        services.required_owner = [&](auto &bank, const auto &child, auto &state, const auto &observe) {
            check(&bank == &memory && bool(observe) && !child.tail, "Child lost source memory or observation");
            targets.push_back(child.target);
            children.push_back(state);
            const auto sp = state.gp[4];
            if (child.target == 0x1e3150u) {
                check(child.instruction == 0x1e2f7eu && sp == stack - 0x28u && state.gp[2] == graph &&
                    memory.get(sp) == 0x1e2f83u && memory.get(sp + 4u) == animation,
                    "Original animation descriptor call ABI differs");
            } else {
                check(child.target == 0x1e5df0u && child.instruction == 0x1e2f9bu && sp == stack - 0x2cu &&
                    state.gp[3] == descriptor && state.gp[7] == stack - 0xcu &&
                    memory.get(sp) == 0x1e2fa0u && memory.get(sp + 4u) == graph &&
                    memory.get(sp + 8u) == animation && !memory.get(state.gp[7]) && !memory.get(state.gp[7] + 4u),
                    "Original animation resource call ABI differs");
            }
            if (external_child) {
                external_child(bank, child, state, observe);
            } else if (child.target == 0x1e3150u) {
                state.gp[0] = descriptor;
                state.gp[4] += 8u;
            } else {
                memory.put(state.gp[7], data);
                memory.put(state.gp[7] + 4u, header);
                state.gp[4] += 12u;
            }
        };
        return native_campaign_animation_distance_tu15_try_run(memory, mode, actual, abi, controls, services,
            [&](const auto &event) { stores.push_back(event); });
    }
};

void check_runs() {
    const auto bits = [](float f) { return std::bit_cast<Word>(f); };
    for (const auto fixture : {Fixture::middle, Fixture::first, Fixture::absent, Fixture::empty_header,
            Fixture::other_mode, Fixture::negative_frames, Fixture::null_outputs, Fixture::aliased_outputs,
            Fixture::aliased_local, Fixture::rounded, Fixture::negative_zero}) {
        Run run(fixture);
        const auto in = run.abi;
        check(run.run() && run.targets == std::vector<Word>{0x1e3150u, 0x1e5df0u}, "Native animation children incomplete");
        check(run.abi.gp[0] == run.prefix_address && run.abi.gp[4] == stack + 16u &&
            run.abi.gp[3] == in.gp[3] && run.abi.gp[5] == in.gp[5] && run.abi.gp[6] == in.gp[6] &&
            run.abi.gp[7] == in.gp[7], "Native animation result or preserved GP differs");
        const bool zero = fixture == Fixture::empty_header || fixture == Fixture::other_mode || fixture == Fixture::negative_frames;
        Word expect_total = zero ? 0u : bits(3.25f);
        Word expect_prefix = zero || fixture == Fixture::absent ? 0u : fixture == Fixture::first ? bits(1.5f) : bits(1.25f);
        if (fixture == Fixture::rounded) { expect_total = bits(3.0f) + 1u; expect_prefix = bits(1.0f) + 1u; }
        if (fixture == Fixture::negative_zero) expect_prefix = expect_total = 0x80000000u;
        if (fixture == Fixture::aliased_local) expect_prefix = expect_total;
        if (fixture == Fixture::aliased_outputs) expect_total = expect_prefix;
        if (run.total_address) check(run.memory.get(run.total_address) == expect_total, "Wrong cumulative binary32 result");
        if (run.prefix_address) check(run.memory.get(run.prefix_address) == expect_prefix, "Wrong marker binary32 result");
        check(run.abi.xmm[1] == in.xmm[1] && run.abi.x87_status == in.x87_status,
            "Unrelated vector or x87 state changed");
    }
}

void check_fault_and_scope() {
    Run fault(Fixture::fault);
    bool stopped = false;
    try { fault.run(); } catch (const RefusedStore &) { stopped = true; }
    check(stopped && fault.memory.pc == 0x1e3048u && fault.memory.get(total) == std::bit_cast<Word>(3.25f) &&
        fault.stores.back().instruction == 0x1e3036u && fault.abi.gp[4] == stack - 20u,
        "Output fault lost precise cursor, committed prefix of effects, or physical stack");

    Run rejected(Fixture::middle);
    check(!rejected.run({0x11231au, call.target, false}) && !rejected.run({call.instruction, call.target, true}) &&
        !rejected.run(call, SourcePhysicsContactImage::MapPack11) && rejected.memory.accesses.empty() &&
        rejected.children.empty() && rejected.stores.empty(), "Unrelated animation caller was not excluded before effects");
}
}

int main() try {
    check_runs();
    check_fault_and_scope();
    std::cout << "Native TU1.5 animation distance checks passed\n";
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
