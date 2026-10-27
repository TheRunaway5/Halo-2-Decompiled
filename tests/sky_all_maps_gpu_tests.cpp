#include "halo2/renderer.hpp"
#include "halo2/sky.hpp"
#include "halo2/appearance.hpp"
#include "halo2/math.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/source_render_policy.hpp"
#include "halo2/texture_program.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/sky_input_identity.hpp"
#include "halo2/sky_reference_inventory.hpp"
#include "halo2/pstencil/combined_environment.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

// A real-program GPU coverage regression. Native on/off and repeat images are
// controls, not original Xbox framebuffer references. Keep that distinction in
// the machine-readable report, including when every control passes.
namespace {
using namespace halo2;
using json=nlohmann::json;
constexpr unsigned width=320,height=240;
constexpr Vec3 diagnostic_clear{.125f,.25f,.375f};
void require(bool value,const std::string& why){if(!value)throw FormatError(why);}
std::string file_sha256(const std::filesystem::path& path) {
    return sky_reference_detail::file_sha256(path);
}
const json& input_file_identity(json& files,const std::filesystem::path& supplied) {
    const auto path=std::filesystem::weakly_canonical(supplied);
    const auto key=path.string();
    if(!files.contains(key))files[key]={{"bytes",std::filesystem::file_size(path)},
        {"sha256",file_sha256(path)}};
    return files.at(key);
}
struct Image {std::vector<unsigned char> rgb;};
Image image(const std::filesystem::path& path,bool retain) {
    std::ifstream file(path,std::ios::binary);std::string magic;unsigned w{},h{},maximum{};
    file>>magic>>w>>h>>maximum;file.get();
    require(magic=="P6" && w==width && h==height && maximum==255,"sky capture dimensions/format changed");
    Image out;out.rgb.resize(std::size_t(w)*h*3);
    file.read(reinterpret_cast<char*>(out.rgb.data()),out.rgb.size());
    require(bool(file),"truncated sky capture");
    file.close();if(!retain)std::filesystem::remove(path);return out;
}
Image capture(Renderer& renderer,const ModelView& view,const std::filesystem::path& path,bool retain=false) {
    for(unsigned attempt=0;attempt<5;++attempt)
        if(renderer.draw(view.camera,path,{},false,{},{},{},{&view,1}))return image(path,retain);
    throw FormatError("sky GPU frame did not submit");
}
ModelView view(Vec3 eye,Vec3 forward,std::span<const Vertex> vertices,std::int16_t scenario_type) {
    // ModelView's full-surface rectangle uses source logical640x480 units;
    // capture dimensions are independent and Renderer maps this rectangle.
    ModelView out;out.width=640;out.height=480;out.full_surface=true;
    out.background=vertices;out.atmosphere=BspFogSample{};out.effect_time=0;
    // Declared host backdrop for the isolated on/off probe. Backwash's
    // original sky is opaque black (white stage1 times black stage2); a
    // black clear makes its valid draw indistinguishable from omission.
    out.atmosphere->clear=diagnostic_clear;
    // The actual runtime always supplies its completed ordinary1E370 drain.
    // An empty, completed drain is valid and owns no dedicated4C690 sky part.
    out.world_transparent.emplace();
    auto& camera=out.camera;camera.position=eye;camera.oriented=true;camera.forward=forward;
    camera.up=std::abs(forward.z)>.5f?Vec3{0,1,0}:Vec3{0,0,1};
    constexpr float angle=1.2217304764f;
    camera.projection_vertical_angle=angle;camera.field_of_view=70;
    const RenderViewport outer{0,0,float(width),float(height)};
    const RenderViewport safe{width*.075f,height*.075f,width*.85f,height*.85f};
    const auto clip=source_camera_projection(angle,outer,safe,false);
    camera.projection_clip_scale=clip.clip_scale;camera.projection_clip_offset=clip.clip_offset;
    SourceFrustumProjectionInput input;input.projection_angle=angle;
    input.outer={0,0,std::int16_t(height),std::int16_t(width)};
    input.safe={std::int16_t(height*.075f),std::int16_t(width*.075f),
        std::int16_t(height*.925f),std::int16_t(width*.925f)};
    input.scenario_present=true;input.scenario_type=scenario_type;
    const auto bank=source_view_projection({eye,forward,camera.up,.015625f,1024.f,{},
        source_frustum_projection(input).projection_program});
    out.source_view_shader_constants.emplace();
    out.source_view_shader_constants->world=source_view_shader_constants(bank,1024.f,false);
    return out;
}
struct Input {
    std::filesystem::path path,prepared_root;std::string source,authority_map;
    std::filesystem::path shared_directory;
    std::string map_key()const{return authority_map.empty()?path.stem().string():authority_map;}
};
std::vector<Input> inputs(const std::filesystem::path& root,json& inventories) {
    std::vector<Input> result;
    const auto append=[&](const std::filesystem::path& directory,const std::string& source,
                          const std::filesystem::path& prepared) {
        if(!std::filesystem::is_directory(directory))return;
        for(const auto& file:std::filesystem::directory_iterator(directory))
            if(file.is_regular_file() && file.path().extension()==".map")
                result.push_back({file.path(),prepared,source});
    };
    append(root/"assets/disc/maps","original-disc",{});
    append(root/"assets/map-pack/maps","original-map-pack",{});
    if(const char* supplied=std::getenv("HALO2_SKY_MCC_ROOT");supplied && *supplied) {
        const std::filesystem::path prepared(supplied);
        const auto owned=[&](const std::filesystem::path& owner,const std::string& source) {
            std::ifstream stream(owner/"native-shaders/map-owners.json");
            require(bool(stream),"prepared sky input has no declared native map owner index: "+owner.string());
            const auto index=json::parse(stream);require(index.at("schema")=="H2_MCC_NATIVE_MAP_OWNERS_V1","sky map owner index has another schema");
            std::set<std::filesystem::path> declared,actual;
            json item={{"prepared_root",owner.string()},{"source",source},{"missing",json::array()},{"unlisted",json::array()}};
            for(const auto& row:index.at("maps")) {
                const auto relative=std::filesystem::path(row.at("map").get<std::string>());
                require(!relative.empty()&&!relative.is_absolute()&&relative==relative.lexically_normal()&&relative.extension()==".map",
                    "sky map owner path is not a normalized relative map");
                for(const auto& component:relative)require(component!="..","sky map owner escapes its prepared context");
                require(declared.insert(relative).second,"duplicate declared sky map owner");
                const auto shared=std::filesystem::path(row.at("shared_directory").get<std::string>());
                require(!shared.empty()&&!shared.is_absolute()&&shared==shared.lexically_normal(),
                    "sky shared-map owner path is not normalized and relative");
                for(const auto& component:shared)require(component!="..","sky shared-map owner escapes its prepared context");
                result.push_back({owner/relative,owner,source,{},owner/shared});
                if(!std::filesystem::is_regular_file(owner/relative))item["missing"].push_back(relative.generic_string());
            }
            require(!declared.empty(),"prepared sky owner index declares no maps");
            // Each custom context has its own owner index. Its nested maps
            // are validated separately, rather than aliasing the stock bank.
            for(auto iterator=std::filesystem::recursive_directory_iterator(owner);iterator!=std::filesystem::recursive_directory_iterator{};++iterator) {
                const auto relative=iterator->path().lexically_relative(owner);
                if(source=="prepared-mcc"&&iterator->is_directory()&&relative=="custom-maps") {iterator.disable_recursion_pending();continue;}
                if(iterator->is_regular_file()&&iterator->path().extension()==".map")actual.insert(relative);
            }
            for(const auto& path:actual)if(!declared.contains(path))item["unlisted"].push_back(path.generic_string());
            item["declared_maps"]=declared.size();item["actual_maps"]=actual.size();
            item["expected"]=declared.size();item["executed"]=declared.size()-item["missing"].size();
            item["complete"]=item["missing"].empty()&&item["unlisted"].empty();inventories.push_back(std::move(item));
        };
        owned(prepared,"prepared-mcc");
        for(const auto& reference:original_sky_reference_inputs(root,prepared)) {
            bool found=false;
            for(auto& input:result)if(!input.source.starts_with("prepared-mcc")&&
                std::filesystem::weakly_canonical(input.path)==std::filesystem::weakly_canonical(reference.path)) {
                input.authority_map=reference.authority_map;input.shared_directory=root/"assets/disc/maps";found=true;
            }
            if(!found)result.push_back({reference.path,{},"original-preserved",reference.authority_map,root/"assets/disc/maps"});
        }
        if(std::filesystem::is_directory(prepared/"custom-maps")) {
            std::vector<std::filesystem::path> custom;
            for(const auto& entry:std::filesystem::directory_iterator(prepared/"custom-maps"))if(entry.is_directory())custom.push_back(entry.path());
            std::sort(custom.begin(),custom.end());
            for(const auto& owner:custom) {
                if(std::filesystem::is_regular_file(owner/"native-shaders/map-owners.json"))owned(owner,"prepared-mcc-custom");
                else for(const auto& entry:std::filesystem::recursive_directory_iterator(owner))
                    require(!entry.is_regular_file()||entry.path().extension()!=".map","custom sky input contains an unowned map context: "+entry.path().string());
            }
        }
    }
    // Complete original asset cases before their imported equivalents, even
    // when a prepared directory sorts before the original DLC directory.
    std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){
        return std::tuple{a.source.starts_with("prepared-mcc"),a.path}<std::tuple{b.source.starts_with("prepared-mcc"),b.path};});
    std::set<std::filesystem::path> seen;
    std::erase_if(result,[&](const auto& row){return !seen.insert(std::filesystem::weakly_canonical(row.path)).second;});
    return result;
}
std::set<std::string> filter() {
    std::set<std::string> result;
    if(const char* value=std::getenv("HALO2_SKY_MAP_FILTER")) {
        std::istringstream stream(value);std::string word;
        while(std::getline(stream,word,','))if(!word.empty())result.insert(word);
    }
    return result;
}
json material_identity(const Material& material,const Cache& cache) {
    json out={{"name",material.name},{"shader_datum",material.source_shader_datum},
        {"passes",material.passes.size()},{"rendering_gaps",material.rendering_gaps},
        {"forward_pass",material.forward_pass},{"forward_pass_index",material.forward_pass_index},
        {"vertex_program_bytes",material.vertex_program.size()},
        {"texture_program_supported",texture_program_supported(material.forward_program)},
        {"complete_semantic_sha256",sky_audit::semantic_digest(material,cache)}};
    const auto layer=source_material_layer(material);
    out["source_layer"]=layer?json(*layer):json(nullptr);
    out["texture_byte_fnv1a64"]=json::array();
    for(const auto* texture:{&material.diffuse,&material.detail,&material.bump,&material.auxiliary})
        out["texture_byte_fnv1a64"].push_back(texture->pixels.fingerprint());
    return out;
}
void admit_sky_program(const Material& material) {
    require(material.forward_pass,"sky material has no admitted forward program: "+material.name);
    require(material.forward_pass_index<material.passes.size(),"sky forward pass index is outside its decoded source passes: "+material.name);
    const auto& pass=material.passes[material.forward_pass_index];
    require(!material.vertex_program.empty() && !pass.vertex_code.empty(),
        "sky material has no decoded source vertex program: "+material.name);
    require(decode_vertex_program(material.vertex_program).count>0,
        "sky source vertex program is empty: "+material.name);
    require(texture_program_supported(material.forward_program),
        "sky forward pixel program uses an unsupported texture operation: "+material.name);
    const auto layer=source_material_layer(material);
    require(layer && *layer==15,"sky forward program is not the authored dedicated layer15: "+material.name);
    // This generic adapter note describes the explicitly excluded fog/full
    // schedule scope. Missing selected-pass textures are actual binding gaps.
    for(const auto& gap:material.rendering_gaps)
        require(!gap.starts_with("forward texture:") &&
            !gap.starts_with("forward pass requires an external texture:"),
            "sky forward program has an unresolved texture binding: "+material.name+": "+gap);
}
std::vector<std::uint32_t> geometry_words(std::span<const Vertex> vertices) {
    std::vector<std::uint32_t> result;
    result.reserve(vertices.size()*48);
    const auto word=[&](std::uint32_t value){result.push_back(value);};
    for(const auto& vertex:vertices) {
        for(float value:{vertex.position.x,vertex.position.y,vertex.position.z,
                vertex.color.x,vertex.color.y,vertex.color.z,
                vertex.normal.x,vertex.normal.y,vertex.normal.z,
                vertex.tangent.x,vertex.tangent.y,vertex.tangent.z,
                vertex.binormal.x,vertex.binormal.y,vertex.binormal.z,vertex.uv.x,vertex.uv.y,
                vertex.lightmap_uv.x,vertex.lightmap_uv.y,
                vertex.incident.x,vertex.incident.y,vertex.incident.z,
                vertex.prt_cluster,vertex.prt_weights[0],vertex.prt_weights[1],vertex.prt_weights[2],vertex.prt_weights[3],
                vertex.source_local_position.x,vertex.source_local_position.y,vertex.source_local_position.z,
                vertex.source_color_alpha})
            word(std::bit_cast<std::uint32_t>(value));
        word(vertex.material);word(vertex.lightmap);word(std::bit_cast<std::uint32_t>(vertex.bsp_cluster));
        word(vertex.source_section);word(vertex.source_part);
        word(vertex.source_stipple_level);word(vertex.source_part_type);word(vertex.source_part_flags);
    }
    return result;
}
std::uint64_t geometry_identity(std::span<const Vertex> vertices) {
    std::uint64_t result=14695981039346656037ull;
    for(const auto value:geometry_words(vertices))for(unsigned i=0;i<4;++i)
        result=(result^((value>>(i*8))&255u))*1099511628211ull;
    return result;
}
json image_difference(const Image& actual,const Image& expected) {
    require(actual.rgb.size()==expected.rgb.size(),"original/native-asset comparison dimensions differ");
    std::uint64_t absolute_sum{},pixels{};unsigned maximum{};
    for(unsigned pixel=0;pixel<width*height;++pixel) {
        bool changed{};for(unsigned channel=0;channel<3;++channel) {
            const auto index=pixel*3+channel;
            const auto difference=unsigned(std::abs(int(actual.rgb[index])-int(expected.rgb[index])));
            maximum=std::max(maximum,difference);absolute_sum+=difference;changed|=difference!=0;
        }
        pixels+=changed;
    }
    return {{"changed_pixels",pixels},{"maximum_channel_error",maximum},
        {"mean_absolute_channel_error",double(absolute_sum)/actual.rgb.size()},
        {"byte_exact",absolute_sum==0}};
}
using SkyKey=std::tuple<std::string,unsigned,int>;
struct OriginalSky {
    std::filesystem::path path;
    Vec3 eye{};
    std::vector<std::uint32_t> geometry;
    json material_inputs;
    json definition;
    std::array<Image,6> queued,independent;
    std::array<bool,6> available{};
    bool definition_known{},drawable{};
};
using ComparisonKey=std::tuple<std::string,unsigned,int,std::string>;
using PixelKey=std::tuple<std::string,unsigned,int,std::string,unsigned>;
json identity(const SkyKey& key) {return {{"map",std::get<0>(key)},{"bsp",std::get<1>(key)},{"sky",std::get<2>(key)}};}
json identity(const ComparisonKey& key) {auto out=identity(SkyKey{std::get<0>(key),std::get<1>(key),std::get<2>(key)});out["original_asset"]=std::get<3>(key);return out;}
json identity(const PixelKey& key) {auto out=identity(ComparisonKey{std::get<0>(key),std::get<1>(key),std::get<2>(key),std::get<3>(key)});out["direction"]=std::get<4>(key);return out;}
ComparisonKey comparison_key(const SkyKey& key,const OriginalSky& source) {return {std::get<0>(key),std::get<1>(key),std::get<2>(key),source.path.string()};}
PixelKey pixel_key(const SkyKey& key,const OriginalSky& source,unsigned direction) {return {std::get<0>(key),std::get<1>(key),std::get<2>(key),source.path.string(),direction};}
template<class Key>json coverage(const std::set<Key>& expected,const std::set<Key>& executed) {
    json out={{"expected",expected.size()},{"executed",executed.size()},{"missing",json::array()},{"unexpected",json::array()}};
    for(const auto& key:expected)if(!executed.contains(key))out["missing"].push_back(identity(key));
    for(const auto& key:executed)if(!expected.contains(key))out["unexpected"].push_back(identity(key));
    out["complete"]=out["missing"].empty()&&out["unexpected"].empty();return out;
}
struct Window {
    SDL_Window* value{};
    Window(){value=SDL_CreateWindow("All authored sky programs",0,0,width,height,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);require(value,SDL_GetError());}
    ~Window(){if(value)SDL_DestroyWindow(value);}
};
struct Diagnostics {
    std::ostringstream output;std::streambuf* previous=std::cerr.rdbuf(output.rdbuf());
    ~Diagnostics(){std::cerr.rdbuf(previous);if(!output.str().empty())std::cerr<<output.str();}
};
json rejected_sky_fallbacks(const Mesh& original,std::span<const Vertex> vertices,
    const BspAtmosphere& atmosphere,Vec3 eye,std::int16_t scenario_type,const std::filesystem::path& output) {
    std::set<unsigned> used;for(const auto& v:vertices) {
        require(v.source_section!=0xffffffffu&&v.source_part!=0xffffffffu,"native sky negative fixture lost its original section/part owner");used.insert(v.material);
    }
    json probes=json::array();
    for(unsigned mutation=0;mutation<3;++mutation) {
        auto mesh=original;mesh.vertices.clear();
        for(unsigned index:used) {
            auto& material=mesh.materials.at(index);
            if(mutation==0)material.vertex_program.clear();
            else if(mutation==1) {
                material.forward_pass=false;material.forward_vertex=0;material.forward_blend=0;material.world_pass=false;material.forward_program={};
            } else material=Material{};
        }
        const auto label=std::array{"missing-vertex-program","disabled-forward-program-with-source-owner","default-material-with-source-geometry"}[mutation];
        // Missing VS makes this source tuple ineligible for the world vertex
        // catalog. Its unregistered fixed-family fragment is rejected while
        // building the pipeline, before the dedicated sky submission guard.
        const auto expected=std::array{
            "No exact compiled native fragment kernel for family world-fixed",
            "Sky material has no exact native Vulkan kernel",
            "Sky material has no exact native Vulkan kernel"}[mutation];
        json probe={{"mutation",label},{"expected_exception",expected},{"rejected",false}};
        Window window;Diagnostics diagnostics;
        try {
            Renderer renderer(window.value,mesh,true);renderer.scene_bsp_atmosphere(atmosphere);
            const auto present=view(eye,{1,0,0},vertices,scenario_type);
            (void)capture(renderer,present,output/(std::string("negative-")+label+".ppm"));
            probe["error"]="authored sky was accepted through a generic or missing pipeline";
        }catch(const std::exception& error) {
            probe["observed_exception"]=error.what();probe["rejected"]=std::string_view(error.what())==expected;
        }
        probe["diagnostics"]=diagnostics.output.str();probes.push_back(std::move(probe));
    }
    return probes;
}
std::vector<const Material*> window_material_upload_sources(const Mesh& mesh) {
    // Match make_textures' explicit source-bank expansion. These BSP/sky
    // fixtures own no dynamically manufactured visor companion. Reject a
    // changed fixture instead of attributing an unknown expanded upload to a
    // root's different pass. Fixed/procedural roles need no world-catalog ID.
    std::vector<const Material*> result;
    for(const auto& material:mesh.materials)result.push_back(&material);
    for(const auto& material:mesh.materials)if(material.environment_pass)
        result.push_back(material.environment_pass.get());
    for(const auto& material:mesh.materials) {
        for(const auto& pass:material.world_overlays_before)result.push_back(pass.get());
        for(const auto& pass:material.world_overlays_after)result.push_back(pass.get());
        for(const auto& pass:material.world_bloom_passes)result.push_back(pass.get());
        require(!pstencil::combined_visor_environment(material),
            "Elongation window fixture unexpectedly requires a generated visor owner");
    }
    for(const auto& material:mesh.materials)if(material.planar_fog_pass)
        result.push_back(material.planar_fog_pass.get());
    for(const auto& material:mesh.materials)for(const auto& pass:material.world_planar_passes)
        if(pass)result.push_back(pass.get());
    return result;
}
json elongation_windows(const Cache& cache,const std::filesystem::path& output) {
    // These cameras are inside original leaf389/cluster28 and leaf380/cluster27.
    // The target panes are original instance8 and11, at y=+/-8.4. They are
    // decoded authored geometry coordinates, not screenshot-fitted regions.
    auto mesh=cache.load_bsp(0,true);
    // Renderer discovers the resident BSP material/lightmap tuples from its
    // constructor mesh. Keep those declarations while full_surface selects
    // the per-control ModelView spans for actual drawing.
    const auto world=mesh.vertices;
    const auto atmosphere=cache.load_bsp_atmosphere(0);
    const auto definition=cache.load_sky(0);require(bool(definition),"Elongation's authored space sky is absent");
    auto model=cache.load_model(definition->model,true);prepare_forward_materials(cache,model.mesh);
    std::set<unsigned> sky_materials;
    for(const auto& vertex:model.mesh.vertices)sky_materials.insert(vertex.material);
    for(unsigned material:sky_materials)admit_sky_program(model.mesh.materials.at(material));
    const auto material_base=unsigned(mesh.materials.size());
    for(auto& material:model.mesh.materials)mesh.materials.push_back(std::move(material));
    std::vector<Vertex> no_glass;std::set<unsigned> glass_materials;
    for(const auto& vertex:world) {
        // Explicit original/prepared fixture identities, authenticated from
        // their BSP material references. This is a diagnostic omission only.
        const auto& name=mesh.materials.at(vertex.material).name;
        if(name==R"(scenarios\shaders\human\military\glass\glass_interior)" ||
           (cache.is_mcc() && name==R"(scenarios\shaders\human\military\glass\glass_inte_dlc)"))
            glass_materials.insert(vertex.material);
        else no_glass.push_back(vertex);
    }
    require(!glass_materials.empty() && no_glass.size()<world.size(),"Elongation authored glass control has no geometry");
    const auto upload_materials=window_material_upload_sources(mesh);
    Window window;Diagnostics diagnostics;json result=json::array();
    {
        Renderer renderer(window.value,mesh,true);renderer.scene_bsp_atmosphere(atmosphere);
        for(const auto eye:std::array{Vec3{14,7,1.2f},Vec3{-14,-7,1.2f}}) {
            const Vec3 forward{0,eye.y>0?1.f:-1.f,0};
            const auto prefix=std::string(cache.is_mcc()?"mcc-window-":"original-window-")+
                (eye.y>0?"positive-":"negative-");
            auto background=pose_sky(model,*definition,eye);
            for(auto& vertex:background)vertex.material+=material_base;
            auto present=view(eye,forward,background,1);present.vertices=world;
            // Ordinary Slayer's BSP/glass path is the unqueued baseline. The
            // independent empty-drain control below exercises the other owner.
            present.world_transparent.reset();
            present.atmosphere=atmosphere.sample(eye,forward);present.atmosphere_source=&atmosphere;
            present.source_render_policy=source_geometry_lod_policy(1);
            require(present.atmosphere->visible_sky==0,"Elongation window camera selected another sky");
            json upload_observations=json::array();
            const auto observed_capture=[&](ModelView control,const std::string& scene,
                const std::filesystem::path& path,bool retain,bool require_positive) {
                unsigned uploads{},native_world{},fixed_native{},instruction_words{};
                std::set<unsigned> roots,slots,glass_roots,sky_roots;
                control.completed_material_upload_observer=[&](const RendererCompletedMaterialUpload& upload) {
                    require(upload.root_material_count==mesh.materials.size()&&
                        upload.expanded_material_count==upload_materials.size()&&
                        upload.material_slot<upload_materials.size(),
                        "Elongation upload escaped its complete source material bank");
                    const auto& source=*upload_materials.at(upload.material_slot);
                    require(upload.source_shader==source.source_shader_datum&&
                        upload.selected_shader_lod==source.selected_shader_lod,
                        "Elongation completed upload changed its typed shader/LOD owner");
                    // This is the exact authored-world catalog predicate in
                    // Renderer::make_textures. HUD, light-volume and fixed
                    // procedural roles use their separate native families.
                    const bool expects_world=(source.forward_pass||source.static_lightmap_pass)&&
                        !source.hud_pass&&!source.light_volume_pass&&!source.vertex_program.empty();
                    if(expects_world)require(upload.native_world_kernel.has_value(),
                        "Elongation authored world upload has no native AOT kernel");
                    require(upload.uploaded_vertex_instruction_words==0,
                        "Elongation window uploaded executable vertex instructions");
                    ++uploads;native_world+=upload.native_world_kernel.has_value();
                    fixed_native+=!upload.native_world_kernel.has_value();
                    instruction_words+=upload.uploaded_vertex_instruction_words;
                    roots.insert(unsigned(upload.root_slot));slots.insert(unsigned(upload.material_slot));
                    if(glass_materials.contains(unsigned(upload.root_slot)))glass_roots.insert(unsigned(upload.root_slot));
                    if(upload.root_slot>=material_base)sky_roots.insert(unsigned(upload.root_slot));
                };
                auto captured=capture(renderer,control,path,retain);
                if(require_positive)require(uploads>0,"Elongation "+scene+" completed no material uploads");
                if(scene=="baseline")require(!glass_roots.empty()&&!sky_roots.empty(),
                    "Elongation baseline did not observe actual glass and sky material uploads");
                upload_observations.push_back({{"scene",scene},{"completed_uploads",uploads},
                    {"native_world_catalog_uploads",native_world},{"fixed_native_family_uploads",fixed_native},
                    {"uploaded_vertex_instruction_words",instruction_words},{"root_slots",roots},
                    {"expanded_slots",slots},{"glass_root_slots",glass_roots},{"sky_root_slots",sky_roots},
                    {"positive_observation",uploads>0}});
                return captured;
            };
            // These four bounded full-world diagnostic inputs remain inside
            // the enclosing run_test owner's scratch for visual inspection.
            // Its lifecycle removes them; ordinary sky sweep frames are erased
            // immediately after readback and are never retained in the repo.
            const auto a=observed_capture(present,"baseline",output/(prefix+"baseline.ppm"),true,true);
            auto absent=present;absent.background={};
            const auto b=observed_capture(absent,"no-sky",output/(prefix+"no-sky.ppm"),true,false);
            auto glass_removed=present;glass_removed.vertices=no_glass;
            const auto c=observed_capture(glass_removed,"no-glass",output/(prefix+"no-glass.ppm"),true,true);
            auto bare=present;bare.vertices={};
            const auto d=observed_capture(bare,"sky-only",output/(prefix+"sky-only.ppm"),true,true);
            auto queued=present;queued.world_transparent.emplace();
            const auto q=observed_capture(queued,"empty-queue",output/(prefix+"empty-queue.ppm"),true,true);
            queued.background={};
            const auto q0=observed_capture(queued,"empty-queue-no-sky",output/(prefix+"empty-queue-no-sky.ppm"),true,false);
            glass_removed.background={};
            const auto e=observed_capture(glass_removed,"no-glass-no-sky",output/"window-no-glass-no-sky.ppm",false,false);
            const auto repeat=observed_capture(present,"repeat",output/"window-repeat.ppm",false,true);
            require(a.rgb==repeat.rgb,"Elongation window changed after independent geometry/sky controls");
            const auto changed=[](const Image& first,const Image& second) {
                unsigned n{};for(unsigned pixel=0;pixel<width*height;++pixel)
                    n+=!std::equal(first.rgb.begin()+pixel*3,first.rgb.begin()+pixel*3+3,second.rgb.begin()+pixel*3);
                return n;
            };
            const auto center=[](const Image& input) {
                const auto index=(height/2*width+width/2)*3;
                return std::array{input.rgb[index],input.rgb[index+1],input.rgb[index+2]};
            };
            const auto sky_with_glass=changed(a,b),sky_empty_queue=changed(q,q0),
                sky_without_glass=changed(c,e),glass=changed(a,c);
            // Repetition alone also passes an unchanged opaque pane. Each
            // authored window camera must reveal sky with its real glass,
            // with an independently completed queue, and with glass omitted.
            // The glass omission must itself affect the rendered scene.
            require(sky_with_glass>0,prefix+"sky is absent behind authored window glass");
            require(sky_empty_queue>0,prefix+"sky is absent behind window glass with an empty transparent queue");
            require(sky_without_glass>0,prefix+"sky is absent after omitting window glass");
            require(glass>0,prefix+"authored window glass has no rendered effect");
            result.push_back({{"eye",{eye.x,eye.y,eye.z}},{"forward",{forward.x,forward.y,forward.z}},
                {"device",renderer.device_name()},{"completed_material_uploads",upload_observations},
                {"uploaded_vertex_instruction_words",0},
                {"camera_cluster",present.atmosphere->cluster},{"sky_ordinal",present.atmosphere->visible_sky},
                {"world_vertices",world.size()},{"glass_removed_vertices",world.size()-no_glass.size()},
                {"glass_materials",glass_materials},{"sky_changes_with_glass",sky_with_glass},
                {"baseline_has_transparent_queue",false},{"empty_queue_sky_changes",sky_empty_queue},
                {"sky_changes_without_glass",sky_without_glass},{"glass_changes",glass},
                {"baseline_center",center(a)},{"no_sky_center",center(b)},
                {"no_glass_center",center(c)},{"sky_only_center",center(d)},
                {"empty_queue_center",center(q)},{"empty_queue_no_sky_center",center(q0)},
                {"diagnostic_captures",{(output/(prefix+"baseline.ppm")).string(),
                    (output/(prefix+"no-sky.ppm")).string(),(output/(prefix+"no-glass.ppm")).string(),
                    (output/(prefix+"sky-only.ppm")).string(),(output/(prefix+"empty-queue.ppm")).string(),
                    (output/(prefix+"empty-queue-no-sky.ppm")).string()}},
                {"repeat_exact",true},{"original_framebuffer_parity",false}});
        }
    }
    require(diagnostics.output.str().empty(),"Elongation window GPU diagnostics: "+diagnostics.output.str().substr(0,2048));
    return result;
}
}
int main(int argc,char** argv)try {
    require(argc==2,"sky_all_maps_gpu_tests requires a scratch output directory");
    const std::filesystem::path output(argv[1]);std::filesystem::create_directories(output);
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR);
    const auto selected=filter();
    json report={{"schema","HALO2_ALL_MAP_SKY_GPU_V1"},
        {"scope","Actual authored sky programs, texture bindings, six-direction on/off and deterministic repeat GPU controls; isolated background only"},
        {"diagnostic_clear_rgb",{diagnostic_clear.x,diagnostic_clear.y,diagnostic_clear.z}},
        {"original_framebuffer_parity",false},{"physical_xbox_precision",false},
        {"filtered",!selected.empty()},{"map_filter",selected},{"maps",json::array()},
        {"prepared_map_inventories",json::array()},{"native_kernel_negative_probes",json::array()},
        {"input_files",json::object()},{"devices",json::array()},
        {"original_reference_priority","map-pack overrides disc for the same map/BSP/sky key; all original controls still run"},
        {"superseded_original_references",json::array()},
        {"prepared_mcc_requested",std::getenv("HALO2_SKY_MCC_ROOT")!=nullptr}};
    report["test_executable_sha256"]=file_sha256(std::filesystem::absolute(argv[0]));
    report["harness_source_sha256"]=file_sha256(root/"tests/sky_all_maps_gpu_tests.cpp");
    std::vector<Input> all_inputs;
    try {all_inputs=inputs(root,report["prepared_map_inventories"]);}
    catch(const std::exception& error) {
        report["passed"]=false;report["inventory_error"]=error.what();
        std::ofstream(output/"report.json")<<report.dump(2)<<'\n';
        std::ofstream(output/"summary.json")<<report.dump(2)<<'\n';throw;
    }
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    unsigned cases{},errors{},sky_maps{},pairs{},different_pairs{},input_pairs{},different_inputs{};
    std::map<SkyKey,OriginalSky> originals;
    std::set<ComparisonKey> compared_cases,compared_inputs,compared_absences;
    std::set<PixelKey> compared_pixels;
    std::set<SkyKey> unpaired_cases,unexpected_cases;
    std::set<std::string> imported_stems,original_stems;
    for(const auto& input:all_inputs)if(input.source=="prepared-mcc")imported_stems.insert(input.map_key());
        else if(!input.source.starts_with("prepared-mcc"))original_stems.insert(input.map_key());
    bool negative_probes_ran{};
    const bool compare_import=report["prepared_mcc_requested"].get<bool>();
    for(const auto& input:all_inputs) {
        if(!selected.empty() && !selected.contains(input.path.stem().string()) &&
            !selected.contains(input.path.filename().string())&&!selected.contains(input.map_key()))continue;
        json map={{"path",input.path.string()},{"source",input.source},{"authority_map",input.map_key()},{"prepared_context",input.prepared_root.string()},{"bsps",json::array()}};
        try {
            map["input_sha256"]=input_file_identity(report["input_files"],input.path).at("sha256");
            map["input_bytes"]=std::filesystem::file_size(input.path);
            const auto requested_shared=input.shared_directory.empty()?input.path.parent_path():input.shared_directory;
            map["requested_shared_directory"]=requested_shared.string();
            map["existing_resource_inputs"]=json::array();
            for(const auto* name:{"shared.map","single_player_shared.map","textures.dat"})
                if(const auto path=requested_shared/name;std::filesystem::is_regular_file(path)) {
                    (void)input_file_identity(report["input_files"],path);
                    map["existing_resource_inputs"].push_back(std::filesystem::weakly_canonical(path).string());
                }
            Cache cache(input.path,input.shared_directory,{},input.prepared_root);
            if(input.source=="prepared-mcc")for(const auto& reference:all_inputs)
                if(!reference.authority_map.empty()&&!reference.source.starts_with("prepared-mcc")&&reference.map_key()==input.map_key()) {
                    Cache original(reference.path,reference.shared_directory);
                    validate_sky_reference_pair(original,cache,{reference.path,reference.authority_map});
                }
            if(cache.scenario_tag().is_null() || cache.bsps().empty()) {
                map["status"]="no-scenario-bsp";report["maps"].push_back(std::move(map));continue;
            }
            const auto scenario_type=cache.load_source_geometry_scenario_type();
            map["scenario_type"]=scenario_type;map["mcc"]=cache.is_mcc();
            if(input.path.stem()=="elongation")map["window_controls"]=elongation_windows(cache,output);
            bool map_has_sky{};
            for(unsigned bsp=0;bsp<cache.bsps().size();++bsp) {
                json entry={{"bsp",bsp},{"skies",json::array()}};
                try {
                    const auto atmosphere=cache.load_bsp_atmosphere(bsp);
                    std::map<int,unsigned> ordinals;
                    for(unsigned cluster=0;cluster<atmosphere.clusters.size();++cluster) {
                        auto sky=atmosphere.clusters[cluster].visible_sky;
                        if(sky<0 && (atmosphere.scenario_flags&2u) && !atmosphere.skies.empty())sky=0;
                        if(sky>=0)ordinals.try_emplace(sky,cluster);
                    }
                    entry["cluster_count"]=atmosphere.clusters.size();
                    if(ordinals.empty()){entry["status"]="no-visible-sky";map["bsps"].push_back(std::move(entry));continue;}
                    Vec3 eye{};bool spawn_eye{};
                    for(const auto& spawn:cache.spawns())if(spawn.bsp==bsp){eye=spawn.position;spawn_eye=true;break;}
                    if(!spawn_eye) {
                        const auto geometry=cache.load_bsp(bsp,false);
                        if(!geometry.vertices.empty())eye=(geometry.minimum+geometry.maximum)*.5f;
                    }
                    entry["diagnostic_eye"]={eye.x,eye.y,eye.z};
                    entry["eye_source"]=spawn_eye?"authored-player-start":"authored-BSP-geometry-bounds-center";
                    for(const auto& [ordinal,cluster]:ordinals) {
                        json sky={{"ordinal",ordinal},{"selector_cluster",cluster},{"directions",json::array()}};
                        try {
                            const SkyKey key{input.map_key(),bsp,ordinal};
                            OriginalSky* reference=nullptr;
                            if(compare_import&&!cache.is_mcc()) {
                                auto& saved=originals[key];
                                if(!saved.path.empty())report["superseded_original_references"].push_back({{"case",identity(key)},{"superseded",saved.path.string()},{"selected",input.path.string()}});
                                saved=OriginalSky{};saved.path=input.path;saved.eye=eye;reference=&saved;
                            } else if(compare_import&&input.source=="prepared-mcc") {
                                if(const auto found=originals.find(key);found!=originals.end())reference=&found->second;
                                if(!reference&&original_stems.contains(input.map_key())) {
                                    unexpected_cases.insert(key);throw FormatError("imported visible sky has no original map/BSP/ordinal counterpart");
                                }
                            }
                            if(cache.is_mcc()&&!reference) {
                                const SkyKey unpaired{input.source=="prepared-mcc-custom"?
                                    "custom:"+input.prepared_root.filename().string()+":"+input.path.stem().string():input.path.stem().string(),bsp,ordinal};
                                unpaired_cases.insert(unpaired);
                                sky["comparison_scope"]=input.source=="prepared-mcc-custom"?"custom authored input; no original map counterpart":"PC map; no original map counterpart";
                            }
                            const auto definition=cache.load_sky(unsigned(ordinal));
                            if(reference&&!cache.is_mcc()){reference->definition_known=true;reference->drawable=bool(definition);}
                            if(reference&&cache.is_mcc()) {
                                require(reference->definition_known,"original visible sky definition was not successfully read");
                                require(reference->drawable==bool(definition),"original/imported visible sky NONE/renderable kind differs");
                            }
                            if(!definition) {
                                sky["status"]="authored-NONE-or-model-less";
                                if(reference&&cache.is_mcc()) {
                                    compared_cases.insert(comparison_key(key,*reference));compared_absences.insert(comparison_key(key,*reference));
                                    sky["original_asset_absence_exact"]=true;sky["original_asset_reference"]=reference->path.string();
                                }
                                entry["skies"].push_back(std::move(sky));continue;
                            }
                            auto selected_eye=eye;
                            if(reference&&cache.is_mcc())selected_eye=reference->eye;
                            map_has_sky=true;auto model=cache.load_model(definition->model,true);
                            prepare_forward_materials(cache,model.mesh);
                            const auto vertices=pose_sky(model,*definition,selected_eye);
                            require(!vertices.empty(),"authored visible sky produced no vertices");
                            sky["diagnostic_eye"]={selected_eye.x,selected_eye.y,selected_eye.z};
                            sky["definition"]=definition->name;sky["vertices"]=vertices.size();
                            sky["definition_identity"]=sky_audit::sky(*definition,cache);
                            sky["posed_geometry_fnv1a64"]=geometry_identity(vertices);
                            sky["materials"]=json::array();
                            std::set<unsigned> used_materials;
                            for(const auto& vertex:vertices)used_materials.insert(vertex.material);
                            for(unsigned material=0;material<model.mesh.materials.size();++material) {
                                auto identity=material_identity(model.mesh.materials[material],cache);
                                identity["used_for_geometry"]=used_materials.contains(material);
                                sky["materials"].push_back(std::move(identity));
                            }
                            // Unreferenced NONE/diagnostic slots are retained
                            // for namespace identity, but cannot prove a draw.
                            for(unsigned material:used_materials) {
                                require(material<model.mesh.materials.size(),"sky geometry names an absent source material");
                                admit_sky_program(model.mesh.materials[material]);
                            }
                            sky["used_material_programs_admitted"]=true;
                            if(!negative_probes_ran&&!cache.is_mcc()) {
                                report["native_kernel_negative_probes"]=rejected_sky_fallbacks(model.mesh,vertices,atmosphere,selected_eye,scenario_type,output);
                                report["native_kernel_negative_fixture"]={{"path",input.path.string()},{"bsp",bsp},{"sky",ordinal}};
                                for(const auto& probe:report["native_kernel_negative_probes"])errors+=!probe.at("rejected").get<bool>()||!probe.at("diagnostics").get<std::string>().empty();
                                negative_probes_ran=true;
                            }
                            auto input_identity=sky["materials"];
                            for(auto& material:input_identity)material.erase("shader_datum");
                            if(compare_import && !cache.is_mcc()) {
                                auto& saved=originals[key];saved.path=input.path;saved.eye=selected_eye;saved.material_inputs=input_identity;
                                saved.definition=sky["definition_identity"];
                                saved.geometry=geometry_words(vertices);reference=&saved;
                            } else if(compare_import&&input.source=="prepared-mcc") {
                                if(const auto found=originals.find(key);found!=originals.end())reference=&found->second;
                                sky["original_asset_reference"]=reference?reference->path.string():"unavailable";
                                sky["original_asset_materials_exact"]=reference && reference->material_inputs==input_identity;
                                sky["original_asset_definition_exact"]=reference && reference->definition==sky["definition_identity"];
                                sky["original_asset_posed_geometry_exact"]=reference && reference->geometry==geometry_words(vertices);
                                if(reference) {
                                    compared_cases.insert(comparison_key(key,*reference));compared_inputs.insert(comparison_key(key,*reference));
                                    ++input_pairs;
                                    different_inputs+=sky["original_asset_materials_exact"]==false ||
                                        sky["original_asset_definition_exact"]==false ||
                                        sky["original_asset_posed_geometry_exact"]==false;
                                }
                            }
                            Mesh mesh=std::move(model.mesh);mesh.vertices.clear();
                            Window window;Diagnostics diagnostics;std::uint64_t changed{};
                            bool repeat_exact=true,queue_independent=true;
                            unsigned completed_uploads{};
                            std::set<unsigned> native_materials,used_sky_materials;
                            for(const auto& vertex:vertices)used_sky_materials.insert(vertex.material);
                            {
                                Renderer renderer(window.value,mesh,true);
                                renderer.scene_bsp_atmosphere(atmosphere);
                                sky["device"]=renderer.device_name();
                                if(std::find(report["devices"].begin(),report["devices"].end(),sky["device"])==report["devices"].end())
                                    report["devices"].push_back(sky["device"]);
                                unsigned index{};
                                for(const auto direction:std::array{Vec3{1,0,0},Vec3{-1,0,0},Vec3{0,1,0},Vec3{0,-1,0},Vec3{0,0,1},Vec3{0,0,-1}}) {
                                    auto present=view(selected_eye,direction,vertices,scenario_type);
                                    const auto absent=view(selected_eye,direction,{},scenario_type);
                                    present.completed_material_upload_observer=[&](const RendererCompletedMaterialUpload& upload) {
                                        if(!used_sky_materials.contains(unsigned(upload.root_slot)))return;
                                        require(upload.native_world_kernel.has_value(),"sky upload has no native AOT kernel");
                                        require(upload.uploaded_vertex_instruction_words==0,"native sky uploaded executable instruction words");
                                        ++completed_uploads;
                                        native_materials.insert(unsigned(upload.root_slot));
                                    };
                                    auto a=capture(renderer,present,output/"sky.ppm");
                                    const auto b=capture(renderer,absent,output/"absent.ppm");
                                    auto queue_disabled=present;queue_disabled.world_transparent.reset();
                                    auto independent=capture(renderer,queue_disabled,output/"queue-disabled.ppm");
                                    const auto repeat=capture(renderer,present,output/"repeat.ppm");
                                    std::uint64_t different{};for(unsigned pixel=0;pixel<width*height;++pixel)
                                        different+=!std::equal(a.rgb.begin()+pixel*3,a.rgb.begin()+pixel*3+3,b.rgb.begin()+pixel*3);
                                    const bool repeated=a.rgb==repeat.rgb,queue_equal=a.rgb==independent.rgb;
                                    repeat_exact&=repeated;queue_independent&=queue_equal;
                                    changed+=different;++cases;
                                    json ray={{"index",index},{"forward",{direction.x,direction.y,direction.z}},
                                        {"changed_pixels",different},{"repeat_exact",repeated},
                                        {"dedicated_sky_independent_of_transparent_queue",queue_equal},
                                        {"queue_toggle",image_difference(a,independent)}};
                                    if(reference && cache.is_mcc() && reference->available[index]) {
                                        ray["original_assets_native_renderer_queued"]=image_difference(a,reference->queued[index]);
                                        ray["original_assets_native_renderer_independent"]=image_difference(independent,reference->independent[index]);
                                        ++pairs;different_pairs+=ray["original_assets_native_renderer_independent"]["byte_exact"]==false;
                                        compared_pixels.insert(pixel_key(key,*reference,index));
                                    } else if(reference && !cache.is_mcc()) {
                                        reference->queued[index]=std::move(a);reference->independent[index]=std::move(independent);
                                        reference->available[index]=true;
                                    }
                                    sky["directions"].push_back(std::move(ray));++index;
                                }
                            }
                            require(diagnostics.output.str().empty(),"sky emitted Vulkan validation diagnostics: "+diagnostics.output.str().substr(0,2048));
                            require(native_materials==used_sky_materials,"sky native-kernel upload coverage was incomplete");
                            require(completed_uploads>0,"sky controls completed no observed native material uploads");
                            sky["native_aot_materials"]=native_materials.size();
                            sky["completed_material_uploads"]=completed_uploads;
                            sky["uploaded_vertex_instruction_words"]=0;
                            sky["repeat_exact"]=repeat_exact;sky["dedicated_sky_independent_of_transparent_queue"]=queue_independent;
                            if(changed==0 || !repeat_exact || !queue_independent) {
                                sky["status"]="failed";sky["error"]=!queue_independent?
                                    "completed ordinary transparent drain suppressed independent dedicated sky":
                                    !repeat_exact?"sky changed after no-sky view with identical time/input bank":
                                    "authored sky was invisible in every axis direction";++errors;
                            }else sky["status"]="native-controls-passed";
                            std::cout<<input.source<<' '<<input.path.filename().string()<<" bsp="<<bsp<<" sky="<<ordinal<<" vertices="<<vertices.size()<<" changed="<<changed<<'\n';
                        }catch(const std::exception& error){sky["status"]="failed";sky["error"]=error.what();++errors;}
                        entry["skies"].push_back(std::move(sky));
                    }
                }catch(const std::exception& error){entry["status"]="failed";entry["error"]=error.what();++errors;}
                map["bsps"].push_back(std::move(entry));
                // Publish bounded progress before a later independent map can fail.
                report["cases"]=cases;report["errors"]=errors;
                std::ofstream(output/"report.json")<<report.dump(2)<<'\n';
            }
            sky_maps+=map_has_sky;map["status"]="enumerated";
        }catch(const std::exception& error){map["status"]="failed";map["error"]=error.what();++errors;}
        report["maps"].push_back(std::move(map));
        report["cases"]=cases;report["errors"]=errors;
        std::ofstream(output/"report.json")<<report.dump(2)<<'\n';
    }
    SDL_Quit();report["sky_maps"]=sky_maps;report["native_controls_passed"]=errors==0 && cases>0;
    std::set<ComparisonKey> expected_cases,expected_inputs,expected_absences;
    std::set<PixelKey> expected_pixels;report["original_cases_without_prepared_map"]=json::array();
    for(const auto& [key,reference]:originals) {
        if(!imported_stems.contains(std::get<0>(key))) {report["original_cases_without_prepared_map"].push_back(identity(key));continue;}
        expected_cases.insert(comparison_key(key,reference));
        if(reference.definition_known&&reference.drawable) {
            expected_inputs.insert(comparison_key(key,reference));
            for(unsigned direction=0;direction<6;++direction)expected_pixels.insert(pixel_key(key,reference,direction));
        } else if(reference.definition_known)expected_absences.insert(comparison_key(key,reference));
    }
    report["counterpart_case_coverage"]=coverage(expected_cases,compared_cases);
    report["input_comparison_coverage"]=coverage(expected_inputs,compared_inputs);
    report["pixel_comparison_coverage"]=coverage(expected_pixels,compared_pixels);
    report["authored_absence_comparison_coverage"]=coverage(expected_absences,compared_absences);
    report["unpaired_authored_cases"]=json::array();for(const auto& key:unpaired_cases)report["unpaired_authored_cases"].push_back(identity(key));
    report["unexpected_imported_visible_cases"]=json::array();for(const auto& key:unexpected_cases)report["unexpected_imported_visible_cases"].push_back(identity(key));
    bool inventories_complete=!compare_import||!report["prepared_map_inventories"].empty();
    for(const auto& item:report["prepared_map_inventories"])inventories_complete&=item.at("complete").get<bool>();
    report["prepared_map_inventory_complete"]=inventories_complete;
    const bool comparisons_complete=report["counterpart_case_coverage"]["complete"].get<bool>()&&
        report["input_comparison_coverage"]["complete"].get<bool>()&&report["pixel_comparison_coverage"]["complete"].get<bool>()&&
        report["authored_absence_comparison_coverage"]["complete"].get<bool>()&&unexpected_cases.empty();
    report["native_kernel_negative_probes_executed"]=negative_probes_ran;
    const bool negative_passed=negative_probes_ran&&report["native_kernel_negative_probes"].size()==3&&
        std::all_of(report["native_kernel_negative_probes"].begin(),report["native_kernel_negative_probes"].end(),
            [](const auto& probe){return probe.at("rejected").template get<bool>()&&probe.at("diagnostics").template get<std::string>().empty();});
    report["native_kernel_negative_probes_passed"]=negative_passed;
    report["original_asset_native_pixel_pairs"]=pairs;
    report["original_asset_native_pixel_pairs_with_differences"]=different_pairs;
    report["original_asset_native_input_pairs"]=input_pairs;
    report["original_asset_native_input_pairs_with_differences"]=different_inputs;
    report["original_asset_native_pixels_exact"]=pairs>0&&different_pairs==0&&comparisons_complete;
    report["original_asset_native_inputs_exact"]=input_pairs>0&&different_inputs==0&&comparisons_complete;
    const bool passed=errors==0&&cases>0&&negative_passed&&inventories_complete&&
        (!compare_import||(comparisons_complete&&(!selected.empty()||!expected_inputs.empty())&&different_pairs==0&&different_inputs==0));
    report["passed"]=passed;
    report["pixel_comparison_scope"]="Original assets and prepared MCC through the same native renderer; no original NV2A/Xbox framebuffer reference";
    std::ofstream(output/"report.json")<<report.dump(2)<<'\n';
    // The complete per-ray report stays in owned scratch. Retain this bounded
    // cohort summary through run_tests.py's1MiB individual report allowance.
    auto summary=report;
    for(auto& map:summary["maps"])if(map.contains("bsps"))for(auto& bsp:map["bsps"])
        if(bsp.contains("skies"))for(auto& sky:bsp["skies"]) {
            sky.erase("materials");sky.erase("directions");
        }
    std::ofstream(output/"summary.json")<<summary.dump(2)<<'\n';
    std::cout<<"Sky GPU coverage: "<<sky_maps<<" maps, "<<cases<<" direction cases, "<<errors<<" failures. Original framebuffer parity remains unverified.\n";
    return passed?0:1;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';SDL_Quit();return 1;}
