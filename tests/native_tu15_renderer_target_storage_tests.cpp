#include "halo2/source_renderer_target_storage_tu15.hpp"
#include "halo2/source_runtime_host_physical_memory.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
using Json = nlohmann::json;
namespace {
constexpr std::uint32_t definitions = 0x4c1378u, arena_base = 0x81000000u;
constexpr std::uint32_t arena_bytes = 0x4000000u, stack = 0x2008000u;
constexpr std::size_t definition_bytes = 28 * 0x98;
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void put(SourceRuntimeMemory& memory, std::uint32_t address, std::uint32_t value) {
    std::array<std::byte, 4> bytes{};
    for (unsigned i = 0; i < bytes.size(); ++i) bytes[i] = std::byte(value >> (i * 8));
    memory.write(address, bytes);
}
std::uint32_t word(const SourceRuntimeMemory& memory, std::uint32_t address) {
    const auto bytes = memory.view(address, 4);
    std::uint32_t value = 0;
    for (unsigned i = 0; i < bytes.size(); ++i) value |= std::to_integer<std::uint32_t>(bytes[i]) << (i * 8);
    return value;
}
std::vector<std::byte> unhex(const std::string& text) {
    check(text.size() % 2 == 0, "Original target-storage fixture has odd byte encoding");
    auto nibble = [](char c) -> unsigned {
        if (c >= '0' && c <= '9') return unsigned(c - '0');
        if (c >= 'a' && c <= 'f') return unsigned(c - 'a' + 10);
        throw std::runtime_error("Original target-storage fixture has invalid byte encoding");
    };
    std::vector<std::byte> result(text.size() / 2);
    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = std::byte((nibble(text[i * 2]) << 4) | nibble(text[i * 2 + 1]));
    return result;
}
template<class F> void reject(F&& action, const char* message) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    check(rejected, message);
}
const NativeTu15TargetStorageDefinition& find(
    const std::vector<NativeTu15TargetStorageDefinition>& targets, unsigned index) {
    const auto found = std::ranges::find(targets, index, &NativeTu15TargetStorageDefinition::index);
    check(found != targets.end(), "Native target set omits an original definition");
    return *found;
}
unsigned validate_exports(SourceRuntimeMemory& memory) {
    const auto targets = native_tu15_renderer_target_definitions(memory);
    check(targets.size() == 18, "Native target storage has another definition count");
    unsigned surfaces = 0;
    for (const auto& target : targets) {
        const auto source = definitions + target.index * 0x98;
        check(target.pixels.value > 0xffffffffu &&
            target.pixels == memory.native_address(word(memory, source + 0x88), target.bytes),
            "Native target storage narrows or detaches its retained physical pages");
        check(target.width == word(memory, source + 0x80) && target.height == word(memory, source + 0x84) &&
            target.format_word == word(memory, source + 0x10) && target.size_word == word(memory, source + 0x14) &&
            target.bytes == word(memory, source + 0x7c), "Native target definition loses original constructor words");
        std::uint64_t offset = 0;
        for (unsigned level = 0; level < target.surfaces.size(); ++level) {
            const auto& mip = target.surfaces[level];
            check(mip.pixels == target.pixels + offset && mip.width == (target.width >> level) &&
                mip.height == (target.height >> level), "Native target mip detaches from its original parent range");
            check(memory.source_address(mip.pixels, mip.bytes) == word(memory, source + 0x88) + offset,
                "Native target mip escapes actual retained storage");
            offset += mip.bytes;
            ++surfaces;
        }
        check(offset == target.bytes, "Native target mip byte ranges do not cover their reservation");
    }
    check(surfaces == 20, "Native target storage loses original mip surfaces");
    const auto& first = find(targets, 1);
    const auto& depth = find(targets, 9);
    check(first.pixels == find(targets, 5).pixels && first.pixels == find(targets, 23).pixels &&
        find(targets, 6).pixels == first.pixels + 0x4b000u, "Native first target family loses original overlaps");
    for (const auto index : {7u, 10u, 13u, 21u, 25u})
        check(find(targets, index).pixels == depth.pixels, "Native shared target family loses original overlapping base");
    check(find(targets, 8).pixels == depth.pixels + 0x16800u &&
        find(targets, 22).pixels == depth.pixels + 0x80000u &&
        find(targets, 26).pixels == depth.pixels + 0x4b000u &&
        find(targets, 27).pixels == depth.pixels + 0x96000u,
        "Native shared target family loses original suballocation offsets");
    check(find(targets, 7).pitch == 640 && find(targets, 7).bytes == 0x16800u,
        "Native target pitch was inferred from padded reservation width");
    const auto& mip_target = find(targets, 13);
    check(mip_target.surfaces.size() == 3 && mip_target.surfaces[1].pixels == depth.pixels + 0x40000u &&
        mip_target.surfaces[2].pixels == depth.pixels + 0x50000u && mip_target.bytes == 0x54000u,
        "Native target mip chain differs from the original constructor");

    // Real writes through one native view must remain visible through each
    // overlapping source/native view. Restore the original untouched pixel.
    const auto old = memory.view(depth.pixels, 4);
    const std::array saved{old[0], old[1], old[2], old[3]};
    const std::array marker{std::byte{0x21}, std::byte{0xb4}, std::byte{0x67}, std::byte{0xda}};
    memory.write(depth.pixels, marker);
    for (const auto index : {7u, 10u, 13u, 21u, 25u})
        check(std::ranges::equal(memory.view(find(targets, index).pixels, 4), marker),
            "Native target views copied formerly shared physical bytes");
    memory.write(depth.pixels, saved);

    unsigned rejected = 0;
    auto mutate_word = [&](std::uint32_t address, std::uint32_t replacement) {
        const auto original = word(memory, address);
        put(memory, address, replacement);
        const std::vector before(memory.view(definitions, definition_bytes).begin(),
                                 memory.view(definitions, definition_bytes).end());
        reject([&] { native_tu15_renderer_target_definitions(memory); },
            "Native target exporter admitted inconsistent constructor headers");
        check(std::ranges::equal(memory.view(definitions, definition_bytes), before),
            "Rejected native target export changed constructor bytes");
        put(memory, address, original);
        ++rejected;
    };
    const auto linear = definitions + 5 * 0x98u;
    mutate_word(linear + 0x80, 321); // Same rounded allocation size; header width still320.
    mutate_word(linear + 0x10, word(memory, linear + 0x10) ^ 0x100u);
    mutate_word(linear + 0x14, word(memory, linear + 0x14) ^ 1u);
    mutate_word(linear + 0x18 + 12, word(memory, linear + 0x18 + 12) ^ 0x100u);
    mutate_word(linear + 0x18 + 16, word(memory, linear + 0x18 + 16) ^ 1u);
    mutate_word(linear + 0x18 + 20, linear + 8);
    mutate_word(linear + 0x18 + 4, word(memory, linear + 0x18 + 4) + 4);
    mutate_word(linear + 8, word(memory, linear + 8) + 4);
    mutate_word(linear + 4, 0x50001u);
    const auto swizzled = definitions + 13 * 0x98u;
    mutate_word(swizzled + 0x10, word(memory, swizzled + 0x10) ^ 0x10000u);
    mutate_word(swizzled + 0x14, 1);
    const std::array<std::byte, 1> zero{std::byte{}}, one{std::byte{1}};
    memory.write(linear + 0x94, zero);
    reject([&] { native_tu15_renderer_target_definitions(memory); },
        "Native target exporter admitted contradictory linear/header storage");
    memory.write(linear + 0x94, one);
    ++rejected;
    memory.write(linear + 0x95, one);
    reject([&] { native_tu15_renderer_target_definitions(memory); },
        "Native target exporter admitted a retired definition");
    memory.write(linear + 0x95, zero);
    ++rejected;

    SourceRuntimeMemory foreign;
    foreign.mount(definitions, memory.view(definitions, definition_bytes), SourceRuntimeMemory::Access::ReadWrite,
        "Copied descriptors with no physical arena owner");
    reject([&] { native_tu15_renderer_target_definitions(foreign); },
        "Copied source descriptors admitted unowned native target bytes");
    return rejected + 1;
}
void run(const Json& expected, Json& report) {
    const auto mode = expected.at("mode").get<std::string>();
    check(mode == "normal" || mode == "exhausted" || mode == "occupied", "Unknown original target fixture mode");
    SourceRuntimeMemory memory;
    auto physical = std::make_unique<SourceRuntimeHostPhysicalMemory>(memory);
    check(physical->allocate({arena_bytes, arena_base & 0x7fffffffu,
        (arena_base & 0x7fffffffu) + arena_bytes - 1, 4096, 4}) == arena_base,
        "Declared original physical arena has no native page owner");
    mount_native_tu15_renderer_target_storage_data(memory);
    memory.mount(0x4f2cf0, std::array<std::byte, 13 * 4>{}, SourceRuntimeMemory::Access::ReadWrite,
        "Declared original target-storage physical partition inputs");
    memory.mount(0x41b648, std::array{std::byte{0xb6}, std::byte{}, std::byte{}, std::byte{0x80}},
        SourceRuntimeMemory::Access::ReadOnly, "Declared MmSetAddressProtect import");
    memory.mount(0x2000000, std::vector<std::byte>(0x20000), SourceRuntimeMemory::Access::ReadWrite,
        "Declared original target constructor caller stack");
    put(memory, stack, 0x2100000);
    put(memory, 0x4f2cfc, arena_base & 0x7fffffffu);
    put(memory, 0x4f2d10, (arena_base & 0x7fffffffu) + (mode == "exhausted" ? 4096 : arena_bytes));
    if (mode == "exhausted") put(memory, 0x56e190, 10);
    if (mode == "occupied") memory.write(definitions + 0x98 + 0x95, std::array{std::byte{1}});
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp[4] = stack;
    abi.eflags = 2;
    abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x27f, 0x1f80};
    Json protections = Json::array();
    std::size_t instructions = 0;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](auto) { ++instructions; };
    services.required_owner = [&](SourcePhysicsContactMemory& bank, const SourcePhysicsContactRuntimeCall& call,
        SourcePhysicsWorldRuntimeAbi& state, const SourcePhysicsContactRecordObserver&) {
        const auto sp = state.gp[4];
        check(&bank == &memory && call.instruction == 0x2db4d1u && call.target == 0x800000b6u && call.tail &&
            word(memory, sp) == 0x1db4bu && word(memory, 0x41b648) == call.target && word(memory, sp + 12) == 0x404u,
            "Original target constructor reached another platform call or frame");
        const auto address = word(memory, sp + 4), bytes = word(memory, sp + 8);
        physical->protect(address, bytes, 0x404);
        check(memory.native_address(address, bytes).value > 0xffffffffu,
            "Target protection lost its actual wide page backing");
        protections.push_back({{"address", address}, {"bytes", bytes}, {"protection", 0x404}});
        state.gp[4] += 16; // Only the authentic kernel void stdcall ABI; no supplied game result.
    };
    source_run_renderer_target_storage_tu15(memory, 0x1d750, abi, {0x27f, 0x1f80}, services);
    const auto original = unhex(expected.at("bank_hex").get<std::string>());
    check(original.size() == definition_bytes && std::ranges::equal(memory.view(definitions, definition_bytes), original),
        "Lowered target constructor differs from original TU1.5 definition bytes");
    check(abi.gp == expected.at("returned_gp").get<decltype(abi.gp)>(),
        "Lowered target constructor differs from original returned registers");
    // TEST leaves AF undefined. Compare CF/PF/ZF/SF/OF and retained DF only.
    constexpr std::uint32_t defined_flags = 0xcc5;
    check((abi.eflags & defined_flags) == (expected.at("returned_eflags").get<std::uint32_t>() & defined_flags),
        "Lowered target constructor differs from original defined returned flags");
    check(abi.gp[4] == stack + 4 && (abi.gp[0] & 255u) == expected.at("returned_bool"),
        "Lowered target constructor lost the original caller or bool result");
    check(instructions == expected.at("original_instructions") &&
        word(memory, 0x4f2d10) == expected.at("physical_top") &&
        word(memory, 0x56e190) == expected.at("missing_storage_count"),
        "Lowered target constructor differs from original control flow or allocator state");
    check(protections.size() == expected.at("protections").size(), "Original target protection count differs");
    for (std::size_t i = 0; i < protections.size(); ++i)
        for (const auto* field : {"address", "bytes", "protection"})
            check(protections[i].at(field) == expected.at("protections")[i].at(field),
                "Original target protection address, extent or policy differs");
    unsigned guards = 0;
    if (mode == "normal") guards = validate_exports(memory);
    else {
        const std::vector before(memory.view(definitions, definition_bytes).begin(),
                                 memory.view(definitions, definition_bytes).end());
        reject([&] { native_tu15_renderer_target_definitions(memory); },
            "Original false-success/occupied construction admitted incomplete native targets");
        check(std::ranges::equal(memory.view(definitions, definition_bytes), before),
            "Incomplete native target admission changed original constructor bytes");
        ++guards;
    }
    physical.reset();
    reject([&] { memory.native_address(arena_base, 1); }, "Retired physical owner retained its native target backing");
    reject([&] { native_tu15_renderer_target_definitions(memory); }, "Retired physical owner admitted native target definitions");
    guards += 2;
    report.push_back({{"mode", mode}, {"instructions", instructions}, {"protection_requests", protections},
        {"whole_definition_bank_compared_bytes", definition_bytes}, {"returned_gp", abi.gp},
        {"defined_flags_mask", defined_flags}, {"rejections_checked", guards}, {"passed", true}});
}
}
int main(int argc, char** argv) try {
#if !defined(__linux__)
    return 77;
#endif
    check(argc == 2 || argc == 4, "Usage: target-storage-test ORIGINAL_REPORT [--report OUTPUT]");
    std::ifstream input(argv[1]);
    check(bool(input), "Original target constructor report is unavailable");
    const auto original = Json::parse(input);
    check(original.at("passed").get<bool>() && original.at("original_xbe_sha256") ==
        "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        original.at("instruction_patches").empty() && original.at("supplied_game_results").empty() &&
        original.at("cases").size() == 3, "Original target constructor fixture identity or execution contract differs");
    Json cases = Json::array();
    std::vector<std::string> modes;
    for (const auto& expected : original.at("cases")) {
        modes.push_back(expected.at("mode").get<std::string>());
        run(expected, cases);
    }
    check(modes == std::vector<std::string>{"normal", "exhausted", "occupied"}, "Original target fixture cases differ");
    Json report{{"schema", "H2_NATIVE_TU15_TARGET_STORAGE_DIFFERENTIAL_V1"}, {"passed", true}, {"cases", cases},
        {"native_page_backings", true}, {"real_host_mprotect", true}, {"supplied_game_results", Json::array()},
        {"gpu_upload_or_frame_parity_verified", false}};
    if (argc == 4) {
        check(std::string_view(argv[2]) == "--report", "Unknown target-storage test argument");
        std::ofstream output(argv[3]);
        check(bool(output), "Target-storage report output is unavailable");
        output << report.dump(2) << '\n';
    }
    std::cout << "PASS three original target constructor paths, complete bytes/ABI/protections, shared native mip views and ownership guards\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
