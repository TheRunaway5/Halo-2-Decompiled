#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_object_draw_collector.hpp"
#include "halo2/source_object_admission.hpp"
#include "halo2/source_live_model_draws_tu15.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_effect_light_opaque.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/content.hpp"
#include <algorithm>
#include <set>
#include <map>
#include <bit>
#include <cstring>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char**argv)try {
    if(argc!=4 && argc!=5 && argc!=6)throw FormatError("supply TU1.5 image, Xbox map, original model name fragment, optional --build, optional diagnostic directory");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    Cache cache(argv[2]);
    const auto model=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto&t){
        return t.group=="mode" && t.name.find(argv[3])!=std::string::npos;});
    if(model==cache.tags().end())throw FormatError("requested authored render model absent");
    auto temporary=(std::filesystem::temp_directory_path()/"halo2-model-resource-tu15-XXXXXX").string();
    if(!::mkdtemp(temporary.data()))throw FormatError("native model resource temporary directory failed");
    struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}} cleanup{temporary};
    PhysicsLiveWorldTu15 world(cache,argv[1],temporary);
    const auto word=[&](std::uint32_t p){return Bytes(world.memory().view(p,4)).u32(0);};
    bool rejected=false;try{world.preload_render_model(model->datum);}catch(const FormatError&){rejected=true;}
    if(!rejected)throw FormatError("model resource load accepted an unconfigured session");
    world.configure_session(source_game_variant_default(0),1,30,0x12345678u);
    world.preload_render_model(model->datum);
    const auto mode=word(word(0x4f0414u)+(model->datum&65535u)*16u+8u);
    const auto sections=word(mode+0x28u),count=word(mode+0x24u);
    unsigned checked=0,resources=0;std::size_t bytes=0;
    for(unsigned section=0;section<count;++section) {
        const auto metadata=sections+section*0x5cu;
        if(!Bytes(world.memory().view(metadata+4u,2)).u16(0))continue;
        const auto raw=load_menu_world_source_model_section(cache,model->datum,section);
        const auto resident=word(metadata+0x34u);
        if(!resident || world.memory().view(metadata+0x56u,1)[0]!=std::byte{1})
            throw FormatError("original source request did not publish resident geometry");
        const Bytes header(raw.raw_model_header());
        for(const auto&resource:raw.resources()) {
            if(resource.type!=0 && resource.type!=8 && resource.type!=16 &&
               resource.type!=32 && resource.type!=100)continue;
            if(word(resident+resource.type)!=header.u32(resource.type))
                throw FormatError("published source geometry count differs from original resource bytes");
            const auto published=world.memory().view(word(resident+resource.type+4u),resource.bytes.size());
            if(!std::ranges::equal(published,resource.bytes))
                throw FormatError("published part/index/bound/node-map bytes differ from authored resource");
            ++resources;bytes+=published.size();
        }
        ++checked;
    }
    if(!checked || !resources || !bytes)throw FormatError("no original model resources exercised");
    world.preload_render_model(model->datum);
    rejected=false;try{world.preload_render_model(model->datum^0x10000u);}catch(const FormatError&){rejected=true;}
    if(!rejected)throw FormatError("model resource load accepted a stale full tag handle");
    if(argc>=5) {
        if(std::string_view(argv[4])!="--build")throw FormatError("unknown model-resource diagnostic option");
        const auto biped=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto&t){
            return t.group=="bipd" && cache.load_initial_model_definition(t.datum).render_model==model->datum;});
        if(biped==cache.tags().end())throw FormatError("resource model has no authored biped owner");
        world.initialize_map();
        const auto position=cache.spawns().at(0).position;
        const auto warmup=world.create_biped(biped->datum,position,{1,0,0});
        world.begin_model_frame();
        (void)world.build_world_model(warmup,0,1,false,10.f,1.f);
        // The first real builder loads the location's lightmap resources using
        // native cache-sector I/O. Compare a different freshly born owner with
        // these actually resident resources; no completion or sample is given.
        const auto object=world.create_biped(biped->datum,position,{1,0,0});
        // A declared all-cluster sphere query still traverses the actual
        // creation/registration pools. No spatial links or sphere outputs
        // are installed for the test.
        SourcePortalVisibilitySet visible;
        const auto cluster_count=cache.load_source_bsp_admission(0).clusters.size();
        for(unsigned n=0;n<cluster_count;++n){SourceClusterFrustumRanges r;r.source_cluster=std::int16_t(n);visible.clusters.push_back(r);}
        std::vector<std::uint8_t> fog(cluster_count);
        SourceObjectAdmissionInput admission;admission.mode=admission.context_2a78=1;
        admission.sphere_center=position;admission.sphere_radius=2.f;admission.containing_fog=fog;
        const auto admitted=world.admit_render_objects(visible,admission);
        if(!admitted.object(object) || !admitted.object(warmup))
            throw FormatError("live receiver query lost a genuinely registered biped");
        world.begin_model_frame();
        for(unsigned view=0;view<4;++view) {
            const auto players=view+1;
            const auto metric=std::array{10.f,0.f,30.f,1.f}[view];
            const auto directory=argc==6?std::filesystem::path(argv[5])/std::to_string(view):std::filesystem::path{};
            const auto dump=[&](const char* name) {
                if(argc!=6)return;
                std::filesystem::create_directories(directory);
                std::ofstream out(directory/name,std::ios::binary);
                world.memory().visit_committed([&](auto address,auto bytes) {
                    const auto count=std::uint32_t(bytes.size());
                    out.write(reinterpret_cast<const char*>(&address),4);
                    out.write(reinterpret_cast<const char*>(&count),4);
                    out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
                });
                if(!out)throw FormatError("model builder diagnostic export failed");
            };
            dump("before.bin");
            const auto collector=world.build_world_model(object,view,players,false,metric,1.f);
            dump("after.bin");
            if(argc==6) {
                std::ofstream out(directory/"call.json");
                out<<"{\"owner\":"<<object<<",\"collector\":"<<collector.address
                   <<",\"view\":"<<view<<",\"players\":"<<players
                   <<",\"modifier\":false,\"metric\":"<<metric<<",\"lighting_metric\":1}\n";
                if(!out)throw FormatError("model builder diagnostic call export failed");
            }
            const Bytes produced(collector.bytes);
            if(produced.u32(0)!=object || word(collector.address)!=object || !produced.u16(0x166))
                throw FormatError("live original model builder produced no owned model for the current body");
            const auto context=produced.u32(0x48);
            if(!context || word(context)!=object || word(context+4)!=0xc08e0u)
                throw FormatError("live model context lacks the TU1.5 owner/callback identity");
            std::cout<<"Live model collector: view "<<view<<" / "<<players<<" players / "
                <<produced.u16(0x166)<<" model(s), source LOD "<<produced.u16(0x168)<<'\n';
        }
        rejected=false;try{world.build_world_model(object^0x10000u,0,1,false,10.f,1.f);}catch(const FormatError&){rejected=true;}
        if(!rejected)throw FormatError("model builder accepted a stale full object handle");
        world.begin_model_frame();
        auto receivers=admitted;
        SourceLiveModelDrawContextTu15 view;view.entry.mode=view.draw.draw.mode=1;
        view.entry.eye={position.x-4.f,position.y,position.z};view.forward={1,0,0};
        view.entry.distance_scale=1.f;
        view.entry.coverage_multiplier=source_geometry_lod_policy(1).coverage_multiplier;
        SourceFrustumProjectionInput projection;projection.projection_angle=1.2217304706573486f;
        projection.outer={0,0,480,640};projection.safe=projection.outer;
        view.draw.rendering=source_view_projection({view.entry.eye,view.forward,{0,0,1},.015625f,1024.f,{},
            source_frustum_projection(projection).projection_program});
        view.entry.coverage=source_shader_coverage_camera(view.draw.rendering);
        view.draw.eye=view.entry.eye;view.draw.viewport=projection.outer;
        view.draw.draw.mask_arena_address=0x71001000u;
        SourceDrawUnitState units;
        const auto dump_collection=[&](const char* name) {
            if(argc!=6)return;
            const auto directory=std::filesystem::path(argv[5])/"collect";
            std::filesystem::create_directories(directory);
            std::ofstream out(directory/name,std::ios::binary);
            world.memory().visit_committed([&](auto address,auto bytes) {
                const auto count=std::uint32_t(bytes.size());
                out.write(reinterpret_cast<const char*>(&address),4);out.write(reinterpret_cast<const char*>(&count),4);
                out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
            });
            if(!out)throw FormatError("live collection diagnostic bank export failed");
        };
        nlohmann::json diagnostic;
        if(argc==6) {
            const auto words=[](const auto& value){
                static_assert(sizeof(value)%4==0);std::array<std::uint32_t,sizeof(value)/4> out;
                std::memcpy(out.data(),&value,sizeof(value));return out;
            };
            diagnostic["camera"]=words(view.draw.rendering);diagnostic["eye"]=words(view.entry.eye);
            diagnostic["forward"]=words(view.forward);diagnostic["viewport"]=view.draw.viewport;
            diagnostic["distance_scale"]=std::bit_cast<std::uint32_t>(view.entry.distance_scale);
            diagnostic["coverage_multiplier"]=std::bit_cast<std::uint32_t>(view.entry.coverage_multiplier);
            diagnostic["view"]=view.view;diagnostic["players"]=view.local_players;
            diagnostic["modifier"]=view.view_modifier;diagnostic["mode"]=view.entry.mode;
            diagnostic["children"]=view.children;diagnostic["arena_address"]=view.draw.draw.mask_arena_address;
            for(const auto& r:receivers.objects)diagnostic["queue_before"].push_back({r.id,r.flags,r.other,r.association});
            diagnostic["associations"]=receivers.associations;
        }
        dump_collection("before.bin");
        source_live_model_draws_tu15(world,receivers,units,view);
        dump_collection("after.bin");
        if(argc==6) {
            diagnostic["counts"]={units.main_prefix,units.draw_count,units.override_count,units.section_highwater};
            for(const auto& unit:units.units)diagnostic["units"].push_back(unit.words);
            diagnostic["coverage"]=units.coverage;diagnostic["override_slots"]=units.override_slots;
            diagnostic["overrides"]=units.overrides;
            for(const auto& section:units.sections) {
                std::array<std::uint32_t,9> words;std::memcpy(words.data(),&section,sizeof(section));
                diagnostic["sections"].push_back(words);
            }
            diagnostic["arena_used"]=units.arena.used;diagnostic["arena"]=units.arena.words;
            for(const auto& r:receivers.objects)diagnostic["queue_after"].push_back({r.id,r.flags,r.other,r.association});
            std::ofstream out(std::filesystem::path(argv[5])/"collect"/"call.json");out<<diagnostic.dump(2)<<'\n';
            if(!out)throw FormatError("live collection diagnostic call export failed");
        }
        if(!units.draw_count)throw FormatError("live model rendering produced no actual draw units");
        std::cout<<"Live model receiver collector: "<<receivers.objects.size()<<" owners / "<<units.draw_count<<" draw units\n";
        // Feed the genuinely produced units and cache palettes into the
        // original secondary command owner. No expected commands are inputs.
        SourceBspPartCommandState commands;
        for(unsigned n=0;n<3;++n) {
            commands.banks[n].header[0]=std::array{0xa00u,0x400u,0x200u}[n];
            commands.banks[n].storage.resize(commands.banks[n].header[0]);
        }
        std::map<std::array<std::uint32_t,8>,SourceModelCommandGeometry> live_geometry;
        std::map<std::uint32_t,SourceBspCommandShader> shaders;
        SourceBspPartCommandWorld command_world;
        command_world.model=[&](const auto& unit)->const SourceModelCommandGeometry& {
            auto at=live_geometry.find(unit.words);
            if(at==live_geometry.end())at=live_geometry.emplace(unit.words,
                source_live_model_command_geometry_tu15(world,unit)).first;
            return at->second;
        };
        command_world.shader=[&](auto id)->const SourceBspCommandShader& {
            auto at=shaders.find(id);
            if(at==shaders.end()) {
                SourceBspCommandShader value;value.lod=cache.load_source_shader_lod_definition(id);
                const auto tag=cache.load_source_tag_metadata(id);
                const auto read=[&](auto address,auto size){return Bytes(cache.read_tag_bytes(address,size));};
                const auto stem=cache.load_source_tag_metadata(read(read(tag.source_address(),0x28).u32(0x24),4).u32(0));
                const auto post=read(read(stem.source_address(),0x60).u32(0x5c),8);
                for(unsigned n=0;n<post.u32(0);++n)value.layers.push_back(read(post.u32(4)+n*10,10).u32(2));
                at=shaders.emplace(id,std::move(value)).first;
            }
            return at->second;
        };
        SourceBspPartCommandContext command_context;
        command_context.mode=1;command_context.main_prefix=units.main_prefix;
        command_context.draw_count=units.draw_count;command_context.mask_arena_address=view.draw.draw.mask_arena_address;
        command_context.coverage=units.coverage;command_context.override_slots=units.override_slots;
        command_context.overrides=units.overrides;command_context.player_multiplier=source_shader_lod_player_multiplier(1);
        command_context.lod_view=source_shader_lod_view(0,480,false,false);
        dump_collection("commands-before.bin");
        source_bsp_build_part_commands(commands,units.units,units.arena,command_world,command_context,0,
            [&](auto& state,auto mode){world.append_render_attachments(state,receivers,mode);});
        dump_collection("commands-after.bin");
        if(!commands.banks[1].header[1])throw FormatError("live model light collector produced no diffuse commands");
        if(argc==6) {
            nlohmann::json j;
            j["player_multiplier"]=std::bit_cast<std::uint32_t>(command_context.player_multiplier);
            for(auto value:command_context.lod_view.multipliers)j["lod_view"].push_back(std::bit_cast<std::uint32_t>(value));
            j["calls"]=commands.calls;
            for(const auto& bank:commands.banks) {
                nlohmann::json b;b["header"]=bank.header;
                for(const auto& record:bank.storage)b["storage"].push_back(record.words);
                j["banks"].push_back(b);
            }
            for(const auto& u:commands.special_units)j["special_units"].push_back(u.words);
            j["special_count"]=commands.special_count;
            std::ofstream out(std::filesystem::path(argv[5])/"collect"/"commands.json");out<<j.dump(2)<<'\n';
            if(!out)throw FormatError("live model command export failed");
        }
        std::cout<<"Live model light commands: "<<commands.banks[1].header[1]<<" diffuse / "
            <<commands.banks[2].header[1]<<" secondary, actual attachment traversal\n";
        const auto light_tag=std::ranges::find_if(cache.tags(),[](const auto& tag){
            return tag.group=="ligh" && tag.name=="effects\\objects\\weapons\\grenade\\frag_grenade\\grenade_frag_explosion";
        });
        if(light_tag==cache.tags().end())throw FormatError("live model light test lacks authored frag light");
        const std::array light_ids{light_tag->datum},model_ids{model->datum};
        Mesh mesh;SourceBspEffectLights light_draws(cache,mesh,0,light_ids,model_ids);
        const auto light_definition=load_effect_light(cache,light_tag->datum);
        SourceEffectLightView light_view;SourceEffectLightViewRecord light{};
        light.definition=&light_definition;light.definition_identity=light_tag->datum;
        // Declared isolated light input. Birth/lifetime/reference pixels are
        // separate tests; this exercises fresh live receiver-to-packet wiring.
        light.runtime.identity=0xe96c0000u;light.runtime.owner=0xffffffffu;
        light.runtime.position=light.runtime.spatial_center=position;light.runtime.spatial_radius=2.f;
        light.runtime.forward={1,0,0};light.runtime.up={0,0,1};
        light.runtime.state={1,1,{.4f,.25f,.125f},{},1};light.fades={1,1,1,1,1};
        light_view.records.push_back(light);
        WorldEffectLightReceiver receiver;receiver.identity=light.runtime.identity;receiver.selected_fade=1;
        receiver.view.visibility=visible;
        SourceEffectLightModelViewTu15 objects{world,{},view,fog};
        world.begin_model_frame();
        const auto packets=light_draws.draw(light_view,std::span(&receiver,1),{},view.draw.rendering,{},
            view.entry.eye,view.draw.viewport,{640,480},1,&objects);
        if(packets.empty())throw FormatError("live model shared light frame produced no raw packets");
        std::set<std::uint32_t> packet_owners;unsigned indices=0;
        for(const auto& packet:packets) {
            if(packet.source.owner().kind!=MenuWorldSourceOwnerKind::dynamic_object || !packet.source.owner().generation)
                throw FormatError("live model packet lost its full object owner");
            packet_owners.insert(*packet.source.owner().generation);
            indices+=unsigned(packet.source.draw().indices().size());
        }
        if(packet_owners!=std::set{object,warmup})throw FormatError("live light packets lost one receiver's palette/owner");
        std::cout<<"Live model shared light frame: "<<packets.size()<<" raw packets / "<<indices
            <<" indices / "<<packet_owners.size()<<" actual object palettes\n";
    }
    std::cout<<"PASS "<<model->name<<": "<<checked<<" live sections / "<<resources<<" original resources / "<<bytes
        <<" exact authored bytes; model collection and pixels remain separate gates\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
