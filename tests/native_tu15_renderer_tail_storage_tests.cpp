#include "halo2/source_renderer_tail_storage_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
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
void run(const Json& expected, const std::filesystem::path& directory, Json& reports,bool native_prefix) {
    constexpr std::uint32_t arena = 0x81000000, arena_bytes = 0x100000;
    SourceRuntimeMemory memory;
    const auto fill = expected.at("fill").get<unsigned>();
    const auto prior_success = expected.at("declared_prior_success").get<bool>();
    memory.mount(arena, std::vector<std::byte>(arena_bytes, std::byte(fill)), SourceRuntimeMemory::Access::ReadWrite,
        "Declared independently owned renderer-tail validation arena");
    memory.mount(0x4f2950, std::array<std::byte, 16>{}, SourceRuntimeMemory::Access::ReadWrite,
        "Declared original arena binding, cursor and checksum state");
    put(memory, 0x4f2950, arena);
    put(memory, 0x4f2954, 0x100);
    put(memory, 0x4f295c, 0x12345678);
    memory.mount(0x46a0d0, std::array{std::byte{}, std::byte{}, std::byte{0x80}, std::byte{0x3f}},
        SourceRuntimeMemory::Access::ReadOnly, "Original TU1.5 shared one operand");
    mount_native_tu15_renderer_tail_storage_data(memory);
    std::ranges::fill(memory.mutable_view(0x52dfd8, 0x40c0), std::byte(fill));
    memory.mount(0x56e225, std::array<std::byte, 1>{}, SourceRuntimeMemory::Access::ReadWrite,
        "Declared original CRC table cold flag");
    memory.mount(0x56e258, std::vector<std::byte>(0x400, std::byte(fill)), SourceRuntimeMemory::Access::ReadWrite,
        "Declared original CRC table initial backing");
    memory.mount(0x4922a8, std::array<std::byte, 1>{}, SourceRuntimeMemory::Access::ReadWrite,
        "Declared original renderer initialized flag");
    if(native_prefix)memory.write(0x4922a8,std::array{std::byte(prior_success)});
    memory.mount(0x2000000, std::vector<std::byte>(0x20000), SourceRuntimeMemory::Access::ReadWrite,
        "Declared original renderer-tail caller stack");
    SourcePhysicsWorldRuntimeAbi state{};
    state.eflags = 2;
    state.floating_point = SourcePhysicsFactoryFloatingPoint{0x27f, 0x1f80};
    std::vector<std::array<std::uint32_t, 5>> stores;
    std::size_t instructions = 0;
    const auto observe = [&](const SourcePhysicsContactRecordStore& store) {
        const auto address = store.address;
        if ((address >= arena && address < arena + arena_bytes) ||
            (address >= 0x52dfd8 && address < 0x532098) || (address >= 0x56e258 && address < 0x56e658) ||
            address == 0x4f2954 || address == 0x4f295c || address == 0x515ec0 || address == 0x515ec4 ||
            address == 0x515e2c || address == 0x56e225 || address == 0x4922a8)
            stores.push_back({store.instruction, address, std::uint32_t(store.written.size()),
                word(store.before), word(store.written)});
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction = [&](auto) { ++instructions; };
    services.required_owner = [&](auto&, const auto& call, auto&, const auto&) {
        throw std::runtime_error("Bounded renderer tail reached undeclared dependency " +
            std::to_string(call.target) + " at " + std::to_string(call.instruction));
    };
    for (const auto& call : expected.at("calls")) {
        const auto entry = call.at("entry").get<std::uint32_t>();
        state.gp[4] = stack;
        if (entry == 0x1fb90u) put(memory, stack, 0x2100000);
        else {
            check(entry == 0x12822u, "Original bounded renderer-tail entry differs");
            for (const auto [offset, value] : std::array<std::pair<unsigned, unsigned>, 4>{{
                {0, 0xeeeeeeee}, {4, 0x51515151}, {8, 0xbbbbbbbb}, {20, 0x2100000}}})
                put(memory, stack + offset, value);
            memory.write(stack + 15, std::array{std::byte(prior_success)});
            state.gp[3] = 0;
        }
        const auto previous = instructions;
        if(native_prefix && entry==0x12822u) {
            source_renderer_cpu_interface_initialize_tu15(memory,[&](auto target,auto eax) {
                state.gp[4]=stack;put(memory,stack,0x2100000);
                if(eax)state.gp[0]=*eax;
                if(target==0x126850u)
                    source_run_campaign_cinematic_tu15(memory,target,state,{0x27f,0x1f80},services,observe);
                else source_run_renderer_tail_storage_tu15(memory,target,state,{0x27f,0x1f80},services,observe);
                return state.gp[0];
            });
        }else source_run_renderer_tail_storage_tu15(memory, entry, state, {0x27f, 0x1f80}, services, observe);
        if(!native_prefix || entry==0x1fb90u) {
        check(state.gp == call.at("returned_gp").get<decltype(state.gp)>() &&
            (state.eflags & 0xcc5u) == (call.at("returned_eflags").get<std::uint32_t>() & 0xcc5u),
            "Bounded renderer tail differs from original returned ABI");
        check(instructions - previous == call.at("original_instructions"),
            "Bounded renderer tail differs from original instruction execution count");
        }
    }
    if(!native_prefix) {
    const auto original_stores = file(directory / expected.at("ordered_resource_stores").get<std::string>());
    check(stores.size() == expected.at("resource_store_count") && original_stores.size() == stores.size() * 20,
        "Bounded renderer tail differs from original ordered resource store count");
    for (std::size_t i = 0; i < stores.size(); ++i)
        for (unsigned j = 0; j < 5; ++j)
            check(stores[i][j] == word(std::span(original_stores).subspan(i * 20 + j * 4, 4)),
                "Bounded renderer tail differs from an original ordered resource store");
    }
    std::size_t compared_bytes = 0;
    for (const auto [address, key] : std::array<std::pair<std::uint32_t, const char*>, 3>{{
        {arena + 0x100, "arena_storage"}, {0x52dfd8, "query_storage"}, {0x56e258, "crc_storage"}}}) {
        const auto original = file(directory / expected.at(key).get<std::string>());
        check(std::ranges::equal(memory.view(address, original.size()), original),
            "Bounded renderer tail differs from original arena, query or computed CRC table bytes");
        compared_bytes += original.size();
    }
    check(word(memory, 0x4f2954) == expected.at("arena_cursor") &&
        word(memory, 0x4f295c) == expected.at("crc") &&
        memory.view(0x4922a8, 1)[0] == std::byte(prior_success),
        "Bounded renderer tail differs from original arena accounting or declared admission path");
    const auto views = native_tu15_renderer_tail_storage_views(memory);
    check(views.state_1fb90 == memory.native_address(arena + 0x100, 0x3fc) &&
        views.state_12560 == views.state_1fb90 + 0x3fcu &&
        views.state_20e90 == views.state_12560 + 0x34u &&
        views.query_bank == memory.native_address(0x52dfd8, 0x40c0),
        "Renderer tail views lose original arena adjacency or retained query storage");
    memory.release(arena);
    reject([&] { native_tu15_renderer_tail_storage_views(memory); },
        "Renderer tail admitted stale arena bindings after real backing retirement");
    reports.push_back({{"declared_fill", fill}, {"declared_prior_success", prior_success},
        {"instructions", instructions}, {"ordered_resource_stores_compared", native_prefix?0:stores.size()},
        {"whole_resource_bytes_compared", compared_bytes}, {"arena_bytes_consumed", 0x454},
        {"native_cpu_prefix",native_prefix},{"whole_suffix_abi_compared",!native_prefix},{"passed", true}});
}
}
int main(int argc, char** argv) try {
    const bool native_prefix=argc>2 && std::string_view(argv[argc-1])=="--native-prefix";
    if(native_prefix)--argc;
    check(argc == 2 || argc == 4, "Usage: renderer-tail-test ORIGINAL_REPORT [--report OUTPUT]");
    std::ifstream input(argv[1]);
    check(bool(input), "Original renderer-tail constructor report is unavailable");
    const auto original = Json::parse(input);
    check(original.at("passed").get<bool>() && original.at("original_xbe_sha256") ==
        "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        original.at("instruction_patches").empty() && original.at("supplied_game_results").empty() &&
        original.at("cases").size() == 6, "Original renderer-tail fixture identity or execution contract differs");
    Json cases = Json::array();
    std::vector<std::pair<unsigned, bool>> paths;
    for (const auto& expected : original.at("cases")) {
        paths.emplace_back(expected.at("fill"), expected.at("declared_prior_success"));
        run(expected, std::filesystem::path(argv[1]).parent_path(), cases,native_prefix);
    }
    check(paths == std::vector<std::pair<unsigned, bool>>{{0, false}, {0, true}, {0x5a, false},
        {0x5a, true}, {0xa5, false}, {0xa5, true}}, "Original renderer-tail fixture paths differ");
    const Json report{{"schema", "H2_NATIVE_TU15_RENDERER_TAIL_DIFFERENTIAL_V1"}, {"passed", true},
        {"cases", cases}, {"entire_initializer32_or_gpu_frame_parity_verified", false}};
    if (argc == 4) {
        check(std::string_view(argv[2]) == "--report", "Unknown renderer-tail test argument");
        std::ofstream output(argv[3]);
        check(bool(output), "Renderer-tail report output is unavailable");
        output << report.dump(2) << '\n';
    }
    std::cout << (native_prefix?"PASS six native CPU prefixes match complete original arena/query/CRC bytes and preserve the caller's graphics flag; suffix ABI/stores are not equivalent\n":
        "PASS six original bounded renderer tail paths, whole arena/query/CRC bytes, ordered stores, ABI and native views\n");
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
