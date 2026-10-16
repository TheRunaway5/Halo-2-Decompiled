#include "halo2/multiplayer_world.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <algorithm>
#include <iostream>

using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=4)throw FormatError("Supply TU15_XBE BACKWASH_MAP DISC_MAPS");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];
    options.shared_maps=argv[3];options.local_players=1;options.starting_weapon=5;options.mute=true;
    MultiplayerPreparedWorld world(options);const auto player=world.match.join("Source swarm frame test");
    const auto current=world.match.state();world.source_objects->set_local_player(current.players[player].source_player,0);
    SourceWorldParticleDrawsTu15 graphics(*world.cache,world.content.objects);
    if(!graphics.active())throw FormatError("Actual Backwash swarm resources are absent");
    const Vec3 eye{0,11.4f,-20.85f};const auto angle=source_camera_angles(1.2217304763960306f);
    SourceFrustumProjectionInput pi;pi.projection_angle=angle.projection_angle;
    pi.outer={0,0,480,640};pi.safe=pi.outer;pi.scenario_present=true;
    pi.scenario_type=world.cache->load_source_geometry_scenario_type();
    const auto projection=source_frustum_projection(pi);
    const auto camera=source_frustum_camera({eye,{1,0,0},{0,0,1},.015625f,1024.f,projection.clipping_rectangle()});
    SourcePortalMaskState mask;mask.initialize(world.cache->load_source_portal_mask_definition(0),{});
    const auto visibility=source_portal_visibility(world.cache->load_source_portal_bsp(0),std::span{&camera,1},
        world.cache->load_bsp_atmosphere(0).cluster_at(eye),mask.words);
    MuzzleEffectView view{};view.width=640;view.height=480;view.position=eye;
    view.forward={1,0,0};view.up={0,0,1};view.orientation={1,0,0,0,1,0,0,0,1};
    auto &memory=world.source_objects->memory();
    const auto word=[&](unsigned p){return Bytes(memory.view(p,4)).u32(0);};
    const auto copy=[&](unsigned p,unsigned n){const auto bytes=memory.view(p,n);return std::vector<std::byte>(bytes.begin(),bytes.end());};
    std::array<Controls,multiplayer_capacity> commands{};unsigned peak{},last{};
    for(unsigned tick=0;tick<240;++tick) {
        const auto observer_history=copy(0x4f655cu+0xc,0x44);
        world.source_objects->render_model_view([&](auto &source) {
            graphics.publish_observer(source,0,eye,{1,0,0},{0,0,1},1.2217304763960306f);
        });
        if(copy(0x4f655cu+0xc,0x44)!=observer_history)
            throw FormatError("Particle lens publication changed the original observer's retained aiming history");
        if(word(0x4f65acu)!=0x3f800000u)
            throw FormatError("Original observer FOV ratio was not published for WORLD particle emission");
        // Original12E142 runs the game ticks before12E151/13AA28. The
        // preceding render union survives until the later2B630 reset.
        world.match.step(commands);
        world.source_objects->render_model_view([&](auto &source){graphics.advance(source,1.f/30.f);});
        world.source_objects->render_model_view([&](auto &source) {
            source.begin_model_frame();
            const auto first=graphics.draw(source,visibility,view,0,pi.projection_angle,angle.lighting_distance_scale);
            const auto rng=word(0x4f3cd8u),pool=word(0x52d714u),data=word(pool+0x44),bytes=word(pool+0x38)*64u;
            const auto saved_rng=copy(rng,8),saved_particles=copy(data,bytes);
            const auto repeat=graphics.draw(source,visibility,view,0,pi.projection_angle,angle.lighting_distance_scale);
            if(first.size()!=repeat.size())throw FormatError("Repeated source view changed draw count");
            for(unsigned n=0;n<first.size();++n)if(first[n].vertex.attributes!=repeat[n].vertex.attributes)
                throw FormatError("Repeated source view changed particle attributes");
            if(copy(rng,8)!=saved_rng || copy(data,bytes)!=saved_particles)
                throw FormatError("Repeated source view consumed RNG or advanced particles");
            if(word(0x4c2aa0u)>2)throw FormatError("Transparent source commands accumulated across views");
            last=unsigned(first.size());peak=std::max(peak,last);
        });
    }
    if(peak!=14 || last!=14 || word(0x4c6854u)!=240)
        throw FormatError("Incomplete original swarm frame/admission continuity");
    std::cout<<"PASS240 actual match-world particle frames,14 final draws, repeated-view particle/RNG invariance and bounded queue reuse\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
