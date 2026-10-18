#include "halo2/source_campaign_model.hpp"
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
#include "halo2/source_transparent_parts.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <iostream>
#include <set>
#include <unistd.h>
using namespace halo2;
namespace {
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
    std::map<std::uint32_t,std::unique_ptr<SourceCampaignModel>> resources;
    for(const auto &weapon:weapons) {
        auto model=std::make_unique<SourceCampaignModel>(cache,weapon.definition);
        model->prepare_materials(cache,resident);resources.emplace(weapon.object,std::move(model));
    }
    if(word(rng)!=before)throw FormatError("Armed campaign resource preparation consumed the source RNG");
    nlohmann::json report{{"authority","OG Xbox TU1.5"},{"squad",selected},{"actor",actor->object},
        {"definition",actor->definition},{"attached_weapons",weapons.size()},{"collectors",nlohmann::json::array()}};
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
    if(std::string(argv[3])=="cairo-armed") {
        if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
        const auto original=OriginalData::load_xbe(argv[1]);
        if(original->image!=OriginalData::Image::TitleUpdate15)throw FormatError("Armed actor requires OG Xbox TU1.5");
        Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),original);
        return cairo_armed(cache,argv[1]);
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
