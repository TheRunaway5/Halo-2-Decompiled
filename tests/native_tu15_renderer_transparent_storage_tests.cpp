#include "halo2/source_renderer_transparent_storage_tu15.hpp"
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
    check(bytes.size() <= 4, "Original transparent store exceeds its declared width");
    std::uint32_t value = 0;
    for (unsigned i = 0; i < bytes.size(); ++i) value |= std::to_integer<std::uint32_t>(bytes[i]) << (i * 8);
    return value;
}
std::uint32_t word(const SourceRuntimeMemory& memory, std::uint32_t address) {
    return word(memory.view(address, 4));
}
std::vector<std::byte> file(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    check(bool(stream), "Original transparent fixture file is unavailable");
    const auto size = stream.tellg();
    check(size >= 0, "Original transparent fixture file extent is invalid");
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    check(bool(stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size())),
        "Original transparent fixture file is truncated");
    return bytes;
}
template<class F> void reject(F&& action, const char* message) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    check(rejected, message);
}
void mount(SourceRuntimeMemory& memory) {
    mount_native_tu15_network_storage_data(memory);
    mount_native_tu15_renderer_transparent_storage_data(memory);
    memory.mount(0x41b6d4, std::array{std::byte{0xb8}, std::byte{}, std::byte{}, std::byte{0x80}},
        SourceRuntimeMemory::Access::ReadOnly, "Original TU1.5 virtual-memory allocation import");
    memory.mount(0x2000000, std::vector<std::byte>(0x20000), SourceRuntimeMemory::Access::ReadWrite,
        "Declared standalone transparent constructor caller stack");
    put(memory, stack, 0x2100000);
}
SourcePhysicsWorldRuntimeAbi caller() {
    SourcePhysicsWorldRuntimeAbi state{};
    state.gp[4] = stack;
    state.eflags = 2;
    state.floating_point = SourcePhysicsFactoryFloatingPoint{0x27f, 0x1f80};
    return state;
}
unsigned validate_guards(SourceRuntimeMemory& memory, SourcePhysicsPoolHostStorage& host) {
    const auto view = native_tu15_renderer_transparent_storage_initial_views(memory);
    const auto planes = word(memory, 0x4c2aa8), indices = word(memory, 0x4c2aac),
        container = word(memory, 0x52dfd0);
    check(view.planes == memory.native_address(planes, 0x1c000) &&
        view.indices == memory.native_address(indices, 0x800) &&
        view.container == memory.native_address(container, 0x1203c) &&
        view.buckets == view.container + 0x3cu && view.free_nodes == view.container + 0x403cu &&
        view.bucket_count == 4096 && view.node_count == 2048 && view.node_payload_bytes == 16,
        "Transparent native64 views detach from original owned allocations");
    unsigned guards = 0;
    auto mutate = [&](std::uint32_t address, std::uint32_t replacement) {
        const auto original = word(memory, address);
        put(memory, address, replacement);
        const auto before = memory.checkpoint();
        reject([&] { native_tu15_renderer_transparent_storage_initial_views(memory); },
            "Transparent admission accepted an inconsistent or unowned resource");
        check(memory.checkpoint() == before, "Rejected transparent admission mutated source storage");
        put(memory, address, original);
        ++guards;
    };
    mutate(0x4c2aa8, 0);
    mutate(0x4c2aac, 0); // Original AL alone does not reject this missing second allocation.
    mutate(0x52dfd0, 0); // Original AL alone does not reject a missing container allocation.
    mutate(0x4c2aa8, indices);
    mutate(0x4c2aac, planes); // Complete but overlapping source objects.
    mutate(0x4c2aa0, 1);
    mutate(0x4c2aa4, 1);
    for (const auto offset : {0x20u, 0x24u, 0x28u, 0x2cu, 0x30u, 0x34u, 0x38u, 0x3cu, 0x4044u})
        mutate(container + offset, word(memory, container + offset) ^ 4u);
    mutate(container, word(memory, container) ^ 1u);
    for (const auto [offset, value] : std::array<std::pair<unsigned, unsigned>, 2>{{{0, 1}, {1, 0}}}) {
        const auto old = memory.view(0x4c2ab0 + offset, 1)[0];
        memory.write(0x4c2ab0 + offset, std::array{std::byte(value)});
        reject([&] { native_tu15_renderer_transparent_storage_initial_views(memory); },
            "Transparent admission accepted active or uninitialized constructor flags");
        memory.write(0x4c2ab0 + offset, std::array{old});
        ++guards;
    }
    auto repeated = caller();
    const auto before = memory.checkpoint();
    reject([&] { source_run_renderer_transparent_storage_with_native_storage(
        memory, repeated, {0x27f, 0x1f80}, {}, host); }, "Transparent cold constructor was run twice");
    check(memory.checkpoint() == before && repeated.gp == caller().gp,
        "Repeated transparent constructor changed storage or its caller");
    ++guards;
    SourceRuntimeMemory foreign;
    mount(foreign);
    auto foreign_caller = caller();
    const auto foreign_before = foreign.checkpoint();
    reject([&] { source_run_renderer_transparent_storage_with_native_storage(
        foreign, foreign_caller, {0x27f, 0x1f80}, {}, host); },
        "Transparent constructor accepted another memory bank's allocator owner");
    check(foreign.checkpoint() == foreign_before && foreign_caller.gp == caller().gp,
        "Rejected foreign allocator changed storage or its caller");
    return guards + 1;
}
void run(const Json& expected, const std::filesystem::path& directory, Json& reports) {
    SourceRuntimeMemory memory;
    mount(memory);
    const auto fill = expected.at("declared_allocation_fill").get<unsigned>();
    std::vector<SourcePhysicsPoolResourceEvent> allocations;
    auto host = std::make_unique<SourcePhysicsPoolHostStorage>(memory, 0x10000000,
        [&](const SourcePhysicsPoolResourceEvent& event) {
            check(event.allocation && allocations.size() < 3, "Transparent source requested an unexpected resource");
            const auto& reference = expected.at("allocations")[allocations.size()];
            check(event.request == std::array<std::uint32_t, 5>{0, 0,
                reference.at("requested_bytes"), 0x101000, 4} && event.result ==
                std::array<std::uint32_t, 3>{0, reference.at("source"), reference.at("reserved_bytes")},
                "Transparent native allocation differs from the original platform contract");
            auto bytes = memory.mutable_view(event.result[1], event.result[2]);
            std::ranges::fill(bytes, std::byte(fill)); // Declared allocation input, before source construction.
            check(memory.native_address(event.result[1], event.result[2]).value > 0xffffffffu,
                "Transparent native storage narrows its host backing");
            allocations.push_back(event);
        });
    auto state = caller();
    std::size_t instructions = 0;
    std::vector<std::array<std::uint32_t, 5>> stores;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](auto) { ++instructions; };
    services.required_owner = [&](auto&, const auto&, auto&, const auto&) {
        throw std::runtime_error("Transparent original constructor reached an undeclared dependency");
    };
    source_run_renderer_transparent_storage_with_native_storage(memory, state, {0x27f, 0x1f80}, services,
        *host, [&](const SourcePhysicsContactRecordStore& store) {
            const bool globals = (store.address >= 0x4c2aa0 && store.address < 0x4c2ab2) || store.address == 0x52dfd0;
            const bool allocated = std::ranges::any_of(allocations, [&](const auto& row) {
                return store.address >= row.result[1] && store.address < row.result[1] + row.result[2];
            });
            if (globals || allocated) stores.push_back({store.instruction, store.address,
                std::uint32_t(store.written.size()), word(store.before), word(store.written)});
        });
    check(allocations.size() == 3 && host->live_allocations() == 3, "Transparent owner loses constructed storage");
    check(state.gp == expected.at("returned_gp").get<decltype(state.gp)>() &&
        (state.eflags & 0xcc5u) == (expected.at("returned_eflags").get<std::uint32_t>() & 0xcc5u) &&
        state.gp[4] == stack + 4 && (state.gp[0] & 255u) == 1,
        "Transparent source lowering differs from the original returned ABI");
    check(instructions == expected.at("original_instructions"),
        "Transparent source lowering differs from original instruction execution count");
    const auto original_stores = file(directory / expected.at("ordered_resource_stores").get<std::string>());
    check(stores.size() == expected.at("original_resource_store_count") && original_stores.size() == stores.size() * 20,
        "Transparent source lowering differs from original ordered resource store count");
    for (std::size_t i = 0; i < stores.size(); ++i)
        for (unsigned j = 0; j < 5; ++j)
            check(stores[i][j] == word(std::span(original_stores).subspan(i * 20 + j * 4, 4)),
                "Transparent source lowering differs from an original ordered resource store");
    std::size_t compared_bytes = 0;
    for (const auto& row : expected.at("allocations")) {
        const auto original = file(directory / row.at("storage").get<std::string>());
        check(original.size() == row.at("reserved_bytes") &&
            std::ranges::equal(memory.view(row.at("source"), original.size()), original),
            "Transparent allocation contents or untouched padding differ from original construction");
        compared_bytes += original.size();
    }
    const auto guards = validate_guards(memory, *host);
    host.reset();
    for (const auto& row : allocations)
        reject([&] { memory.native_address(row.result[1], 1); }, "Retired transparent allocator retained native pages");
    reject([&] { native_tu15_renderer_transparent_storage_initial_views(memory); },
        "Retired transparent owner admitted stale constructor bindings");
    reports.push_back({{"declared_allocation_fill", fill}, {"instructions", instructions},
        {"ordered_resource_stores_compared", stores.size()}, {"whole_reserved_bytes_compared", compared_bytes},
        {"admission_rejections", guards}, {"retirement_rejections", 4}, {"passed", true}});
}
}
int main(int argc, char** argv) try {
#if !defined(__linux__)
    return 77;
#endif
    check(argc == 2 || argc == 4, "Usage: transparent-storage-test ORIGINAL_REPORT [--report OUTPUT]");
    std::ifstream input(argv[1]);
    check(bool(input), "Original transparent constructor report is unavailable");
    const auto original = Json::parse(input);
    check(original.at("passed").get<bool>() && original.at("original_xbe_sha256") ==
        "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        original.at("instruction_patches").empty() && original.at("supplied_game_results").empty() &&
        original.at("cases").size() == 3, "Original transparent fixture identity or execution contract differs");
    Json cases = Json::array();
    std::vector<unsigned> fills;
    for (const auto& expected : original.at("cases")) {
        fills.push_back(expected.at("declared_allocation_fill"));
        run(expected, std::filesystem::path(argv[1]).parent_path(), cases);
    }
    check(fills == std::vector<unsigned>{0, 0x5a, 0xa5}, "Original transparent fixture allocation fills differ");
    const Json report{{"schema", "H2_NATIVE_TU15_TRANSPARENT_STORAGE_DIFFERENTIAL_V1"}, {"passed", true},
        {"cases", cases}, {"actual_native64_pool_storage", true}, {"retained_callbacks_executed", false},
        {"gpu_submission_or_full_initializer32_verified", false}};
    if (argc == 4) {
        check(std::string_view(argv[2]) == "--report", "Unknown transparent-storage test argument");
        std::ofstream output(argv[3]);
        check(bool(output), "Transparent-storage report output is unavailable");
        output << report.dump(2) << '\n';
    }
    std::cout << "PASS three original transparent constructor fills, complete allocation bytes, ordered stores, ABI and native ownership guards\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
