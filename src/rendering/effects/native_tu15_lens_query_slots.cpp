#include "halo2/native_tu15_lens_query_slots.hpp"
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory& memory, std::uint32_t address) {
    std::array<std::byte, 4> bytes{};
    memory.read(address, bytes);
    std::uint32_t value{};
    for (unsigned i = 0; i < bytes.size(); ++i)
        value |= std::to_integer<std::uint32_t>(bytes[i]) << (i * 8);
    return value;
}
void store(SourcePhysicsContactMemory& memory, std::uint32_t address,
    std::uint32_t value, std::uint32_t site, const SourcePhysicsContactRecordObserver& observe) {
    std::array<std::byte, 4> bytes{};
    for (unsigned i = 0; i < bytes.size(); ++i) bytes[i] = std::byte(value >> (i * 8));
    SourcePhysicsContactRecordStore event{site, address, {}, {}};
    if (observe) {
        event.before.resize(bytes.size());
        memory.read(address, event.before);
        event.written.assign(bytes.begin(), bytes.end());
    }
    memory.write(address, bytes);
    if (observe) observe(event);
}
}
std::uint32_t native_tu15_retire_lens_query_slot(SourcePhysicsContactMemory& memory,
    std::uint32_t slot, const SourcePhysicsContactRecordObserver& observe) {
    if (slot >= native_tu15_lens_query_slot_count)
        throw std::out_of_range("Native TU1.5 lens query slot exceeds its 511-record owner");
    const auto keep = ~(std::uint32_t{1} << (slot & 31));
    for (unsigned bank = 0; bank < 3; ++bank) {
        const auto address = 0x531fc0u + bank * 0x40u + (slot >> 5) * 4;
        store(memory, address, word(memory, address) & keep, 0x20e60u, observe);
    }
    const auto record = 0x52dfdcu + slot * 32;
    constexpr std::array sites{0x20e6cu, 0x20e6eu, 0x20e71u, 0x20e74u,
        0x20e77u, 0x20e7au, 0x20e7du, 0x20e80u};
    for (unsigned i = 0; i < sites.size(); ++i)
        store(memory, record + i * 4, 0, sites[i], observe);
    // Reread after the zero stores: retained-memory adapters and observers
    // can expose aliases. The source performs OR, not an unconditional preset.
    const auto flags = word(memory, record + 4) | 0xff80u;
    store(memory, record + 4, flags, 0x20e8bu, observe);
    return flags;
}
}
