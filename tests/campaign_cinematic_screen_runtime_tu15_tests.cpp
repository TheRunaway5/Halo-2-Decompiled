#include "halo2/source_campaign_screen_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/cache.hpp"
#include <iostream>
#include <unistd.h>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    auto path=(std::filesystem::temp_directory_path()/"halo2-source-screen-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Source screen storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    std::cout<<"PHASE source-construction\n"<<std::flush;
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);auto &w=objects.tu15_world();
    w.initialize_map();w.configure_campaign_session(1,1,30,0x12345678);
    auto &m=w.memory();const auto p=Bytes(m.view(0x515ec0u,4)).u32(0);
    if(!p || m.native_address(p,0x3fcu).value<=0xffffffffu || m.view(0x4922a8u,1)[0]!=std::byte{})
        throw FormatError("Source screen storage lacks its actual CPU arena owner");
    source_campaign_screen_clock_tu15(m,0.);
    source_campaign_screen_start_tu15(m,true);source_campaign_crossfade_set_tu15(m,1.f);
    source_campaign_screen_update_tu15(w);auto frame=source_campaign_crossfade_frame_tu15(m);
    if(!frame.capture || !frame.composite || frame.composite->words[43]!=0xff000000u || m.view(p+0x170u,1)[0]!=std::byte{})
        throw FormatError("Whole source update did not publish and consume the actual capture request");
    const auto saved=objects.authority_checkpoint();
    // Acknowledgement is an explicit completed-capture boundary in this CPU
    // test. Production calls it only after the Vulkan consumer confirms copy.
    const auto advance=[&] {
        source_campaign_screen_capture_complete_tu15(w);
        source_campaign_screen_clock_tu15(m,.25);source_campaign_screen_update_tu15(w);
        auto result=source_campaign_crossfade_frame_tu15(m);
        const auto state=source_campaign_crossfade_state_tu15(m);
        if(result.capture || !result.composite || state.alpha!=.75f || state.output_strength!=1.f)
            throw FormatError("Source crossfade clock/completion did not publish its actual output");
        source_campaign_screen_start_tu15(m,false);
        if(source_campaign_crossfade_state_tu15(m).alpha!=.75f)throw FormatError("Source nonreset start lost output");
        return result.composite->words;
    };
    const auto first=advance();
    if(!objects.commit_authority(objects.prepare_authority(saved)))throw FormatError("Source screen checkpoint did not publish");
    if(!source_campaign_crossfade_frame_tu15(m).capture || first!=advance())throw FormatError("Source screen checkpoint repeat differs");
    source_campaign_crossfade_set_tu15(m,1.f,2.f);source_campaign_screen_update_tu15(w);
    if(!source_campaign_crossfade_frame_tu15(m).capture || source_campaign_crossfade_state_tu15(m).strength!=2.f)
        throw FormatError("Source crossfade2 did not retain strength or request another capture");
    source_campaign_screen_stop_tu15(w);frame=source_campaign_crossfade_frame_tu15(m);
    if(frame.capture || frame.composite || std::ranges::any_of(m.view(p,0x3fcu),[](auto b){return b!=std::byte{};}))
        throw FormatError("Whole source stop did not clear its complete screen bank");
    std::cout<<"PASS actual source arena, whole screen update, clock, capture request/ack, crossfade2, stop and checkpoint repeat\n";
    std::cout<<"LIMIT acknowledgement is a declared CPU boundary; no GPU or whole opening playback claim\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
