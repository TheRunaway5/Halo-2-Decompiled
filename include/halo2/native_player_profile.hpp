#pragma once
#include "halo2/source_player_profile.hpp"
#include <string>
#include <string_view>

namespace halo2 {
// Native UTF8 frontend/provider boundary. Source configurations remain UTF16;
// no machine identity, account metadata or original input member is invented.
SourcePlayerConfiguration native_selected_player_configuration(std::string_view name,
    std::span<const std::uint8_t,9> appearance,std::int32_t committed_team);
std::u16string source_player_configuration_name(const SourcePlayerConfiguration&);
std::string native_player_configuration_name(const SourcePlayerConfiguration&);
} // namespace halo2
