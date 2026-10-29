#pragma once
#include "halo2/source_event_messages_tu15.hpp"
#include "halo2/source_hud_messages.hpp"
#include <array>
#include <span>

namespace halo2 {
struct MultiplayerState;
struct MultiplayerEventMessageViewTu15 {
    std::uint32_t player=0xffffffffu;
    SourceHudMessages messages,source_messages;
    SourceHudPrimaryMessageTu15 primary;
};
// Local presentation state only. Retained with the authentic receipt frontier;
// neither localized text nor output-user indices enter an authority snapshot.
struct MultiplayerEventMessageStateTu15 {
    bool initialized{};
    std::uint32_t serial{};
    std::uint8_t sequence{};
    std::array<MultiplayerEventMessageViewTu15,16> views;
};
// Drain actual source publications once, independently of audio/mute and of
// network role. A newly connected client skips old broadcasts but retains its
// explicit admission event. Existing literal notices share each four-row queue.
void consume_multiplayer_event_messages_tu15(const SourceEventMessageAssetsTu15 &,
    const MultiplayerState &,std::span<const unsigned> local_slots,
    MultiplayerEventMessageStateTu15 &,bool client,float hold=2.f,float fade=2.f);
}
