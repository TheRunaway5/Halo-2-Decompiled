#include "halo2/multiplayer_world.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/scene.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
void require(bool value,const char* reason){if(!value)throw FormatError(reason);}
int main(int argc,char** argv){std::ostringstream diagnostics;auto* prior=std::cerr.rdbuf(diagnostics.rdbuf());try {
    require(argc==7 || argc==8,"Supply XBE MAP SHARED NEW_OUTPUT Original|pStencil TICK [third-person]");
    const std::filesystem::path output=argv[4];require(!std::filesystem::exists(output),"capture output exists");
    std::filesystem::create_directories(output);
    MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];options.shared_maps=argv[3];
    options.width=1280;options.height=720;options.local_players=1;options.hidden=true;options.validation=true;options.mute=true;
    options.source_object_world=true;options.source_object_savegame_directory=output/"savegame";
    options.capture_tick=std::stoul(argv[6]);options.reference_tick_cadence=true;options.reference_render_fraction=0;
    options.reference_auxiliary_seed=305419896u;options.session_random_seed=305419896u;
    options.desktop=std::make_shared<DesktopSession>();
    require(std::string(argv[5])=="Original" || std::string(argv[5])=="pStencil","invalid graphics mode");
    options.desktop->display_settings.graphics_mode=std::string(argv[5])=="pStencil"?pstencil::GraphicsMode::pStencil:pstencil::GraphicsMode::Original;
    options.input_script=output/"inputs.txt";options.screenshot=output/"game.ppm";
    {std::ofstream f(options.input_script);f.precision(9);
        for(unsigned tick=0;tick<options.capture_tick;++tick)
            f<<tick<<" 0 0 0 "<<(tick==20?1:0)<<" 0.996 -1.3"<<(tick==5?" switch-grenade":"")<<'\n';}
    options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
    auto& world=*options.prepared_world;const auto slot=world.local_slots.at(0);
    const auto state=world.match.state();const auto& player=state.players.at(slot);
    require(world.source_objects && world.source_objects->owns_physics(),"capture requires actual TU1.5 source owner");
    // Explicit fixture inventory, before the first command: the real source
    // unit's plasma-count byte, independently documented by player_equipment.
    // All selection, throw, motion, light/effect and rendering owners run in
    // the actual game. No light, color, pose or detonation is synthesized.
    auto& source=world.source_objects->tu15_world();const auto body=source.source_object_body(player.source_unit);
    const std::array<std::byte,1> plasma{std::byte{2}};source.write_source_call_input(body+0x23fu,plasma);
    require(source.player_equipment(player.source_unit).grenades[1]==2,"source fixture inventory was not applied");
    const auto initial_detail=Bytes(source.memory().view(body+4,4)).u32(0)&0x80000u;
    unsigned player_lod5_draws=0;
    if(argc==8) {
        require(std::string(argv[7])=="third-person","unknown camera fixture");
        Camera camera;camera.position=player.position+Vec3{2.2f,2.2f,1.3f};
        const auto delta=player.position+Vec3{0,0,.5f}-camera.position;
        const auto length=std::sqrt(dot(delta,delta));camera.forward=delta/length;camera.up={0,0,1};camera.oriented=true;camera.field_of_view=70;
        options.capture_camera=camera;options.capture_third_person=true;
        options.unit_source_draw_observer=[&](unsigned,std::uint32_t owner,std::uint32_t,const ObjectLightingState&,
            bool,unsigned lod,std::span<const Vertex>) {
            if(owner!=player.source_unit)return;
            if(options.desktop->display_settings.graphics_mode==pstencil::GraphicsMode::pStencil) {
                require(lod==5,"pStencil player did not select authored high-detail LOD5");++player_lod5_draws;
            }
            const auto flags=Bytes(source.memory().view(source.source_object_body(owner)+4,4)).u32(0);
            require((flags&0x80000u)==initial_detail,"pStencil detail flag escaped the render-model view");
        };
    }
    bool plasma_seen=false;
    options.input_observer=[&](unsigned,std::span<const Controls>,const MultiplayerState& current) {
        for(const auto& p:current.grenade_projectiles)if(p.type==1)plasma_seen=true;
    };
    const auto result=run_multiplayer(options);require(result==0,"game capture failed");
    require(plasma_seen,"game never created a real plasma grenade");
    if(argc==8 && options.desktop->display_settings.graphics_mode==pstencil::GraphicsMode::pStencil)
        require(player_lod5_draws>0,"pStencil high-detail player was never drawn");
    std::ofstream report(output/"fixture.txt");report<<"Actual TU1.5 game with current Lockout assets. Declared initial plasma inventory: two grenades.\n"
        <<"Source player="<<player.source_player<<" unit="<<player.source_unit<<"\n"
        <<"Camera="<<(argc==8?"declared third-person diagnostic":"ordinary first-person gameplay")<<"\n";
    report<<"pStencil LOD5 player draws="<<player_lod5_draws<<"; native quality flag restored after every model view.\n";
    std::cerr.rdbuf(prior);require(diagnostics.str().find("Vulkan validation:")==std::string::npos,"Vulkan validation diagnostics");
    std::ofstream(output/"diagnostics.txt")<<diagnostics.str();return 0;
}catch(const std::exception& e){std::cerr.rdbuf(prior);std::cerr<<diagnostics.str()<<e.what()<<'\n';return 1;}}
