#include "native_tu15_frame_misc_harness.hpp"
#include "campaign/native_tu15_fog_resolution.hpp"

using namespace frame_misc_harness;
namespace fog = halo2::native_fog_resolution_detail;

#include "fog_resolution_body.inc"

namespace {
struct Access {
    std::uint32_t address;
    std::size_t width;
    bool write;
    bool operator==(const Access &) const = default;
};
struct LoggedBank : Memory {
    mutable std::vector<Access> accesses;
    std::size_t fail_at{};
    explicit LoggedBank(const Memory &memory) { pages = memory.clone().pages; }
    void access(std::uint32_t address, std::size_t length, bool write) const {
        accesses.push_back({address, length, write});
        if (accesses.size() == fail_at)
            throw std::runtime_error("Injected fog memory fault");
    }
    void read(std::uint32_t address, std::span<std::byte> out) const override {
        access(address, out.size(), false);
        Memory::read(address, out);
    }
    void write(std::uint32_t address, std::span<const std::byte> in) override {
        access(address, in.size(), true);
        Memory::write(address, in);
    }
};
struct FogResult {
    SourcePhysicsWorldRuntimeAbi abi{};
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::uint32_t cursor{}, mxcsr{};
    std::size_t observations{};
    bool admitted{};
    std::string fault;
};
bool bridge(WorldState &state) {
    return fog::bridge(state, native_fog_resolution_reference_131ac0);
}
constexpr std::array service_sites{0x38846eu, 0x21f590u, 0x33b595u};
void check_replaced_cursor(const Case &c) {
    LoggedBank memory{c.memory};
    SourcePhysicsContactRecordObserver observe;
    SourcePhysicsWorldRuntimeServices services;
    std::uint32_t pc = 0x132628u;
    services.instruction = [](auto) {};
    auto &cursor = services.instruction_cursor;
    cursor.position = &pc;
    cursor.service_sites = service_sites;
    cursor.callback_type = &services.instruction.target_type();
    cursor.executor = std::this_thread::get_id();
    cursor.literal_service_sites = true;
    WorldState state{memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    services.instruction = [&pc](auto observed) { pc = observed; };
    if (bridge(state) || pc != 0x132628u || !memory.accesses.empty())
        throw std::runtime_error("Fog handoff used a replaced cursor");
}
FogResult execute_case(LoggedBank &memory, const Case &c, bool native, bool callbacks, std::size_t observer_fault) {
    FogResult result;
    SourcePhysicsContactRecordObserver observe = [&](const auto &event) {
        result.stores.push_back(event);
        if (result.stores.size() == observer_fault)
            throw std::runtime_error("Injected fog observer fault");
    };
    SourcePhysicsWorldRuntimeServices services;
    if (native && callbacks) {
        services.instruction = [](auto) { throw std::runtime_error("Fog tail delivered a translated instruction"); };
        if (!census_mode) {
            auto &cursor = services.instruction_cursor;
            cursor.position = &result.cursor;
            cursor.service_sites = service_sites;
            cursor.callback_type = &services.instruction.target_type();
            cursor.executor = std::this_thread::get_id();
            cursor.literal_service_sites = true;
            cursor.observation_only_stores = true;
        }
    } else if (callbacks) {
        services.instruction = [&](auto pc) { result.cursor = pc; ++result.observations; };
    }
    const auto mxcsr = _mm_getcsr();
    std::uint16_t x87cw;
    asm volatile("fnstcw %0" : "=m"(x87cw));
    WorldState state{memory, observe, services, SourcePhysicsContactImage::TitleUpdate15};
    state.load(c.abi);
    _mm_setcsr(c.mxcsr);
    try {
        if (native)
            result.admitted = bridge(state);
        else
            native_fog_resolution_reference_131ac0(state);
    } catch (const std::exception &error) {
        result.fault = error.what();
    }
    result.mxcsr = _mm_getcsr();
    result.abi = state.save();
    asm volatile("fnclex\n\tfldcw %0" :: "m"(x87cw));
    _mm_setcsr(mxcsr);
    return result;
}
constexpr std::array weight_offsets{0x4c, 0x40, 0x2c, 0x20, 0x88, 0x7c, 0xa8, 0x9c, 0xe8, 0xdc, 0x64};
constexpr std::uint32_t one = 0x3f800000u;
constexpr std::uint32_t two = 0x40000000u;
constexpr std::uint32_t minus_one = 0xbf800000u;
constexpr std::uint32_t quiet_nan = 0x7fc00123u;
constexpr std::uint32_t signaling_nan = 0x7f801234u;
constexpr std::uint32_t negative_nan = 0xffc00100u;
constexpr std::uint32_t positive_zero = 0u;
constexpr std::uint32_t negative_zero = 0x80000000u;
constexpr std::uint32_t stack_padding = 0x4830u;
Case make_case(std::mt19937 &rng, std::mt19937_64 &floats, unsigned variant) {
    Case c;
    const auto stack = variant % 13 == 0 ? 0xfffffffdu : 0x201000u + (variant % 5);
    const auto result = variant % 7 == 0 ? stack - 0x1cu : 0x339fc0u + (variant % 17);
    random_abi(rng, c, stack);
    c.abi.gp[5] = variant % 11 == 0 ? 0xffffffffu : unsigned(rng());
    c.abi.gp[6] = result;
    c.abi.xmm[3][0] = variant < 100 || variant % 3 ? one : special_float(floats);
    c.abi.xmm[7][0] = variant % 13 != 4 ? positive_zero : special_float(floats);
    for (unsigned offset = 0; offset < 0xf0u; offset += 4) {
        c.memory.put(result + offset, std::uint32_t(rng()));
        c.memory.put(stack + offset, std::uint32_t(rng()));
    }
    for (unsigned offset = 0x14; offset <= 0xec; offset += 4)
        c.memory.put(stack + offset, variant < 100 ? one : (rng() & 1) ? std::uint32_t(rng()) : special_float(floats));
    c.memory.put(stack + stack_padding, std::uint32_t(rng()));
    constexpr std::array magnitudes{one, two, positive_zero, negative_zero, minus_one,
                                   quiet_nan, signaling_nan, 1u, 0x7f800000u};
    for (auto offset : weight_offsets)
        c.memory.put(stack + offset, variant < 100 ? one : magnitudes[rng() % magnitudes.size()]);
    if (variant < 33) {
        if (variant > 3 && variant < 15)
            c.memory.put(stack + weight_offsets[variant - 4], positive_zero);
        c.memory.put(stack + 0x54, variant % 3 == 0 ? minus_one : two);
        c.memory.put(stack + 0x58, variant % 3 == 1 ? minus_one : two);
        c.memory.put(stack + 0x5c, variant % 3 == 2 ? minus_one : two);
        c.memory.put(stack + 0x68, variant % 9 < 3 ? positive_zero : one);
    }
    if (variant == 33) {
        c.abi.xmm[3][0] = negative_nan;
        c.memory.put(stack + 0x34, signaling_nan);
        c.memory.put(stack + 0xc0, quiet_nan);
        c.memory.put(stack + 0x58, signaling_nan);
    }
    if (variant == 34) {
        c.abi.xmm[3][0] = negative_nan;
        c.memory.put(stack + 0x68, signaling_nan);
        c.memory.put(stack + 0x54, quiet_nan);
    }
    return c;
}
struct DifferentialTotals {
    std::size_t compared{}, faults{}, stores{}, observations{};
};
void compare_case(const Case &c, unsigned index, DifferentialTotals &total, std::size_t memory_fault = 0,
                  std::size_t observer_fault = 0, bool callbacks = true) {
    LoggedBank native_bank{c.memory}, original_bank{c.memory};
    native_bank.fail_at = original_bank.fail_at = memory_fault;
    const auto native = execute_case(native_bank, c, true, callbacks, observer_fault);
    const auto original = execute_case(original_bank, c, false, callbacks, observer_fault);
    if ((!native.admitted && native.fault.empty()) || native.fault != original.fault ||
        ((memory_fault || observer_fault) && original.fault.empty()) ||
        !same_abi(native.abi, original.abi) || native.mxcsr != original.mxcsr ||
        (!census_mode && native.cursor != original.cursor) || native.stores != original.stores ||
        native_bank.accesses != original_bank.accesses || !native_bank.same(original_bank)) {
        std::fprintf(stderr, "Fog %u mismatch, memory fault %zu observer fault %zu, cursors %x/%x, "
            "abi %u, MXCSR %x/%x, accesses %zu/%zu, stores %zu/%zu, errors '%s'/'%s'\n",
            index, memory_fault, observer_fault, native.cursor, original.cursor,
            unsigned(same_abi(native.abi, original.abi)), native.mxcsr, original.mxcsr,
            native_bank.accesses.size(), original_bank.accesses.size(), native.stores.size(), original.stores.size(),
            native.fault.c_str(), original.fault.c_str());
        for (unsigned lane = 0; lane < 8; ++lane)
            if (native.abi.xmm[lane] != original.abi.xmm[lane])
                std::fprintf(stderr, "  XMM%u low %08x/%08x\n", lane, native.abi.xmm[lane][0], original.abi.xmm[lane][0]);
        throw std::runtime_error("Fog resolution differential mismatch");
    }
    ++total.compared;
    total.faults += !original.fault.empty();
    total.stores += original.stores.size();
    total.observations += original.observations;
}
} // namespace

int main(int argc, char **argv) try {
    const std::string mode = argc > 1 ? argv[1] : "literal";
    const auto count = argc > 2 ? unsigned(std::stoul(argv[2])) : 12000u;
    set_mode(mode, "HALO2_VERIFY_FOG_RESOLUTION");
    std::mt19937 rng{0x131ac0};
    std::mt19937_64 floats{0x13262b};
    const auto dense = make_case(rng, floats, 35);
    if (mode == "reference") {
        setenv("HALO2_REFERENCE_FOG_RESOLUTION", "1", 1);
        LoggedBank bank{dense.memory};
        const auto result = execute_case(bank, dense, true, true, 0);
        if (result.admitted || !result.fault.empty() || !bank.accesses.empty() || !same_abi(result.abi, dense.abi))
            throw std::runtime_error("Forced reference touched the guest state");
        std::puts("{\"forced_reference_refusals\":1}");
        return 0;
    }
    DifferentialTotals totals;
    check_replaced_cursor(dense);
    if (!census_mode)
        check_refusals(dense, bridge);
    for (unsigned n = 0; n < count; ++n)
        compare_case(make_case(rng, floats, n), n, totals, 0, 0, n % 8 != 5);
    LoggedBank original{dense.memory};
    const auto replay = execute_case(original, dense, false, true, 0);
    if (!replay.fault.empty() || replay.stores.size() != 37)
        throw std::runtime_error("Dense fog row did not exercise every store");
    for (std::size_t access = 1; access <= original.accesses.size(); ++access)
        compare_case(dense, count, totals, access);
    for (std::size_t store = 1; store <= replay.stores.size(); ++store)
        compare_case(dense, count, totals, 0, store);
    const auto coverage = fog::coverage().load();
    if (coverage != 0xfffffff)
        throw std::runtime_error("Fog continuation did not reach all 28 outcomes");
    std::printf("{\"mode\":\"%s\",\"compared\":%zu,\"faults\":%zu,\"stores\":%zu,"
        "\"reference_observations\":%zu,\"dense_observations\":%zu,\"dense_accesses\":%zu,\"coverage\":%llu}\n",
        mode.c_str(), totals.compared, totals.faults, totals.stores, totals.observations,
        replay.observations, original.accesses.size(), static_cast<unsigned long long>(coverage));
    return 0;
} catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}
