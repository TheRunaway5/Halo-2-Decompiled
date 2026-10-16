#include "halo2/source_renderer_transparent_storage_tu15.hpp"
#include "halo2/source_renderer_cache_pools_tu15.hpp"
#include "halo2/source_renderer_pool_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_network_storage_startup_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
using Json = nlohmann::json;
namespace {
constexpr std::uint32_t stack = 0x2008000u;
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void put(SourceRuntimeMemory& memory, std::uint32_t address, std::uint32_t value) {
    std::array<std::byte, 4> bytes{};
    for (unsigned i = 0; i < 4; ++i) bytes[i] = std::byte(value >> (i * 8));
    memory.write(address, bytes);
}
std::uint32_t word(std::span<const std::byte> bytes) {
    check(bytes.size() <= 4, "Original renderer cache store exceeds its declared width");
    std::uint32_t value = 0;
    for (unsigned i = 0; i < bytes.size(); ++i) value |= std::to_integer<std::uint32_t>(bytes[i]) << (i * 8);
    return value;
}
std::uint32_t word(const SourceRuntimeMemory& memory, std::uint32_t address) {
    return word(memory.view(address, 4));
}
std::vector<std::byte> file(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    check(bool(stream), "Original renderer cache fixture file is unavailable");
    const auto size = stream.tellg();
    check(size >= 0, "Original renderer cache fixture file extent is invalid");
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    check(bool(stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size())),
        "Original renderer cache fixture file is truncated");
    return bytes;
}
template<class F> void reject(F&& action, const char* message) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    check(rejected, message);
}
void mount(SourceRuntimeMemory& memory) {
    mount_native_tu15_network_storage_data(memory);
    mount_native_tu15_renderer_pool_data(memory);
    memory.mount(0x44abec, std::array<std::byte, 12>{}, SourceRuntimeMemory::Access::ReadOnly,
        "Original shared zero render vector");
    mount_native_tu15_renderer_cache_pools_data(memory);
    memory.mount(0x41b6d4, std::array{std::byte{0xb8}, std::byte{}, std::byte{}, std::byte{0x80}},
        SourceRuntimeMemory::Access::ReadOnly, "Original TU1.5 virtual-memory allocation import");
    memory.mount(0x2000000, std::vector<std::byte>(0x20000), SourceRuntimeMemory::Access::ReadWrite,
        "Declared standalone renderer cache constructor caller stack");
    put(memory, stack, 0x2100000);
}
SourcePhysicsWorldRuntimeAbi caller() {
    SourcePhysicsWorldRuntimeAbi state{};
    state.gp[4] = stack;
    state.eflags = 2;
    state.floating_point = SourcePhysicsFactoryFloatingPoint{0x27f, 0x1f80};
    return state;
}
unsigned validate_views(SourceRuntimeMemory& memory, const Json& expected) {
    const auto result = native_tu15_renderer_cache_pool_views(memory);
    const std::array views{result.geometry, result.predicted_geometry, result.geometry_cache,
        result.texture, result.predicted_texture, result.texture_cache};
    unsigned guards = 0;
    for (unsigned i = 0; i < views.size(); ++i) {
        const auto& row = expected.at("allocations")[i];
        check(views[i].value > 0xffffffffu && views[i] == memory.native_address(
            row.at("source"), row.at("requested_bytes")), "Cache pool view loses its actual native64 allocation");
        const auto cell = expected.at("binding_cells")[i].get<std::uint32_t>();
        const auto base = word(memory, cell), header = base + ((i == 2 || i == 5) ? 0x70u : 0u);
        auto mutate = [&](std::uint32_t address, std::uint32_t replacement) {
            const auto original = word(memory, address);
            put(memory, address, replacement);
            const auto before = memory.checkpoint();
            reject([&] { native_tu15_renderer_cache_pool_views(memory); },
                "Cache pool view admitted an inconsistent constructor or allocation");
            check(memory.checkpoint() == before, "Rejected cache pool view modified source state");
            put(memory, address, original);
            ++guards;
        };
        mutate(cell, 0);
        for (const auto offset : {0x20u, 0x24u, 0x2cu, 0x44u, 0x48u})
            mutate(header + offset, word(memory, header + offset) ^ 4u);
        if (i == 5) mutate(cell, word(memory, 0x4f2d6c)); // Same valid layout, independent allocation required.
    }
    return guards;
}
void run(const Json& expected, const std::filesystem::path& directory, Json& reports) {
    SourceRuntimeMemory memory;
    mount(memory);
    const auto fill = expected.at("declared_allocation_fill").get<unsigned>();
    std::vector<SourcePhysicsPoolResourceEvent> allocations;
    auto host = std::make_unique<SourcePhysicsPoolHostStorage>(memory, 0x10000000,
        [&](const SourcePhysicsPoolResourceEvent& event) {
            check(event.allocation && allocations.size() < 6, "Cache source requested an unexpected resource");
            const auto& reference = expected.at("allocations")[allocations.size()];
            check(event.request == std::array<std::uint32_t, 5>{0, 0,
                reference.at("requested_bytes"), 0x101000, 4} && event.result ==
                std::array<std::uint32_t, 3>{0, reference.at("source"), reference.at("reserved_bytes")},
                "Cache native allocation differs from original platform contract");
            std::ranges::fill(memory.mutable_view(event.result[1], event.result[2]), std::byte(fill));
            allocations.push_back(event);
        });
    auto state = caller();
    std::size_t instructions = 0;
    std::vector<std::array<std::uint32_t, 5>> stores;
    const auto observe = [&](const SourcePhysicsContactRecordStore& store) {
        const bool binding = std::ranges::any_of(expected.at("binding_cells"),
            [&](const auto& cell) { return store.address == cell.template get<std::uint32_t>(); });
        const bool allocated = std::ranges::any_of(allocations, [&](const auto& row) {
            return store.address >= row.result[1] && store.address < row.result[1] + row.result[2];
        });
        if (binding || allocated) stores.push_back({store.instruction, store.address,
            std::uint32_t(store.written.size()), word(store.before), word(store.written)});
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](auto) { ++instructions; };
    services.required_owner = [&](SourcePhysicsContactMemory& bank, const SourcePhysicsContactRuntimeCall& call,
        SourcePhysicsWorldRuntimeAbi& abi, const SourcePhysicsContactRecordObserver& observer) {
        if (host->try_call(bank, call, abi)) return;
        if (call.target == 0x11f3f0u) {
            source_run_renderer_transparent_storage_tu15(bank, call.target, abi, {0x27f, 0x1f80}, services, observer);
            return;
        }
        throw std::runtime_error("Renderer cache constructor reached an undeclared dependency");
    };
    for (const auto& row : expected.at("calls")) {
        put(memory, stack, 0x2100000);
        state.gp[4] = stack;
        const auto previous = instructions;
        source_run_physics_live_support_tu15(memory, row.at("entry"), state, {0x27f, 0x1f80}, services, observe);
        check(state.gp == row.at("returned_gp").get<decltype(state.gp)>() &&
            (state.eflags & 0xcc5u) == (row.at("returned_eflags").get<std::uint32_t>() & 0xcc5u) &&
            state.gp[4] == stack + 4, "Renderer cache constructor differs from original returned ABI");
        check(instructions - previous == row.at("original_instructions"),
            "Renderer cache source lowering differs from original instruction execution count");
    }
    check(allocations.size() == 6 && host->live_allocations() == 6, "Cache owner loses constructed storage");
    const auto original_stores = file(directory / expected.at("ordered_resource_stores").get<std::string>());
    check(stores.size() == expected.at("resource_store_count") && original_stores.size() == stores.size() * 20,
        "Cache source lowering differs from original ordered resource store count");
    for (std::size_t i = 0; i < stores.size(); ++i)
        for (unsigned j = 0; j < 5; ++j)
            check(stores[i][j] == word(std::span(original_stores).subspan(i * 20 + j * 4, 4)),
                "Cache source lowering differs from an original ordered resource store");
    std::size_t compared_bytes = 0;
    for (const auto& row : expected.at("allocations")) {
        const auto original = file(directory / row.at("storage").get<std::string>());
        check(original.size() == row.at("reserved_bytes") &&
            std::ranges::equal(memory.view(row.at("source"), original.size()), original),
            "Cache allocation contents or untouched padding differ from original construction");
        compared_bytes += original.size();
    }
    const auto guards = validate_views(memory, expected);
    host.reset();
    for (const auto& row : allocations)
        reject([&] { memory.native_address(row.result[1], 1); }, "Retired cache allocator retained native pages");
    reject([&] { native_tu15_renderer_cache_pool_views(memory); }, "Retired cache owner admitted stale bindings");
    reports.push_back({{"declared_allocation_fill", fill}, {"instructions", instructions},
        {"ordered_resource_stores_compared", stores.size()}, {"whole_reserved_bytes_compared", compared_bytes},
        {"admission_rejections", guards}, {"retirement_rejections", 7}, {"passed", true}});
}
}
int main(int argc, char** argv) try {
#if !defined(__linux__)
    return 77;
#endif
    check(argc == 2 || argc == 4, "Usage: cache-pools-test ORIGINAL_REPORT [--report OUTPUT]");
    std::ifstream input(argv[1]);
    check(bool(input), "Original renderer cache constructor report is unavailable");
    const auto original = Json::parse(input);
    check(original.at("passed").get<bool>() && original.at("original_xbe_sha256") ==
        "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        original.at("instruction_patches").empty() && original.at("supplied_game_results").empty() &&
        original.at("cases").size() == 3, "Original cache fixture identity or execution contract differs");
    Json cases = Json::array();
    std::vector<unsigned> fills;
    for (const auto& expected : original.at("cases")) {
        fills.push_back(expected.at("declared_allocation_fill"));
        run(expected, std::filesystem::path(argv[1]).parent_path(), cases);
    }
    check(fills == std::vector<unsigned>{0, 0x5a, 0xa5}, "Original cache fixture allocation fills differ");
    const Json report{{"schema", "H2_NATIVE_TU15_RENDERER_CACHE_POOLS_DIFFERENTIAL_V1"}, {"passed", true},
        {"cases", cases}, {"actual_native64_pool_storage", true}, {"retained_callbacks_executed", false},
        {"gpu_submission_or_full_initializer32_verified", false}};
    if (argc == 4) {
        check(std::string_view(argv[2]) == "--report", "Unknown cache-pools test argument");
        std::ofstream output(argv[3]);
        check(bool(output), "Cache-pools report output is unavailable");
        output << report.dump(2) << '\n';
    }
    std::cout << "PASS three original renderer cache pool fills, complete allocation bytes, ordered stores, ABI and native ownership guards\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
