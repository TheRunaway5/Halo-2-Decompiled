#pragma once
#include "halo2/physics_contact_record.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>

namespace halo2::physics_step_detail {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;

inline std::uint32_t r(const M &m, std::uint32_t a, unsigned n = 4) {
    if (n == 0 || n > 4)
        throw std::runtime_error("Invalid original physics scalar read width");
    std::array<std::byte, 4> b{};
    // Complete committed ordinary pages have no read side effects. Device
    // adapters and page-crossing operands keep their ordered virtual access.
    if (const auto *page = m.direct_page(a, n)) {
        const auto *const bytes = page->bytes + (a & 4095u);
        switch (n) {
        case 1: std::memcpy(b.data(), bytes, 1); break;
        case 2: std::memcpy(b.data(), bytes, 2); break;
        case 3: std::memcpy(b.data(), bytes, 3); break;
        case 4: std::memcpy(b.data(), bytes, 4); break;
        }
    } else m.read(a, std::span(b).first(n));
    if constexpr (std::endian::native == std::endian::little)
        return std::bit_cast<std::uint32_t>(b);
    std::uint32_t x = 0;
    for (unsigned i = 0; i < n; ++i) x |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
    return x;
}

inline void w(M &m, const O &o, std::uint32_t site, std::uint32_t a,
              unsigned n, std::uint64_t x) {
    // The original store writes its destination; only the optional trace
    // needs its preimage. Keep real write validation in the memory owner.
    // A synthetic read here doubles translated-memory traffic and can also
    // consume a device read that the original instruction never performed.
    if (!o && n <= 8u) {
        std::array<std::byte, 8> written{};
        for (unsigned i = 0; i < n; ++i)
            written[i] = std::byte((x >> (i * 8)) & 255);
        m.write(a, std::span<const std::byte>(written).first(n));
        return;
    }
    SourcePhysicsContactRecordStore s{site, a, {}, {}};
    s.before.resize(n); s.written.resize(n); m.read(a, s.before);
    for (unsigned i = 0; i < n; ++i) s.written[i] = std::byte((x >> (i * 8)) & 255);
    m.write(a, s.written); if (o) o(s);
}
} // namespace halo2::physics_step_detail
