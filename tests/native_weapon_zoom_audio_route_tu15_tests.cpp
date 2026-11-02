#include "halo2/native_weapon_zoom_audio_tu15.hpp"
#include "halo2/multiplayer.hpp"
#include <iostream>
using namespace halo2;
namespace {void need(bool ok,const char *why){if(!ok)throw FormatError(why);}}
int main()try {
    MultiplayerState state;state.tick=42;auto &player=state.players[3];player.connected=true;
    player.source_player=0x80010003;player.source_unit=0x80010005;
    const std::array<unsigned,2> locals{4,3};NativeWeaponZoomAudioCursorTu15 cursor;
    std::vector<NativeWeaponZoomAudioPendingTu15> pending;
    NativeWeaponZoomAudioEventTu15 event;event.serial=7;event.tick=41;event.unit=player.source_unit;
    event.player=player.source_player;event.sound=0x8001000b;event.control_bits=0x3f800000;event.forced_reset=true;
    state.weapon_zoom_audio={event};collect_native_weapon_zoom_audio_tu15(state,locals,cursor,pending);
    need(cursor.baselined && cursor.serial==7 && pending.empty(),"Initial receipt replayed historical zoom");
    event.serial=8;state.weapon_zoom_audio={event};collect_native_weapon_zoom_audio_tu15(state,locals,cursor,pending);
    need(pending.size()==1 && pending[0].view==1 && pending[0].event==event,"Full player/view route lost a real reset");
    const auto saved_cursor=cursor;const auto saved_pending=pending;
    collect_native_weapon_zoom_audio_tu15(state,locals,cursor,pending);need(pending.size()==1,"Repeated receipt duplicated zoom");
    event.serial=9;event.player^=0x10000;state.weapon_zoom_audio={event};collect_native_weapon_zoom_audio_tu15(state,locals,cursor,pending);
    need(cursor.serial==9 && pending.size()==1,"Reused player slot inherited an old player's zoom");
    cursor=saved_cursor;pending=saved_pending;collect_native_weapon_zoom_audio_tu15(state,locals,cursor,pending);
    need(cursor.serial==9 && pending.size()==1 && pending[0].event.serial==8,"Restoring a pending receipt dropped/duplicated its cue");
    player.source_unit=0x80020005;event.player=player.source_player;event.serial=10;
    state.weapon_zoom_audio={event};collect_native_weapon_zoom_audio_tu15(state,locals,cursor,pending);
    need(pending.size()==2 && pending.back().event.unit==0x80010005,"Unowned committed cue was lost after unit retirement");
    event.serial=11;event.sound=null_datum;event.control_bits=0;state.weapon_zoom_audio={event};
    collect_native_weapon_zoom_audio_tu15(state,locals,cursor,pending);
    need(pending.size()==3 && pending.back().event.sound==null_datum,"Silent authored reset acquired a replacement sound");
    event.serial=12;state.weapon_zoom_audio={event};collect_native_weapon_zoom_audio_tu15(state,locals,cursor,pending,false);
    need(cursor.serial==12 && pending.size()==3,"Muted route queued or failed to acknowledge a cue");
    event.serial=15;state.weapon_zoom_audio={event};collect_native_weapon_zoom_audio_tu15(state,locals,cursor,pending);
    need(cursor.missing_prefix==2 && pending.size()==4,"Retention gap was concealed or filled with invented events");
    event.serial=16;player.connected=false;state.weapon_zoom_audio={event};collect_native_weapon_zoom_audio_tu15(state,locals,cursor,pending);
    need(cursor.serial==16 && pending.size()==4,"Disconnected player received a cue");
    event.control_bits=0x7fc00000;state.weapon_zoom_audio={event};bool rejected{};
    try{validate_native_weapon_zoom_audio_tu15(state.weapon_zoom_audio,state.tick);}catch(const FormatError &){rejected=true;}
    need(rejected,"Nonfinite zoom control was admitted");
    std::cout<<"PASS native zoom audience generations, receipt baseline/replay/restore, unit retirement, NONE, mute and retention accounting\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
