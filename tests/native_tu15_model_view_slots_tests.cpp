#include "halo2/native_tu15_model_view_slots.hpp"
#include "rendering/models/native_tu15_model_view_slots_bridge.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace halo2;
namespace {
constexpr std::uint32_t base = 0x4c51d0u;
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
class Memory final : public SourcePhysicsContactMemory {
public:
    std::array<std::byte, 0x9c0> bytes{};
    std::span<std::byte> view(std::uint32_t address, std::size_t count) {
        check(address >= base && address - base <= bytes.size() &&
            count <= bytes.size() - (address - base), "Model slot escaped its exact owner");
        return std::span(bytes).subspan(address - base, count);
    }
    void read(std::uint32_t address, std::span<std::byte> output) const override {
        std::ranges::copy(const_cast<Memory*>(this)->view(address, output.size()), output.begin());
    }
    void write(std::uint32_t address, std::span<const std::byte> input) override {
        std::ranges::copy(input, view(address, input.size()).begin());
    }
};
void functional() {
    for (unsigned fill : {0u, 0x5au, 0xa5u, 0xffu}) {
        Memory memory; memory.bytes.fill(std::byte(fill));
        auto expected = memory.bytes;
        for (unsigned slot = 0; slot < 24; ++slot) {
            const auto at = slot * 104;
            for (unsigned offset : {0u, 1u, 2u, 3u, 4u, 5u, 8u, 9u, 10u, 11u})
                expected[at + offset] = std::byte{255};
            std::fill(expected.begin() + at + 12, expected.begin() + at + 92, std::byte{});
        }
        SourcePhysicsWorldRuntimeAbi input{};
        input.gp = {1, 2, 3, 4, 5, 6, 7, 8}; input.eflags = 0x202;
        SourcePhysicsWorldRuntimeServices services;
        unsigned instructions{}, stores{};
        services.instruction = [&](auto) { ++instructions; };
        SourcePhysicsContactRecordObserver observe = [&](const auto&) { ++stores; };
        physics_world_runtime_detail::WorldState state{memory, observe, services,
            SourcePhysicsContactImage::TitleUpdate15};
        state.load(input);
        native_tu15_model_view_slots_bridge(state);
        const auto result = state.save();
        check(memory.bytes == expected && stores == 552 && instructions == 0,
            "Model slot reset changed its exact fields/stores or used translated instructions");
        check(result.gp == std::array<std::uint32_t, 8>{0, 0x4c5b74u, 0x4c5b94u, 4, 5, 6, 0xffffffffu, 0x4c5b74u} &&
            (result.eflags & 0x8d5u) == 0x44u,
            "Model slot continuation lost the exact original2286E inputs");
        input.eflags |= 1024u; state.load(input);
        unsigned rejected{};
        try { native_tu15_model_view_slots_bridge(state); }
        catch (const std::invalid_argument&) { ++rejected; }
        check(rejected == 1 && stores == 552, "Model slots admitted an incompatible backward-string ABI");
    }
}
void probe() {
    for (;;) {
        Memory memory;
        std::cin.read(reinterpret_cast<char*>(memory.bytes.data()), memory.bytes.size());
        if (std::cin.eof() && std::cin.gcount() == 0) break;
        check(bool(std::cin), "Truncated original model slot bank");
        std::vector<SourcePhysicsContactRecordStore> stores;
        native_tu15_reset_model_view_slots(memory, [&](const auto& event) { stores.push_back(event); });
        std::cout.write(reinterpret_cast<const char*>(memory.bytes.data()), memory.bytes.size());
        const auto emit = [](std::uint32_t value) {
            for (unsigned i = 0; i < 4; ++i) std::cout.put(char(value >> (i * 8)));
        };
        emit(stores.size());
        for (const auto& event : stores) {
            emit(event.instruction); emit(event.address); emit(event.written.size());
            for (unsigned i = 0; i < 4; ++i)
                std::cout.put(i < event.before.size() ? char(event.before[i]) : '\0');
            for (unsigned i = 0; i < 4; ++i)
                std::cout.put(i < event.written.size() ? char(event.written[i]) : '\0');
        }
        check(bool(std::cout), "Cannot publish model slot comparison");
    }
}
}
int main(int argc, char** argv) try {
    if (argc == 2 && std::string_view(argv[1]) == "--probe") probe();
    else {
        check(argc == 1, "Expected optional --probe"); functional();
        std::cout << "PASS all24 model slots, untouched bytes,552 stores and original2286E continuation\n";
    }
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n'; return 1;
}
