#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/native_tu15_fog_cluster_sky.hpp"

using namespace frame_misc_harness;
namespace fog = halo2::native_fog_cluster_sky_detail;

static void run_133a70(WorldState &s);
#include "fog_cluster_sky_body.inc"

namespace {
constexpr auto near_global = 0x4ecc18u;
constexpr auto tags_global = 0x4f0414u;
constexpr auto level = 0x510000u;
constexpr auto clusters = 0x520000u;
constexpr auto scenario = 0x530000u;
constexpr auto skies = 0x540000u;
constexpr auto tags = 0x550000u;
constexpr auto datum = 0x560000u;
constexpr unsigned sky_rows = 13;

struct Access {
    std::uint32_t address;
    std::size_t bytes;
    bool write;
    bool operator==(const Access &) const = default;
};
struct FaultMemory : Memory {
    mutable std::vector<Access> accesses;
    unsigned fail_at{};
    FaultMemory(const Memory &source, unsigned fail) : fail_at(fail) { pages = source.clone().pages; }
    void visit(std::uint32_t address, std::size_t bytes, bool write) const {
        accesses.push_back({address, bytes, write});
        if (accesses.size() == fail_at)
            throw std::runtime_error("Injected campaign fog access fault");
    }
    void read(std::uint32_t address, std::span<std::byte> out) const override {
        visit(address, out.size(), false);
        Memory::read(address, out);
    }
    void write(std::uint32_t address, std::span<const std::byte> data) override {
        visit(address, data.size(), true);
        Memory::write(address, data);
    }
};
struct Observation {
    fog::Next next{};
    SourcePhysicsWorldRuntimeAbi abi{};
    std::optional<SourcePhysicsWorldRuntimeAbi> child;
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::uint32_t cursor{}, parent_literals{};
    unsigned children{};
    std::string fault;
};
thread_local Observation *current{};

bool same_state(const SourcePhysicsWorldRuntimeAbi &a, const SourcePhysicsWorldRuntimeAbi &b) {
    return same_abi(a, b) && a.fs_base == b.fs_base && a.floating_point->mxcsr == b.floating_point->mxcsr;
}
Observation execute(FaultMemory &memory, const Case &c, bool cluster, bool native, unsigned observer_fault = 0) {
    Observation observed;
    SourcePhysicsContactRecordObserver observe = [&](const auto &event) {
        observed.stores.push_back(event);
        if (observed.stores.size() == observer_fault)
            throw std::runtime_error("Injected campaign fog observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) {
        observed.cursor = pc;
        observed.parent_literals += (pc >= fog::cluster_entry && pc <= fog::cluster_exit) ||
                                    (pc >= fog::sky_entry && pc <= fog::sky_exit);
    };
    constexpr std::array sites{0x38846eu, 0x21f590u, 0x33b595u};
    if (!census_mode) {
        auto &policy = services.instruction_cursor;
        policy.position = &observed.cursor;
        policy.callback_type = &services.instruction.target_type();
        policy.executor = std::this_thread::get_id();
        policy.service_sites = sites;
        policy.literal_service_sites = true;
        policy.observation_only_stores = true;
    }
    FpScope scope(*c.abi.floating_point);
    WorldState state{memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(c.abi);
    current = &observed;
    try {
        if (cluster)
            observed.next = native ? fog::cluster(state, native_fog_cluster_sky_cluster_reference_131ac0, run_133a70)
                                   : literal_fog_cluster_sky_cluster_131ac0(state);
        else
            observed.next = native ? fog::sky(state, native_fog_cluster_sky_sky_reference_131ac0)
                                   : native_fog_cluster_sky_sky_reference_131ac0(state);
    } catch (const std::exception &error) {
        observed.fault = error.what();
    }
    current = nullptr;
    observed.abi = state.save();
    return observed;
}

Case fixture(std::mt19937 &rng, unsigned row, bool cluster, bool edge_input) {
    static constexpr std::array<std::array<std::uint32_t,6>,sky_rows> sky_inputs{{
        // Threshold, cap, ambient, spatial distance, full weight, zero.
        {0x3f800000u,0x3f800000u,0x3f800000u,0u,0x3f800000u,0u},
        {0x40000000u,0x3f800000u,0x3f800000u,0x3f000000u,0x3f800000u,0u},
        {0x40000000u,0x40400000u,0x3f800000u,0x3f800000u,0x3f800000u,0u},
        {0u,0x40400000u,0x40000000u,0x3f800000u,0x3f800000u,0u},
        {0x3f800000u,0x3f800000u,0x3f800000u,0x40000000u,0x3f800000u,0u},
        {0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u,0x3f800000u,0u},
        {0x7fc00055u,0x40400000u,0x40000000u,0xff800077u,0x3f800000u,0u},
        {0x3f800000u,0x3f800000u,0x3f800000u,0xbf800000u,0x3f800000u,0u},
        {0x3f800000u,0x7f800077u,0x3f800000u,0x7fc00055u,0xffc00066u,0u},
        {0x3f800000u,0x3f800000u,0x3f800000u,0x3f000000u,0x3f800000u,0x7fc00055u},
        {0x3f800000u,0x80000000u,0x40000000u,0u,0x3f800000u,0u},
        {0x7f800000u,0x7f800000u,0x40000000u,0x7f800000u,0x3f800000u,0u},
        {1u,0x3f800000u,0x40000000u,0x007fffffu,0x3f800000u,0u}
    }};
    Case c;
    const auto stack = row % 19 == 17 ? 2u : 0x801ff8u + row % 19u;
    const auto frame = 0x711ff8u + row % 19u;
    random_abi(rng, c, stack);
    c.abi.gp[5] = frame;
    c.abi.x87_status = std::uint16_t(c.abi.x87_status | (std::uint32_t(rng()) & 0x3fu));
    for (unsigned bias = 0; bias <= 0x20u; bias += 4u)
        c.memory.put(stack - bias, std::uint32_t(rng()));
    for (const auto offset : {0x13u, 0xb8u, 0xbcu, 0x4838u})
        c.memory.put(stack + offset, std::uint32_t(rng()));
    for (const auto offset : {0u, 4u, 8u})
        c.memory.put(frame + offset, std::uint32_t(rng()));
    c.memory.put(near_global, level);
    c.memory.put(level + 0xa0u, clusters);
    c.memory.put(0x4ecc20u, scenario);
    c.memory.put(0x474c24u, 0u, 2);
    c.memory.put(scenario + 0x214u, scenario + 0x1000u);
    c.memory.put(scenario + 0x103cu, row % 4u == 3 ? 0u : 1u, 1);
    c.memory.put(scenario + 0x1040u, 0u, 2);
    c.memory.put(scenario + 8u, 2u);
    c.memory.put(scenario + 12u, skies);
    const auto index = std::uint32_t(rng()) & 7u;
    // IMUL wraps for some valid pointers. Its flags still belong to the call.
    c.memory.put(frame - 8u, index | ((row & 2u) ? 0x10000000u : 0u));
    const auto record = clusters + index * 0xb0u;
    c.memory.put(record + 0x6cu, row % 4u == 0 ? 0u : 255u, 1);
    c.memory.put(record + 0x6eu, row % 4u == 1 ? 1u : 255u, 1);
    c.memory.put(record + 0x6fu, row % 2u ? 255u : std::uint8_t(rng() % 255), 1);
    c.memory.put(skies + 4u, 0x12340005u);
    c.memory.put(skies + 12u, 0xfedc0005u);
    c.memory.put(tags_global, tags);
    c.memory.put(tags + 0x58u, datum);
    c.memory.put(datum + 0x10u, 0u, 1);
    if (!cluster) {
        c.abi.gp[0] = std::uint32_t(rng());
        c.abi.gp[2] = row % sky_rows == 0 ? c.abi.gp[0] : 0xfedc0005u;
        auto inputs = sky_inputs[row % sky_rows];
        if (edge_input) {
            std::mt19937_64 edges{rng()};
            for (auto &word : inputs)
                word = rng() % 2 ? special_float(edges) : std::uint32_t(rng());
        }
        c.memory.put(datum + 0x44u, inputs[0]);
        c.memory.put(0x46a110u, inputs[1]);
        c.memory.put(0x556e10u, inputs[2]);
        c.memory.put(frame - 4u, inputs[3]);
        c.abi.xmm[3][0] = inputs[4];
        c.abi.xmm[7][0] = inputs[5];
    }
    return c;
}

struct StageTotals { unsigned cases{}, faults{}, children{}, accesses{}, stores{}; };
Observation compare(unsigned row, const Case &c, bool cluster, StageTotals &totals, unsigned access = 0, unsigned event = 0) {
    FaultMemory expected_bank(c.memory, access), actual_bank(c.memory, access);
    const auto expect = execute(expected_bank, c, cluster, false, event);
    const auto actual = execute(actual_bank, c, cluster, true, event);
    const bool should_fault = access || event;
    if ((actual.fault.empty() && actual.next == fog::Next::fallback) ||
        expect.fault.empty() == should_fault || actual.fault != expect.fault || actual.next != expect.next ||
        actual.children != expect.children || bool(actual.child) != bool(expect.child) ||
        (actual.child && !same_state(*actual.child, *expect.child)) ||
        !same_state(actual.abi, expect.abi) || actual.stores != expect.stores ||
        actual_bank.accesses != expected_bank.accesses || !actual_bank.same(expected_bank) ||
        actual.parent_literals != 0 || expect.parent_literals == 0 ||
        (!census_mode && actual.cursor != expect.cursor)) {
        std::fprintf(stderr, "row %u (%s), access %u/store %u, cursor %x/%x, next %x/%x, MXCSR %x/%x, error '%s'/'%s'\n",
            row, cluster ? "cluster" : "sky", access, event, actual.cursor, expect.cursor,
            unsigned(actual.next), unsigned(expect.next), actual.abi.floating_point->mxcsr,
            expect.abi.floating_point->mxcsr, actual.fault.c_str(), expect.fault.c_str());
        throw std::runtime_error("Campaign fog cluster or sky differs");
    }
    ++totals.cases;
    totals.children += expect.children;
    totals.accesses += unsigned(expected_bank.accesses.size());
    totals.stores += unsigned(expect.stores.size());
    totals.faults += should_fault;
    return expect;
}
} // namespace

static void run_133a70(WorldState &s) {
    ++current->children;
    current->child = s.save();
    fixture_133a70(s);
}

int main(int argc, char **argv) try {
    const std::string mode = argc > 1 ? argv[1] : "literal";
    const auto count = argc > 2 ? unsigned(std::stoul(argv[2])) : 6000u;
    std::mt19937 rng{0x131ac0u};
    set_mode(mode, "HALO2_VERIFY_FOG_CLUSTER_SKY");
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_FOG_CLUSTER_SKY", "1", 1);
        for (bool cluster : {false, true}) {
            const auto c = fixture(rng, 1, cluster, false);
            FaultMemory bank(c.memory, 0);
            const auto result = execute(bank, c, cluster, true);
            if (result.next != fog::Next::fallback || !bank.accesses.empty() || result.children || result.cursor ||
                !result.fault.empty() || !same_state(result.abi, c.abi))
                throw std::runtime_error("Forced reference touched the fog stage");
        }
        std::puts("{\"forced_reference_refusals\":2}");
        return 0;
    }
    if (!census_mode) {
        const auto initial = fixture(rng, 1, false, false);
        check_refusals(initial, [](WorldState &s) {
            return fog::sky(s, native_fog_cluster_sky_sky_reference_131ac0) != fog::Next::fallback;
        });
        check_refusals(initial, [](WorldState &s) {
            return fog::cluster(s, native_fog_cluster_sky_cluster_reference_131ac0, run_133a70) != fog::Next::fallback;
        });
        FaultMemory bank(initial.memory, 0);
        const SourcePhysicsContactRecordObserver observe;
        const SourcePhysicsWorldRuntimeServices services;
        FpScope scope(*initial.abi.floating_point);
        WorldState state{bank, observe, services, SourcePhysicsContactImage::TitleUpdate15};
        state.load(initial.abi);
        const auto before = state.save();
        const auto csr = _mm_getcsr();
        _mm_setcsr(csr & ~0x80u);
        const auto result = fog::sky(state, native_fog_cluster_sky_sky_reference_131ac0);
        _mm_setcsr(csr);
        if (result != fog::Next::fallback || !bank.accesses.empty() || !same_state(state.save(), before))
            throw std::runtime_error("Unmasked scalar exception admitted to the native sky gate");
    }
    StageTotals clusters_totals, sky_totals;
    for (bool cluster : {false, true}) {
        auto &totals = cluster ? clusters_totals : sky_totals;
        for (unsigned row = 0; row < count; ++row) {
            const auto c = fixture(rng, row, cluster, row >= 2 * sky_rows && row % 3 == 0);
            const auto original = compare(row, c, cluster, totals);
            if (row >= 2 * sky_rows)
                continue;
            FaultMemory bank(c.memory, 0);
            execute(bank, c, cluster, false);
            for (unsigned access = 1; access <= bank.accesses.size(); ++access)
                compare(row, c, cluster, totals, access);
            for (unsigned event = 1; event <= original.stores.size(); ++event)
                compare(row, c, cluster, totals, 0, event);
        }
    }
    const auto cluster_coverage = fog::coverage<fog::cluster_entry>().load();
    const auto sky_coverage = fog::coverage<fog::sky_entry>().load();
    if (cluster_coverage != 3u || sky_coverage != 255u ||
        (mode == "verify" && fog::summary<fog::cluster_entry>().children.load() != count))
        throw std::runtime_error("Incomplete fog cluster sky or child replay coverage");
    std::printf("{\"mode\":\"%s\",\"cluster_cases\":%u,\"sky_cases\":%u,\"faults\":%u,"
        "\"ordered_accesses\":%u,\"ordered_stores\":%u,\"genuine_child_calls\":%u,"
        "\"verified_child_replays\":%llu,\"coverage\":[%llu,%llu]}\n",
        mode.c_str(), clusters_totals.cases, sky_totals.cases, clusters_totals.faults + sky_totals.faults,
        clusters_totals.accesses + sky_totals.accesses, clusters_totals.stores + sky_totals.stores,
        clusters_totals.children, static_cast<unsigned long long>(fog::summary<fog::cluster_entry>().children.load()),
        static_cast<unsigned long long>(cluster_coverage), static_cast<unsigned long long>(sky_coverage));
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
