#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/native_tu15_fog_plane_call.hpp"
#include "halo2/source_vehicle_look_tu15.hpp"

using namespace frame_misc_harness;
namespace fog = halo2::native_fog_plane_call_detail;

// The campaign dispatcher's pinned default handles this target. The
// service below enters the same linked vehicle-look owner as the World.
static void dispatch(WorldState &s, std::uint32_t target, std::uint32_t site, bool tail) {
    s.required(site, target, tail);
}
#include "fog_plane_call_body.inc"

namespace {
enum class Fault { none, push_read, push_write, observer, child_read, missing_child };
struct FaultMemory : Memory {
    mutable unsigned accesses{};
    unsigned fail_at{};
    explicit FaultMemory(const Memory &source, unsigned fail) : fail_at(fail) { pages = source.clone().pages; }
    void access() const {
        if (++accesses == fail_at)
            throw std::runtime_error("Injected fog call memory fault");
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
struct CallResult {
    SourcePhysicsWorldRuntimeAbi after{};
    std::optional<SourcePhysicsWorldRuntimeAbi> child;
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::uint32_t cursor{};
    unsigned calls{}, parent_observations{};
    bool admitted{};
    std::string fault;
};
constexpr std::array sites{0x38846eu, 0x21f590u, 0x33b595u};
bool bridge(WorldState &state) { return fog::bridge(state, native_fog_plane_call_reference_133b40, dispatch); }

CallResult execute_case(FaultMemory &memory, const Case &c, bool native, Fault fault, bool cursor_admission = true) {
    CallResult result;
    SourcePhysicsContactRecordObserver observe = [&](const auto &store) {
        result.stores.push_back(store);
        if (fault == Fault::observer && store.instruction == fog::site)
            throw std::runtime_error("Injected fog call observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](std::uint32_t pc) {
        result.cursor = pc;
        result.parent_observations += pc == fog::site;
    };
    if (cursor_admission && !census_mode) {
        auto &p = services.instruction_cursor;
        p.position = &result.cursor;
        p.service_sites = sites;
        p.callback_type = &services.instruction.target_type();
        p.executor = std::this_thread::get_id();
        p.literal_service_sites = true;
        p.observation_only_stores = true;
    }
    if (fault != Fault::missing_child) {
        services.required_owner = [&](auto &bank, const auto &call, auto &abi, const auto &observer) {
            ++result.calls;
            result.child = abi;
            if (&bank != &memory || call.target != fog::vector_angle || call.instruction != fog::site || call.tail ||
                abi.gp[4] != c.abi.gp[4] - 4u || !source_vehicle_look_tu15_owns_entry(call.target))
                throw std::runtime_error("Wrong required vector-angle boundary");
            source_run_vehicle_look_tu15(bank, call.target, abi, *abi.floating_point, services, observer);
        };
    }
    FpScope scope(*c.abi.floating_point);
    WorldState state{memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(c.abi);
    try {
        if (native)
            result.admitted = bridge(state);
        else
            native_fog_plane_original_133b40(state);
    } catch (const std::exception &error) {
        result.fault = error.what();
    }
    result.after = state.save();
    return result;
}

Case fixture(std::mt19937 &rng, unsigned row) {
    Case c;
    const auto stack = row % 13 == 8 ? 2u : 0x601ff4u + row % 13;
    const auto a = 0x710ff4u + row % 15, b = 0x720ff0u + row % 19;
    random_abi(rng, c, stack);
    c.abi.gp[1] = a;
    c.abi.gp[2] = b;
    c.abi.x87_status = std::uint16_t(c.abi.x87_status | (std::uint32_t(rng()) & 0x3fu));
    for (unsigned bytes = 0; bytes <= 16; bytes += 4)
        c.memory.put(stack - bytes, std::uint32_t(rng()));
    for (unsigned bytes = 0; bytes <= 8; bytes += 4) {
        c.memory.put(a + bytes, 0u);
        c.memory.put(b + bytes, 0x3f800000u + (std::uint32_t(rng()) & 0x7fffu));
    }
    return c;
}

bool abi_equal(const SourcePhysicsWorldRuntimeAbi &a, const SourcePhysicsWorldRuntimeAbi &b) {
    return same_abi(a, b) && a.fs_base == b.fs_base && a.floating_point->mxcsr == b.floating_point->mxcsr;
}
} // namespace

int main(int argc, char **argv) try {
    const std::string mode = argc > 1 ? argv[1] : "literal";
    const auto count = argc > 2 ? unsigned(std::stoul(argv[2])) : 6000u;
    set_mode(mode, "HALO2_VERIFY_FOG_PLANE_CALL");
    std::mt19937 rng{0x133d8au};
    const auto initial = fixture(rng, 0);
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_FOG_PLANE_CALL", "1", 1);
        FaultMemory bank(initial.memory, 0);
        const auto result = execute_case(bank, initial, true, Fault::none);
        if (result.admitted || !result.fault.empty() || bank.accesses || result.calls || result.cursor)
            throw std::runtime_error("Forced reference touched the CALL");
        std::puts("{\"forced_reference_refusals\":1}");
        return 0;
    }
    if (!census_mode)
        check_refusals(initial, bridge);
    constexpr std::array scenarios{Fault::none, Fault::push_read, Fault::push_write, Fault::observer,
                                   Fault::child_read, Fault::missing_child};
    constexpr std::array same_width_accesses{0u, 1u, 2u, 0u, 3u, 0u};
    unsigned compared{}, actual_children{}, observed_stores{};
    for (unsigned row = 0; row < count; ++row) {
        const auto c = fixture(rng, row);
        const auto scenario = scenarios[row % scenarios.size()];
        const auto access = same_width_accesses[row % scenarios.size()];
        FaultMemory expected_bank(c.memory, access), actual_bank(c.memory, access);
        const auto expected = execute_case(expected_bank, c, false, scenario);
        const auto actual = execute_case(actual_bank, c, true, scenario);
        const bool expected_child = scenario == Fault::none || scenario == Fault::child_read;
        if ((!actual.admitted && actual.fault.empty()) || (expected.fault.empty() != (scenario == Fault::none)) ||
            actual.fault != expected.fault || expected.calls != unsigned(expected_child) || actual.calls != expected.calls ||
            actual.stores != expected.stores || !abi_equal(actual.after, expected.after) ||
            bool(actual.child) != bool(expected.child) || (actual.child && !abi_equal(*actual.child, *expected.child)) ||
            actual_bank.accesses != expected_bank.accesses || !actual_bank.same(expected_bank) ||
            actual.parent_observations != 0 || expected.parent_observations != 1 ||
            (!census_mode && actual.cursor != expected.cursor)) {
            std::fprintf(stderr, "Fog call row %u, cursors %x/%x, ESP %x/%x, calls %u/%u, errors '%s'/'%s'\n",
                row, actual.cursor, expected.cursor, actual.after.gp[4], expected.after.gp[4], actual.calls, expected.calls,
                actual.fault.c_str(), expected.fault.c_str());
            return 1;
        }
        ++compared;
        actual_children += actual.calls;
        observed_stores += unsigned(actual.stores.size());
    }
    const auto replayed_children = fog::summary().children.load();
    if (mode == "verify" && replayed_children != (count + 5u) / 6u)
        throw std::runtime_error("Wrong number of recorded successful child replays");
    std::printf("{\"mode\":\"%s\",\"compared\":%u,\"genuine_child_calls\":%u,"
        "\"verified_child_replays\":%llu,\"ordered_stores\":%u}\n",
        mode.c_str(), compared, actual_children, static_cast<unsigned long long>(replayed_children), observed_stores);
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
