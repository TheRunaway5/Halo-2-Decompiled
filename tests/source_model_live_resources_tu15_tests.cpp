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
    auto variant=source_game_variant_default(0);
    const bool scenario_vehicles=argc>=5 && std::string_view(argv[4])=="--preload-scenario-vehicles";
    if(scenario_vehicles && argc==6)source_game_variant_set(variant,78,std::stoi(argv[5]));
    world.configure_session(variant,1,30,0x12345678u);
    if(scenario_vehicles) {
        world.initialize_map();
        const auto content=cache.load_content(0,true,variant);std::set<std::uint32_t> models;
        for(const auto &object:content.objects)if(object.type==1 && object.definition!=null_datum)
            models.insert(cache.load_initial_model_definition(object.definition).render_model);
        for(const auto datum:models) {
            std::cout<<"Source scenario vehicle preload: "<<datum<<'\n'<<std::flush;
            try{world.preload_render_model(datum);}catch(const FormatError&) {
                const auto pool=word(word(0x4f2d6cu)+0x64u);
                std::cout<<"Actual model cache: capacity="<<word(pool+0x20u)
                    <<" slot_extent="<<word(pool+0x38u)<<'\n'<<std::flush;
                throw;
            }
        }
        std::cout<<"PASS actual scenario vehicle model resource set: "<<models.size()<<" models\n";
        return 0;
    }
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
        SourceTagMetadataStore shader_store(std::shared_ptr<const Cache>(&cache,[](const Cache*){}),{"shad","stem","spas","vrtx"});
        std::map<std::pair<std::uint32_t,unsigned>,SourceBspRangeGeometry> selected_geometry;
        std::array<SourceOpaqueSortRecord,1024> selected{};
        const auto selected_count=source_effect_light_opaque_select(commands.banks[1],units.units,shader_store.providers(),
            [&](unsigned index) {
                const auto& unit=units.units.at(index);const auto key=std::pair{unit.words[1],unit.words[3]>>9&511u};
                auto at=selected_geometry.find(key);
                if(at==selected_geometry.end())at=selected_geometry.emplace(key,load_source_model_range_geometry(cache,key.first,key.second)).first;
                return SourceOpaqueGeometrySelection{load_source_opaque_model_geometry_metadata(cache,key.first,key.second),at->second.parts};
            },selected);
        if(!selected_count)throw FormatError("live model light commands lost all selected shader programs");
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
            j["selected_count"]=selected_count;
            for(const auto& record:selected)j["selected"].push_back(record.words);
            for(unsigned n=0;n<units.draw_count;++n) {
                const auto& unit=units.units[n];nlohmann::json rows=nlohmann::json::array();
                for(const auto& matrix:source_live_model_palette_rows_tu15(world,unit,command_world.model(unit))) {
                    std::array<std::uint32_t,12> bits;std::memcpy(bits.data(),matrix.data(),48);rows.push_back(bits);
                }
                j["palettes"].push_back(rows);
            }
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
        SourceObjectAdmissionInput main_input;main_input.containing_fog=fog;
        // The earlier sphere query deliberately has no camera groups. Main
        // admission needs a real163DB0/441B0 frustum, even for this declared
        // all-cluster diagnostic (which does not claim portal traversal).
        auto main_visible=visible;
        const auto camera=source_frustum_camera({view.entry.eye,view.forward,{0,0,1},
            .015625f,1024.f,source_frustum_projection(projection).clipping_rectangle()});
        main_visible.groups.push_back(camera);main_visible.frustums.push_back(camera.full_frustum);
        for(auto& cluster:main_visible.clusters)cluster.counts[0]=1;
        SourceLiveModelBaseSelectionTu15 base_selection;
        std::vector<SourceLiveModelBaseDrawTu15> base_draws;
        unsigned main_copies=0;
        auto main_context=view;
        main_context.built=[&](const auto& collector){base_selection.remember(collector);};
        SourceEffectLightModelViewTu15 objects{world,world.admit_render_objects(main_visible,main_input),main_context,fog};
        objects.main_draws=[&](const auto& state){
            ++main_copies;base_draws=base_selection.copy(world,state);
        };
        world.begin_model_frame();
        world.begin_model_view();
        const auto packets=light_draws.draw(light_view,std::span(&receiver,1),{},view.draw.rendering,{},
            view.entry.eye,view.draw.viewport,{640,480},1,&objects);
        if(packets.empty())throw FormatError("live model shared light frame produced no raw packets");
        if(main_copies!=1 || base_draws.empty())throw FormatError("live model frame lost its completed main-view handoff");
        std::set<std::uint32_t> packet_owners;unsigned indices=0;
        for(const auto& packet:packets) {
            if(packet.source.owner().kind!=MenuWorldSourceOwnerKind::dynamic_object || !packet.source.owner().generation)
                throw FormatError("live model packet lost its full object owner");
            packet_owners.insert(*packet.source.owner().generation);
            const auto base=std::find_if(base_draws.begin(),base_draws.end(),[&](const auto& draw){
                return draw.object==*packet.source.owner().generation && draw.unit.words[1]==packet.source.draw().geometry->owner_datum() &&
                    (draw.unit.words[3]>>9&511u)==packet.source.draw().geometry->section();
            });
            if(base==base_draws.end() || base->palette_rows!=packet.source.draw().object_rows)
                throw FormatError("model illumination differs from the completed main-view geometry/palette");
            indices+=unsigned(packet.source.draw().indices().size());
        }
        if(packet_owners!=std::set{object,warmup})throw FormatError("live light packets lost one receiver's palette/owner");
        std::cout<<"Live model shared light frame: "<<packets.size()<<" raw packets / "<<indices
            <<" indices / "<<packet_owners.size()<<" actual object palettes\n";
        // A frame with no receivers still completes the same base handoff.
        // It cannot rely on an explosion to select ordinary model geometry.
        const auto lit_base=base_draws;
        for(const auto& draw:lit_base) {
            if(!source_live_model_base_matches_tu15(draw,world,draw.unit))
                throw FormatError("completed main unit no longer owns its actual original palette");
            auto wrong=draw;wrong.object^=0x10000u;
            if(source_live_model_base_matches_tu15(wrong,world,draw.unit))
                throw FormatError("base/light handoff admitted a different full owner generation");
            wrong=draw;
            if(wrong.palette_rows.empty())throw FormatError("base/light negative control has no actual c50 row");
            wrong.palette_rows[0][0]=std::bit_cast<float>(std::bit_cast<std::uint32_t>(wrong.palette_rows[0][0])^1u);
            if(source_live_model_base_matches_tu15(wrong,world,draw.unit))
                throw FormatError("base/light handoff accepted a one-bit pose difference behind matching tokens");
        }
        // Every byte is exactly idempotent through the existing renderer's
        // 131ED0/131BB0 boundary; absent source channels retain prior values.
        for(unsigned byte=0;byte<256;++byte)for(unsigned count=0;count<=4;++count) {
            SourceLiveModelBaseDrawTu15 draw;draw.color_count=std::uint8_t(count);
            draw.packed_colors.fill((byte<<16)|((255-byte)<<8)|((byte*73)&255u));
            const ObjectColors prior{{{.1f,.2f,.3f},{.2f,.3f,.4f},{.3f,.4f,.5f},{.4f,.5f,.6f}}};
            const auto actual=source_live_model_base_colors_tu15(draw,prior);
            const auto repeated=source_object_render_colors(actual);
            for(unsigned n=0;n<4;++n) {
                if(n<count) {
                    if(pack_object_lighting_rgb(actual[n])!=draw.packed_colors[n] ||
                        std::memcmp(&actual[n],&repeated[n],sizeof(Vec3)))
                        throw FormatError("completed live source color changed at the renderer boundary");
                } else if(std::memcmp(&actual[n],&prior[n],sizeof(Vec3)))
                    throw FormatError("absent live source color overwrote retained caller state");
            }
        }
        const auto unlit=light_draws.draw({}, {}, {}, view.draw.rendering,{},
            view.entry.eye,view.draw.viewport,{640,480},1,&objects);
        if(!unlit.empty() || main_copies!=2 || base_draws.size()!=lit_base.size())
            throw FormatError("unlit source frame skipped or changed its ordinary model handoff");
        for(unsigned n=0;n<base_draws.size();++n) {
            const auto& a=base_draws[n];const auto& b=lit_base[n];
            if(a.unit.words!=b.unit.words || a.lod!=b.lod || a.lighting_class!=b.lighting_class ||
                a.coverage!=b.coverage || a.palette_rows!=b.palette_rows ||
                std::memcmp(&a.lighting,&b.lighting,sizeof(a.lighting)))
                throw FormatError("secondary query changed retained ordinary model draw inputs");
        }
        std::cout<<"Live model base/light handoff: "<<base_draws.size()<<" owned main units, identical c50 rows; no-light handoff retained\n";
        objects.model_available=[](const auto&){return false;};
        const auto unavailable=light_draws.draw(light_view,std::span(&receiver,1),{},view.draw.rendering,{},
            view.entry.eye,view.draw.viewport,{640,480},1,&objects);
        if(!unavailable.empty() || base_draws.size()!=lit_base.size())
            throw FormatError("unavailable native model base still submitted an unmatched light pass");
        objects.model_available={};

        // Actual smaller queries retain the original attribution gate. These
        // cold bipeds lack mode+74/+7C attribution and legitimately bypass
        // subpart culling. Additional explicitly declared cull requests test
        // the posed adapter without claiming original live admission.
        struct Sphere {Vec3 offset;float radius;};
        const std::array spheres{Sphere{{0,0,.1f},.04f},Sphere{{0,0,.3f},.05f},
            Sphere{{0,0,.6f},.1f},Sphere{{.8f,0,.4f},.3f},
            Sphere{{0,0,0},2.f},Sphere{{4,0,.4f},.1f},
            Sphere{{0,0,.1f},.04f},Sphere{{0,0,.3f},.05f},Sphere{{0,0,.6f},.1f}};
        unsigned partial_calls=0,marked=0;std::map<unsigned,SourceBspRangeGeometry> ranges;
        for(unsigned case_index=0;case_index<spheres.size();++case_index) {
            const auto& sphere=spheres[case_index];auto input=admission;
            input.sphere_center=position+sphere.offset;input.sphere_radius=sphere.radius;
            auto query=world.admit_render_objects(visible,input);
            SourceDrawUnitState partial;
            source_live_model_draws_tu15(world,query,partial,view);
            std::array<SourceCullDrawBanks,850> banks;
            for(unsigned n=partial.main_prefix;n<partial.draw_count;++n) {
                auto& unit=partial.units[n];
                if(case_index<6 && ((unit.words[0]&1u) || !(unit.words[0]&0x40u)))continue;
                const auto section=(unit.words[3]>>9)&511u;
                auto at=ranges.find(section);
                if(at==ranges.end())at=ranges.emplace(section,load_source_model_range_geometry(cache,model->datum,section)).first;
                if(case_index>=6) {
                    // Declared isolated part-cull input, not an admission
                    // output. Retain genuine model/context/pose/geometry.
                    const auto mask=partial.arena.allocate(at->second.subpart_count);
                    if(!mask)continue;
                    unit.words[0]=(unit.words[0]&~1u)|0x40u;
                    unit.words[5]=view.draw.draw.mask_arena_address+*mask*4u;
                }
                banks[n]=source_live_model_cull_geometry_tu15(world,unit,at->second);
            }
            nlohmann::json cull_case;
            if(argc==6) {
                for(const auto& unit:partial.units)cull_case["before"]["units"].push_back(unit.words);
                cull_case["before"]["arena"]=partial.arena.words;
                cull_case["before"]["arena_used"]=partial.arena.used;
                cull_case["declared_subpart_request"]=case_index>=6;
                cull_case["prefix"]=partial.main_prefix;cull_case["total"]=partial.draw_count;
                cull_case["center"]={std::bit_cast<std::uint32_t>(input.sphere_center.x),
                    std::bit_cast<std::uint32_t>(input.sphere_center.y),std::bit_cast<std::uint32_t>(input.sphere_center.z)};
                cull_case["radius"]=std::bit_cast<std::uint32_t>(sphere.radius);
                cull_case["arena_address"]=view.draw.draw.mask_arena_address;
                const auto path=std::string("cull-")+std::to_string(case_index)+"-inputs.bin";
                dump_collection(path.c_str());
            }
            const auto calls=source_draw_unit_sphere_cull(partial.units,banks,partial.arena,
                view.draw.draw.mask_arena_address,{partial.main_prefix,partial.draw_count,1,1,input.sphere_center,sphere.radius});
            partial_calls+=unsigned(calls.size());
            for(unsigned n=partial.main_prefix;n<partial.draw_count;++n)marked+=bool(partial.units[n].words[0]&1u);
            if(argc==6) {
                for(const auto& unit:partial.units)cull_case["after"]["units"].push_back(unit.words);
                cull_case["after"]["arena"]=partial.arena.words;
                cull_case["after"]["arena_used"]=partial.arena.used;
                cull_case["calls"]=nlohmann::json::array();
                for(const auto& call:calls)cull_case["calls"].push_back({call.unit,
                    std::bit_cast<std::uint32_t>(call.center.x),std::bit_cast<std::uint32_t>(call.center.y),
                    std::bit_cast<std::uint32_t>(call.center.z),std::bit_cast<std::uint32_t>(call.radius)});
                std::ofstream out(std::filesystem::path(argv[5])/"collect"/("cull-"+std::to_string(case_index)+".json"));
                out<<cull_case.dump(2)<<'\n';
                if(!out)throw FormatError("live model cull diagnostic export failed");
            }
            std::cout<<(case_index>=6?"Declared subpart sphere ":"Live partial sphere ")<<case_index<<": "<<query.objects.size()<<" objects / "
                <<partial.draw_count<<" units / "<<calls.size()<<" subpart cull calls\n";
        }
        if(!partial_calls || !marked)throw FormatError("live partial sphere suite never consumed model bounds");
    }
    std::cout<<"PASS "<<model->name<<": "<<checked<<" live sections / "<<resources<<" original resources / "<<bytes
        <<" exact authored bytes; model collection and pixels remain separate gates\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
