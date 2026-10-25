#pragma once
#include <cstdint>
namespace halo2 {
// Complete189617..18963D: this held secondary-trigger intent follows the
// independent multiplayer bomb/input2 guard. Unit C6E90 consumes bit11;
// original weapon/unit consumers retain the other command bits.
constexpr std::uint32_t source_secondary_trigger_intent_tu15(std::uint32_t flags,bool held) {
    return held?flags|0x4800u:flags&~0x4800u;
}
}
