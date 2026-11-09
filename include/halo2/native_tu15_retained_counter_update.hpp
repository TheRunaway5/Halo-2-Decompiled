#pragma once

#include "halo2/native_tu15_retained_store_observer.hpp"

#include <array>
#include <stdexcept>

namespace halo2 {

struct NativeTu15RetainedCounterUpdate {
    std::array<std::byte, 4> before{}, written{};
};

// One checked DATA transaction. The storage owner serializes and admits the
// entire writable operand before reading its current value, then commits it
// without repeating write policy or exposing an intermediate read on refusal.
class NativeTu15RetainedCounterStorage {
public:
    virtual ~NativeTu15RetainedCounterStorage() = default;
    virtual NativeTu15RetainedCounterUpdate adjust_counter(
        std::uint32_t address, unsigned width, std::uint32_t adjustment) = 0;
};

inline NativeTu15RetainedCounterUpdate native_tu15_adjust_counter_value(
    const std::array<std::byte, 4> &before, unsigned width, std::uint32_t adjustment) {
    if (width != 2u && width != 4u)
        throw std::invalid_argument("Native counter operand width is not qualified");
    std::uint32_t old{};
    for (unsigned i = 0; i < width; ++i)
        old |= std::to_integer<std::uint32_t>(before[i]) << (8u * i);
    NativeTu15RetainedCounterUpdate result{before, {}};
    const auto written = old + adjustment;
    for (unsigned i = 0; i < width; ++i)
        result.written[i] = std::byte(written >> (8u * i));
    return result;
}

inline void native_tu15_adjust_retained_counter(
    SourcePhysicsContactMemory &memory, std::uint32_t pc, std::uint32_t address,
    unsigned width, std::uint32_t adjustment, const SourcePhysicsContactRecordObserver &explicit_observer) {
    auto *storage = dynamic_cast<NativeTu15RetainedCounterStorage *>(&memory);
    if (!storage)
        throw std::runtime_error("Native counter update requires checked read-modify-write storage");
    if (const auto *domain = dynamic_cast<const NativeTu15RetainedStoreObservationDomain *>(&memory))
        domain->require_retained_extent(address, width);
    const auto &observer = native_tu15_effective_store_observer(memory, explicit_observer);
    const bool observe = native_tu15_observes_retained_store(memory, observer, pc, address, width);
    const auto result = storage->adjust_counter(address, width, adjustment);
    if (observe)
        observer({pc, address, {result.before.begin(), result.before.begin() + width},
                  {result.written.begin(), result.written.begin() + width}});
}

} // namespace halo2
