#include "halo2/multiplayer_event_messages_tu15.hpp"
#include "halo2/multiplayer.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
bool newer(std::uint32_t a,std::uint32_t b) {
    return std::bit_cast<std::int32_t>(a-b)>0;
}
bool expired(std::uint32_t created,std::uint32_t tick,float hold,float fade) {
    return float(std::bit_cast<std::int32_t>(tick-created))*(1.f/30.f)>=hold+fade;
}
std::u16string row_text(const std::array<std::byte,0x88> &row) {
    std::u16string out;
    for(unsigned i=4;i<0x80;i+=2) {
        const auto c=std::to_integer<unsigned>(row[i])|(std::to_integer<unsigned>(row[i+1])<<8);
        if(!c)break;out.push_back(char16_t(c));
    }
    return out;
}
std::uint32_t row_tick(const std::array<std::byte,0x88> &row) {
    std::uint32_t tick{};for(unsigned i=0;i<4;++i)tick|=std::to_integer<unsigned>(row[i])<<(i*8);return tick;
}
}
void consume_multiplayer_event_messages_tu15(const SourceEventMessageAssetsTu15 &assets,
    const MultiplayerState &state,std::span<const unsigned> local_slots,
    MultiplayerEventMessageStateTu15 &presentation,bool client,float hold,float fade) {
    if(local_slots.size()>4)throw FormatError("Event messages exceed four local users");
    std::array<bool,16> local{};
    for(auto slot:local_slots) {
        if(slot>=state.players.size() || local[slot])throw FormatError("Invalid local event message roster");
        local[slot]=true;
        const auto &player=state.players[slot];auto &view=presentation.views[slot];
        if(view.player!=player.source_player) {
            view={};view.player=player.source_player;
        }
        // Existing original literal notices (for example blocked teleports)
        // join the same queue exactly once, without modifying received state.
        std::vector<unsigned> notices;
        for(unsigned n=0;n<player.source_messages.rows.size();++n)
            if(player.source_messages.rows[n]!=view.source_messages.rows[n] &&
                player.source_messages.rows[n][0x82]!=std::byte{} &&
                !expired(row_tick(player.source_messages.rows[n]),state.tick,hold,fade))notices.push_back(n);
        std::stable_sort(notices.begin(),notices.end(),[&](unsigned a,unsigned b) {
            const auto &left=player.source_messages.rows[a],&right=player.source_messages.rows[b];
            const auto delta=std::bit_cast<std::int32_t>(row_tick(left)-row_tick(right));
            return delta?delta<0:std::to_integer<unsigned>(left[0x83])<std::to_integer<unsigned>(right[0x83]);
        });
        for(auto n:notices) {
            const auto &row=player.source_messages.rows[n];
            source_hud_message_submit(view.messages,presentation.sequence,row_tick(row),row_text(row));
        }
        view.source_messages=player.source_messages;
        source_hud_messages_expire(view.messages,state.tick,1.f/30.f,hold,fade);
    }
    const bool initial=!presentation.initialized;
    presentation.initialized=true;
    for(const auto &event:state.source_engine_events)if(newer(event.serial,presentation.serial)) {
        presentation.serial=event.serial;
        // A new client has no preceding receipt. Old broadcast publications
        // are history, whereas an explicit admission can precede this snapshot.
        if(initial && client && event.tick!=state.tick &&
            (event.words[2]==null_datum || std::ranges::none_of(local_slots,[&](auto slot) {
                return state.players[slot].source_player==event.words[2];
            })))continue;
        for(unsigned user=0;user<local_slots.size();++user) {
            const auto slot=local_slots[user];const auto &player=state.players[slot];
            const auto &dispatch=event.players[slot];
            if(!player.connected || player.source_player==null_datum ||
                dispatch.datum!=player.source_player)continue;
            const auto resolver=[&](std::u16string_view token)->std::optional<std::u16string> {
                if(token==u"cause_player" || token==u"effect_player") {
                    const auto datum=event.words[token==u"cause_player"?3:5];
                    if(datum==null_datum)return std::nullopt;
                    const auto index=datum&65535u;
                    // Departures retain their salted roster record and name.
                    // Never substitute a new participant occupying an old slot.
                    if(index>=state.players.size() || state.players[index].source_player!=datum)return std::u16string{};
                    return source_event_message_utf16_tu15(state.players[index].name);
                }
                if(token==u"cause_team" || token==u"effect_team") {
                    const auto team=event.words[token==u"cause_team"?4:6];
                    const bool none_allowed=event.words[0]==0 && (event.words[1]==23 || event.words[1]==45);
                    if(team==null_datum && !none_allowed)return std::u16string{};
                    const auto index=team==null_datum?8u:team;
                    if(index>=assets.team_names.size())return std::u16string{};
                    return assets.team_names[index];
                }
                if(token==u"territory_name" && event.words[0]==9) {
                    const auto index=std::bit_cast<std::int16_t>(event.data);
                    if(index<0 || unsigned(index)>=assets.territory_names.size())return std::u16string{};
                    return assets.territory_names[unsigned(index)];
                }
                // These are the five tokens used by the authored response
                // banks. Other engine token owners retain the literal fallback.
                return std::nullopt;
            };
            const VistaAnnouncerLocalPlayer recipient{dispatch.datum,dispatch.flags,dispatch.membership,std::int16_t(user)};
            auto &view=presentation.views[slot];
            for(const auto &output:source_event_messages_tu15(assets,event,recipient,unsigned(local_slots.size()),resolver)) {
                if(output.channel==SourceEventMessageChannelTu15::primary) {
                    view.primary={output.text.substr(0,62),event.tick,output.duration_seconds,true};
                } else if(output.channel==SourceEventMessageChannelTu15::literal) {
                    if(!expired(event.tick,state.tick,hold,fade))
                        source_hud_message_submit(view.messages,presentation.sequence,event.tick,output.text);
                } else {
                    // None of the actual TU1.5/MCC response rows uses the
                    // quantity channel. Do not silently display an unexpanded
                    // printf/quantity template from a modified cache.
                    throw FormatError("Authored event message quantity channel is unsupported");
                }
            }
        }
    }
    // Expire after draining too: delayed packets cannot revive stale rows.
    for(auto slot:local_slots)
        source_hud_messages_expire(presentation.views[slot].messages,state.tick,1.f/30.f,hold,fade);
}
}
