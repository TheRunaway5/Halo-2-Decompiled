#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/native_weapon_zoom_audio_tu15.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void need(bool ok,const char *why){if(!ok)throw FormatError(why);}
template<class F>void rejects(F run){bool rejected{};try{run();}catch(const FormatError &){rejected=true;}
    need(rejected,"Invalid zoom authority continuation was admitted");}
}
int main(int argc,char **argv)try {
    need(argc==4,"Supply prepared MCC MAP MAPS REPORT");if(!std::filesystem::is_regular_file(argv[1]))return 77;
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Use owned test storage");
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.local_players=2;
    options.starting_weapon=3;options.mute=true;options.source_object_world=true;options.session_random_seed=0x105240;
    options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,3}}};
    options.source_object_savegame_directory=std::filesystem::path(scratch)/"source";
    MultiplayerPreparedWorld source(options);
    options.source_object_savegame_directory=std::filesystem::path(scratch)/"target";
    MultiplayerPreparedWorld target(options);
    std::array<Controls,multiplayer_capacity> controls{};
    for(unsigned n=0;n<120;++n)source.match.step(controls);
    NativeWeaponZoomAudioCursorTu15 cursor;std::vector<NativeWeaponZoomAudioPendingTu15> pending;
    collect_native_weapon_zoom_audio_tu15(source.match.state(),source.local_slots,cursor,pending);
    const auto id=source.local_slots.at(1);
    controls[id].zoom=0;controls[id].zoom_reset_serial=source.match.state().players[id].zoom_reset_serial;
    source.match.step(controls);
    collect_native_weapon_zoom_audio_tu15(source.match.state(),source.local_slots,cursor,pending);
    need(pending.size()==1 && !pending.front().event.forced_reset && pending.front().view==1,
        "Actual scoped cut lacks its pending second-view normal request");
    SourceMatchAuthority from(source.match,*source.source_objects),to(target.match,*target.source_objects);
    const SourceMatchAuthority::AuthorityDigest binding{std::byte{0x15},std::byte{0x24}};
    const auto cut=from.checkpoint(binding);
    const auto encoded=encode_source_authority(cut);const auto decoded=decode_source_authority(encoded);
    need(decoded.match.version==4 && decoded.match.weapon_zoom_audio_journal==cut.match.weapon_zoom_audio_journal &&
        decoded.match.weapon_zoom_audio_serial==cut.match.weapon_zoom_audio_serial &&
        decoded.match.weapon_zoom_audio_journal==source.match.state().weapon_zoom_audio &&
        encode_source_authority(decoded)==encoded,"Native codec changed retained zoom ownership/request/serial bytes");
    {auto invalid=decoded;invalid.match.version=3;rejects([&]{to.prepare(invalid,binding);});}
    {auto invalid=decoded;invalid.match.weapon_zoom_audio_serial=0;rejects([&]{to.prepare(invalid,binding);});}
    auto prepared=to.prepare(decoded,binding);need(to.commit(std::move(prepared)),"Actual native zoom continuation did not commit");
    need(target.match.state().weapon_zoom_audio==source.match.state().weapon_zoom_audio &&
        target.match.state().players[id].source_zoom.current==0,"Committed native scope/journal cut changed");
    // This cursor and the unplayed normal cue belong to the accepted receipt.
    // Re-consuming that cut must preserve the cue exactly once.
    collect_native_weapon_zoom_audio_tu15(target.match.state(),source.local_slots,cursor,pending);
    need(pending.size()==1,"Authority adoption replayed/dropped a pending normal zoom cue");pending.clear();
    bool admitted{},reset{};unsigned forced{};
    for(unsigned n=0;n<36;++n) {
        controls[id].melee=n==0;source.match.step(controls);target.match.step(controls);
        const auto a=source.match.state(),b=target.match.state();
        need(a.weapon_zoom_audio==b.weapon_zoom_audio,"Restored native owner diverged in future zoom journal");
        need(a.players[id].source_zoom.current==b.players[id].source_zoom.current,
            "Restored native owner diverged in future scope reset");
        admitted|=b.players[id].melee_action.active();
        if(b.players[id].source_zoom.current==-1){reset=true;controls[id].zoom=-1;
            controls[id].zoom_reset_serial=b.players[id].zoom_reset_serial;}
        collect_native_weapon_zoom_audio_tu15(b,source.local_slots,cursor,pending);
        for(const auto &cue:pending){need(cue.view==1 && cue.event.forced_reset &&
                cue.event.serial==cut.match.weapon_zoom_audio_serial+1,"Continued reset lost its recipient/serial/action");++forced;}
        pending.clear();
    }
    need(admitted && reset && forced==1 && !cursor.missing_prefix,
        "Continued real melee did not produce exactly one committed zoom reset");
    const nlohmann::json report{{"passed",true},{"runtime_input","prepared MCC"},{"authority_checkpoint_version",4},
        {"native_source_ticks_after_restore",36},{"retained_normal_requests",1},{"continued_forced_resets",forced},
        {"recipient_view",1},{"canonical_codec_roundtrip",true},{"invalid_version_and_serial_rejected",true},
        {"encoded_checkpoint_bytes_temporary",encoded.size()},{"retained_large_artifacts",nlohmann::json::array()}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
