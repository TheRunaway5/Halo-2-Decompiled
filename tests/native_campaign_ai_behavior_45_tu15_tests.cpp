#include "halo2/native_campaign_ai_behavior_45_tu15.hpp"

#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

namespace {
using namespace halo2;
using Word = std::uint32_t;
constexpr auto image = SourcePhysicsContactImage::TitleUpdate15;
constexpr Word descriptor = 0x48a1f8u;
constexpr Word actor = 0xe3610002u;
constexpr Word pool = 0x82fad850u, rows = 0x82fad89cu;
constexpr Word row = rows + (actor & 0xffffu) * 0x888u;

void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

struct Memory final : SourcePhysicsContactMemory {
    std::map<Word, std::byte> bytes;
    Word pc{};

    void read(Word address, std::span<std::byte> output) const override {
        for (std::size_t i = 0; i < output.size(); ++i) {
            auto it = bytes.find(address + Word(i));
            output[i] = it == bytes.end() ? std::byte{0} : it->second;
        }
    }
    void write(Word address, std::span<const std::byte> input) override {
        for (std::size_t i = 0; i < input.size(); ++i) bytes[address + Word(i)] = input[i];
    }
    void put(Word address, Word value, unsigned width = 4) {
        for (unsigned i = 0; i < width; ++i) bytes[address + i] = std::byte(value >> (i * 8u));
    }
    Word get(Word address, unsigned width = 4) const {
        Word value{};
        for (unsigned i = 0; i < width; ++i) {
            auto it = bytes.find(address + i);
            const auto b = it == bytes.end() ? std::byte{0} : it->second;
            value |= std::to_integer<Word>(b) << (i * 8u);
        }
        return value;
    }
};

void test_select_45() {
    auto run_case = [](Word count, Word flag) {
        Memory mem;
        constexpr Word stack = 0x600eff40u;
        mem.put(0x5020c8u, pool);
        mem.put(pool + 0x44u, rows);
        mem.put(0x47c1a4u, 0x0000ffffu);
        mem.put(0x47c1a8u, 0x12340001u);
        mem.put(stack, 0x1afce6u);
        mem.put(stack + 4u, actor);
        mem.put(row + 0x358u, count, 2);
        mem.put(row + 0x35eu, flag, 1);

        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp = {0x1b1fa0u, 0u, 0u, descriptor, stack, 0u, 0u, 0u};
        SourcePhysicsWorldRuntimeServices services;
        services.instruction = [&](Word pc) { mem.pc = pc; };
        check(native_campaign_ai_behavior_select_45_tu15_try_run(
                  mem, image, {0x1afce4u, 0x1b1fa0u, false}, abi, services),
            "select 45 refused valid call");
        check(abi.gp[4] == stack + 16u, "select 45 stack cleanup mismatch");
        return abi.gp[0];
    };
    check(run_case(1, 0) == 0x12340001u, "select 45 should continue when active and flag clear");
    check(run_case(0, 0) == 0x1234ffffu, "select 45 should cancel when count is zero");
    check(run_case(2, 1) == 0x1234ffffu, "select 45 should cancel when flag is set");
}

void test_perform_45() {
    constexpr Word stack = 0x600eff50u;
    // Inactive path skips children and returns al=1.
    {
        Memory mem;
        mem.put(0x5020c8u, pool);
        mem.put(pool + 0x44u, rows);
        mem.put(stack, 0x1b0adbu);
        mem.put(stack + 4u, actor);
        mem.put(row + 0x40u, 0u, 1);
        mem.put(row + 0x358u, 1u, 2);

        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp = {0x1b1fe0u, actor, 0x2fdu, 2u, stack, 0x82fad89cu, descriptor, 0x82fad96cu};
        SourcePhysicsWorldRuntimeServices services;
        services.required_owner = [](auto &, const auto &, auto &, const auto &) {
            throw std::runtime_error("inactive perform 45 called child");
        };
        check(native_campaign_ai_behavior_perform_45_tu15_try_run(
                  mem, image, {0x1b0ad9u, 0x1b1fe0u, false}, abi, services),
            "perform 45 refused valid inactive call");
        check((abi.gp[0] & 0xffu) == 1u && abi.gp[4] == stack + 12u, "perform 45 inactive return mismatch");
        check(abi.gp[3] == 2u && abi.gp[5] == 0x82fad89cu && abi.gp[6] == descriptor && abi.gp[7] == 0x82fad96cu,
            "perform 45 clobbered callee-saved register");
    }

    // Active path invokes 1D2D90 -> 26AF90 -> 26C3C0 -> 1D2E30 in exact order with initialized workspace.
    {
        Memory mem;
        mem.put(0x5020c8u, pool);
        mem.put(pool + 0x44u, rows);
        mem.put(stack, 0x1b0adbu);
        mem.put(stack + 4u, actor);
        mem.put(row + 0x40u, 1u, 1);
        mem.put(row + 0x358u, 3u, 2);

        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp = {0x1b1fe0u, actor, 0x2fdu, 0x11223344u, stack, 0x82fad89cu, descriptor, 0x82fad96cu};
        std::vector<Word> called;
        Word workspace_addr = 0;
        SourcePhysicsWorldRuntimeServices services;
        services.required_owner = [&](SourcePhysicsContactMemory &m, const SourcePhysicsContactRuntimeCall &c,
                                      SourcePhysicsWorldRuntimeAbi &a, const SourcePhysicsContactRecordObserver &) {
            called.push_back(c.target);
            if (c.target == 0x1d2d90u) {
                check(mem.get(a.gp[4]) == 0x1b2021u, "1D2D90 return pc mismatch");
                a.gp[0] = 0x55aa1234u;
                a.gp[4] += 4u;
            } else if (c.target == 0x26af90u) {
                check(mem.get(a.gp[4]) == 0x1b205bu, "26AF90 return pc mismatch");
                check(mem.get(a.gp[4] + 4u) == actor, "26AF90 arg0 actor mismatch");
                check(mem.get(a.gp[4] + 8u) == 0u, "26AF90 arg1 zero mismatch");
                const Word out0 = mem.get(a.gp[4] + 12u);
                check(mem.get(a.gp[4] + 16u) == 0x55aa1234u, "26AF90 arg3 scratch mismatch");
                const Word out1 = mem.get(a.gp[4] + 20u);
                workspace_addr = a.gp[0];
                check(mem.get(workspace_addr, 2) == 6u, "workspace +0x00 should be 6");
                check(mem.get(workspace_addr + 0x14u, 1) == 1u, "workspace +0x14 should be 1");
                check(mem.get(workspace_addr + 0x59u, 1) == 1u, "workspace +0x59 should be 1");
                mem.put(out0, 0x77778888u);
                mem.put(out1, 0x9999aaaau);
                a.gp[0] = 0x42u;
                a.gp[4] += 24u;
            } else if (c.target == 0x26c3c0u) {
                check(mem.get(a.gp[4]) == 0x1b206fu, "26C3C0 return pc mismatch");
                check(mem.get(a.gp[4] + 4u) == actor, "26C3C0 arg0 actor mismatch");
                check(mem.get(a.gp[4] + 8u) == 0x42u, "26C3C0 arg1 result mismatch");
                check(mem.get(a.gp[4] + 12u) == 0x77778888u, "26C3C0 arg2 out0 mismatch");
                check(mem.get(a.gp[4] + 16u) == 0x55aa1234u, "26C3C0 arg3 scratch mismatch");
                check(mem.get(a.gp[4] + 20u) == 0x9999aaaau, "26C3C0 arg4 out1 mismatch");
                check(mem.get(a.gp[4] + 24u) == 1u, "26C3C0 arg5 one mismatch");
                a.gp[4] += 28u;
            } else if (c.target == 0x1d2e30u) {
                check(mem.get(a.gp[4]) == 0x1b2076u, "1D2E30 return pc mismatch");
                check(a.gp[2] == 0x55aa1234u, "1D2E30 edx scratch mismatch");
                a.gp[4] += 4u;
            } else {
                throw std::runtime_error("unexpected child target");
            }
            (void)m;
        };
        check(native_campaign_ai_behavior_perform_45_tu15_try_run(
                  mem, image, {0x1b0ad9u, 0x1b1fe0u, false}, abi, services),
            "perform 45 refused valid active call");
        check(called == std::vector<Word>({0x1d2d90u, 0x26af90u, 0x26c3c0u, 0x1d2e30u}),
            "perform 45 child call order mismatch");
        check((abi.gp[0] & 0xffu) == 1u && abi.gp[4] == stack + 12u, "perform 45 active return mismatch");
        check(abi.gp[3] == 0x11223344u && abi.gp[5] == 0x82fad89cu &&
                  abi.gp[6] == descriptor && abi.gp[7] == 0x82fad96cu,
            "perform 45 clobbered callee-saved register");
    }
}

void test_end_45() {
    auto run_case = [](Word target_handle, Word child_result, Word child_word, Word count) {
        Memory mem;
        constexpr Word stack = 0x600eff40u;
        mem.put(0x5020c8u, pool);
        mem.put(pool + 0x44u, rows);
        mem.put(stack, 0x1b0f58u);
        mem.put(stack + 4u, actor);
        mem.put(row + 0x338u, target_handle);
        mem.put(row + 0x358u, count, 2);
        mem.put(row + 0x488u, 0u, 1);
        if (child_result != 0) mem.put(child_result, child_word, 2);

        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp = {0u, 0x1b2090u, 0u, 0u, stack, 0u, 0xdeadbeefu, 0u};
        SourcePhysicsWorldRuntimeServices services;
        services.required_owner = [&](auto &, const auto &c, auto &a, const auto &) {
            check(c.target == 0x267410u && a.gp[1] == target_handle, "267410 call mismatch");
            a.gp[0] = child_result;
            a.gp[4] += 4u;
        };
        check(native_campaign_ai_behavior_end_45_tu15_try_run(
                  mem, image, {0x1b0f56u, 0x1b2090u, false}, abi, services),
            "end 45 refused valid call");
        check(abi.gp[4] == stack + 12u && abi.gp[6] == 0xdeadbeefu, "end 45 return/esi mismatch");
        return std::tuple<Word, Word, Word>{
            mem.get(row + 0x488u, 1), mem.get(row + 0x41cu, 2), mem.get(row + 0x420u, 2)};
    };

    check(run_case(0x12340001u, 0x83000000u, 6u, 0u) == std::tuple<Word, Word, Word>{1u, 4u, 2u},
        "end 45 high-priority posture mismatch");
    check(run_case(0xffffffffu, 0u, 0u, 2u) == std::tuple<Word, Word, Word>{0u, 2u, 5u},
        "end 45 positive-count posture mismatch");
    check(run_case(0x12340001u, 0x83000000u, 5u, 0u) == std::tuple<Word, Word, Word>{0u, 2u, 2u},
        "end 45 fallback posture mismatch");
}

} // namespace

int main() {
    test_select_45();
    test_perform_45();
    test_end_45();
    std::cout << "native_campaign_ai_behavior_45_tu15_tests: PASS\n";
    return 0;
}
