#include "halo2/multiplayer_event_messages_tu15.hpp"
#include "halo2/multiplayer.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
// These tiny response definitions and journal inputs are synthetic delivery
// controls. They prove queue/receipt behavior, not gameplay event production;
// multiplayer_event_messages_tu15_tests owns the real-engine producer checks.
void need(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
void word(VistaAnnouncerResponse& row,unsigned at,std::uint32_t value,unsigned size=4) {
    for(unsigned n=0;n<size;++n)row[at+n]=std::byte(value>>(8*n));
}
void response(SourceEventMessageAssetsTu15& assets,unsigned type,int subtype,unsigned audience,
    std::uint32_t key,std::u16string text) {
    VistaAnnouncerResponse row{};word(row,0,type<<16);word(row,4,std::uint32_t(subtype),2);
    word(row,6,audience,2);word(row,0xc,key);
    assets.responses.banks[type].definitions.push_back(row);assets.strings[key]=std::move(text);
}
SourceEventMessageAssetsTu15 assets() {
    SourceEventMessageAssetsTu15 out;
    for(unsigned n=0;n<8;++n)response(out,2,100+int(n),4,100+n,
        std::u16string(u"control ")+char16_t(u'A'+n));
    response(out,0,0,0,200,u"killer #effect_player");
    response(out,0,0,2,201,u"victim #cause_player");
    return out;
}
MultiplayerState state() {
    MultiplayerState out;out.tick=100;
    for(unsigned n=0;n<3;++n) {
        auto& player=out.players[n];player.connected=true;
        player.source_player=0x12340000+n;player.source_flags=1;player.source_membership=std::int16_t(n);
        player.name="Player "+std::to_string(n+1);
    }
    return out;
}
SourceMultiplayerEventTu15 packet(const MultiplayerState& state,unsigned serial,unsigned subtype,
    unsigned tick,std::uint32_t target=null_datum,unsigned type=2) {
    SourceMultiplayerEventTu15 out;out.serial=serial;out.tick=tick;out.mode=1;
    out.words={type,subtype,target,state.players[0].source_player,0,state.players[1].source_player,1,0};
    for(unsigned n=0;n<state.players.size();++n)if(state.players[n].connected)
        out.players[n]={state.players[n].source_player,state.players[n].source_flags,
            std::int8_t(state.players[n].source_membership)};
    return out;
}
std::vector<std::u16string> lines(const SourceHudMessages& bank,unsigned tick) {
    std::vector<std::u16string> out;
    for(const auto& row:source_hud_message_lines(bank,tick,1.f/30.f,2.f,2.f,4))out.push_back(row.text);
    return out;
}
void legacy_merge(const SourceEventMessageAssetsTu15& definition) {
    auto current=state();std::uint8_t source_sequence{};
    for(const auto* text:{u"notice A",u"notice B",u"notice C",u"notice D",u"notice E"})
        source_hud_message_submit(current.players[0].source_messages,source_sequence,current.tick,text);
    MultiplayerEventMessageStateTu15 receipt;const std::array local{0u};
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    const auto expected=lines(current.players[0].source_messages,current.tick);
    need(expected==std::vector<std::u16string>{u"notice E",u"notice D",u"notice C",u"notice B"},
        "Synthetic legacy control did not exercise physical-slot replacement");
    need(lines(receipt.views[0].messages,current.tick)==expected,
        "Legacy notices lost source same-tick sequence order during native merge");
    const auto messages=receipt.views[0].messages;const auto sequence=receipt.sequence;
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    need(receipt.views[0].messages==messages && receipt.sequence==sequence,"Unchanged blocked notices replayed");
    ++current.tick;
    source_hud_message_submit(current.players[0].source_messages,source_sequence,current.tick,u"new blocked notice");
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    need(lines(receipt.views[0].messages,current.tick).front()==u"new blocked notice" &&
        std::uint8_t(receipt.sequence-sequence)==1,"Changed blocked notice failed to merge exactly once");
}
void simultaneous_and_restore(const SourceEventMessageAssetsTu15& definition) {
    auto current=state();const std::array local{0u,1u};MultiplayerEventMessageStateTu15 receipt;
    current.source_engine_events={packet(current,1,100,current.tick,current.players[0].source_player),
        packet(current,2,101,current.tick,current.players[1].source_player)};
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    need(lines(receipt.views[0].messages,current.tick)==std::vector<std::u16string>{u"control A"} &&
        lines(receipt.views[1].messages,current.tick)==std::vector<std::u16string>{u"control B"},
        "Simultaneous targeted events crossed local users or were lost");
    current.source_engine_events.push_back(packet(current,3,102,current.tick));
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    need(lines(receipt.views[0].messages,current.tick)==std::vector<std::u16string>{u"control C",u"control A"} &&
        lines(receipt.views[1].messages,current.tick)==std::vector<std::u16string>{u"control C",u"control B"},
        "Same-tick different serial or broadcast local routing was collapsed");
    auto restored=receipt;const auto sequence=receipt.sequence;
    consume_multiplayer_event_messages_tu15(definition,current,local,restored,false);
    need(restored.sequence==sequence && restored.serial==receipt.serial &&
        restored.views[0].messages==receipt.views[0].messages && restored.views[1].messages==receipt.views[1].messages,
        "Copied presentation receipt replayed its journal frontier");
    current.source_engine_events.push_back(packet(current,4,103,current.tick,current.players[0].source_player));
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    consume_multiplayer_event_messages_tu15(definition,current,local,restored,false);
    need(restored.sequence==receipt.sequence && restored.serial==receipt.serial &&
        restored.views[0].messages==receipt.views[0].messages && restored.views[1].messages==receipt.views[1].messages,
        "Restored continuation diverged on the next actual serial");
}
void expiry_and_retention(const SourceEventMessageAssetsTu15& definition) {
    auto current=state();const std::array local{0u};MultiplayerEventMessageStateTu15 receipt;
    for(unsigned n=0;n<6;++n)current.source_engine_events.push_back(packet(current,n+1,100+n,current.tick+n));
    current.tick+=5;
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    need(lines(receipt.views[0].messages,current.tick)==
        std::vector<std::u16string>{u"control F",u"control E",u"control D",u"control C"},
        "Four-row native queue failed to retain the newest events");
    current.tick=225;
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    need(lines(receipt.views[0].messages,current.tick).empty(),"Expired messages remained visible or journal replay revived them");
    current.source_engine_events.push_back(packet(current,7,106,104));
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    need(lines(receipt.views[0].messages,current.tick).empty() && receipt.serial==7,
        "Delayed expired event revived a stale row or failed to advance its receipt frontier");
    std::uint8_t source_sequence{};
    for(const auto* text:{u"fresh A",u"fresh B",u"fresh C",u"fresh D"})
        source_hud_message_submit(current.players[0].source_messages,source_sequence,current.tick,text);
    current.source_engine_events.push_back(packet(current,8,107,104));
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    need(receipt.serial==8 && lines(receipt.views[0].messages,current.tick)==
        std::vector<std::u16string>{u"fresh D",u"fresh C",u"fresh B",u"fresh A"},
        "An already expired journal packet evicted a fresh legacy notice before retiring");
}
void salt_and_late_join(const SourceEventMessageAssetsTu15& definition) {
    auto current=state();const std::array local{0u};MultiplayerEventMessageStateTu15 receipt;
    current.source_engine_events.push_back(packet(current,1,100,current.tick));
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    current.players[0].source_player=0x12350000;
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    need(lines(receipt.views[0].messages,current.tick).empty() && receipt.views[0].player==0x12350000,
        "Changed full salted local identity retained the preceding occupant HUD");
    current.source_engine_events.push_back(packet(current,2,101,current.tick));
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    need(lines(receipt.views[0].messages,current.tick)==std::vector<std::u16string>{u"control B"},
        "New salted occupant did not receive its new journal event");
    auto joining=state();joining.tick=200;joining.source_engine_events={
        packet(joining,1,100,190),packet(joining,2,101,190,joining.players[1].source_player),
        packet(joining,3,102,190,joining.players[0].source_player),packet(joining,4,103,200)};
    MultiplayerEventMessageStateTu15 client;
    consume_multiplayer_event_messages_tu15(definition,joining,local,client,true);
    need(lines(client.views[0].messages,joining.tick)==std::vector<std::u16string>{u"control D",u"control C"} && client.serial==4,
        "Late join replayed old broadcasts or lost its own targeted admission/current event");
    const auto messages=client.views[0].messages;const auto sequence=client.sequence;
    consume_multiplayer_event_messages_tu15(definition,joining,local,client,true);
    need(client.views[0].messages==messages && client.sequence==sequence,"Late-join receipt replayed retained targeted history");
}
void unicode_names(const SourceEventMessageAssetsTu15& definition) {
    auto current=state();current.players[0].name="\xe9\x9b\xaa#alpha";
    current.players[1].name="\xf0\x9f\xa6\x8a#effect_player";
    current.source_engine_events={packet(current,1,0,current.tick,null_datum,0)};
    const std::array local{0u,1u};MultiplayerEventMessageStateTu15 receipt;
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    need(lines(receipt.views[0].messages,current.tick)==std::vector<std::u16string>{u"killer \U0001f98a#effect_player"} &&
        lines(receipt.views[1].messages,current.tick)==std::vector<std::u16string>{u"victim \u96ea#alpha"},
        "Actual roster Unicode/hash names were reparsed, corrupted, or crossed recipient roles");
    current.players[0].source_player=0x12350000;current.players[0].name="new occupant";
    current.source_engine_events.push_back(packet(current,2,0,current.tick,null_datum,0));
    current.source_engine_events.back().words[3]=0x12340000;
    consume_multiplayer_event_messages_tu15(definition,current,local,receipt,false);
    const auto victim=lines(receipt.views[1].messages,current.tick);
    need(std::ranges::none_of(victim,[](const auto& text){return text.find(u"new occupant")!=std::u16string::npos;}),
        "Salted cause name lookup substituted the next participant occupying an old slot");
}
}
int main() {try {
    const auto definition=assets();legacy_merge(definition);simultaneous_and_restore(definition);
    expiry_and_retention(definition);salt_and_late_join(definition);unicode_names(definition);
    std::cout<<"Synthetic delivery controls passed: legacy merge/order/no replay, same-tick serials, isolated locals, receipt restore, retention/expiry, salt reset, late join and Unicode/hash names\n";
    return 0;
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
