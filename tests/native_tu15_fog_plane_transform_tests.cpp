#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/native_tu15_fog_plane_transform.hpp"

using namespace frame_misc_harness;
namespace fog = halo2::native_fog_plane_transform_detail;

#include "fog_plane_transform_body.inc"

namespace {
constexpr std::array entries{fog::Entry::angular, fog::Entry::explicit_normal, fog::Entry::advance};
fog::Entry entry = fog::Entry::angular;
void reference(WorldState &s) { log_call(std::uint32_t(native_fog_plane_transform_reference_133b40(s, entry))); }
bool native(WorldState &s) {
    const auto next = fog::bridge<native_fsin,native_fcos,native_single_spill>(s, entry, native_fog_plane_transform_reference_133b40);
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
            throw std::runtime_error("Injected fog transform access fault");
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
    const auto stack = variant % 13 == 3 ? 0xffffffe7u : 0x601ff5u + row % 13;
    const auto plane = variant % 11 == 4 ? stack + 0xcu : 0x710fe0u + row % 11;
    const auto geometry = variant % 7 == 4 ? stack - 0x3eu : 0x700ff1u + row % 15;
    random_abi(rng, c, stack);
    c.abi.gp[1] = plane;
    c.abi.gp[2] = geometry;
    c.abi.x87_depth = variant < 8 ? 0 : variant % 9;
    for (unsigned lane = 0; lane < c.abi.x87_depth; ++lane)
        c.abi.x87[lane] = static_cast<long double>(lane * 7 + 3) / 11.0L;
    c.abi.x87_status = std::uint16_t(c.abi.x87_status | (std::uint32_t(rng()) & 0x3fu));
    const auto data = [&] {
        return variant < 6 ? one : row & 1 ? std::uint32_t(rng()) : special_float(floats);
    };
    for (unsigned offset = 0; offset <= 0x40u; offset += 4)
        c.memory.put(stack + offset, data());
    c.memory.put(plane + 0xcu, data());
    c.memory.put(plane + 0x10u, data());
    c.memory.put(0x46a2ecu, data());
    c.memory.put(0x46a0ecu, variant == 1 ? 0x7f800000u : variant == 0 ? 0u : data());
    constexpr std::array<std::uint64_t,9> radians{0x3fe0000000000000ull, 0x43f0000000000000ull,
        0x0000000000000001ull, 0x8000000000000000ull, 0x7ff0000000000001ull, 0x7ff8000000000123ull,
        0x7ff0000000000000ull, 0xfff0000000000000ull, 0x400921fb54442d18ull};
    const auto angle = radians[variant % radians.size()];
    c.memory.put(0x46a0f0u, std::uint32_t(angle));
    c.memory.put(0x46a0f4u, std::uint32_t(angle >> 32));
    c.memory.put(geometry + 0x78u, variant == 0 ? 2u : variant == 1 ? 1u : std::uint32_t(rng()));
    c.memory.put(stack + 0x14u, variant < 2 ? 0u : std::uint32_t(rng()));
    c.abi.xmm[2][0] = variant < 6 ? 0u : data();
    c.abi.xmm[6][0] = variant < 6 ? one : data();
    return c;
}

bool compare_exact(unsigned row, const Case &c, Totals &totals, unsigned site = 0) {
    FaultMemory expected_bank(c.memory, site), actual_bank(c.memory, site);
    const auto expected = execute(expected_bank, c, false, native, reference);
    const auto actual = execute(actual_bank, c, true, native, reference);
    if ((!actual.admitted && actual.fault.empty()) || (site && expected.fault.empty()) ||
        actual.fault != expected.fault || actual.mxcsr != expected.mxcsr ||
        (!census_mode && actual.cursor != expected.cursor) || actual.stores != expected.stores ||
        actual.calls != expected.calls || !same_abi(actual.abi, expected.abi) || !actual_bank.same(expected_bank) ||
        actual_bank.accesses != expected_bank.accesses) {
        std::fprintf(stderr, "Fog transform row %u, fault %u, cursors %x/%x, ABI %u, x87 %x/%x (depth %u/%u), "
            "MXCSR %x/%x, errors '%s'/'%s'\n",
            row, site, actual.cursor, expected.cursor, unsigned(same_abi(actual.abi, expected.abi)),
            actual.abi.x87_status, expected.abi.x87_status, actual.abi.x87_depth, expected.abi.x87_depth,
            actual.mxcsr, expected.mxcsr, actual.fault.c_str(), expected.fault.c_str());
        return false;
    }
    ++totals.compared;
    totals.faults += !expected.fault.empty();
    totals.stores += expected.stores.size();
    totals.children += expected.calls.size();
    return true;
}
} // namespace

int main(int argc, char **argv) try {
    const std::string mode = argc > 1 ? argv[1] : "literal";
    const auto count = argc > 2 ? unsigned(std::stoul(argv[2])) : 12000u;
    set_mode(mode, "HALO2_VERIFY_FOG_PLANE_TRANSFORM");
    std::mt19937 rng{0x133c2fu};
    std::mt19937_64 floats{0x134020u};
    auto dense = fixture(rng, floats, 0);
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_FOG_PLANE_TRANSFORM", "1", 1);
        const auto result = execute(dense.memory, dense, true, native, reference);
        if (result.admitted || !result.fault.empty() || !result.stores.empty() || !result.calls.empty() ||
            !same_abi(result.abi, dense.abi) || result.cursor)
            throw std::runtime_error("Forced reference touched the fog transform state");
        std::puts("{\"forced_reference_refusals\":1}");
        return 0;
    }
    if (!census_mode)
        check_refusals(dense, native);
    Totals totals;
    for (unsigned row = 0; row < count; ++row) {
        entry = entries[row % entries.size()];
        if (!compare_exact(row, fixture(rng, floats, row), totals))
            return 1;
    }
    unsigned injected_faults{};
    for (const auto row : {0u, 1u, 2u, 3u, 4u, 5u}) {
        entry = entries[row % entries.size()];
        dense = fixture(rng, floats, row);
        FaultMemory memory(dense.memory);
        const auto original = execute(memory, dense, false, native, reference);
        if (!original.fault.empty() || original.calls.size() != 1)
            throw std::runtime_error("Dense fog transform row missed a continuation");
        for (unsigned site = 1; site <= memory.accesses; ++site) {
            if (!compare_exact(row, dense, totals, site))
                return 1;
            ++injected_faults;
        }
    }
    const auto coverage = fog::coverage().load();
    if (coverage != 127)
        throw std::runtime_error("Fog transform missed an outcome");
    std::printf("{\"mode\":\"%s\",\"compared\":%llu,\"faults\":%llu,\"injected_memory_faults\":%u,"
        "\"ordered_stores\":%llu,\"continuations_checked\":%llu,\"coverage\":%llu}\n",
        mode.c_str(), static_cast<unsigned long long>(totals.compared), static_cast<unsigned long long>(totals.faults),
        injected_faults, static_cast<unsigned long long>(totals.stores), static_cast<unsigned long long>(totals.children),
        static_cast<unsigned long long>(coverage));
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
