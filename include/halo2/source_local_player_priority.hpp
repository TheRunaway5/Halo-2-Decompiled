#pragma once
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Literal observer slot4E8C44 + slot*140. The callback is an executable
// identity, not a native callback invocation. Scalar retains binary32 bits.
struct SourcePriorityObserver {
    std::optional<std::uint32_t> callback, scalar_bits; // +C / +4
};
// Complete155760 classification. The special byte51EC10 is required only for
// callback23D090. Ordered zero matches both signs; NaNs/subnormals do not.
// This preserves the recorded source MXCSR with denormals-as-zero disabled.
std::uint8_t source_priority_observer_class(const SourcePriorityObserver &,
    std::optional<std::uint8_t> special_byte);
struct SourcePriorityOwner {
    std::uint32_t identity;
    std::optional<std::uint32_t> parent; // Actual object+14 full handle.
    std::optional<std::uint8_t> header_kind; // Actual header byte+3.
    std::optional<std::uint32_t> player; // Root object+13C, read for kind0.
};
struct SourceLocalPlayerPriorityContext {
    std::optional<std::int32_t> local_slot; // Actual4B9ED8, NONE=-1.
    std::span<const SourcePriorityObserver> observers; // Four source slots.
    std::span<const std::optional<std::uint32_t>> local_players; // (*4E8C20)+C.
    std::optional<std::uint8_t> observer_special; // Actual51EC10.
};
// Complete3E9C0 plus BAF80 parent traversal. Returns the source AL byte.
// Only fields read on the selected branch are required. Explicit NONE player
// datums may compare equal; missing fields, invalid slots/links and cycles are
// unresolved input errors. This does not produce observer modes, local-user
// mappings, ownership/registration order, or the current render-view slot.
std::uint8_t source_local_player_priority(std::span<const SourcePriorityOwner>,
    std::uint32_t owner,const SourceLocalPlayerPriorityContext &);
}
