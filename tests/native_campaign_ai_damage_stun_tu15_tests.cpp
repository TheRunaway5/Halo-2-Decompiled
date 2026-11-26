#include "halo2/native_campaign_ai_damage_stun_tu15.hpp"

#include <bit>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

namespace {
using namespace halo2;
using Word = std::uint32_t;
constexpr auto image = SourcePhysicsContactImage::TitleUpdate15;

void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

struct Memory final : SourcePhysicsContactMemory {
    std::map<Word, std::byte> bytes;
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

Word fbits(float v) { return std::bit_cast<Word>(v); }

void test_stun_applied_and_fallback_vector() {
    for (bool zero_norm : {false, true}) {
        Memory mem;
        constexpr Word stack = 0x600ef82cu;
        constexpr Word ai_globals = 0x82f00000u;
        constexpr Word obj_pool = 0x82e00000u, obj_rows = 0x82e10000u, obj_data = 0x82e45710u;
        constexpr Word act_pool = 0x82fa0000u, act_rows = 0x82fad89cu;
        constexpr Word obj_handle = 0xe5670002u, act_handle = 0xe3610003u;
        constexpr Word act_row = act_rows + 3u * 0x888u;
        constexpr Word props = 0x83100000u;
        constexpr Word dir_in = 0x600efa1cu;
        constexpr Word fallback_vec = 0x44ac28u;

        mem.put(0x5020a8u, ai_globals);
        mem.put(ai_globals + 1u, 1u, 1);
        mem.put(0x4ecbd0u, obj_pool);
        mem.put(obj_pool + 0x44u, obj_rows);
        mem.put(obj_rows + 2u * 12u + 8u, obj_data);
        mem.put(obj_data + 0x12cu, act_handle);
        mem.put(0x5020c8u, act_pool);
        mem.put(act_pool + 0x44u, act_rows);
        mem.put(act_row + 0x54u, 0x12345678u);
        mem.put(props + 0x50u, fbits(0.25f));
        mem.put(0x46a0e8u, fbits(0.0f));
        mem.put(0x474d08u, fallback_vec);
        mem.put(fallback_vec, fbits(1.0f));
        mem.put(fallback_vec + 4u, fbits(0.0f));
        mem.put(fallback_vec + 8u, fbits(0.0f));
        mem.put(dir_in, fbits(0.0f));
        mem.put(dir_in + 4u, fbits(3.0f));
        mem.put(dir_in + 8u, fbits(4.0f));
        mem.put(stack, 0xcca59u);

        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp = {obj_handle, 0u, 0u, 0u, stack, 0u, 0xdeadbeefu, dir_in};
        abi.xmm[0][0] = fbits(0.75f);
        const SourcePhysicsFactoryFloatingPoint fp{0x027fu, 0x1f80u};
        abi.floating_point = fp;

        std::vector<Word> calls;
        SourcePhysicsWorldRuntimeServices services;
        services.required_owner = [&](auto &, const auto &c, auto &a, const auto &) {
            calls.push_back(c.target);
            if (c.target == 0x1eccc0u) {
                check(a.gp[1] == 0x12345678u, "1ECCC0 character tag mismatch");
                a.gp[0] = props;
                a.gp[4] += 4u;
            } else if (c.target == 0x30bd0u) {
                check(a.gp[0] == act_row + 0x3c0u, "30BD0 vector ptr mismatch");
                a.xmm[0][0] = fbits(zero_norm ? 0.0f : 5.0f);
                a.gp[4] += 4u;
            } else {
                throw std::runtime_error("unexpected child call");
            }
        };

        check(native_campaign_ai_damage_stun_tu15_try_run(
                  mem, image, {0xcb8d4u, 0x1d20c0u, false}, abi, fp, services),
            "1D20C0 refused valid call");
        check(calls == std::vector<Word>({0x1eccc0u, 0x30bd0u}), "1D20C0 child order mismatch");
        check(abi.gp[4] == stack + 4u && abi.gp[6] == 0xdeadbeefu, "1D20C0 return/esi mismatch");
        check(mem.get(act_row + 0x3b8u, 1) == 1u, "stun flag not set");
        check(mem.get(act_row + 0x3bcu) == fbits(0.75f), "stun amount not stored");
        if (zero_norm) {
            check(mem.get(act_row + 0x3c0u) == fbits(1.0f) &&
                      mem.get(act_row + 0x3c4u) == fbits(0.0f) &&
                      mem.get(act_row + 0x3c8u) == fbits(0.0f),
                "fallback vector not copied on zero length");
        } else {
            check(mem.get(act_row + 0x3c0u) == fbits(0.0f) &&
                      mem.get(act_row + 0x3c4u) == fbits(3.0f) &&
                      mem.get(act_row + 0x3c8u) == fbits(4.0f),
                "input vector overwritten on nonzero length");
        }
    }
}

void test_early_exits() {
    Memory mem;
    constexpr Word stack = 0x600ef82cu;
    mem.put(0x5020a8u, 0x82f00000u);
    mem.put(0x82f00001u, 0u, 1); // AI disabled
    mem.put(stack, 0xcca59u);
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp = {0xe5670002u, 0u, 0u, 0u, stack, 0u, 0u, 0u};
    const SourcePhysicsFactoryFloatingPoint fp{0x027fu, 0x1f80u};
    abi.floating_point = fp;
    SourcePhysicsWorldRuntimeServices services;
    check(native_campaign_ai_damage_stun_tu15_try_run(
              mem, image, {0xcb8d4u, 0x1d20c0u, false}, abi, fp, services),
        "1D20C0 refused disabled-AI call");
    check(abi.gp[4] == stack + 4u, "1D20C0 early exit stack mismatch");
}

} // namespace

int main() {
    test_stun_applied_and_fallback_vector();
    test_early_exits();
    std::cout << "native_campaign_ai_damage_stun_tu15_tests: PASS\n";
    return 0;
}
