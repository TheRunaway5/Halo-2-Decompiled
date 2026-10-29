#pragma once
#include "halo2/source_announcer_events_tu15.hpp"
#include "halo2/source_multiplayer_event_tu15.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace halo2 {
class Cache;

// The response definitions and localized mulg runtime+8C unic range belong to
// the current imported MCC map. Symbolic StringID names are not display text.
struct SourceEventMessageAssetsTu15 {
    SourceAnnouncerResponsesTu15 responses;
    std::map<std::uint32_t,std::u16string> strings;
    std::array<std::u16string,9> team_names;
    std::array<std::u16string,8> territory_names;
    unsigned language{};
};
SourceEventMessageAssetsTu15 load_source_event_message_assets_tu15(
    const Cache &,unsigned language=0);

enum class SourceEventMessageChannelTu15 { literal,quantity,primary };
struct SourceEventMessageOutputTu15 {
    SourceEventMessageChannelTu15 channel=SourceEventMessageChannelTu15::literal;
    std::u16string text,plural_text;
    std::int16_t quantity{};
    std::int32_t duration_seconds{};
    std::uint32_t string_id{},plural_string_id{};
    unsigned response_ordinal{},audience{};
};

// Per-view continuation of25AB76's separate primary output. A zero duration
// retains the authored value; its lifetime belongs to the primary draw owner.
struct SourceHudPrimaryMessageTu15 {
    std::u16string text;
    std::uint32_t created_tick{};
    std::int32_t duration_seconds{};
    bool active{};
};

// Receives the exact lowercase/underscore source token without '#'. The live
// caller supplies general1A7CE0 values and then its selected engine+C8 values.
// nullopt keeps the unhandled source token literal, including its '#'. An
// engaged empty string is a handled substitution producing no characters;
// original1A7C99 then also consumes the following input UTF16 unit, if any.
using SourceEventMessageTokenResolverTu15=
    std::function<std::optional<std::u16string>(std::u16string_view)>;

//1A7C10 copies at most maximum UTF16 code units, recognizes only[a-z_], and
// does not reparse inserted replacement text. Private-use glyphs are retained.
std::u16string source_event_message_format_tu15(std::u16string_view,
    const SourceEventMessageTokenResolverTu15 &,std::size_t maximum=255);
std::u16string source_event_message_utf16_tu15(std::string_view);
std::string source_event_message_utf8_tu15(std::u16string_view);

//1A83B0's text-only postgame and local-census suppression; audio is separate.
bool source_event_message_text_allowed_tu15(std::uint32_t type,
    std::int32_t subtype,unsigned audience,unsigned local_count,bool game_over);

// The caller supplies one actual active source local player, including its
// output-user index.1A8740's explicit-target/broadcast filter,1A83B0 locality,
// source recipient priority, and authored empty keys are retained. No online
// mode gate or score-based event synthesis occurs here.
std::vector<SourceEventMessageOutputTu15> source_event_messages_tu15(
    const SourceEventMessageAssetsTu15 &,const SourceMultiplayerEventTu15 &,
    const VistaAnnouncerLocalPlayer &,unsigned local_count,
    const SourceEventMessageTokenResolverTu15 &);
} // namespace halo2
