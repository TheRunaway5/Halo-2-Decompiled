#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// Original642FCC reads the two final configurations only after both6157CA
// calls. Borrowed storage remains live across the second lookup.
using VistaLobbySortConfiguration =
    std::function<std::span<const std::byte>(std::uint32_t)>;
std::int32_t vista_lobby_compare_players(std::uint32_t, std::uint32_t,
    const VistaLobbySortConfiguration&);

// Original68B128/68B0AE/68B087, specialized to the actual lobby's at most16
// DWORD player IDs. Equal-team order follows that CRT, not host std::sort.
void vista_lobby_sort_players(std::span<std::uint32_t>,
    const VistaLobbySortConfiguration&);
} // namespace halo2
