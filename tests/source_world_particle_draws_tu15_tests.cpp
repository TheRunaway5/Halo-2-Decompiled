#include "halo2/multiplayer_world.hpp"
#include "halo2/source_world_particle_draws_tu15.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <algorithm>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>

using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=4)throw FormatError("Supply TU15_XBE BACKWASH_MAP DISC_MAPS");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];
    options.shared_maps=argv[3];options.local_players=1;options.starting_weapon=5;options.mute=true;
    MultiplayerPreparedWorld world(options);
    {
        // Keep this check before the existing fixture's direct local-player
        // setup. Its later native local index intentionally has no Match FP
        // binding; the fresh, empty coupled world has a valid authority seal.
        SourceMatchAuthority native(world.match,*world.source_objects);
        const auto boundary=native.execution_boundary();
        // Keep expanded weapon resource eligibility in its own passive bank;
        // the original map-only graphics control below retains its own inputs.
        SourceWorldParticleDrawsTu15 graphics(*world.cache,world.content.objects);
        if(!graphics.active())throw FormatError("Actual Backwash swarm resources are absent");
        std::set<std::uint32_t> authored_effects,firing_effects,ready_effects,attached_effects;
        const auto collect=[&](const WeaponDefinition &weapon){
            for(const auto &row:weapon.firing_effects)
                firing_effects.insert(row.effects.begin(),row.effects.end());
            if(weapon.ready_effect!=null_datum)ready_effects.insert(weapon.ready_effect);
        };
        for(const auto &[datum,weapon]:world.content.weapons){
            collect(weapon);
            for(const auto &barrel:weapon.barrel_views){
                if(!barrel)throw FormatError("Authored firing resource census has an absent barrel view");
                collect(*barrel);
            }
            // Read the authored object attachment block without evaluating
            // object functions, colors, event clocks or any native producer.
            const auto metadata=world.cache->load_source_tag_metadata(datum);
            const Bytes root(metadata.bytes_at(metadata.source_address(),0x9c));
            const auto count=root.u32(0x94);
            std::optional<SourceTagMetadata> attachment_owner;
            if(count){
                const auto owner=world.cache->query_tag({.contains=TagAddressRange{
                    root.u32(0x98),std::size_t(count)*0x18},.require_address=true});
                if(!owner)throw FormatError("Authored weapon particle attachment block lacks its resident owner");
                attachment_owner.emplace(world.cache->load_source_tag_metadata(owner->datum));
            }
            const Bytes attachments(count?attachment_owner->bytes_at(root.u32(0x98),std::size_t(count)*0x18):
                std::span<const std::byte>{});
            for(unsigned n=0;n<count;++n){
                const auto at=n*0x18;
                if(attachments.u32(at)==0x65666665u&&attachments.u32(at+4)!=null_datum)
                    attached_effects.insert(attachments.u32(at+4));
            }
        }
        for(const auto *category:{&firing_effects,&ready_effects,&attached_effects})
            authored_effects.insert(category->begin(),category->end());
        unsigned none{},non_effect{},missing{},fp_supported{},world_supported{},boundaries{};
        std::vector<std::string> diagnostics;
        const auto diagnose=[&](std::uint32_t effect,const char *pass,const FormatError &error){
            ++boundaries;
            if(diagnostics.size()<16){
                std::ostringstream text;text<<std::hex<<effect<<std::dec<<' '<<pass<<": "<<error.what();
                diagnostics.push_back(text.str().substr(0,240));
            }
        };
        const auto preload=[&](bool first_pass){
            unsigned admitted_fp{},admitted_world{};
            for(const auto effect:authored_effects){
                if(effect==null_datum){if(first_pass)++none;continue;}
                const auto tag=world.cache->query_tag({.handle=TagHandle{effect},.require_address=true});
                if(!tag){if(first_pass)++missing;continue;}
                if(tag->group!="effe"){if(first_pass)++non_effect;continue;}
                try{admitted_fp+=graphics.preload_first_person(effect);}
                catch(const FormatError &error){diagnose(effect,"FP",error);}
                try{admitted_world+=graphics.preload_world(effect);}
                catch(const FormatError &error){diagnose(effect,"WORLD",error);}
            }
            if(first_pass){fp_supported=admitted_fp;world_supported=admitted_world;}
            else if(admitted_fp!=fp_supported||admitted_world!=world_supported)
                throw FormatError("Repeated passive preload changed supported authored effect admission");
        };
        preload(true);const auto admitted=graphics.resource_state();
        if(native.execution_boundary()!=boundary)
            throw FormatError("Passive authored particle preload changed the actual native execution boundary");
        if(!fp_supported||!world_supported||!admitted.first_person_systems||!admitted.world_systems||!admitted.materials)
            throw FormatError("Authored weapon census established no supported FP/WORLD particle resources");
        preload(false);const auto repeated=graphics.resource_state();
        if(admitted.materials!=repeated.materials||admitted.world_systems!=repeated.world_systems||
            admitted.first_person_systems!=repeated.first_person_systems)
            throw FormatError("Repeated passive preload duplicated actual particle resources or systems");
        if(native.execution_boundary()!=boundary)
            throw FormatError("Repeated passive authored preload changed the actual native execution boundary");
        std::cout<<"PASS passive authored weapon particle resources: effects="<<authored_effects.size()
            <<" firing_requested="<<firing_effects.size()<<" ready_requested="<<ready_effects.size()
            <<" attached_requested="<<attached_effects.size()
            <<" fp_supported="<<fp_supported<<" world_supported="<<world_supported
            <<" materials="<<admitted.materials<<" world_systems="<<admitted.world_systems
            <<" fp_systems="<<admitted.first_person_systems<<" none="<<none<<" non_effect="<<non_effect
            <<" missing="<<missing<<" unsupported_boundary_calls="<<boundaries
            <<"; repeated admission and exact native boundary unchanged\n";
        for(const auto &diagnostic:diagnostics)std::cout<<"RESOURCE BOUNDARY "<<diagnostic<<'\n';
    }
    const auto player=world.match.join("Source swarm frame test");
    const auto current=world.match.state();world.source_objects->set_local_player(current.players[player].source_player,0);
    SourceWorldParticleDrawsTu15 graphics(*world.cache,world.content.objects);
    if(!graphics.active())throw FormatError("Actual Backwash swarm resources are absent");
    SourceWorldParticleDrawsTu15 empty_graphics(*world.cache,std::span<const ScenarioObject>{});
    const auto empty_resources=empty_graphics.resource_state();
    if(empty_graphics.active()||empty_resources.materials||empty_resources.world_systems||empty_resources.first_person_systems)
        throw FormatError("Missing-resource control unexpectedly contains authored particle admission");
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
    std::array<Controls,multiplayer_capacity> commands{};unsigned peak{},last{},resident_definitions{};
    bool residency_checked{};
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
        if(last&&!residency_checked){
            // The preceding real source draw requires an occupied native
            // system and particle chain. Reuse the production full-salt pool
            // census; do not manufacture an effect, system or pool layout.
            // This fixture's direct local-player setup intentionally has no
            // coupled Match FP binding, so preserve it and compare actual
            // native mutation stamps, simulation tick and complete memory.
            // Private Match call counters are not exposed for this fixture.
            const auto native_boundary=world.source_objects->execution_mutations();
            const auto match_tick=world.match.simulation().ticks();
            const auto saved_memory=memory.checkpoint();
            const auto rng=word(0x4f3cd8u),pool=word(0x52d714u),data=word(pool+0x44),bytes=word(pool+0x38)*64u;
            const auto saved_rng=copy(rng,8),saved_particles=copy(data,bytes);
            const auto admitted=graphics.resource_state();
            const auto same_resources=[](const auto &a,const auto &b){
                return a.materials==b.materials&&a.world_systems==b.world_systems&&
                    a.first_person_systems==b.first_person_systems;
            };
            const auto unchanged=[&]{
                return world.source_objects->execution_mutations()==native_boundary&&
                    world.match.simulation().ticks()==match_tick&&
                    memory.checkpoint()==saved_memory&&copy(rng,8)==saved_rng&&copy(data,bytes)==saved_particles&&
                    same_resources(graphics.resource_state(),admitted)&&
                    same_resources(empty_graphics.resource_state(),empty_resources);
            };
            const auto definitions=graphics.require_live_world_resident(memory);
            if(definitions.empty()||!unchanged())
                throw FormatError("Occupied native particle residency admission was empty or changed native/resources state");
            bool rejected{};
            try{(void)empty_graphics.require_live_world_resident(memory);}catch(const FormatError &){rejected=true;}
            if(!rejected||!unchanged())
                throw FormatError("Missing active native particle resources were admitted or rejection changed native/resources state");
            resident_definitions=unsigned(definitions.size());residency_checked=true;
        }
    }
    if(peak!=14 || last!=14 || word(0x4c6854u)!=240)
        throw FormatError("Incomplete original swarm frame/admission continuity");
    if(!residency_checked||!resident_definitions)
        throw FormatError("Live native particle residency control never observed an occupied source draw");
    std::cout<<"PASS occupied native particle residency: "<<resident_definitions
        <<" actual effect definitions, admitted bank success, empty bank rejection, unchanged complete native memory/RNG/pools, execution boundary and resource counts\n";
    std::cout<<"PASS240 actual match-world particle frames,14 final draws, repeated-view particle/RNG invariance and bounded queue reuse\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
