#include "halo2/hud.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <tuple>
#include <utility>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
auto values(const Vertex &v){return std::array{
    v.position.x,v.position.y,v.position.z,v.color.x,v.color.y,v.color.z,v.uv.x,v.uv.y,
    v.lightmap_uv.x,v.lightmap_uv.y,v.normal.x,v.normal.y,v.normal.z,
    v.tangent.x,v.tangent.y,v.tangent.z,v.binormal.x,v.binormal.y,v.binormal.z,
    v.incident.x,v.incident.y,v.incident.z,v.prt_cluster,
    v.prt_weights[0],v.prt_weights[1],v.prt_weights[2],v.prt_weights[3],
    v.source_local_position.x,v.source_local_position.y,v.source_local_position.z,v.source_color_alpha};}
bool same_vertices(std::span<const Vertex> a,std::span<const Vertex> b){
    return a.size()==b.size()&&std::equal(a.begin(),a.end(),b.begin(),[](const auto &x,const auto &y){
        return values(x)==values(y)&&std::tie(x.material,x.lightmap,x.bsp_cluster,x.source_section,x.source_part,
            x.source_stipple_level,x.source_part_type,x.source_part_flags)==
            std::tie(y.material,y.lightmap,y.bsp_cluster,y.source_section,y.source_part,
            y.source_stipple_level,y.source_part_type,y.source_part_flags);
    });
}
bool same_programs(const Mesh &a,const Mesh &b){
    if(a.materials.size()!=b.materials.size())return false;
    for(unsigned n=0;n<a.materials.size();++n){
        const auto &x=a.materials[n],&y=b.materials[n];
        if(x.forward_program.words!=y.forward_program.words||x.program_constants!=y.program_constants||
            x.forward_transforms!=y.forward_transforms||x.forward_diffuse!=y.forward_diffuse||
            x.forward_specular!=y.forward_specular||x.texture_frames!=y.texture_frames||
            x.diffuse.pixels!=y.diffuse.pixels)return false;
    }
    return true;
}
void bindings(MultiplayerHud &hud){
    for(unsigned view=0;view<4;++view){
        hud.prompt_gamepad(view,view&1);hud.prompt_gamepad_buttons(view,{0xe103,0xe101});
        hud.prompt_keyboard_buttons(view,{"F","Mouse 4"});
    }
}
}
int main(int argc,char **argv)try{
    check(argc==3||argc==4,"Supply TU1.5 XBE, original maps and optional Vista artwork");
    const auto maps=std::filesystem::path(argv[2]);const auto vista=argc==4?std::filesystem::path(argv[3]):std::filesystem::path{};
    if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(maps/"lockout.map")||
       (!vista.empty()&&!std::filesystem::exists(vista/"hud.tsv")))return 77;
    const auto original=OriginalData::load_xbe(argv[1]);
    check(original->image==OriginalData::Image::TitleUpdate15,"HUD handoff requires TU1.5");
    Cache cache(maps/"lockout.map",maps,original);const auto level=cache.load_content(0,true);
    unsigned cases{},draws{},negative_controls{};
    for(unsigned representation:{2u,3u}){
        auto authored=cache.load_hud(level.player_representations.at(representation).biped.datum,cache.slayer_starting_weapon());
        check(std::any_of(authored.widgets.begin(),authored.widgets.end(),[](const auto &w){return w.special==5;}),
            "HUD handoff lacks an actual shield-history widget");
        check(!authored.action_messages.weapons.empty(),"HUD handoff lacks actual action prompt definitions");
        const auto prompt=std::find_if(authored.action_messages.weapons.begin(),authored.action_messages.weapons.end(),
            [&](const auto &row){return row.second[0]&&authored.strings.contains(row.second[0]);});
        check(prompt!=authored.action_messages.weapons.end(),"HUD handoff lacks an actual pickup prompt string");
        for(bool equal_tick:{false,true})for(bool teams:{false,true}){
            Mesh used_mesh,fresh_mesh;MultiplayerHud used(authored,maps/"fonts",used_mesh,4,"Slayer",
                original->hud_bitmap_geometry_constants,vista),fresh(authored,maps/"fonts",fresh_mesh,4,"Slayer",
                original->hud_bitmap_geometry_constants,vista);
            bindings(used);bindings(fresh);
            MultiplayerState future;future.team_mode=teams;future.tick=90;future.clock_seconds=120;
            for(unsigned view=0;view<4;++view){
                auto &p=future.players[view];p.connected=true;p.shield=p.health=1;p.loaded=60;p.reserve=120;
                p.source_player=0x70000000+view;p.source_unit=0x20000+view;p.source_weapon=0x30000+view;
                p.source_flags=1;p.source_membership=view&1;p.position={float(view),0,0};p.velocity={2,0,0};p.score=10-int(view);
                p.source_configuration=SourcePlayerConfiguration{};(*p.source_configuration)[0x44]=representation==3;
                p.action_prompt.definition=prompt->first;p.action_prompt.target.action=1;
                p.action_prompt.target.index=1;p.action_prompt.target.object=0x90000+view;
            }
            future.team_scores={10,4};
            const auto observe=[](MultiplayerHud &hud,const MultiplayerState &state){
                for(unsigned view=0;view<4;++view){hud.update_motion_sensor(state,view,view);hud.animate(state,view,view,25,14400,double(state.tick)*tick_seconds);}
            };
            for(unsigned tick=90;tick<=120;++tick){future.tick=tick;observe(used,future);}
            future.tick=121;for(unsigned view=0;view<4;++view)future.players[view].shield=.8f;
            observe(used,future);used.voice_text("discarded future voice");
            for(unsigned view=0;view<4;++view)(void)used.draw(future,view,view,4,1280,720,25,14400);
            auto incoming=future;incoming.tick=equal_tick?121:5;incoming.clock_seconds=4;incoming.team_scores={1,8};
            for(unsigned view=0;view<4;++view){auto &p=incoming.players[view];p.shield=.4f;p.velocity={};p.score=int(view);}
            observe(used,incoming);observe(fresh,incoming);
            // Some earlier-tick shader clocks already discard their histories.
            // Compare geometry too: winning meters and retained speaker text
            // can differ without changing a shader constant or radar page.
            bool stale=!same_programs(used_mesh,fresh_mesh);
            for(unsigned view=0;view<4;++view)
                stale|=!same_vertices(used.draw(incoming,view,view,4,1280,720,25,14400),
                    fresh.draw(incoming,view,view,4,1280,720,25,14400));
            check(stale,"HUD handoff negative control did not retain stale presentation");++negative_controls;
            const auto count=used_mesh.materials.size();std::array<std::optional<unsigned>,4> sensor_slots;
            std::vector<const std::byte*> images;for(const auto &m:std::as_const(used_mesh).materials)images.push_back(m.diffuse.pixels.data());
            for(unsigned view=0;view<4;++view)sensor_slots[view]=used.motion_sensor_material(view);
            used.reset_lifetimes();used.reset_lifetimes();
            check(used_mesh.materials.size()==count,"HUD lifetime reset changed resident materials");
            for(unsigned n=0;n<count;++n)check(std::as_const(used_mesh).materials[n].diffuse.pixels.data()==images[n],
                "HUD lifetime reset replaced a resident image");
            for(unsigned view=0;view<4;++view)check(used.motion_sensor_material(view)==sensor_slots[view],"HUD reset lost a radar material slot");
            observe(used,incoming);
            check(same_programs(used_mesh,fresh_mesh),"Reset HUD shader inputs or sensor pages differ from independent fresh accepted state");
            for(unsigned view=0;view<4;++view){
                check(!used.action_text(incoming,view,view).empty()&&used.action_text(incoming,view,view)==fresh.action_text(incoming,view,view),
                    "HUD reset lost keyboard/gamepad action bindings");
            }
            for(unsigned width:{640u,1280u})for(unsigned players=1;players<=4;++players)for(unsigned view=0;view<players;++view){
                const auto a=used.draw(incoming,view,view,players,width,width==640?480:720,25,14400);
                const auto b=fresh.draw(incoming,view,view,players,width,width==640?480:720,25,14400);
                check(!a.empty()&&same_vertices(a,b),"Rebased HUD differs from independent fresh complete vertex stream");++draws;
            }
            ++incoming.tick;observe(used,incoming);observe(fresh,incoming);
            check(same_programs(used_mesh,fresh_mesh),"Rebased HUD continuation differs after its next tick");++cases;
        }
    }
    std::cout<<"PASS "<<cases<<" HUD lifetime replacements, "<<negative_controls<<" stale-history controls and "<<draws
        <<" complete local-view streams; TU1.5 assets, shield/score/sensor clocks, unchanged image/material identities and keyboard/gamepad prompts; Vista="<<!vista.empty()<<'\n';
    std::cout<<"LIMIT declared presentation banks and CPU outputs; no rendered migration or original Xbox migration reset entry point asserted\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
