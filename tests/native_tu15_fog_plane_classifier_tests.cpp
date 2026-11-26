#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/native_tu15_fog_plane_classifier.hpp"

using namespace frame_misc_harness;
namespace fog = halo2::native_fog_plane_classifier_detail;

#include "fog_plane_classifier_body.inc"

namespace {
constexpr std::array entries{fog::Entry::initialize, fog::Entry::classify, fog::Entry::normal, fog::Entry::member};
fog::Entry entry = fog::Entry::initialize;
void reference(WorldState &s) { log_call(std::uint32_t(native_fog_plane_classifier_reference_133b40(s, entry))); }
bool native(WorldState &s) {
    const auto next = fog::bridge(s, entry, native_fog_plane_classifier_reference_133b40);
    if (next == fog::Next::fallback)
        return false;
    log_call(std::uint32_t(next));
    return true;
}
struct FaultMemory : Memory {
    mutable unsigned accesses{};
    unsigned fail_at{};
    explicit FaultMemory(const Memory &source, unsigned fail = 0) : fail_at(fail) { pages = source.clone().pages; }
    void access() const {
        if (++accesses == fail_at)
            throw std::runtime_error("Injected fog plane access fault");
    }
    void read(std::uint32_t at, std::span<std::byte> out) const override {
        access();
        Memory::read(at, out);
    }
    void write(std::uint32_t at, std::span<const std::byte> in) override {
        access();
        Memory::write(at, in);
    }
};

constexpr std::uint32_t one = 0x3f800000u;
Case fixture(std::mt19937 &rng, std::mt19937_64 &floats, unsigned row) {
    Case c;
    const unsigned variant = row / entries.size();
    const auto stack = variant % 13 == 10 ? 0x17u : 0x60100bu + row % 13;
    const auto recorded = 0x620ff2u + variant % 7;
    const auto plane = variant % 7 == 5 ? stack + 0x20u : 0x640ff0u + row % 9;
    constexpr auto table = 0x630000u;
    random_abi(rng, c, stack);
    for (unsigned offset = 0; offset <= 0x80u; offset += 4) {
        c.memory.put(stack + offset - 0x38u, std::uint32_t(rng()));
        c.memory.put(recorded + offset, std::uint32_t(rng()));
        c.memory.put(plane + offset, std::uint32_t(rng()));
    }
    const auto data = [&] {
        return variant < 10 ? one : row & 1 ? std::uint32_t(rng()) : special_float(floats);
    };
    switch (entries[row % entries.size()]) {
    case fog::Entry::initialize: {
        const auto output0 = variant % 11 == 8 ? recorded + 0x68u : 0x650ff9u + row % 5;
        const auto output1 = variant % 3 == 2 ? output0 + 3u : 0x660ff2u + row % 7;
        const auto output2 = variant % 7 == 4 ? stack - 0x31u : 0x670ff9u + row % 6;
        for (unsigned offset = 0; offset < 32; offset += 4) {
            c.memory.put(output0 + offset, data());
            c.memory.put(output1 + offset, data());
            c.memory.put(output2 + offset, data());
        }
        c.abi.gp[7] = output0;
        c.abi.gp[3] = output1;
        c.abi.gp[0] = (c.abi.gp[0] & 0xffff0000u) | (variant % 41);
        c.memory.put(stack + 8u, output2);
        c.memory.put(0x4f0414u, table);
        c.memory.put(table + (variant % 41) * 16u + 8u, recorded);
        c.memory.put(0x46a0d0u, data());
        constexpr std::array counts{1u, 0u, 0x80000000u, 2u, 0xffffffffu, 0x7fffffffu};
        c.memory.put(recorded + 0x78u, counts[variant % counts.size()]);
        break;
    }
    case fog::Entry::classify: {
        constexpr std::array offsets{0u, 0xfff00000u, 48u, 0x80000000u};
        const auto offset = offsets[variant % offsets.size()];
        c.abi.gp[2] = recorded;
        c.memory.put(stack + 0x40u, offset);
        c.memory.put(recorded + 0x7cu, plane - offset);
        for (unsigned component = 0; component < 3; ++component) {
            const auto value = variant < 4 ? (variant == component + 1 ? one : component == 1 ? 0x80000000u : 0u) : data();
            c.memory.put(plane + component * 4, value);
        }
        break;
    }
    case fog::Entry::normal:
        c.abi.xmm[1][0] = variant == 1 ? 0u : data();
        c.abi.xmm[3][0] = variant == 1 ? 0u : data();
        c.abi.xmm[4][0] = variant == 1 ? 0u : data();
        c.abi.xmm[2][0] = variant < 10 || variant % 3 ? 0u : data();
        [[fallthrough]];
    case fog::Entry::member:
        c.abi.gp[0] = variant % 3;
        c.abi.gp[1] = plane;
        c.memory.put(plane + 0x1cu, (variant / 3) % 2 ? 0u : 1u);
        c.memory.put(plane + 0x20u, recorded);
        c.memory.put(plane + 0x24u, (variant / 3) % 2 ? 0x80000000u : 1u);
        c.memory.put(plane + 0x28u, recorded + 0x20u);
        break;
    }
    return c;
}
} // namespace

int main(int argc, char **argv) try {
    const std::string mode = argc > 1 ? argv[1] : "literal";
    const auto count = argc > 2 ? unsigned(std::stoul(argv[2])) : 12000u;
    set_mode(mode, "HALO2_VERIFY_FOG_PLANE_CLASSIFIER");
    std::mt19937 rng{0x133b40u};
    std::mt19937_64 floats{0x133d1du};
    auto dense = fixture(rng, floats, 0);
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_FOG_PLANE_CLASSIFIER", "1", 1);
        const auto result = execute(dense.memory, dense, true, native, reference);
        if (result.admitted || !result.fault.empty() || !result.stores.empty() || !result.calls.empty() ||
            !same_abi(result.abi, dense.abi) || result.cursor)
            throw std::runtime_error("Forced reference touched the fog plane state");
        std::puts("{\"forced_reference_refusals\":1}");
        return 0;
    }
    if (!census_mode)
        check_refusals(dense, native);
    Totals totals;
    for (unsigned row = 0; row < count; ++row) {
        entry = entries[row % entries.size()];
        if (!compare(row, fixture(rng, floats, row), native, reference, totals))
            return 1;
    }
    for (const auto row : {0u, 1u, 2u, 7u}) {
        entry = entries[row % entries.size()];
        dense = fixture(rng, floats, row);
        FaultMemory memory(dense.memory);
        const auto original = execute(memory, dense, false, native, reference);
        if (!original.fault.empty() || original.calls.empty())
            throw std::runtime_error("Dense fog plane row missed its continuation");
        for (unsigned site = 1; site <= memory.accesses; ++site) {
            FaultMemory expected_bank(dense.memory, site), actual_bank(dense.memory, site);
            const auto expected = execute(expected_bank, dense, false, native, reference);
            const auto actual = execute(actual_bank, dense, true, native, reference);
            if (expected.fault.empty() || actual.fault != expected.fault ||
                actual.mxcsr != expected.mxcsr || (!census_mode && actual.cursor != expected.cursor) ||
                actual.stores != expected.stores || !same_abi(actual.abi, expected.abi) || !actual_bank.same(expected_bank)) {
                std::fprintf(stderr, "Fault %u in entry %x, cursor %x/%x, ABI %u, errors '%s'/'%s'\n",
                    site, std::uint32_t(entry), actual.cursor, expected.cursor, unsigned(same_abi(actual.abi, expected.abi)),
                    actual.fault.c_str(), expected.fault.c_str());
                return 1;
            }
            ++totals.compared;
            ++totals.faults;
            totals.stores += actual.stores.size();
        }
    }
    const auto coverage = fog::coverage().load();
    if (coverage != 8191)
        throw std::runtime_error("Fog plane classifier missed an outcome");
    std::printf("{\"mode\":\"%s\",\"compared\":%llu,\"memory_faults\":%llu,"
        "\"ordered_stores\":%llu,\"continuations_checked\":%llu,\"coverage\":%llu}\n",
        mode.c_str(), static_cast<unsigned long long>(totals.compared), static_cast<unsigned long long>(totals.faults),
        static_cast<unsigned long long>(totals.stores), static_cast<unsigned long long>(totals.children),
        static_cast<unsigned long long>(coverage));
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
