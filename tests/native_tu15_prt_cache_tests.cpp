#include "rendering/models/native_tu15_prt_cache.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace halo2;
#include HALO2_PRT_CACHE_CHILD_REFERENCE
namespace {
constexpr std::array bases{0x475000u, 0x4ec000u, 0x4f0000u, 0x4f2000u, 0x61000000u, 0x62000000u};
constexpr std::array sizes{4096u, 4096u, 4096u, 8192u, 32768u, 8192u};
class Memory final : public SourcePhysicsContactMemory {
public:
    std::array<std::vector<std::byte>, 6> banks;
    Memory() { for (unsigned i = 0; i < banks.size(); ++i) banks[i].resize(sizes[i]); }
    std::span<std::byte> view(std::uint32_t address, std::size_t count) {
        for (unsigned i = 0; i < banks.size(); ++i)
            if (address >= bases[i] && address - bases[i] <= sizes[i] &&
                count <= sizes[i] - (address - bases[i]))
                return std::span(banks[i]).subspan(address - bases[i], count);
        throw std::out_of_range("PRT helper escaped declared original banks");
    }
    void read(std::uint32_t address, std::span<std::byte> bytes) const override {
        std::ranges::copy(const_cast<Memory*>(this)->view(address, bytes.size()), bytes.begin());
    }
    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        std::ranges::copy(bytes, view(address, bytes.size()).begin());
    }
};
std::uint32_t word() {
    std::array<unsigned char, 4> bytes{};
    std::cin.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (!std::cin) throw std::runtime_error("Truncated native PRT request");
    return bytes[0] | (std::uint32_t(bytes[1]) << 8) |
        (std::uint32_t(bytes[2]) << 16) | (std::uint32_t(bytes[3]) << 24);
}
void emit(std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) std::cout.put(char(value >> (8 * i)));
}
void emit(const std::vector<std::byte>& bytes) {
    std::cout.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}
}
int main() try {
    while (std::cin.peek() != std::char_traits<char>::eof()) {
        const auto entry = word();
        SourcePhysicsWorldRuntimeAbi abi{};
        for (auto& value : abi.gp) value = word();
        abi.eflags = word();
        const auto control = word();
        abi.floating_point = SourcePhysicsFactoryFloatingPoint{std::uint16_t(word()), control};
        for (auto& lane : abi.xmm) for (auto& value : lane) value = word();
        Memory memory;
        for (auto& bytes : memory.banks) {
            std::cin.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
            if (!std::cin) throw std::runtime_error("Truncated native PRT bank");
        }
        std::vector<SourcePhysicsContactRecordStore> stores;
        SourcePhysicsContactRecordObserver observer = [&](const auto& event) { stores.push_back(event); };
        unsigned instructions{}, parent_instructions{}; bool child_active{};
        SourcePhysicsWorldRuntimeServices services;
        services.instruction = [&](auto) { ++instructions; if(!child_active)++parent_instructions; };
        services.required_owner = [&](auto& m, const auto& call, auto& child_abi, const auto& o) {
            if(call.target != 0x130980u) throw std::runtime_error("Unexpected PRT cache child");
            physics_world_runtime_detail::WorldState child{m, o, services, SourcePhysicsContactImage::TitleUpdate15};
            child.load(child_abi); child_active = true; cache_reference::run_130980(child);
            child_active = false; child_abi = child.save();
        };
        physics_world_runtime_detail::WorldState state{memory, observer, services,
            SourcePhysicsContactImage::TitleUpdate15};
        state.load(abi);
        if (entry != 0x235e0u) throw std::runtime_error("Unexpected PRT sample entry");
        prt_cache_detail::bridge(state);
        if (parent_instructions) throw std::runtime_error("Native PRT executed translated instructions");
        const auto result = state.save();
        for (auto value : result.gp) emit(value);
        emit(result.eflags);
        emit(result.floating_point->mxcsr);
        emit(result.x87_status);
        emit(result.floating_point->x87_control_word);
        for (const auto& lane : result.xmm) for (auto value : lane) emit(value);
        for (const auto& bytes : memory.banks) emit(bytes);
        emit(stores.size());
        for (const auto& event : stores) {
            emit(event.instruction); emit(event.address); emit(event.written.size());
            for (unsigned i = 0; i < 4; ++i)
                std::cout.put(i < event.before.size() ? char(event.before[i]) : '\0');
            for (unsigned i = 0; i < 4; ++i)
                std::cout.put(i < event.written.size() ? char(event.written[i]) : '\0');
        }
    }
    if (!std::cout) throw std::runtime_error("Cannot publish native PRT results");
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n'; return 1;
}
