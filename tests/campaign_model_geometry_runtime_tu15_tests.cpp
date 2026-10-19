#include "halo2/source_campaign_model.hpp"
#include "halo2/source_campaign_weapons.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/source_draw_resources.hpp"
#include "halo2/source_object_presentation.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_campaign_view_tu15.hpp"
#include "halo2/source_campaign_environment_view_tu15.hpp"
#include "halo2/source_campaign_screen_tu15.hpp"
#include "halo2/menu_world_depth.hpp"
#include "halo2/scenery.hpp"
#include "halo2/bsp_lighting.hpp"
#include "halo2/world_material.hpp"
#include "halo2/source_transparent_parts.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <iostream>
#include <fstream>
#include <set>
#include <unistd.h>
using namespace halo2;
namespace {
int cairo_copy_dispatch(const Cache &cache,const std::filesystem::path &xbe,bool native_cold=false) {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
    if(!scratch)throw FormatError("Live copy dispatch test requires its owned scratch runner");
    const auto storage=std::filesystem::path(scratch)/"copy-world";std::filesystem::create_directories(storage);
    auto runtime=native_cold?std::make_unique<MultiplayerObjectRuntime>(cache,0,storage):
        std::make_unique<MultiplayerObjectRuntime>(xbe,cache,0,storage);
    auto &objects=*runtime;
    auto &world=objects.tu15_world();auto &memory=world.memory();
    world.initialize_map();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto hud=word(0x51df94u),table=word(0x4f0414u);
    bool authored_hud=false;
    for(const auto &tag:cache.select_tags({.require_address=true}))if(tag.group=="hudg")
        authored_hud|=hud!=0 && hud==word(table+(tag.datum&65535u)*16u+8u);
    if(!authored_hud)throw FormatError("Original HUD new-map owner did not publish a resident authored hudg address");
    constexpr std::uint32_t base=0x71000000u;
    std::vector<std::byte> initial(512);
    for(unsigned i=0;i<initial.size();++i)initial[i]=std::byte((i*37u+i/7u)&255u);
    memory.mount(base,initial,SourceRuntimeMemory::Access::ReadWrite,"declared live CRT copy comparison operands");
    unsigned cases{};
    for(unsigned count:{0u,1u,2u,3u,4u,7u,8u,15u,16u,31u,32u,33u,63u,64u,65u,127u})
        for(unsigned source=64;source<68;++source)
            for(unsigned destination:{source,source-1u,source+1u,source-2u,source+2u,source-3u,source+3u,256u,257u,258u,259u}) {
                auto expected=initial;std::memmove(expected.data()+destination,expected.data()+source,count);
                memory.write(base,initial);
                const auto result=world.call_actual(0x32a990u,std::array{base+destination,base+source,count});
                if(result!=base+destination || !std::ranges::equal(memory.view(base,initial.size()),expected))
                    throw FormatError("Live original CRT copy dispatch changed overlap bytes or return identity");
                ++cases;
            }
    nlohmann::json report{{"passed",true},{"cases",cases},{"actual_live_world_entry",0x32a990u},
        {"native_cold_world_constructor",native_cold || xbe.empty()},
        {"runtime_cache_source",cache.is_mcc()?"MCC":"Xbox reference"},
        {"runtime_cache_path",cache.path().string()},{"reference_cache_metadata_used",!cache.is_mcc()},
        {"source_selected_hud_address",hud},{"source_selected_hud_address_is_resident_hudg",true},
        {"complete_process_owner_canonical_entry",0x326e20u},{"comparison","Independent std::memmove semantics over all512 operand bytes and returned destination"},
        {"source_instruction_or_return_substitutions",false},{"full_weapon_parity",false},
        {"boundary","Live shared-entry routing and bounded memory-copy behavior; no new whole-ABI or framebuffer comparison claim."}};
    if(const auto path=std::getenv("HALO2_TEST_CAMPAIGN_COPY_REPORT"))std::ofstream(path)<<report.dump(2)<<'\n';
    std::cout<<report.dump(2)<<'\n';return 0;
}
void verify_scene_weapons(nlohmann::json &report,const Cache &cache,const LevelContent &content,
    MultiplayerObjectRuntime &objects,const SourceCampaignViewFrameTu15 &frame,const SourceModelShaderView &view) {
    Mesh resident;SourceCampaignWeapons weapons(cache,content,resident);
    const auto submitted=weapons.draw(objects,frame,view,0,1.f,0.f);
    if(!submitted.pending_overrides.empty())throw FormatError("Captured campaign weapon scene requires an unresolved override owner");
    std::set<std::uint32_t> definitions;std::set<std::string> names;unsigned vertices{};
    report["weapon_submissions"]=nlohmann::json::array();
    for(const auto &draw:submitted.draws) {
        const auto source=std::ranges::find_if(frame.base_draws,[&](const auto &v){
            return v.object==draw.object && v.definition==draw.definition && v.ordinal==draw.ordinal;
        });
        if(source==frame.base_draws.end())throw FormatError("Campaign scene weapon lost its actual original main unit");
        const auto nodes=objects.rendered_world_nodes(draw.object);
        if(!nodes)throw FormatError("Campaign scene weapon lost its completed WORLD pose");
        const std::array sections{source->unit.words[3]>>9&511u};
        const auto model=cache.load_model_sections(draw.definition,false,0,source->lod,sections);
        const auto authored=cache.load_source_transparent_model_parts(source->unit.words[1],sections);
        std::vector<Vertex> expected;std::vector<std::pair<std::uint32_t,unsigned>> materials;
        for(const auto &vertex:skin_model_world_nodes(model,*nodes)) {
            const auto &parts=authored.sections.at(0).parts;
            const auto part=std::ranges::find_if(parts,[&](const auto &v){return unsigned(v.ordinal)==vertex.source_part;});
            if(part==parts.end())throw FormatError("Independent campaign scene weapon lost its authored shader part");
            const auto choice=source_model_shader_part({0,source->coverage},part->lod_scale>0?part->lod_scale:1.f,
                cache.load_source_shader_lod_definition(part->shader),view).choice;
            if(!choice.accepted || choice.index<0)continue;
            expected.push_back(vertex);materials.emplace_back(part->shader,unsigned(choice.index));
        }
        if(draw.vertices.empty() || draw.vertices.size()!=expected.size())
            throw FormatError("Campaign scene weapon vertex count differs from independent original sections");
        std::set<std::pair<std::uint32_t,unsigned>> selected;
        for(unsigned i=0;i<expected.size();++i) {
            auto vertex=draw.vertices[i];const auto &m=resident.materials.at(vertex.material);
            if(m.source_shader_datum!=materials[i].first || m.selected_shader_lod!=materials[i].second)
                throw FormatError("Campaign scene weapon selected another authored shader or shader LOD");
            selected.insert(materials[i]);vertex.material=expected[i].material;
            if(std::memcmp(&vertex,&expected[i],sizeof(Vertex)))
                throw FormatError("Campaign scene weapon differs from original section geometry and WORLD nodes");
        }
        if(std::memcmp(&draw.lighting,&source->lighting,sizeof(draw.lighting)))
            throw FormatError("Campaign scene weapon changed its original cached lighting bytes");
        constexpr float scale=std::bit_cast<float>(0x3b808081u);
        for(unsigned i=0;i<source->color_count;++i) {
            const auto rgb=source->packed_colors.at(i);
            const Vec3 expected{float(rgb>>16&255u)*scale,float(rgb>>8&255u)*scale,float(rgb&255u)*scale};
            if(std::memcmp(&draw.colors.at(i),&expected,sizeof(expected)))
                throw FormatError("Campaign scene weapon changed its original packed color channel");
        }
        const auto tag=cache.query_tag({.handle=TagHandle{draw.definition}});
        if(!tag || tag->group!="weap")throw FormatError("Campaign scene weapon lost its authored definition");
        const auto parent=Bytes(objects.memory().view(objects.object_body(draw.object)+0x14,4)).u32(0);
        nlohmann::json shaders=nlohmann::json::array();for(const auto &[shader,lod]:selected)shaders.push_back({shader,lod});
        report["weapon_submissions"].push_back({{"object",draw.object},{"definition",draw.definition},{"name",tag->name},
            {"ordinal",draw.ordinal},{"original_parent",parent},{"attached",parent!=null_datum},
            {"vertices",draw.vertices.size()},{"geometry_lod",source->lod},{"coverage",source->coverage},
            {"selected_shaders",shaders},{"source_unit_words",source->unit.words},{"cached_lighting_bytes_equal",true},
            {"original_color_channels_verified",source->color_count}});
        definitions.insert(draw.definition);names.insert(tag->name);vertices+=draw.vertices.size();
    }
    if(!names.contains(R"(objects\weapons\rifle\battle_rifle\battle_rifle)") ||
       !names.contains(R"(objects\weapons\rifle\smg\smg)") ||
       !names.contains(R"(objects\weapons\fixed\h_turret_mp\item\h_turret_mp_item)"))
        throw FormatError("Captured campaign scene did not cover its original BR/SMG/turret-item definitions");
    report["scene_weapon_geometry_verified"]=true;report["scene_weapon_definitions"]=definitions;
    report["scene_weapon_vertices_verified"]=vertices;report["scene_weapon_units_verified"]=submitted.draws.size();
}
int cairo_scenery_lighting(const Cache &cache,const std::filesystem::path &xbe) {
    const auto content=cache.load_content(0,false);
    const auto starts=cache.spawns();
    const auto start=std::ranges::find_if(starts,[](const auto &s){return s.bsp==0;});
    if(start==starts.end())throw FormatError("Cairo source scene has no authored BSP0 start");
    auto source=std::make_shared<CampaignSourceRuntime>(cache,xbe,0,1,0x12345678u);
    Simulation simulation(content,0,*start,{},{},{},{},{},{},source);
    simulation.place_squad(content.squads.at(0).name);
    auto &objects=source->objects();auto &world=objects.tu15_world();
    // Exact first-view camera retained by application-validation-v2. The
    // former flat, model-only diagnostic did not reach the rejected cap.
    const Vec3 eye{60.49140167236328f,-9.179409980773926f,7.682519912719727f};
    const Vec3 forward{.5352398753166199f,-.8401711583137512f,.08735382556915283f};
    const Vec3 up{-.04693465679883957f,.0736737921833992f,.9961773157119751f};
    constexpr float angle=.9769777655601501f;
    objects.publish_native_observer_position(0,eye);
    const auto atmosphere=cache.load_bsp_atmosphere(0);
    const auto configured=source_campaign_clip_planes_tu15(world);
    const auto environment=source_campaign_environment_view_tu15(world,atmosphere,eye,forward,configured.x,configured.y);
    SourceFrustumProjectionInput p;p.projection_angle=angle;p.outer=p.safe={0,0,480,640};
    p.scenario_present=true;p.scenario_type=0;
    const auto projected=source_frustum_projection(p);
    const auto rendering=source_view_projection({eye,forward,up,configured.x,environment.source.far_distance,{},projected.projection_program});
    SourceCampaignViewInputTu15 input;input.rendering=rendering;
    input.clipping=source_frustum_camera({eye,forward,up,configured.x,environment.source.far_distance,projected.clipping_rectangle()});
    input.eye=eye;input.forward=forward;input.viewport={0,0,480,640};
    input.distance_scale=.8949999809265137f;input.coverage_multiplier=1.f;input.view_modifier=projected.view_modifier;
    MuzzleEffectView particles;particles.camera=source_view_shader_bank(rendering,environment.source.far_distance,eye,{0,0,640,480},false);
    particles.fog=bsp_fog_constants(environment.fog,eye,forward);
    const auto &b=rendering.camera_to_world.basis;
    particles.orientation={b[0].x,b[0].y,b[0].z,b[1].x,b[1].y,b[1].z,b[2].x,b[2].y,b[2].z};
    particles.position=eye;particles.forward=forward;particles.up=up;particles.width=640;particles.height=480;
    const auto depth=menu_world_depth_clip_planes({configured.x,configured.y,MenuWorldDepthBoundary::none,0,false});
    particles.clip_range={0,16777215.f,depth.minimum_w,depth.maximum_w};
    input.particles=SourceCampaignParticleViewTu15{particles,angle,1.2217304706573486f};input.environment=&environment.source;
    SourceCampaignViewTu15 view(cache,0);
    SourceCampaignViewFrameTu15 frame;
    objects.render_model_view([&](auto &w){frame=view.collect(w,input,atmosphere,environment.fog);});
    const auto lighting=cache.load_bsp_lighting(0);
    const SourceModelShaderView shader_view{source_shader_coverage_camera(rendering),
        source_shader_lod_view(0,480,false,false),1.f,source_shader_lod_player_multiplier(1),false};
    nlohmann::json report{{"passed",false},{"actual_application_rendering_fixed",false},
        {"source_tick",simulation.ticks()},{"main_units",frame.base_draws.size()},
        {"source_particle_commands",frame.source_particle_commands},{"scenery_units",nlohmann::json::array()},
        {"replayed_application_camera",true},{"full_weapon_parity",false},{"fully_certified_weapons",0}};
    struct ReportWriter {nlohmann::json &r;~ReportWriter(){if(const auto path=std::getenv("HALO2_TEST_CAMPAIGN_SCENERY_REPORT"))std::ofstream(path)<<r.dump(2)<<'\n';}} writer{report};
    unsigned baked_vertices{},dynamic_vertices{},controls{};bool cap_reached=false;
    Mesh resident;std::map<std::uint64_t,std::unique_ptr<SourceCampaignModel>> banks;
    std::map<std::tuple<std::uint32_t,unsigned,bool>,Material> proof_materials;
    for(const auto &object:simulation.scene().objects()) {
        if(object.handle==null_datum || !object.visible || !frame.drawn.contains(object.handle))continue;
        const auto &placement=content.objects.at(object.placement);
        if(placement.type!=6)continue;
        const auto model=cache.load_model(placement.definition,true,placement.variant);
        if(!source_campaign_lit_scenery(model,placement))continue;
        const auto key=(std::uint64_t(placement.variant)<<32)|placement.definition;
        auto &bank=banks[key];
        if(!bank) {
            const auto first=std::ranges::find_if(content.objects,[&](const auto &o){return o.definition==placement.definition && o.variant==placement.variant;});
            bank=std::make_unique<SourceCampaignModel>(cache,*first);bank->prepare_materials(cache,resident,true);
        }
        const auto nodes=objects.rendered_world_nodes(object.handle);
        if(!nodes)throw FormatError("Actual scenery view did not publish its WORLD pose");
        for(const auto &draw:frame.base_draws)if(draw.object==object.handle) {
            report["current_unit"]={{"object",object.handle},{"placement",object.placement},
                {"definition",placement.definition},{"ordinal",draw.ordinal},{"class",draw.lighting_class},
                {"words",draw.unit.words}};
            const auto actual=bank->draw(object.handle,std::span{&draw,1},*nodes,shader_view,&lighting);
            if(actual.empty())throw FormatError("Actual admitted scenery unit produced no selected geometry");
            const bool baked=draw.lighting_class==1;
            const auto instance=draw.unit.words[3]>>18&2047u;
            const std::array sections{draw.unit.words[3]>>9&511u};
            auto independent=cache.load_model_sections(placement.definition,false,placement.variant,draw.lod,sections);
            if(baked) {
                const auto &record=lighting.scenery.at(instance);
                if(record.unique_id!=placement.unique_id || record.type!=placement.type || record.source!=placement.source ||
                    record.checksum!=independent.checksum)throw FormatError("Original scenery unit did not select its own authored lighting identity");
                const auto &bucket=lighting.buckets.at(record.reference.bucket);
                for(unsigned i=0;i<independent.mesh.vertices.size();++i) {
                    const auto &origin=independent.vertex_origins.at(i);
                    const auto at=std::size_t(record.reference.section_offsets.at(origin.section))+origin.vertex;
                    auto &v=independent.mesh.vertices[i];v.incident=decode_bsp_incident(bucket.incident.at(at));
                    const auto &rgb=bucket.radiance.at(at);constexpr float scale=.003921568859368563f;
                    v.color={float(rgb[0])*scale,float(rgb[1])*scale,float(rgb[2])*scale};v.lightmap=0;
                }
            }
            const auto authored=cache.load_source_transparent_model_parts(draw.unit.words[1],sections);
            std::vector<Vertex> expected;std::vector<std::pair<std::uint32_t,unsigned>> materials;
            for(const auto &vertex:skin_model_world_nodes(independent,*nodes)) {
                const auto &parts=authored.sections.at(0).parts;
                const auto part=std::ranges::find_if(parts,[&](const auto &v){return unsigned(v.ordinal)==vertex.source_part;});
                if(part==parts.end())throw FormatError("Independent scenery geometry lost its authored shader part");
                const auto choice=source_model_shader_part({0,draw.coverage},part->lod_scale>0?part->lod_scale:1.f,
                    cache.load_source_shader_lod_definition(part->shader),shader_view).choice;
                if(!choice.accepted || choice.index<0)continue;
                expected.push_back(vertex);materials.emplace_back(part->shader,unsigned(choice.index));
            }
            if(expected.size()!=actual.size())throw FormatError("Scenery consumer differs from independently selected original sections");
            for(unsigned i=0;i<actual.size();++i) {
                auto v=actual[i];const auto &m=resident.materials.at(v.material);
                if(m.source_shader_datum!=materials[i].first || m.selected_shader_lod!=materials[i].second)
                    throw FormatError("Scenery consumer selected another shader or shader LOD");
                auto [proof,fresh]=proof_materials.try_emplace(std::tuple{materials[i].first,materials[i].second,baked});
                if(fresh) {
                    proof->second=cache.load_material(materials[i].first);proof->second.selected_shader_lod=materials[i].second;
                    if(!(baked?prepare_world_material(cache,proof->second,true):prepare_dynamic_material(cache,proof->second,{},true,0.f)))
                        throw FormatError("Independently selected original scenery shader has unresolved inputs");
                }
                // Lighting class selects layer3. Authored independent passes
                // retain implementation0, including emissive-only materials.
                const auto same_pass=[](const Material &a,const Material &b) {
                    return a.forward_pass_index==b.forward_pass_index && a.per_vertex_lighting==b.per_vertex_lighting &&
                        a.vertex_program==b.vertex_program && a.forward_program.words==b.forward_program.words;
                };
                const auto &want=proof->second;
                if(!same_pass(m,want) || bool(m.environment_pass)!=bool(want.environment_pass) ||
                    (m.environment_pass && !same_pass(*m.environment_pass,*want.environment_pass)))
                    throw FormatError("Scenery shader implementation differs from its independently prepared original class: "+m.name);
                v.material=expected[i].material;
                if(std::memcmp(&v,&expected[i],sizeof(Vertex)))throw FormatError("Scenery incident/radiance/WORLD geometry differs from its original instance streams");
            }
            (baked?baked_vertices:dynamic_vertices)+=actual.size();
            if(placement.definition==0xf9d6179eu && placement.unique_id==1227359031u) {
                if(!baked || instance!=146)throw FormatError("Application's rejected dress-cap unit was not reproduced");
                cap_reached=true;
                const auto reject=[&](auto mutate) {
                    auto changed=draw;BspLighting bank_input;bank_input.structure_matches=lighting.structure_matches;
                    bank_input.bitmap_datum=lighting.bitmap_datum;bank_input.scenery=lighting.scenery;bank_input.buckets=lighting.buckets;
                    mutate(changed,bank_input);bool stopped=false;
                    try{(void)bank->draw(object.handle,std::span{&changed,1},*nodes,shader_view,&bank_input);}catch(const FormatError &){stopped=true;}
                    if(!stopped)throw FormatError("Invalid scenery lighting control was admitted");++controls;
                };
                reject([](auto &d,auto &){d.override_slot=0;});
                reject([](auto &d,auto &){d.unit.words[0]&=~1u;});
                reject([](auto &d,auto &){d.lighting_class=3;d.unit.words[0]=(d.unit.words[0]&0x1fffffffu)|(3u<<29);});
                reject([](auto &d,auto &){d.object^=0x10000u;});
                reject([](auto &,auto &l){l.structure_matches=false;});
                reject([](auto &,auto &l){l.scenery.at(146).checksum^=1;});
                reject([](auto &d,auto &){d.unit.words[3]|=2047u<<18;});
                reject([](auto &,auto &l){l.scenery.at(146).reference.bucket=-1;});
                reject([&](auto &,auto &l){l.scenery.at(146).reference.section_offsets.at(sections[0])=0xffff;});
                reject([](auto &,auto &l){l.buckets.at(l.scenery.at(146).reference.bucket).radiance.clear();});
                reject([](auto &,auto &l){l.buckets.at(l.scenery.at(146).reference.bucket).incident.clear();});
                bool stopped=false;try{(void)bank->draw(object.handle,std::span{&draw,1},*nodes,shader_view);}catch(const FormatError &){stopped=true;}
                if(!stopped)throw FormatError("Class1 scenery drew without the current BSP lighting owner");++controls;
            }
            const auto tag=cache.query_tag({.handle=TagHandle{placement.definition}});
            report["scenery_units"].push_back({{"object",object.handle},{"definition",placement.definition},{"name",tag?tag->name:""},
                {"placement",object.placement},{"ordinal",draw.ordinal},{"lighting_class",draw.lighting_class},
                {"override_slot",draw.override_slot},{"override_shader",draw.shader_override},
                {"source_unit_words",draw.unit.words},{"instance",instance},{"verified_vertices",actual.size()}});
        }
    }
    if(!cap_reached || !baked_vertices || !dynamic_vertices || controls!=12)
        throw FormatError("Scenery regression did not cover the rejected cap and both live lighting classes");
    verify_scene_weapons(report,cache,content,objects,frame,shader_view);
    report["passed"]=true;report["rejected_application_cap_reached"]=true;
    report["baked_vertices_verified"]=baked_vertices;report["dynamic_vertices_verified"]=dynamic_vertices;
    report["negative_controls"]=controls;
    report["boundary"]="Exact retained application camera with original full environment/particle main collection; CPU shader/geometry and per-instance lighting comparison. No Vulkan, pixels or Xbox framebuffer claim.";
    std::cout<<report.dump(2)<<'\n';return 0;
}
int cairo_armed(const Cache &cache,const std::filesystem::path &xbe) {
    auto path=(std::filesystem::temp_directory_path()/"halo2-source-armed-geometry-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Campaign armed actor source storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    MultiplayerObjectRuntime objects(xbe,cache,0,path);auto &world=objects.tu15_world();auto &memory=world.memory();
    world.initialize_map();world.configure_campaign_session(1,1,30,0x12345678);
    const auto word=[&](std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);};
    const auto scenario=word(0x4ecc20u),count=word(scenario+0x160u),squads=word(scenario+0x164u);
    const auto atmosphere=cache.load_bsp_atmosphere(0);
    std::uint32_t selected=null_datum;
    for(unsigned i=0;i<count;++i) {
        const auto row=squads+i*0x74u,starts=word(row+0x48u),rows=word(row+0x4cu);
        if(!starts || std::bit_cast<std::int16_t>(Bytes(memory.view(row+0x2cu,2)).u16(0))<=0)continue;
        bool admitted=true;
        for(unsigned n=0;n<starts;++n) {
            const Bytes start(memory.view(rows+n*0x64u,0x64));
            if(start.u16(0x10)!=0xffff || atmosphere.cluster_at({start.f32(4),start.f32(8),start.f32(12)})<0)admitted=false;
        }
        if(admitted){selected=i;break;}
    }
    if(selected==null_datum)throw FormatError("No original ordinary Cairo squad admitted in BSP0");
    std::cout<<"PHASE original Cairo ai_place squad="<<selected<<'\n'<<std::flush;
    world.place_ai(selected,1);
    const auto live=objects.live_objects();
    const auto actor=std::ranges::find_if(live,[&](const auto &o){return o.type==0 && word(objects.source_object_body(o.object)+0x12cu)!=null_datum;});
    if(actor==live.end())throw FormatError("Original Cairo ai_place did not create a real actor unit");
    std::vector<SourceLiveObjectTu15> weapons;
    for(const auto &o:live)if(o.type==2 && o.parent==actor->object)weapons.push_back(o);
    if(weapons.empty())throw FormatError("Original Cairo actor has no actual attached weapon; armed case is vacuous");
    const auto rng=word(0x4f3cd8u),before=word(rng);
    SourceCampaignModel body(cache,actor->definition);Mesh resident;body.prepare_materials(cache,resident);
    SourceCampaignWeapons weapon_scene(cache,cache.load_content(0,false),resident);
    if(word(rng)!=before)throw FormatError("Armed campaign resource preparation consumed the source RNG");
    const auto squad_bytes=memory.view(squads+selected*0x74u,32);
    const auto squad_end=std::ranges::find(squad_bytes,std::byte{});
    if(squad_end==squad_bytes.end())throw FormatError("Authored campaign squad name is unterminated");
    const std::string squad_name(reinterpret_cast<const char*>(squad_bytes.data()),std::size_t(squad_end-squad_bytes.begin()));
    nlohmann::json report{{"authority","OG Xbox TU1.5"},{"squad",selected},{"squad_name",squad_name},
        {"runtime_cache_source",cache.is_mcc()?"MCC":"Xbox reference"},
        {"runtime_cache_path",cache.path().string()},{"native_world_constructor",xbe.empty()},{"actor",actor->object},
        {"definition",actor->definition},{"attached_weapons",weapons.size()},{"collectors",nlohmann::json::array()}};
    struct ReportWriter {
        nlohmann::json &report;
        ~ReportWriter(){if(const auto path=std::getenv("HALO2_TEST_CAMPAIGN_WEAPON_REPORT"))std::ofstream(path)<<report.dump(2)<<'\n';}
    } writer{report};
    const auto hex=[](std::span<const std::byte> bytes) {
        constexpr char digits[]="0123456789abcdef";std::string out;out.reserve(bytes.size()*2);
        for(const auto byte:bytes){const auto b=std::to_integer<unsigned>(byte);out.push_back(digits[b>>4]);out.push_back(digits[b&15u]);}return out;
    };
    // Finite, explicit caller inputs exercise the actual collector/section
    // route; they do not claim to be the game's current camera or visibility.
    const auto root=objects.object_transform(actor->object);
    const auto eye=root.position;
    report["declared_eye"]={eye.x,eye.y,eye.z};
    report["declared_forward"]={root.basis[0].x,root.basis[0].y,root.basis[0].z};
    SourceFrustumProjectionInput projection;projection.projection_angle=1.f;
    projection.outer=projection.safe={0,0,480,640};projection.scenario_present=true;projection.scenario_type=0;
    const auto projected=source_frustum_projection(projection);
    const auto rendering=source_view_projection({eye,root.basis[0],root.basis[2],.015625f,1024.f,{},projected.projection_program});
    const SourceModelShaderView shader_view{source_shader_coverage_camera(rendering),
        source_shader_lod_view(0,480,false,false),1.f,source_shader_lod_player_multiplier(1),false};
    SourceCampaignViewInputTu15 input;input.rendering=rendering;
    input.clipping=source_frustum_camera({eye,root.basis[0],root.basis[2],.015625f,1024.f,projected.clipping_rectangle()});
    input.eye=eye;input.forward=root.basis[0];input.viewport={0,0,480,640};
    input.distance_scale=input.coverage_multiplier=1.f;input.view_modifier=projected.view_modifier;
    SourceCampaignViewTu15 view(cache,0);SourceCampaignViewFrameTu15 frame;
    std::cout<<"PHASE actual campaign view admission/render entry/children/main units\n"<<std::flush;
    objects.render_model_view([&](auto &w){frame=view.collect(w,input,atmosphere,atmosphere.sample(eye,root.basis[0]));});
    if(!frame.drawn.contains(actor->object))throw FormatError("Actual campaign view did not submit its declared armed actor");
    const auto verify_collector=[&](std::uint32_t owner) {
        const auto at=frame.built.find(owner);
        if(at==frame.built.end())throw FormatError("Actual campaign presentation did not separately rebuild its actor/held weapon");
        const Bytes c(at->second.bytes);const auto nodes=objects.rendered_world_nodes(owner);
        if(!nodes)throw FormatError("Original actor/held weapon builder did not publish its rendered WORLD pose");
        if(c.u32(0)!=owner || c.u16(0x166)<1 || c.u16(0x166)>5)
            throw FormatError("Actual campaign presentation collector lost its primary owner/model rows");
        nlohmann::json rows=nlohmann::json::array();
        for(unsigned n=0;n<c.u16(0x166);++n)rows.push_back({{"owner",c.u32(0x34+n*4)},
            {"render_model",c.u32(0x5c+n*4)},{"regions",c.u32(0x18+n*4)}});
        report["collectors"].push_back({{"owner",owner},{"definition",objects.object_definition(owner)},
            {"model_rows",rows},{"geometry_lod",c.u16(0x168)},{"override_flags",c.u32(0x170)&255u},
            {"override_shader",c.u32(0x1d0)},{"coverage_bits",c.u32(0x1b4)},{"nodes",nodes->size()},
            {"collector_hex",hex(at->second.bytes)},{"WORLD_nodes_hex",hex(std::as_bytes(std::span(*nodes)))}});
    };
    verify_collector(actor->object);for(const auto &weapon:weapons)verify_collector(weapon.object);
    std::vector<SourceLiveModelBaseDrawTu15> ordinary;
    report["main_units"]=nlohmann::json::array();
    for(const auto &unit:frame.base_draws) {
        report["main_units"].push_back({{"owner",unit.object},{"ordinal",unit.ordinal},{"class",unit.lighting_class},
            {"override_slot",unit.override_slot},{"override_shader",unit.shader_override},{"coverage",unit.coverage},
            {"section",unit.unit.words[3]>>9&511u},{"geometry_lod",unit.lod},{"words",unit.unit.words},
            {"palette_rows_hex",hex(std::as_bytes(std::span(unit.palette_rows)))}});
        if(unit.object==actor->object && unit.lighting_class==2 && unit.override_slot==255 && (unit.unit.words[0]&1u))ordinary.push_back(unit);
    }
    if(ordinary.empty())throw FormatError("Actual armed actor produced no ordinary class2 main units; positive is vacuous");
    const auto nodes=objects.rendered_world_nodes(actor->object);
    const auto actual=body.draw(actor->object,ordinary,*nodes,shader_view);
    // Independent authored section decoding and source shader-LOD arithmetic:
    // the prepared SourceCampaignModel never supplies the expected vertices,
    // authored material identity, part scale, LOD threshold or shader choice.
    std::vector<Vertex> expected;std::vector<std::pair<std::uint32_t,unsigned>> expected_materials;
    const auto render_model=cache.load_initial_model_definition(actor->definition).render_model;
    for(const auto &unit:ordinary) {
        if(!unit.geometry.geometry.resident)continue;
        const std::array sections{unit.unit.words[3]>>9&511u};
        const auto model=cache.load_model_sections(actor->definition,false,0,unit.lod,sections);
        const auto authored=cache.load_source_transparent_model_parts(render_model,sections);
        for(const auto &vertex:skin_model_world_nodes(model,*nodes)) {
            const auto &parts=authored.sections.at(0).parts;
            const auto part=std::ranges::find_if(parts,[&](const auto &p){return unsigned(p.ordinal)==vertex.source_part;});
            if(part==parts.end())throw FormatError("Independent armed actor section lost its authored part");
            const auto choice=source_model_shader_part({0,unit.coverage},part->lod_scale>0?part->lod_scale:1.f,
                cache.load_source_shader_lod_definition(part->shader),shader_view).choice;
            if(!choice.accepted || choice.index<0)continue;
            expected.push_back(vertex);expected_materials.emplace_back(part->shader,unsigned(choice.index));
        }
    }
    if(actual.empty() || actual.size()!=expected.size())throw FormatError("Actual ordinary main-unit selected vertex count differs from independent authored sections");
    for(unsigned n=0;n<actual.size();++n) {
        auto vertex=actual[n];const auto &material=resident.materials.at(vertex.material);
        if(material.source_shader_datum!=expected_materials[n].first || material.selected_shader_lod!=expected_materials[n].second)
            throw FormatError("Actual armed actor material slot differs from its independent authored shader/selected LOD");
        vertex.material=expected[n].material;
        if(std::memcmp(&vertex,&expected[n],sizeof(Vertex)))throw FormatError("Actual ordinary main-unit geometry differs from authored sections skinned by its completed WORLD pose");
    }
    const auto submitted=weapon_scene.draw(objects,frame,shader_view,0,1.f,0.f);
    std::set<std::uint32_t> submitted_owners;unsigned weapon_vertices{},weapon_inputs{};
    report["weapon_submissions"]=nlohmann::json::array();
    for(const auto &draw:submitted.draws) {
        const auto unit=std::ranges::find_if(frame.base_draws,[&](const auto &u){return u.ordinal==draw.ordinal;});
        if(unit==frame.base_draws.end() || unit->object!=draw.object || unit->definition!=draw.definition)
            throw FormatError("Campaign submitted weapon lost its actual original main ordinal/owner");
        const auto nodes=objects.rendered_world_nodes(draw.object);
        const std::array sections{unit->unit.words[3]>>9&511u};
        const auto authored_model=cache.load_model_sections(draw.definition,false,0,unit->lod,sections);
        const auto render=cache.load_initial_model_definition(draw.definition).render_model;
        const auto parts=cache.load_source_transparent_model_parts(render,sections);
        std::vector<Vertex> expected_weapon;std::vector<std::pair<std::uint32_t,unsigned>> shaders;
        for(const auto &vertex:skin_model_world_nodes(authored_model,*nodes)) {
            const auto part=std::ranges::find_if(parts.sections.at(0).parts,
                [&](const auto &p){return unsigned(p.ordinal)==vertex.source_part;});
            if(part==parts.sections.at(0).parts.end())throw FormatError("Independent weapon section lost its authored part");
            const auto choice=source_model_shader_part({0,unit->coverage},part->lod_scale>0?part->lod_scale:1.f,
                cache.load_source_shader_lod_definition(part->shader),shader_view).choice;
            if(!choice.accepted || choice.index<0)continue;
            expected_weapon.push_back(vertex);shaders.emplace_back(part->shader,unsigned(choice.index));
        }
        if(draw.vertices.empty() || draw.vertices.size()!=expected_weapon.size())
            throw FormatError("Campaign weapon vertices differ from independent original section selection");
        for(unsigned n=0;n<draw.vertices.size();++n) {
            auto vertex=draw.vertices[n];const auto &material=resident.materials.at(vertex.material);
            if(material.source_shader_datum!=shaders[n].first || material.selected_shader_lod!=shaders[n].second)
                throw FormatError("Campaign weapon material identity/LOD differs from original selected section");
            vertex.material=expected_weapon[n].material;
            if(std::memcmp(&vertex,&expected_weapon[n],sizeof(Vertex)))
                throw FormatError("Campaign weapon geometry differs from original section/WORLD input");
        }
        if(std::bit_cast<std::array<std::uint32_t,21>>(draw.lighting.source_values())!=
           std::bit_cast<std::array<std::uint32_t,21>>(unit->lighting.source_values()))
            throw FormatError("Campaign weapon did not retain original cached lighting words");
        const auto definition=cache.load_source_world_weapon_definition(draw.definition);
        const auto b=objects.object_body(draw.object);
        float ammunition=0.f;
        if(!definition.magazines.empty() && definition.magazines[0].maximum_loaded)
            ammunition=std::clamp(float(std::bit_cast<std::int16_t>(Bytes(memory.view(b+0x22c,2)).u16(0)))/
                float(definition.magazines[0].maximum_loaded),0.f,1.f);
        const auto ammunition_id=cache.is_mcc()?0x12000576u:0x1200056au;
        const auto one_id=cache.is_mcc()?0x030005b2u:0x030005a6u;
        const auto zero_id=cache.is_mcc()?0x040005b3u:0x040005a7u;
        if(cache.string_id(ammunition_id)!="primary_ammunition" ||
           cache.string_id(one_id)!="one" || cache.string_id(zero_id)!="zero")
            throw FormatError("Campaign weapon fixture builtin identities differ from the actual cache namespace");
        if(std::bit_cast<std::uint32_t>(draw.input(ammunition_id))!=std::bit_cast<std::uint32_t>(ammunition) ||
           draw.input(one_id)!=1.f || draw.input(zero_id)!=0.f)
            throw FormatError("Campaign weapon shader input did not use actual source magazine/builtin state");
        if(cache.is_mcc())for(const auto foreign:{0x1200056au,0x030005a6u,0x040005a7u}) {
            bool rejected=false;
            try{(void)draw.input(foreign);}catch(const FormatError&){rejected=true;}
            if(!rejected)throw FormatError("MCC campaign material accepted an unbound Xbox builtin identifier");
            report["material_namespace_rejection_controls"]=report.value("material_namespace_rejection_controls",0u)+1;
        }
        weapon_inputs+=3;weapon_vertices+=unsigned(draw.vertices.size());submitted_owners.insert(draw.object);
        report["weapon_submissions"].push_back({{"object",draw.object},{"definition",draw.definition},
            {"ordinal",draw.ordinal},{"vertices",draw.vertices.size()},{"shader_inputs_verified",3},
            {"source_lighting_words_equal",true},{"original_magazine_fraction",ammunition},
            {"authored_builtin_ids",{ammunition_id,one_id,zero_id}}});
    }
    for(const auto &weapon:weapons)if(!submitted_owners.contains(weapon.object))
        throw FormatError("Campaign weapon submission omitted an actual original attached weapon");
    if(!submitted.pending_overrides.empty())throw FormatError("Armed weapon case requires an unresolved override owner");
    if(!weapon_scene.draw(objects,{},shader_view,0,1.f,0.f).draws.empty())
        throw FormatError("Campaign weapon reused a draw without original admission");
    report["source_weapon_submission_verified"]=true;report["source_weapon_vertices"]=weapon_vertices;
    report["source_weapon_shader_inputs"]=weapon_inputs;report["source_weapon_full_owners"]=submitted_owners.size();
    report["source_weapon_no_stale_draw_without_admission"]=true;
    unsigned rejected{};
    const auto reject=[&](const char *name,const auto &mutate) {
        auto changed=ordinary;mutate(changed.front());bool stopped=false;
        try{(void)body.draw(actor->object,changed,*nodes,shader_view);}catch(const FormatError &){stopped=true;}
        if(!stopped)throw FormatError(std::string("Armed ordinary main-unit control admitted ")+name);
        ++rejected;
    };
    reject("foreign full owner",[](auto &v){v.object^=0x10000u;});
    reject("foreign definition",[](auto &v){v.definition^=0x10000u;});
    reject("foreign render-model",[](auto &v){v.unit.words[1]^=0x10000u;});
    reject("lighting class",[](auto &v){v.lighting_class=3;});
    reject("shader override",[](auto &v){v.override_slot=0;});
    reject("partial part range",[](auto &v){v.unit.words[0]&=~1u;});
    reject("out of bank geometry LOD",[](auto &v){v.lod=6;});
    auto unresident=ordinary.front();unresident.geometry.geometry.resident=false;
    if(!body.draw(actor->object,std::span{&unresident,1},*nodes,shader_view).empty())
        throw FormatError("Original no-resident ordinary unit drew cached resource-union geometry");
    report["ordinary_main_unit_vertices"]=actual.size();report["ordinary_main_units"]=ordinary.size();
    report["main_unit_rejection_controls"]=rejected;
    report["no_resident_unit_preserves_no_draw"]=true;
    if(word(rng)!=before)throw FormatError("Armed campaign rendering consumed the source RNG");
    report["passed"]=true;report["source_rng_unchanged"]=true;
    report["source_clock_24_hex"]=hex(memory.view(objects.source_clock(),0x24));
    report["boundary"]="Declared finite camera and view metrics; actual ai_place, campaign visibility/admission, render entry, collectors, child traversal, copied main units and WORLD poses. No original full main query, live shader-function inputs, pixels or matched original frame proof.";
    std::cout<<report.dump(2)<<'\n';return 0;
}
}
int main(int argc,char **argv)try {
    if(argc!=4)throw FormatError("Supply TU15_XBE OPENING_MAP SCENE08_OBJECT_INDEX");
    if(std::string(argv[3])=="cairo-armed" || std::string(argv[3])=="cairo-scenery-lighting" ||
       std::string(argv[3])=="cairo-copy-dispatch" || std::string(argv[3])=="cairo-native-hud") {
        const bool mcc=std::string(argv[1])=="native-mcc";
        if(!std::filesystem::is_regular_file(argv[2]) || (!mcc && !std::filesystem::is_regular_file(argv[1])))return 77;
        const auto original=mcc?std::shared_ptr<const OriginalData>{}:OriginalData::load_xbe(argv[1]);
        if(original && original->image!=OriginalData::Image::TitleUpdate15)throw FormatError("Armed actor requires OG Xbox TU1.5");
        Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),original);
        if(mcc && !cache.is_mcc())throw FormatError("Weapon runtime testing requires an MCC cache; Xbox/Vista fallback is forbidden");
        const auto xbe=mcc?std::filesystem::path{}:std::filesystem::path(argv[1]);
        if(std::string(argv[3])=="cairo-copy-dispatch")return cairo_copy_dispatch(cache,xbe);
        if(std::string(argv[3])=="cairo-native-hud")return cairo_copy_dispatch(cache,xbe,true);
        if(std::string(argv[3])=="cairo-scenery-lighting") {
            if(mcc)throw FormatError("Captured ISO reference camera is not an MCC runtime acceptance case");
            return cairo_scenery_lighting(cache,xbe);
        }
        return cairo_armed(cache,xbe);
    }
    const auto requested=std::stoul(argv[3]);if(requested>=3)throw FormatError("Scene08 object index must be 0..2");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto placements=[&] {
        const auto c=cache.load_content(0,false);std::map<unsigned,ScenarioObject> out;
        for(unsigned name:{0u,1u,2u,22u}) {
            const auto &n=c.object_names.at(name);const auto p=std::ranges::find_if(c.objects,[&](const auto &p){return p.type==n.type && p.placement==n.placement;});
            if(p==c.objects.end())throw FormatError("Authored scene08 placement unavailable");out.emplace(name,*p);
        }
        return out;
    }();
    auto path=(std::filesystem::temp_directory_path()/"halo2-source-model-geometry-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Campaign source model storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);auto &w=objects.tu15_world();auto &m=w.memory();
    w.initialize_map();w.configure_campaign_session(1,1,30,0x12345678);source_campaign_cinematic_control_tu15(w,true);
    source_campaign_camera_control_tu15(w,true);source_campaign_camera_field_of_view_tu15(w,47.f,0);
    source_campaign_camera_animation_tu15(w,0xe9d90864u,0x06001ca3u,null_datum,1);
    const auto create=[&](unsigned name) {
        const auto owner=objects.create_scenario_object(placements.at(name),SourceScenarioBirth::runtime);
        if(owner==null_datum)throw FormatError("Original scene08 birth returned NONE");source_campaign_cinematic_lod_tu15(w,owner,true);return owner;
    };
    const auto anchor=create(22);unsigned random_models{},vertices{};
    const std::array graphs{0xe9ea0875u,0xe9ed0878u,0xe9ec0877u},clips{0x0a001cddu,0x0b001cfbu,0x0c001ce7u};
    const auto word=[&](auto at){return Bytes(m.view(at,4)).u32(0);};
    for(unsigned name=unsigned(requested);name<=requested;++name) {
        std::cout<<"PHASE source scene08 object="<<name<<'\n'<<std::flush;
        const auto owner=create(name);const auto &placement=placements.at(name);
        if(!source_campaign_cinematic_animation_tu15(w,owner,graphs[name],clips[name],anchor,true,false,false))
            throw FormatError("Actual scene08 custom start failed");
        w.step_objects();
        const auto random=word(0x4f3cd8u),before=word(random);
        const auto active=objects.object_model_regions(owner).selection;
        if(!active.resolved())throw FormatError("Actual source object did not publish its region bank");
        const auto preview=cache.load_initial_model_selection(placement.definition,placement.variant);
        random_models+=!preview.resolved();
        std::cout<<"PHASE resource union\n"<<std::flush;SourceCampaignModel geometry(cache,placement);
        if(word(random)!=before)throw FormatError("Campaign resource preparation consumed source RNG");
        SourceCampaignModel actor_geometry(cache,placement.definition,placement.variant);
        if(word(random)!=before)throw FormatError("Dynamic actor resource preparation consumed source RNG");
        w.begin_model_frame();w.begin_model_view();
        // Declared test metrics, rather than a guessed production camera.
        std::cout<<"PHASE original model builder\n"<<std::flush;
        const auto collector=objects.build_world_model(owner,0,1,false,1.f,1.f);
        const Bytes c(collector.bytes);if(c.u16(0x166)!=1 || c.u16(0x168)>=6)
            throw FormatError("Actual scene08 collector did not retain its model/LOD");
        const auto nodes=objects.rendered_world_nodes(owner);
        if(!nodes)throw FormatError("Actual scene08 builder did not publish its rendered pose");
        const auto shown=geometry.vertices(owner,collector,*nodes);
        const auto actor_shown=actor_geometry.vertices(owner,collector,*nodes);
        const auto selected=cache.load_model(placement.definition,false,placement.variant,active,c.u16(0x168));
        const auto expected=skin_model_world_nodes(selected,*nodes);
        if(shown.empty() || shown.size()!=expected.size() ||
            std::memcmp(shown.data(),expected.data(),shown.size()*sizeof(Vertex)))
            throw FormatError("Actual source section order/geometry differs from its resolved live region bank");
        if(actor_shown.size()!=expected.size() ||
            std::memcmp(actor_shown.data(),expected.data(),actor_shown.size()*sizeof(Vertex)))
            throw FormatError("Dynamic actor resources differ from the completed original selection and WORLD pose");
        if(word(random)!=before)throw FormatError("Campaign render readback consumed source RNG");
        vertices+=unsigned(shown.size());std::cout<<"PASS scene08 object="<<name<<" vertices="<<shown.size()<<" unseeded_preview_unresolved="<<!preview.resolved()<<'\n'<<std::flush;
        objects.destroy_object(owner);
    }
    if(requested==1 && !random_models)throw FormatError("Scene08 check did not exercise its unresolved random model");
    std::cout<<"PASS actual source selections/resource union/rendered poses; random_models="<<random_models<<" vertices="<<vertices<<'\n';
    std::cout<<"LIMIT declared camera metrics; no shader pixels, full scene VM or converted MCC acceptance\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
