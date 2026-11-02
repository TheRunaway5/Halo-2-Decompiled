#include "halo2/native_tu15_model_view_slots.hpp"
#include <array>

namespace halo2 {
namespace {
void store(SourcePhysicsContactMemory& memory, std::uint32_t address,
    std::uint32_t value, unsigned width, std::uint32_t site,
    const SourcePhysicsContactRecordObserver& observe) {
    std::array<std::byte, 4> bytes{};
    for (unsigned i = 0; i < width; ++i) bytes[i] = std::byte(value >> (i * 8));
    SourcePhysicsContactRecordStore event{site, address, {}, {}};
    if (observe) {
        event.before.resize(width);
        memory.read(address, event.before);
        event.written.assign(bytes.begin(), bytes.begin() + width);
    }
    memory.write(address, std::span(bytes).first(width));
    if (observe) observe(event);
}
}
void native_tu15_reset_model_view_slots(SourcePhysicsContactMemory& memory,
    const SourcePhysicsContactRecordObserver& observe) {
    for (unsigned slot = 0; slot < 24; ++slot) {
        const auto row = 0x4c51d0u + slot * 0x68u;
        store(memory, row + 8, 0xffffffffu, 4, 0x22840u, observe);
        store(memory, row + 4, 0xffffu, 2, 0x22843u, observe);
        store(memory, row, 0xffffffffu, 4, 0x22846u, observe);
        for (unsigned i = 0; i < 16; ++i)
            store(memory, row + 12 + i * 4, 0, 4, 0x22853u, observe);
        constexpr std::array sites{0x22858u, 0x2285au, 0x22866u, 0x22869u};
        for (unsigned i = 0; i < sites.size(); ++i)
            store(memory, row + 76 + i * 4, 0, 4, sites[i], observe);
    }
}
}
