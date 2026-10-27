#pragma once
#include "halo2/netcode_mode.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace halo2 {
// Owning continuation value of the lag-compensation rewind history
// (docs/NETCODE_ENHANCED_GOAL.md item 2). It travels inside the Match
// authority checkpoint, so a replay world or a promoted host resolves the
// same rewound shots as the host that produced the cut. Every address is a
// TU1.5 bank address (object body, cluster entry), never a process pointer;
// the rewind re-validates each one against the live bank before writing.
struct NativeLagCompensationHistory {
    struct Object {
        std::uint32_t object=0xffffffffu,body{},parent=0xffffffffu;
        std::int16_t node_bytes{},node_offset{};
        std::vector<std::byte> nodes;         // WORLD node bank (52-byte nodes) or empty.
        std::array<std::byte,0x20> spheres{}; // object+30..+4F
        std::array<std::byte,12> position{};  // object+64
        bool operator==(const Object &)const=default;
    };
    struct Body {
        std::uint32_t slot{};
        std::uint32_t root=0xffffffffu;
        std::vector<Object> objects;           // Root first, then its attachment tree.
        bool registered{};
        std::array<std::byte,16> primitive{};  // Collidable entry words 4..7.
        std::vector<std::int16_t> clusters;
        bool operator==(const Body &)const=default;
    };
    struct Frame {
        std::uint32_t tick{};
        std::vector<Body> bodies;
        bool operator==(const Frame &)const=default;
    };
    NetcodeMode mode=NetcodeMode::original;
    std::vector<Frame> frames; // Oldest first; at most the retained window.
    bool operator==(const NativeLagCompensationHistory &)const=default;
};
// Retained window and per-frame bounds of the history above.
inline constexpr std::size_t lag_compensation_history_bodies=16;
inline constexpr std::size_t lag_compensation_history_objects=64;
}
