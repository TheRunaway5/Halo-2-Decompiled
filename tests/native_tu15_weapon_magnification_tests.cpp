#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/vehicles/native_tu15_weapon_magnification.hpp"
#include <cmath>

using namespace frame_misc_harness;
namespace magnification = halo2::native_weapon_magnification_detail;
namespace {
constexpr std::uint32_t object_array = 0x10000000u;
constexpr std::uint32_t headers = 0x10100000u;
constexpr std::uint32_t object = 0x10200000u;
constexpr std::uint32_t definitions = 0x10300000u;
constexpr std::uint32_t scratch = 0x10500000u;

void power_owner(WorldState &s) {
    constexpr std::uint32_t target = 0x37cde8u;
    auto hash = mix(entry_hash(s, target), r(s.memory, s.gp[4], 4));
    hash = mix(hash, s.fp.word());
    for (unsigned lane = 0; lane < s.fp.count; ++lane) {
        std::array<std::byte, 10> bytes;
        std::memcpy(bytes.data(), &s.fp.st(lane), bytes.size());
        for (auto byte : bytes) hash = mix(hash, std::to_integer<unsigned>(byte));
    }
    for (unsigned slot = 1; slot <= 3; ++slot)
        hash = mix(hash, r(s.memory, s.gp[4] + slot * 4u, 4));
    log_call(hash);
    w(s.memory, s.observe, target, scratch, 4, std::uint32_t(hash));
    // The real child uses the double-precision CRT power route. Preserve the
    // same x87 input and output stack shape while recording its entry state.
    const auto exponent = static_cast<double>(s.fp.st(0));
    const auto base = static_cast<double>(s.fp.st(1));
    const auto powered = std::pow(base, exponent);
    s.fp.pop();
    s.fp.st(0) = static_cast<long double>(powered);
    s.fp.status = (s.fp.status & ~0x4700u) | (std::uint32_t(hash) & 0x4700u);
    s.gp[0] = std::uint32_t(hash);
    s.flags.load(std::uint32_t(hash) & 0x8d5u);
    s.gp[4] += (policy & 2u) ? 8u : 4u;
}
} // namespace

namespace original {
[[maybe_unused]] std::uint32_t selected(std::uint32_t address, SourcePhysicsContactImage) { return address; }
[[maybe_unused]] void run_37cde8(WorldState &s) { power_owner(s); }
[[maybe_unused]] void dispatch(WorldState &s, std::uint32_t target, std::uint32_t site, bool tail) {
    if (target != 0x37cde8u || site != 0x10620du || tail)
        throw std::runtime_error("Unexpected weapon magnification dispatch");
    power_owner(s);
}
std::uint32_t native_single_spill(long double value, std::uint16_t &status) {
    float result;
    std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1"
        : "=m"(result), "=m"(actual) : "m"(value) : "st");
    status = (status & ~0x200u) | (actual & 0x200u);
    return std::bit_cast<std::uint32_t>(result);
}
#include "body.inc"
#include "reference.inc"
} // namespace original

namespace {
Case author(std::mt19937 &random, unsigned n) {
    Case result;
    auto &memory = result.memory;
    const auto stack = 0x20010040u + 4u * std::uint32_t(random() % 64);
    random_abi(random, result, stack);
    if (n % 23 == 0) result.abi.x87_depth = 7;

    constexpr std::pair<std::int16_t, std::int16_t> level_and_count[]{
        {-32768, 4}, {-1, 5}, {0, 0}, {2, 2}, {1, -1}, {0, 1},
        {0, 3}, {1, 2}, {2, 5}, {5, 6}, {43, 89}, {32610, 32767}
    };
    const auto [level, count] = level_and_count[n % std::size(level_and_count)];
    const auto object_index = std::uint32_t(random() % 512);
    const auto tag_index = std::uint32_t(random() % 512);
    result.abi.gp[0] = (result.abi.gp[0] & 0xffff0000u) | object_index;
    result.abi.gp[1] = (result.abi.gp[1] & 0xffff0000u) | std::uint16_t(level);

    for (std::uint32_t address = stack - 0x20u; address < stack + 0x20u; address += 4)
        memory.put(address, std::uint32_t(random()));
    const auto definition = n % 13 == 0 ? stack - 0x20cu : 0x10400000u;
    memory.put(0x4ecbd0u, object_array);
    memory.put(object_array + 0x44u, headers);
    memory.put(headers + object_index * 12u + 8u, object);
    memory.put(object, (std::uint32_t(random()) & 0xffff0000u) | tag_index);
    memory.put(0x4f0414u, definitions);
    memory.put(definitions + tag_index * 16u + 8u, definition);
    memory.put(definition + 0x1feu, std::uint16_t(count), 2);

    std::mt19937_64 wide(random());
    const auto bits = [](float value) { return std::bit_cast<std::uint32_t>(value); };
    memory.put(0x46a0d0u, n % 11 == 0 ? special_float(wide) : bits(1.0f));
    memory.put(definition + 0x200u, n % 3 == 0 ? special_float(wide)
        : bits(1.1f + float(random() % 15) / 7.0f));
    memory.put(definition + 0x204u, n % 5 == 0 ? special_float(wide)
        : bits(3.3f + float(random() % 130) / 11.0f));
    memory.put(scratch, 0u);
    if (n % 13 == 1) {
        const std::uint32_t missing[]{0x4ecbd0u, object_array, headers + object_index * 12u + 8u,
            object, definitions + tag_index * 16u + 8u, definition, stack, scratch};
        memory.drop(missing[random() % std::size(missing)]);
    }
    return result;
}

bool native(WorldState &s) {
    return magnification::bridge(s, original::native_frame_misc_reference_106160, power_owner);
}
} // namespace

int main(int argc, char **argv) try {
    const unsigned cases = argc > 1 ? unsigned(std::stoul(argv[1])) : 4000;
    const std::string mode = argc > 2 ? argv[2] : "";
    set_mode(mode, "HALO2_VERIFY_WEAPON_MAGNIFICATION");
    std::mt19937 random(0x106160u + (census_mode ? 1u : 0u) + (mode == "verify" ? 2u : 0u));
    Totals totals;
    for (unsigned n = 0; n < cases; ++n) {
        if (!compare(n, author(random, n), native, original::run_106160, totals)) return 1;
    }
    if (!census_mode) check_refusals(author(random, 1), native);
    report(cases, totals, magnification::coverage().load(), unsigned(magnification::branch_count), mode);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
