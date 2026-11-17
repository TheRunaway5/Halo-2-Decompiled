#include "halo2/cache.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using namespace halo2;
using Json = nlohmann::json;
using Banks = std::map<std::uint32_t, std::vector<std::byte>>;
constexpr std::array<std::uint32_t, 2> private_banks{0x61000000u, 0x62000000u};
constexpr std::array<std::pair<std::uint32_t, std::size_t>, 3> data_cells{{
    {0x4ecbd0u, 4}, {0x4f0414u, 4}, {0x51df20u, 2},
}};

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<std::byte> from_hex(const std::string& text) {
    require(text.size() % 2 == 0, "Invalid D3950 fixture length");
    std::vector<std::byte> bytes(text.size() / 2);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        unsigned value{};
        const auto* begin = text.data() + index * 2;
        const auto parsed = std::from_chars(begin, begin + 2, value, 16);
        require(parsed.ec == std::errc{} && parsed.ptr == begin + 2, "Invalid D3950 fixture byte");
        bytes[index] = std::byte(value);
    }
    return bytes;
}

std::string to_hex(std::span<const std::byte> bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string text;
    text.reserve(bytes.size() * 2);
    for (auto byte : bytes) {
        const auto value = std::to_integer<unsigned>(byte);
        text += digits[value >> 4];
        text += digits[value & 15];
    }
    return text;
}

std::span<const std::byte> fixture_data(const Banks& banks, std::uint32_t address, std::size_t size) {
    const auto base = address & ~0xfffu;
    return std::span(banks.at(base)).subspan(address - base, size);
}

SourcePhysicsWorldRuntimeAbi initial_abi(const Json& row) {
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp = row.at("gp");
    abi.eflags = row.at("eflags");
    abi.xmm = row.at("xmm");
    const auto& fp = row.at("before_fp");
    abi.floating_point = SourcePhysicsFactoryFloatingPoint{fp[0].get<std::uint16_t>(), fp[2]};
    abi.x87_status = fp[1];
    abi.x87_depth = row.at("live_x87").size();
    require(abi.x87_depth <= abi.x87.size(), "Invalid D3950 fixture x87 depth");
    for (unsigned index = 0; index < abi.x87_depth; ++index) {
        const auto lane = from_hex(row.at("live_x87")[index]);
        require(lane.size() == 10, "Invalid D3950 fixture x87 lane");
        std::memcpy(&abi.x87[index], lane.data(), lane.size());
    }
    return abi;
}

Json returned_abi(const SourcePhysicsWorldRuntimeAbi& abi) {
    require(abi.floating_point.has_value() && !abi.active_mmx, "Unexpected D3950 returned FP state");
    Json x87 = Json::array();
    for (unsigned index = 0; index < abi.x87_depth; ++index)
        x87.push_back(to_hex(std::span(reinterpret_cast<const std::byte*>(&abi.x87[index]), 10)));
    return {{"gp", abi.gp}, {"eflags", abi.eflags}, {"fs_base", abi.fs_base}, {"xmm", abi.xmm}, {"x87", x87},
        {"fp", {abi.floating_point->x87_control_word, abi.x87_status, abi.floating_point->mxcsr}}};
}

void collect_changes(SourceRuntimeMemory& bank, std::uint32_t address,
    std::span<const std::byte> initial, Json& changes) {
    std::vector<std::byte> returned(initial.size());
    bank.read(address, returned);
    for (std::size_t offset = 0; offset < initial.size(); ++offset) {
        if (returned[offset] != initial[offset])
            changes.push_back(Json::array({address + offset, std::to_integer<unsigned>(returned[offset])}));
    }
}

void refuse_direct_call(PhysicsLiveWorldTu15& world, std::uint32_t target) {
    auto abi = world.prepare_source_call();
    unsigned stores{};
    try {
        world.run_actual(target, abi, [&](const auto&) { ++stores; });
    } catch (const std::runtime_error& error) {
        require(stores == 0 && std::string_view(error.what()).find("world owner missing") != std::string_view::npos,
            "Unexpected refusal at a direct D3950 child root");
        return;
    }
    throw std::runtime_error("A direct D3950 child root acquired an unqualified World route");
}
} // namespace

int main(int argc, char** argv) try {
    require(argc == 6, "Supply Coagulation, TU1.5 XBE, save directory, fixture and observation mode");
    const bool observed = std::string_view(argv[5]) == "observed";
    require(observed || std::string_view(argv[5]) == "quiet", "Unknown D3950 observation mode");
    require(!source_ai_frame_tu15_owns_entry(0x1cf0e0u), "Generic AIFrame admission widened");
    std::ifstream fixture(argv[4]);
    const auto rows = Json::parse(fixture);
    Cache cache(argv[1]);
    PhysicsLiveWorldTu15 world(cache, argv[2], argv[3]);
    auto& bank = world.memory();
    for (auto address : private_banks)
        bank.mount(address, std::vector<std::byte>(65536), SourceRuntimeMemory::Access::ReadWrite, "D3950 fixture");
    Json output = Json::array();
    for (const auto& row : rows) {
        Banks initial;
        for (const auto& region : row.at("banks"))
            initial.emplace(region.at("address"), from_hex(region.at("before")));
        for (auto address : private_banks) bank.write(address, initial.at(address));
        for (auto [address, width] : data_cells) bank.write(address, fixture_data(initial, address, width));
        const auto expected_constant = fixture_data(initial, 0x46a0d0u, 4);
        std::array<std::byte, 4> constant{};
        bank.read(0x46a0d0u, constant);
        require(std::equal(constant.begin(), constant.end(), expected_constant.begin()), "Unexpected TU1.5 clamp word");
        auto abi = initial_abi(row);
        Json stores = Json::array();
        SourcePhysicsContactRecordObserver observer;
        if (observed) observer = [&](const auto& event) {
            stores.push_back(Json::array({event.instruction, event.address, to_hex(event.before), to_hex(event.written)}));
        };
        world.run_actual(0xd3950u, abi, observer);
        Json changes = Json::array();
        for (auto address : private_banks) collect_changes(bank, address, initial.at(address), changes);
        for (auto [address, width] : data_cells) collect_changes(bank, address, fixture_data(initial, address, width), changes);
        output.push_back({{"name", row.at("name")}, {"abi", returned_abi(abi)}, {"changes", changes}, {"stores", stores}});
    }
    refuse_direct_call(world, 0x1cf0e0u);
    refuse_direct_call(world, 0x1cf0e1u);
    std::cout << Json{{"rows", output}, {"direct_refusals", 2}}.dump() << '\n';
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
