#include "native_tu15_frame_misc_harness.hpp"
#include "native_tu15_world_leaves_remap.hpp"
#include "physics/runtime/native_tu15_collection_filter_world.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

using namespace frame_misc_harness;
namespace owner = halo2::native_collection_filter_world_detail;
namespace world = halo2::native_world_leaves_detail;

namespace original {
struct ObservedWorldState : WorldState {
    using WorldState::WorldState;
    bool selected_observation_pcs{};
};

std::uint32_t world_observation_pc(std::uint32_t canonical, std::uint32_t selected,
                                   const ObservedWorldState &state) {
    return state.selected_observation_pcs && state.image == SourcePhysicsContactImage::TitleUpdate15
        ? selected : canonical;
}

void dispatch(ObservedWorldState &, std::uint32_t, std::uint32_t, bool);
void group_filter(ObservedWorldState &);
#include "resolvers.inc"
#include "bodies.inc"

enum class ResultFlags { move, zero, ones };
struct ConstantMethod { std::uint32_t address, value; ResultFlags flags; };
constexpr ConstantMethod methods[]{
#include "methods.inc"
};

void dispatch(ObservedWorldState &state, std::uint32_t target, std::uint32_t site, bool tail) {
    log_call(mix(entry_hash(state, target), site));
    if (tail) throw std::runtime_error("Unexpected tail call");
    if (target == 0x306220u) {
        run_306220(state);
    } else if (target == 0x185020u) {
        run_185020(state);
    } else {
        const auto method = std::ranges::find(methods, target, &ConstantMethod::address);
        if (method == std::end(methods)) throw std::runtime_error("Unknown shape test method");
        state.gp[0] = method->flags == ResultFlags::move ? method->value : state.flags.logic(method->value, 4);
        state.pop();
    }
}

void group_filter(ObservedWorldState &state) {
    log_call(mix(entry_hash(state, owner::filter_groups), 0x1c6d5eu));
    run_2e5e20(state);
}
} // namespace original

namespace {
using original::ObservedWorldState;
constexpr std::uint32_t base = 0x2100000u, negative = 0x90000000u, stack = 0x21fff00u;
constexpr std::uint32_t output = 0x2103f00u, global = 0x48b860u, tables = 0x185044u;
constexpr std::array<std::uint8_t, 36> table_bytes{
#include "tables.inc"
};

void read_bank(Memory &memory, std::uint32_t address, unsigned size, const std::string &bytes) {
    if (bytes.size() != size * 2u) throw std::runtime_error("Invalid collection filter fixture bank");
    for (unsigned n = 0; n < size; ++n)
        memory.put(address+n, std::stoul(bytes.substr(n*2u, 2u), nullptr, 16), 1);
}

std::vector<Case> read_cases(std::istream &input) {
    std::vector<Case> cases;
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        std::uint32_t entry, receiver, flags, special_body;
        row >> std::hex >> entry >> receiver >> flags >> special_body;
        if (!row || entry != 0x1cbe70u) throw std::runtime_error("Invalid collection filter fixture entry");
        auto &test = cases.emplace_back();
        auto &memory = test.memory;
        memory.put(stack - 0x120u, 0u);
        memory.put(stack, 0x3000000u);
        for (unsigned n = 0; n < 6; ++n) {
            std::uint32_t argument;
            if (!(row >> argument)) throw std::runtime_error("Missing fixture argument");
            memory.put(stack + 4u * (n + 1u), argument);
        }
        std::string bank, extra;
        if (!(row >> bank >> extra)) throw std::runtime_error("Missing fixture banks");
        read_bank(memory, base, 0x4000u, bank);
        read_bank(memory, negative, 4096u, extra);
        memory.put(global, special_body);
        for (unsigned n = 0; n < table_bytes.size(); ++n) memory.put(tables + n, table_bytes[n], 1);

        test.abi.gp = {0x11223344u, receiver, 0x33445566u, 0x44556677u, stack, 0x55667788u, 0x66778899u, 0x778899aau};
        test.abi.eflags = flags;
        test.abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x27fu, 0x1f80u};
        for (unsigned n = 0; n < 32; ++n) test.abi.xmm[n / 4][n % 4] = 0x3f000000u + n;
    }
    return cases;
}

bool native(ObservedWorldState &state) {
    return owner::bridge(state, original::native_collection_filter_reference_1c6c30,
        [](auto &value, auto target, auto site) { original::dispatch(value, target, site, false); },
        original::group_filter);
}

Result run(Memory &memory, const Case &test, bool run_native) {
    Result result;
    const SourcePhysicsContactRecordObserver stores = [&](const auto &event) { result.stores.push_back(event); };
    SourcePhysicsWorldRuntimeServices services, outer;
    services.instruction = [&](std::uint32_t site) {
        if (run_native && site >= 0x1cbe70u && site <= 0x1cbfabu)
            throw std::runtime_error("Native filter delivered an original parent instruction");
        result.cursor = site;
    };
    if (run_native && !census_mode && leaves_remap::live_cursor) {
        auto &cursor = services.instruction_cursor;
        cursor.position = &result.cursor;
        cursor.service_sites = halo2::native_census_detail::service_sites;
        cursor.callback_type = &services.instruction.target_type();
        cursor.executor = std::this_thread::get_id();
        cursor.literal_service_sites = true;
        cursor.observation_only_stores = true;
    }
    leaves_remap::wrap(services, outer);
    const auto saved = _mm_getcsr();
    std::uint16_t x87;
    asm volatile("fnstcw %0" : "=m"(x87));
    ObservedWorldState state{memory, stores, services, SourcePhysicsContactImage::TitleUpdate15};
    state.selected_observation_pcs = !leaves_remap::enabled;
    state.load(test.abi);
    call_log = &result.calls;
    try {
        if (run_native) result.admitted = native(state);
        else original::run_1c6c30(state);
    } catch (const std::exception &e) {
        result.fault = e.what();
    }
    call_log = nullptr;
    result.abi = state.save();
    result.mxcsr = _mm_getcsr();
    asm volatile("fnclex\n\tfldcw %0" :: "m"(x87));
    _mm_setcsr(saved);
    return result;
}

void dump_bank(Memory &memory, std::uint32_t address, unsigned width) {
    static constexpr char hex[] = "0123456789abcdef";
    for (unsigned n = 0; n < width; ++n) {
        const auto byte = memory.get(address+n, 1);
        std::cout.put(hex[byte >> 4]);
        std::cout.put(hex[byte & 0xfu]);
    }
}

void record_physical(Memory &memory, const Result &result) {
    const auto &abi = result.abi;
    std::cout << std::hex;
    for (const auto value : abi.gp) std::cout << value << ' ';
    std::cout << abi.eflags << ' ' << abi.floating_point->x87_control_word << ' '
              << abi.x87_status << ' ' << result.mxcsr << ' ';
    for (const auto &lanes : abi.xmm)
        for (const auto bits : lanes) std::cout << bits << ' ';
    dump_bank(memory, base, 0x4000u);
    std::cout.put(' ');
    dump_bank(memory, negative, 4096u);
    std::cout.put('\n');
}

void check_fixture(unsigned index, const Case &test, Totals &totals, bool physical) {
    auto a = test.memory.clone(), b = test.memory.clone();
    const auto mine = run(a, test, true), theirs = run(b, test, false);
    if ((!mine.admitted && mine.fault.empty()) || mine.fault != theirs.fault || mine.stores != theirs.stores ||
        mine.calls != theirs.calls || !a.same(b) || (!census_mode && mine.cursor != theirs.cursor) ||
        (theirs.fault.empty() && (!same_abi(mine.abi, theirs.abi) || mine.mxcsr != theirs.mxcsr))) {
        std::fprintf(stderr, "Case %u native fault '%s' source '%s', cursors %x/%x, stores %zu/%zu, children %zu/%zu\n",
            index, mine.fault.c_str(), theirs.fault.c_str(), mine.cursor, theirs.cursor,
            mine.stores.size(), theirs.stores.size(), mine.calls.size(), theirs.calls.size());
        throw std::runtime_error("World collection filter differential mismatch");
    }
    if (physical) {
        if (!mine.fault.empty()) throw std::runtime_error("Unexpected fault in physical fixture");
        record_physical(a, mine);
    }
    ++totals.compared;
    totals.stores += mine.stores.size();
    totals.children += mine.calls.size();
    if (!mine.fault.empty()) ++totals.faults;
}
} // namespace

int main(int argc, char **argv) try {
    if (argc != 3) throw std::runtime_error("Expected fixture path and test mode");
    const std::string mode = argv[2];
    const bool physical = mode == "physical";
    const auto base_mode = leaves_remap::parse(mode);
    set_mode(base_mode, "HALO2_VERIFY_PHYSICS_WORLD_LEAVES");
    std::ifstream input(argv[1]);
    const auto cases = read_cases(input);
    if (cases.empty()) throw std::runtime_error("Collection filter fixture unavailable");
    Totals totals;
    unsigned index = 0;
    for (const auto &test : cases) check_fixture(index++, test, totals, physical);
    if (physical) return 0;

    for (const auto address : {stack, base, output, global, tables, negative}) {
        for (unsigned n = 0; n < cases.size(); n += 5u) {
            auto fault = cases[n];
            fault.memory = fault.memory.clone();
            fault.memory.drop(address);
            check_fixture(index++, fault, totals, false);
        }
    }
    if (leaves_remap::enabled && !census_mode) {
        auto check = [&] {
            auto memory = cases.front().memory.clone();
            const auto result = run(memory, cases.front(), true);
            return result.admitted || !result.fault.empty();
        };
        if (!leaves_remap::refuses_bad_remaps(0x1c6c30u, 0x1c6d6bu, check))
            throw std::runtime_error("World collection filter accepted an invalid remap");
    }
    report(index, totals, 0u, 0u, mode);
    return 0;
} catch (const std::exception &e) {
    std::fprintf(stderr, "%s\n", e.what());
    return 1;
}
