#include "halo2/native_tu15_lens_query_slots.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace halo2;
namespace {
constexpr std::uint32_t base = 0x52dfd8u;
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
class Memory final : public SourcePhysicsContactMemory {
public:
    std::array<std::byte, 0x40c0> bytes{};
    std::optional<std::uint32_t> reject_write;
    void read(std::uint32_t address, std::span<std::byte> output) const override {
        check(address >= base && address - base <= bytes.size() &&
            output.size() <= bytes.size() - (address - base), "Query read escapes its owner");
        std::copy_n(bytes.begin() + (address - base), output.size(), output.begin());
    }
    void write(std::uint32_t address, std::span<const std::byte> input) override {
        check(address >= base && address - base <= bytes.size() &&
            input.size() <= bytes.size() - (address - base), "Query write escapes its owner");
        if (reject_write == address) throw std::runtime_error("Declared rejected query store");
        std::copy(input.begin(), input.end(), bytes.begin() + (address - base));
    }
    std::uint32_t word(std::uint32_t address) const {
        std::array<std::byte, 4> value{}; read(address, value);
        std::uint32_t result{};
        for (unsigned i = 0; i < 4; ++i) result |= std::to_integer<unsigned>(value[i]) << (i * 8);
        return result;
    }
    void word(std::uint32_t address, std::uint32_t value) {
        std::array<std::byte, 4> result{};
        for (unsigned i = 0; i < 4; ++i) result[i] = std::byte(value >> (i * 8));
        write(address, result);
    }
};
template<class F> void rejects(F&& action) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    check(rejected, "Declared invalid query operation was accepted");
}
void functional() {
    for (unsigned fill : {0u, 0x5au, 0xa5u, 0xffu}) {
        for (std::uint32_t slot = 0; slot < native_tu15_lens_query_slot_count; ++slot) {
            Memory memory; memory.bytes.fill(std::byte(fill));
            const auto original = memory.bytes;
            std::vector<SourcePhysicsContactRecordStore> stores;
            const auto flags = native_tu15_retire_lens_query_slot(memory, slot,
                [&](const auto& event) { stores.push_back(event); });
            check(flags == 0xff80u && stores.size() == 12, "Query retirement result/store count differs");
            const auto record = base + 4 + slot * 32;
            for (unsigned n = 0; n < 8; ++n)
                check(memory.word(record + n * 4) == (n == 1 ? 0xff80u : 0), "Retired query fields differ");
            for (unsigned bank = 0; bank < 3; ++bank) {
                const auto address = 0x531fc0u + bank * 64 + (slot / 32) * 4;
                check(memory.word(address) == ((fill * 0x01010101u) & ~(1u << (slot % 32))),
                    "Retirement changed other visibility bits");
                check(stores[bank].address == address && stores[bank].instruction == 0x20e60u,
                    "Retirement changed ordered visibility stores");
            }
            auto changed = original;
            for (const auto& event : stores) {
                check(event.before.size() == 4 && event.written.size() == 4 &&
                    std::equal(event.before.begin(), event.before.end(), changed.begin() + event.address - base),
                    "Observer lost the actual preceding store bytes");
                std::copy(event.written.begin(), event.written.end(), changed.begin() + event.address - base);
            }
            check(changed == memory.bytes, "Retirement changed unobserved bytes");
        }
    }
    Memory memory; memory.bytes.fill(std::byte{0xa5});
    const auto initial = memory.bytes;
    for (auto slot : {511u, 512u, 0xffffffffu}) {
        rejects([&] { native_tu15_retire_lens_query_slot(memory, slot); });
        check(memory.bytes == initial, "Rejected slot changed the bank");
    }
    memory.reject_write = 0x532000u;
    unsigned observed{};
    rejects([&] { native_tu15_retire_lens_query_slot(memory, 0, [&](const auto&) { ++observed; }); });
    check(observed == 1 && memory.word(0x531fc0u) == 0xa5a5a5a4u &&
        memory.word(0x532000u) == 0xa5a5a5a5u && memory.word(base + 4) == 0xa5a5a5a5u,
        "Rejected store lost its real committed prefix");
    memory.reject_write.reset(); memory.bytes = initial;
    rejects([&] { native_tu15_retire_lens_query_slot(memory, 0,
        [&](const auto&) { throw std::runtime_error("Declared observer rejection"); }); });
    check(memory.word(0x531fc0u) == 0xa5a5a5a4u && memory.word(0x532000u) == 0xa5a5a5a5u,
        "Observer rejection preceded its committed store");
    memory.bytes = initial;
    const auto result = native_tu15_retire_lens_query_slot(memory, 0, [&](const auto& event) {
        if (event.instruction == 0x20e80u) memory.word(base + 8, 0x12340001u);
    });
    check(result == 0x1234ff81u && memory.word(base + 8) == result,
        "Final query flags lost the actual post-store reread");
}
// Binary protocol used only by the independent unchanged-XBE oracle. All
// bytes stay in the CTest owner's scratch/pipes; no capture is retained.
void probe() {
    for (;;) {
        std::array<unsigned char, 4> index{};
        std::cin.read(reinterpret_cast<char*>(index.data()), index.size());
        if (std::cin.eof() && std::cin.gcount() == 0) break;
        check(bool(std::cin), "Truncated oracle slot");
        const auto slot = unsigned(index[0]) | (unsigned(index[1]) << 8) |
            (unsigned(index[2]) << 16) | (unsigned(index[3]) << 24);
        Memory memory;
        std::cin.read(reinterpret_cast<char*>(memory.bytes.data()), memory.bytes.size());
        check(bool(std::cin), "Truncated oracle query bank");
        std::vector<SourcePhysicsContactRecordStore> stores;
        native_tu15_retire_lens_query_slot(memory, slot, [&](const auto& event) { stores.push_back(event); });
        std::cout.write(reinterpret_cast<const char*>(memory.bytes.data()), memory.bytes.size());
        const auto emit = [](std::uint32_t value) {
            for (unsigned i = 0; i < 4; ++i) std::cout.put(char(value >> (i * 8)));
        };
        emit(stores.size());
        for (const auto& event : stores) {
            emit(event.instruction); emit(event.address);
            std::cout.write(reinterpret_cast<const char*>(event.before.data()), 4);
            std::cout.write(reinterpret_cast<const char*>(event.written.data()), 4);
        }
        check(bool(std::cout), "Cannot publish native query result");
    }
}
}
int main(int argc, char** argv) try {
    if (argc == 2 && std::string_view(argv[1]) == "--probe") probe();
    else {
        check(argc == 1, "Expected optional --probe"); functional();
        std::cout << "PASS all 511 lens slots, four bank patterns, bounds and failure/store ordering\n";
    }
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n'; return 1;
}
