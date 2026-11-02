#include "halo2/native_tu15_lens_query_slots.hpp"
#include "halo2/source_lens_flare_tu15.hpp"
#include "halo2/source_lens_query_tu15.hpp"
#include "halo2/source_renderer_tail_storage_tu15.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace halo2;
namespace {
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
class Memory final : public SourcePhysicsContactMemory {
public:
    std::array<std::byte, 0x40c0> bank{};
    std::array<std::byte, 0x1000> stack{};
    std::span<std::byte> view(std::uint32_t address, std::size_t bytes) {
        if (address >= 0x52dfd8u && address - 0x52dfd8u <= bank.size() &&
            bytes <= bank.size() - (address - 0x52dfd8u))
            return std::span(bank).subspan(address - 0x52dfd8u, bytes);
        if (address >= 0x62000000u && address - 0x62000000u <= stack.size() &&
            bytes <= stack.size() - (address - 0x62000000u))
            return std::span(stack).subspan(address - 0x62000000u, bytes);
        throw std::out_of_range("Bridge escaped its declared bank/stack");
    }
    void read(std::uint32_t address, std::span<std::byte> bytes) const override {
        const auto source = const_cast<Memory*>(this)->view(address, bytes.size());
        std::copy(source.begin(), source.end(), bytes.begin());
    }
    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        std::ranges::copy(bytes, view(address, bytes.size()).begin());
    }
};
using Run = void (*)(SourcePhysicsContactMemory&, std::uint32_t, SourcePhysicsWorldRuntimeAbi&,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsWorldRuntimeServices&,
    const SourcePhysicsContactRecordObserver&);
constexpr std::array<Run, 3> routes{source_run_renderer_tail_storage_tu15,
    source_run_lens_flare_tu15, source_run_lens_query_tu15};
SourcePhysicsWorldRuntimeAbi caller(std::uint32_t slot) {
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp = {slot, 0x22222222u, 0x33333333u, 0x44444444u,
        0x62000800u, 0x66666666u, 0x77777777u, 0x88888888u};
    abi.eflags = 0x202u;
    for (unsigned i = 0; i < 8; ++i)
        for (unsigned j = 0; j < 4; ++j) abi.xmm[i][j] = 0x7fc12000u + i * 4 + j;
    abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x27fu, 0x1f80u};
    return abi;
}
void probe(Run run) {
    for (;;) {
        std::array<unsigned char, 4> index{};
        std::cin.read(reinterpret_cast<char*>(index.data()), index.size());
        if (std::cin.eof() && std::cin.gcount() == 0) break;
        check(bool(std::cin), "Truncated bridge slot");
        const auto slot = unsigned(index[0]) | (unsigned(index[1]) << 8) |
            (unsigned(index[2]) << 16) | (unsigned(index[3]) << 24);
        Memory memory;
        std::cin.read(reinterpret_cast<char*>(memory.bank.data()), memory.bank.size());
        check(bool(std::cin), "Truncated bridge query bank");
        auto abi = caller(slot);
        std::vector<SourcePhysicsContactRecordStore> stores;
        run(memory, 0x20e30u, abi, {0x27fu, 0x1f80u}, {}, [&](const auto& event) { stores.push_back(event); });
        std::cout.write(reinterpret_cast<const char*>(memory.bank.data()), memory.bank.size());
        const auto emit = [](std::uint32_t value) {
            for (unsigned i = 0; i < 4; ++i) std::cout.put(char(value >> (i * 8)));
        };
        emit(stores.size());
        for (const auto& event : stores) {
            emit(event.instruction); emit(event.address);
            check(event.before.size() == 4 && event.written.size() == 4, "Unexpected bridge store width");
            std::cout.write(reinterpret_cast<const char*>(event.before.data()), 4);
            std::cout.write(reinterpret_cast<const char*>(event.written.data()), 4);
        }
        for (auto value : abi.gp) emit(value);
        emit(abi.eflags & 0x8d5u);
        check(bool(std::cout), "Cannot publish bridge comparison");
    }
}
}
int main(int argc, char** argv) try {
    if (argc == 3 && std::string_view(argv[1]) == "--probe") {
        const auto route = std::string_view(argv[2]);
        check(route == "0" || route == "1" || route == "2", "Unknown bridge probe route");
        probe(routes[route[0] - '0']); return 0;
    }
    check(argc == 1, "Expected optional --probe ROUTE");
    for (Run run : routes) {
        for (std::uint32_t slot = 0; slot < native_tu15_lens_query_slot_count; ++slot) {
            Memory memory; memory.bank.fill(std::byte{0xa5});
            auto abi = caller(slot);
            const auto before = abi;
            unsigned instructions{}, stores{};
            bool saved_register_store{};
            SourcePhysicsWorldRuntimeServices services;
            services.instruction = [&](auto) { ++instructions; };
            services.required_owner = [](auto&, const auto&, auto&, const auto&) {
                throw std::runtime_error("Native slot requested an emulated dependency");
            };
            run(memory, 0x20e30u, abi, {0x27fu, 0x1f80u}, services, [&](const auto& event) {
                if (!stores) saved_register_store = event.instruction == 0x20e30u &&
                    event.address == before.gp[4] - 4 &&
                    event.written == std::vector<std::byte>(4, std::byte{0x77});
                ++stores;
            });
            auto expected = before.gp;
            expected[0] = 0xff80u;
            expected[1] = 0x52dfdcu + slot * 32;
            expected[2] = ~(1u << (slot & 31));
            expected[4] += 4;
            check(abi.gp == expected && (abi.eflags & 0x8c5u) == 0,
                "Native slot lost its unchanged-original successful caller ABI");
            check(abi.xmm == before.xmm && abi.x87_depth == 0 && abi.x87_status == before.x87_status &&
                abi.floating_point && abi.floating_point->x87_control_word == 0x27fu &&
                abi.floating_point->mxcsr == 0x1f80u,
                "Native slot changed unrelated SSE/x87 state");
            check(instructions == 0 && stores == 13 && saved_register_store,
                "A local copy still interprets the slot body or changes its stores");
        }
    }
    std::cout << "PASS all 511 slots through all three public routes: original successful ABI, no translated instructions\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n'; return 1;
}
