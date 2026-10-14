#pragma once
#include "halo2/source_player_profile.hpp"
#include <cstdint>

namespace halo2 {
// A full native allocation and its original identity, supplied independently
// of an incoming authority cut by the accepted session's current host.
struct SourcePlayerAdmission {
    unsigned slot{};
    std::uint32_t player=0xffffffffu;
    SourcePlayerIdentity identity{};
    bool operator==(const SourcePlayerAdmission &)const=default;
};
}
