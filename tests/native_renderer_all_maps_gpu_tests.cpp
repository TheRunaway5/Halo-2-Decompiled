#define SDL_MAIN_HANDLED
#include "helpers/native_renderer_map_inventory.hpp"
#include "helpers/native_renderer_weather_inventory.hpp"
#include "helpers/native_renderer_texture_camera_fixture.hpp"
#include "halo2/renderer.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/source_weather_assets.hpp"
#include "halo2/sky.hpp"
#include "halo2/math.hpp"
#include "halo2/shader_animation.hpp"
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/water.hpp"
#include <openssl/evp.h>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

namespace {
using namespace halo2;
using namespace halo2::renderer_test;
using json=nlohmann::json;
constexpr unsigned width=320,height=240;
struct Digest {
    EVP_MD_CTX* context=EVP_MD_CTX_new();
    Digest(){need(context&&EVP_DigestInit_ex(context,EVP_sha256(),nullptr)==1,"Cannot initialize frame digest");}
    ~Digest(){EVP_MD_CTX_free(context);}
    void add(std::span<const std::byte> bytes){need(EVP_DigestUpdate(context,bytes.data(),bytes.size())==1,"Cannot hash renderer input");}
    std::string finish(){std::array<unsigned char,32> out{};unsigned count{};need(EVP_DigestFinal_ex(context,out.data(),&count)==1&&count==out.size(),"Cannot finish renderer digest");std::ostringstream text;text<<std::hex<<std::setfill('0');for(auto byte:out)text<<std::setw(2)<<unsigned(byte);return text.str();}
};
std::string hash(std::span<const std::byte> bytes){Digest digest;digest.add(bytes);return digest.finish();}
std::string file_hash(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);need(bool(input),"Cannot open renderer input: "+path.string());Digest digest;
    std::array<char,65536> bank;while(input){input.read(bank.data(),bank.size());digest.add(std::as_bytes(std::span(bank).first(input.gcount())));}
    need(input.eof(),"Cannot hash complete renderer input");return digest.finish();
}
// Module-local specialization bytes observed at actual Vulkan pipeline creation.
// SpecID0 also selects native W-mask/depth options; it is not a VM indicator.
std::map<std::string,std::set<std::string>> observed_module_paths;
void index_shader_paths(const std::filesystem::path& directory) {
    std::error_code error;
    if(!std::filesystem::is_directory(directory,error))return;
    for(std::filesystem::recursive_directory_iterator it(directory,error),end;it!=end&&!error;it.increment(error))
        if(it->is_regular_file(error)&&it->path().extension()==".spv")
            observed_module_paths[file_hash(it->path())].insert(it->path().lexically_relative(directory).generic_string());
}
struct PipelineSpecializations {
    std::map<std::tuple<std::string,unsigned,std::string,std::string>,json> modes;
    void observe(const RendererObservedPipelineShader& shader) {
        const auto module=hash(std::as_bytes(std::span(shader.words)));
        const auto entries=hash(std::as_bytes(std::span(shader.specialization_entries)));
        const auto data=hash(shader.specialization_bytes);
        const auto key=std::tuple{module,shader.stage,entries,data};
        if(auto found=modes.find(key);found!=modes.end()) {
            found->second["observed_bindings"]=found->second["observed_bindings"].get<std::size_t>()+1;return;
        }
        json values=json::array(),zero=nullptr;
        for(const auto& entry:shader.specialization_entries) {
            json value={{"constant_id",entry[0]},{"offset",entry[1]},{"size",entry[2]}};
            if(entry[2]==4&&entry[1]<=shader.specialization_bytes.size()&&shader.specialization_bytes.size()-entry[1]>=4) {
                std::uint32_t word{};
                for(unsigned byte=0;byte<4;++byte)word|=std::uint32_t(std::to_integer<unsigned>(shader.specialization_bytes[entry[1]+byte]))<<(8*byte);
                value["raw_u32_le"]=word;if(entry[0]==0)zero=word;
            }
            values.push_back(std::move(value));
        }
        const auto paths=observed_module_paths.find(module);
        const auto family=module=="7fd3cbf3a04778c4706d9a60849a4217ff21498f6f4d919a112180d8a0905b14"?
            "legacy-world-fragment-combiner":"module-local-native-or-other";
        modes.emplace(key,json{{"module_sha256",module},{"stage",shader.stage},
            {"module_paths",paths==observed_module_paths.end()?std::set<std::string>{}:paths->second},
            {"specialization_family",family},{"map_sha256",entries},{"data_sha256",data},
            {"data_bytes",shader.specialization_bytes.size()},{"entries",std::move(values)},
            {"spec_id0_raw_u32_le",std::move(zero)},{"observed_bindings",1}});
    }
    json report() const {json result=json::array();for(const auto& [key,value]:modes)result.push_back(value);return result;}
};
struct Image {std::vector<std::byte> pixels;std::string digest;};
struct RendererFrameFailure:FormatError {using FormatError::FormatError;};
Image image(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);std::string magic;unsigned w{},h{},maximum{};
    input>>magic>>w>>h>>maximum;input.get();need(magic=="P6"&&w==width&&h==height&&maximum==255,"Renderer capture shape changed");
    Image result;result.pixels.resize(std::size_t(w)*h*3);input.read(reinterpret_cast<char*>(result.pixels.data()),result.pixels.size());
    need(bool(input),"Renderer capture was truncated");result.digest=hash(result.pixels);return result;
}
json pixel_difference(const Image& actual,const Image& expected) {
    need(actual.pixels.size()==expected.pixels.size(),"Compared renderer images have different extents");
    std::size_t pixels{},channels{},absolute{};unsigned maximum{};
    for(std::size_t i=0;i<actual.pixels.size();i+=3) {
        bool changed=false;
        for(unsigned component=0;component<3;++component) {
            const auto difference=unsigned(std::abs(int(std::to_integer<unsigned>(actual.pixels[i+component]))-
                int(std::to_integer<unsigned>(expected.pixels[i+component]))));
            changed|=difference!=0;channels+=difference!=0;absolute+=difference;maximum=std::max(maximum,difference);
        }
        pixels+=changed;
    }
    return {{"differing_pixels",pixels},{"differing_channels",channels},{"maximum_channel_difference",maximum},{"absolute_channel_difference",absolute}};
}
std::filesystem::path baseline_capture(const std::filesystem::path& report,const json& row) {
    const std::filesystem::path name=row.at("capture").get<std::string>();
    need(!name.empty()&&name==name.filename(),"Baseline capture is not an owned local filename");
    return report.parent_path()/name;
}
Image capture(Renderer& renderer,const ModelView& view,const std::filesystem::path& path) {
    for(unsigned attempt=0;attempt<5;++attempt) {
        bool submitted{};
        try{submitted=renderer.draw(view.camera,path,{},false,{},{},{},{&view,1});}
        catch(const std::exception& error){throw RendererFrameFailure(error.what());}
        if(submitted)return image(path);
    }
    throw RendererFrameFailure("Complete BSP renderer frame did not submit");
}
SourceViewProjectionBank camera_bank(const Camera& camera) {
    need(camera.projection_vertical_angle.has_value(),"Controlled source view has no raw projection angle");
    SourceFrustumProjectionInput projection;projection.projection_angle=*camera.projection_vertical_angle;
    projection.outer=projection.safe={0,0,std::int16_t(height),std::int16_t(width)};
    return source_view_projection({camera.position,camera.forward,camera.up,.015625f,1024.f,{},
        source_frustum_projection(projection).projection_program});
}
ModelView view(Vec3 eye,Vec3 direction) {
    ModelView result;result.width=640;result.height=480;result.full_surface=true;result.effect_time=0;
    auto& camera=result.camera;camera.position=eye;camera.oriented=true;camera.forward=direction;
    camera.up=std::abs(direction.z)>.5f?Vec3{0,1,0}:Vec3{0,0,1};camera.field_of_view=70;
    constexpr float angle=1.2217304764f;camera.projection_vertical_angle=angle;
    const RenderViewport outer{0,0,float(width),float(height)};
    const auto clip=source_camera_projection(angle,outer,outer,false);
    camera.projection_clip_scale=clip.clip_scale;camera.projection_clip_offset=clip.clip_offset;
    const auto bank=camera_bank(camera);
    result.source_view_shader_constants.emplace();
    result.source_view_shader_constants->world=source_view_shader_constants(bank,1024.f,false);
    result.source_view_shader_constants->reflection=result.source_view_shader_constants->world;
    const auto foreground=source_view_shader_constants(bank,1024.f,true);
    std::copy_n(foreground.begin(),4,result.source_view_shader_constants->foreground.begin());
    result.source_w_distance=1024.f;result.projection_clip_planes=Vec2{.015625f,1024.f};
    return result;
}
struct BspCameraProbe {
    Vec3 eye{};
    std::string source;
    BspAtmosphere::CameraLocation location;
    std::size_t candidates{};
    std::optional<std::size_t> triangle,spawn;
    float normal_offset{};
};
BspCameraProbe bsp_camera(const Cache& cache,const Mesh& mesh,const BspAtmosphere& atmosphere,unsigned bsp,
    const std::optional<std::pair<Vec3,std::string>>& paired={}) {
    BspCameraProbe result;
    const auto admitted=[&](Vec3 eye) {
        ++result.candidates;const auto location=atmosphere.location_at(eye);
        if(location.cluster<0)return false;
        need(std::size_t(location.cluster)<atmosphere.clusters.size(),"Camera leaf references an absent authored cluster");
        result.eye=eye;result.location=location;return true;
    };
    if(paired && admitted(paired->first)){result.source=paired->second+"-paired-original";return result;}
    const auto spawns=cache.spawns();
    for(std::size_t index=0;index<spawns.size();++index)if(spawns[index].bsp==bsp && admitted(spawns[index].position)) {
        result.source="authored-player-start";result.spawn=index;return result;
    }
    // A controlled diagnostic eye still needs a real non-solid source leaf.
    // Search adjacent to authored triangles, accepting only14A280's own
    // leaf/cluster classification. A bounds center can lie inside solid BSP.
    need(mesh.vertices.size()%3==0,"BSP camera search requires the actual triangle list");
    for(std::size_t first=0;first<mesh.vertices.size();first+=3) {
        const auto a=mesh.vertices[first].position,b=mesh.vertices[first+1].position,c=mesh.vertices[first+2].position;
        const auto normal=cross(b-a,c-a);const auto magnitude=length(normal);
        if(!std::isfinite(magnitude)||magnitude<=.000001f)continue;
        const auto center=(a+b+c)*(1.f/3.f),unit=normal*(1.f/magnitude);
        for(const float offset:{.0625f,-.0625f,.25f,-.25f,1.f,-1.f,.015625f,-.015625f,0.f})
            if(admitted(center+unit*offset)) {
                result.source="source-classified-authored-triangle-air-leaf";
                result.triangle=first/3;result.normal_offset=offset;return result;
            }
    }
    throw FormatError("No source-classified non-solid camera leaf beside the authored BSP triangles");
}
std::string csv(std::string value){std::string out="\"";for(char c:value){if(c=='\"')out+='\"';out+=c;}return out+'\"';}
void material_rows(std::ostream& output,const std::string& owner,unsigned slot,
    const Material& material,const std::string& path,std::size_t& rows,unsigned depth=0) {
    need(depth<32,"Renderer material graph is cyclic or unbounded");
    const auto& pixel=material.forward_pass?material.forward_program:material.lightmap_program;
    output<<csv(owner)<<','<<slot<<','<<csv(path)<<','<<material.source_shader_datum<<','<<material.selected_shader_lod<<','
          <<material.forward_pass_index<<','<<material.forward_vertex<<','<<material.forward_pass<<','<<material.static_lightmap_pass<<','
          <<hash(std::as_bytes(std::span(pixel.words)))<<','<<hash(material.vertex_program)<<','<<csv(material.name)<<'\n';++rows;
    const auto child=[&](const auto& pointer,const std::string& name){if(pointer)material_rows(output,owner,slot,*pointer,path+'/'+name,rows,depth+1);};
    child(material.environment_pass,"environment");child(material.planar_fog_pass,"planar-fog");
    for(unsigned i=0;i<material.world_overlays_before.size();++i)child(material.world_overlays_before[i],"before"+std::to_string(i));
    for(unsigned i=0;i<material.world_overlays_after.size();++i)child(material.world_overlays_after[i],"after"+std::to_string(i));
    for(unsigned i=0;i<material.world_bloom_passes.size();++i)child(material.world_bloom_passes[i],"bloom"+std::to_string(i));
    for(unsigned i=0;i<material.world_planar_passes.size();++i)child(material.world_planar_passes[i],"planar"+std::to_string(i));
}
json representative_model(const Cache& cache,SDL_Window* window,const std::filesystem::path& output,
    const std::string& id,const std::string& input_sha,bool reference_mode,std::ostream& matrix,std::size_t& rows) {
    json result={{"id",id},{"input_sha256",input_sha},{"clock_seconds",0},
        {"scope","First typed resident biped/hlmt/mode, authored bind pose, explicit controlled lighting/collector/camera; no original gameplay-frame claim"}};
    const Tag* selected=nullptr;std::uint32_t model_datum=0xffffffffu;
    unsigned no_model{};
    for(const auto& tag:cache.tags())if(tag.group=="bipd"&&tag.address) {
        const Bytes body(cache.tag_payload(tag.handle()));const auto hlmt=body.u32(0x38);
        if(hlmt==0xffffffffu){++no_model;continue;}
        const auto model=cache.query_tag({.handle=TagHandle{hlmt},.group=TagGroup::from_string("hlmt"),.require_address=true});
        need(model,"Representative biped references a missing typed hlmt owner");
        model_datum=Bytes(cache.tag_payload(model->handle())).u32(4);
        if(model_datum==0xffffffffu){++no_model;continue;}
        need(cache.query_tag({.handle=TagHandle{model_datum},.group=TagGroup::from_string("mode"),.require_address=true}),
             "Representative biped references a missing typed mode owner");selected=&tag;break;
    }
    result["authored_model_absences"]=no_model;
    if(!selected){result["status"]="authored-no-resident-biped-model";return result;}
    result["biped"]=selected->name;result["biped_datum"]=selected->datum;result["model_datum"]=model_datum;
    auto model=cache.load_model(model_datum,true);need(!model.mesh.vertices.empty(),"Resident representative biped has no geometry");
    ObjectLightingState lighting;lighting.ambient={.4f,.4f,.4f};lighting.primary_color={.6f,.6f,.6f};
    lighting.primary_direction={0,0,1};lighting.accuracy=1;
    for(auto& material:model.mesh.materials)if(!material.lods.empty()) {
        need(prepare_dynamic_material(cache,material,lighting,true),"Representative model material has no source dynamic pass: "+material.name);
        animate_forward_material(material,0,{});apply_object_lighting(material,lighting);
    }
    SourceDynamicModelShaders shader_bank(cache,model,model_datum);
    auto geometry=pose_model(model);SourceModelShaderView shader_view;
    shader_view.shader=source_shader_lod_view(0,480,false,false);shader_view.part_shader_multiplier=1;
    shader_bank.select(geometry,{10000,255},shader_view);
    need(!geometry.empty(),"Representative bind-pose shader selection emitted no triangles");
    for(unsigned slot=0;slot<model.mesh.materials.size();++slot)material_rows(matrix,id,slot,model.mesh.materials[slot],"model-root",rows);
    Vec3 low{INFINITY,INFINITY,INFINITY},high{-INFINITY,-INFINITY,-INFINITY};
    for(const auto& vertex:geometry){low=minimum(low,vertex.position);high=maximum(high,vertex.position);}
    const auto center=(low+high)*.5f;const auto distance=std::max(1.f,length(high-low));
    const Vec3 eye=center+Vec3{-distance,0,0};auto current=view(eye,{1,0,0});
    current.vertices=geometry;current.atmosphere=BspFogSample{};current.atmosphere->clear=Vec3{.02f,.03f,.04f};
    const auto atmosphere=cache.load_bsp_atmosphere(0);
    current.atmosphere_source=&atmosphere;
    result["eye"]={eye.x,eye.y,eye.z};result["forward"]={1,0,0};result["vertices"]=geometry.size();
    result["diagnostic_lighting"]={{"ambient",{.4f,.4f,.4f}},{"primary",{.6f,.6f,.6f}},{"direction",{0,0,1}}};
    std::map<unsigned,std::array<unsigned,2>> slot_uploads;
    unsigned words{},uploads{};current.completed_material_upload_observer=[&](const RendererCompletedMaterialUpload& value){
        words+=value.uploaded_vertex_instruction_words;++uploads;
        auto& slot=slot_uploads[unsigned(value.material_slot)];++slot[0];slot[1]+=value.uploaded_vertex_instruction_words;
    };
    const auto* shader_override=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
    std::ostringstream diagnostics;auto* previous=std::cerr.rdbuf(diagnostics.rdbuf());
    try {
        Renderer renderer(window,model.mesh,true,{},nullptr,{},nullptr,shader_override?shader_override:std::filesystem::path{});
        renderer.scene_bsp_atmosphere(atmosphere);
        const auto owner_key=hash(std::as_bytes(std::span(id))).substr(0,16);
        const auto first_path=output/("model-"+owner_key+".ppm"),repeat_path=output/("model-"+owner_key+"-repeat.ppm");
        const auto first=capture(renderer,current,first_path),repeat=capture(renderer,current,repeat_path);
        need(first.pixels==repeat.pixels,"Representative authored bind pose is not deterministic");
        need(uploads>0,"Representative model frame uploaded no selected material");
        if(!reference_mode)need(words==0,"Native representative model uploaded executable vertex instruction words");
        std::set<std::array<std::byte,3>> colors;
        for(std::size_t i=0;i<first.pixels.size();i+=3)colors.insert({first.pixels[i],first.pixels[i+1],first.pixels[i+2]});
        need(colors.size()>1,"Representative authored model rendered no visible samples");
        result["pixel_sha256"]=first.digest;result["repeat_exact"]=true;result["uploaded_vertex_instruction_words"]=words;
        result["material_uploads"]=uploads;result["device"]=renderer.device_name();result["distinct_rgb"]=colors.size();
        result["uploaded_material_slots"]=json::array();result["material_slot_uploads"]=json::array();
        for(const auto& [slot,counts]:slot_uploads) {
            result["uploaded_material_slots"].push_back(slot);
            result["material_slot_uploads"].push_back({{"slot",slot},{"upload_count",counts[0]},{"instruction_words",counts[1]}});
        }
        result["capture"]=first_path.filename().string();
        std::filesystem::remove(repeat_path);
    }catch(...){std::cerr.rdbuf(previous);throw;}
    std::cerr.rdbuf(previous);need(diagnostics.str().empty(),"Representative model produced Vulkan/renderer diagnostics: "+diagnostics.str().substr(0,512));
    result["status"]="passed";return result;
}
void save(const std::filesystem::path& output,json& report,const std::map<std::string,json>& cases) {
    report["cases"]=json::array();for(const auto& [key,row]:cases)report["cases"].push_back(row);
    std::ofstream file(output/"report.json");file<<report.dump(2)<<'\n';need(bool(file),"Cannot retain complete all-map renderer report");
}
}

int main(int argc,char** argv)try {
    std::filesystem::path output,prepared,reference_path;bool reference_mode=false;std::set<std::string> filter;
    for(int i=1;i<argc;++i){const std::string option=argv[i];
        if(option=="--reference"){reference_mode=true;continue;}
        need(i+1<argc,"Missing all-map renderer option value");const std::string value=argv[++i];
        if(option=="--output")output=value;else if(option=="--prepared-root")prepared=value;
        else if(option=="--compare-reference")reference_path=value;else if(option=="--map-filter")filter.insert(value);
        else throw FormatError("Unknown all-map renderer option: "+option);
    }
    need(!output.empty(),"Supply --output for the all-map renderer case ledger");
    if(prepared.empty())if(const auto* value=std::getenv("HALO2_RENDERER_MCC_ROOT"))prepared=value;
    if(prepared.empty())if(const auto* value=std::getenv("HALO2_SKY_MCC_ROOT"))prepared=value;
    need(!prepared.empty(),"Supply the complete prepared runtime cache owner");
    need(!reference_mode||reference_path.empty(),"Reference mode cannot compare against itself");
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch&&*scratch,"Run renderer acceptance through tools/run_tests.py");
    output=std::filesystem::absolute(output);std::filesystem::create_directories(output);
    const auto relative_output=std::filesystem::weakly_canonical(output).lexically_relative(std::filesystem::weakly_canonical(scratch));
    need(!relative_output.empty()&&!relative_output.is_absolute(),"Raw renderer outputs require an owned scratch descendant");
    for(const auto& part:relative_output)need(part!="..","Renderer output escapes owned scratch");
    const auto root=std::filesystem::path(HALO2_SOURCE_DIR);
    json report={{"schema","HALO2_ALL_MAP_RENDERER_GPU_V1"},{"reference_mode",reference_mode},{"filtered",!filter.empty()},
        {"scope","All declared map files, every BSP/resident shader LOD and material graph, controlled six-direction complete-geometry GPU frames; family-specific live owners remain separate"},
        {"clock_seconds",0},{"capture_extent",{width,height}},{"original_framebuffer_parity",false},{"physical_xbox_precision",false},
        {"prepared_inventories",json::array()},{"maps",json::array()},{"errors",json::array()},
        {"reference_report",reference_path.string()},{"family_execution_boundary","Static all-BSP diagnostic does not manufacture live particle/weather/shadow/HUD owners; dedicated source GPU tests and ordinary runtime delivery are separate required gates"}};
    report["test_executable_sha256"]=file_hash(std::filesystem::absolute(argv[0]));
    const auto* indexed_shader_override=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
    index_shader_paths(indexed_shader_override?std::filesystem::path(indexed_shader_override):std::filesystem::absolute(argv[0]).parent_path()/"shaders");
    report["pipeline_specialization_scope"]="Actual BSP draw pipeline modules, deduplicated per case by SHA/stage/map/data; all values raw and module-local. SpecID0 alone does not identify runtime shader interpretation. Representative object callback exposes no pipeline module payload.";
    const auto camera_fixture_path=root/"tests/fixtures/tu15_texture_camera_initial_inputs.json";
    const auto camera_fixture_sha=file_hash(camera_fixture_path);
    need(camera_fixture_sha=="0f0b3254d2a0f7cc3102f650c43e0585aaecc126290c0009eded1a4f3a77541d",
        "Authenticated complete initial texture-camera fixture bytes changed");
    const NativeRendererTextureCameraFixture camera_fixture(camera_fixture_path);
    const auto texture_camera_binding=camera_fixture.binding();
    report["texture_camera_fixture"]={{"sha256",camera_fixture_sha},
        {"source_xbe_sha256","21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"},
        {"selection","default2d"},{"scope","Explicit authenticated XBE initial target/aux/view data plus completed2B520 camera NONE reset; controlled noncinematic/noactivecamera fixture. Live camera activation and target18/20 production remain separate."}};
    std::map<std::string,json> cases,reference_cases;
    if(!reference_path.empty()) {
        std::ifstream input(reference_path);need(bool(input),"Missing baseline renderer report");const auto baseline=json::parse(input);
        need(baseline.at("schema")==report.at("schema")&&baseline.at("reference_mode")==true,"Baseline report is not an explicit offline reference run");
        need(baseline.at("clock_seconds")==0&&baseline.at("capture_extent")==report.at("capture_extent"),"Baseline view/clock contract changed");
        need(baseline.at("filtered")==report.at("filtered"),"Filtered and complete renderer cohorts cannot be conflated");
        need(baseline.at("counts").at("uploaded_vertex_instruction_words").get<std::size_t>()>0,
            "Reference report does not demonstrate the preserved instruction-executing baseline");
        for(const auto& row:baseline.at("cases"))need(reference_cases.emplace(row.at("id").get<std::string>(),row).second,"Duplicate baseline renderer case");
        report["reference_report_sha256"]=file_hash(reference_path);
        report["reference_material_pass_matrix"]=baseline.at("material_pass_matrix");
    }
    std::vector<MapInput> inputs;
    try{inputs=map_inventory(root,prepared,report["prepared_inventories"]);}
    catch(const std::exception& error){report["passed"]=false;report["errors"].push_back(error.what());save(output,report,cases);throw;}
    for(const auto& inventory:report["prepared_inventories"])need(inventory.at("complete")==true,"Prepared map inventory is incomplete");
    std::ofstream matrix(output/"material-pass-matrix.csv");need(bool(matrix),"Cannot open complete material-pass matrix");
    matrix<<"case,root_slot,graph_path,shader_datum,shader_lod,pass_index,vertex_class,forward,static_lightmap,pixel_sha256,vertex_sha256,name\n";
    std::size_t material_rows_count{},map_count{},bsp_count{},frame_count{},model_count{},errors{},instruction_words{},observed_draws{},weather_admission_errors{};
    SDL_SetMainReady();SDL_setenv("HALO2_CAPTURE_BSP_DRAW_BINDINGS","1",1);
    need(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto* window=SDL_CreateWindow("All-map native renderer acceptance",0,0,width,height,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    need(window,SDL_GetError());
    const std::array<Vec3,6> directions{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
    std::map<std::pair<std::string,unsigned>,std::pair<Vec3,std::string>> original_eyes;
    std::map<std::tuple<std::string,unsigned,unsigned>,std::filesystem::path> original_images;
    std::map<std::tuple<std::string,unsigned,unsigned>,std::string> original_input_hashes;
    unsigned asset_pairs{},asset_differences{},conversion_pairs{},conversion_differences{};
    for(const auto& input:inputs) {
        if(!filter.empty()&&!filter.contains(input.key())&&!filter.contains(input.path.filename().string()))continue;
        ++map_count;json map={{"id",input.cohort_key()},{"path",input.path.string()},{"logical_name",input.key()},
                            {"source",input.source},{"shared_directory",input.shared.string()},{"bsps",json::array()}};
        try {
            const auto input_sha=file_hash(input.path);map["input_sha256"]=input_sha;
            Cache cache(input.path,input.shared,{},input.prepared_root);
            map["tag_count"]=cache.tags().size();map["bsp_count"]=cache.bsps().size();
            json families=json::object();for(const auto& tag:cache.tags())if(tag.address)
                if(tag.group=="prt3"||tag.group=="PRTM"||tag.group=="weat"||tag.group=="nhdt"||tag.group=="lens"||tag.group=="deca"||tag.group=="cont")
                    families[tag.group]=families.value(tag.group,0u)+1;
            map["resident_effect_owner_tags"]=families;
            map["weather_inventory"]=native_renderer_weather_inventory(cache);
            const auto weather_errors=map.at("weather_inventory").at("required_errors").size();
            weather_admission_errors+=weather_errors;errors+=weather_errors;
            if(cache.scenario_tag().is_null()||cache.bsps().empty()) {
                map["status"]="authored-resource-file-without-scenario-BSP";report["maps"].push_back(std::move(map));continue;
            }
            const auto model_id=input.cohort_key()+":representative-biped";
            json model_case={{"id",model_id},{"input_sha256",input_sha},{"status","failed"}};
            try {
                model_case=representative_model(cache,window,output,model_id,input_sha,reference_mode,matrix,material_rows_count);
                if(model_case.at("status")=="passed") {
                    ++model_count;instruction_words+=model_case.at("uploaded_vertex_instruction_words").get<unsigned>();
                    if(!reference_path.empty()) {
                        need(reference_cases.contains(model_id),"Representative model lacks a baseline case");const auto& baseline=reference_cases.at(model_id);
                        need(baseline.at("status")=="passed"&&baseline.at("input_sha256")==input_sha&&baseline.at("eye")==model_case.at("eye"),"Representative model baseline input/owner/view differs");
                        ++conversion_pairs;const bool exact=baseline.at("pixel_sha256")==model_case.at("pixel_sha256");conversion_differences+=!exact;
                        model_case["baseline_pixel_exact"]=exact;
                        if(!exact) {
                            const auto capture=baseline_capture(reference_path,baseline);
                            if(std::filesystem::is_regular_file(capture))model_case["baseline_pixel_difference"]=pixel_difference(image(output/model_case.at("capture").get<std::string>()),image(capture));
                            else model_case["baseline_raw_capture_available"]=false;
                        }
                        need(exact,"Native representative model differs from the identical-input baseline pixels");
                    }
                    if(!reference_mode)std::filesystem::remove(output/model_case.at("capture").get<std::string>());
                }else if(!reference_path.empty()) {
                    need(reference_cases.contains(model_id)&&reference_cases.at(model_id).at("status")==model_case.at("status"),
                        "Representative model authored absence differs from the baseline");
                }
            }catch(const std::exception& error){model_case["status"]="failed";model_case["error"]=error.what();++errors;}
            map["representative_model_case"]=model_id;cases.emplace(model_id,std::move(model_case));
            for(unsigned bsp=0;bsp<cache.bsps().size();++bsp) {
                ++bsp_count;const auto owner=input.cohort_key()+":bsp"+std::to_string(bsp);
                json entry={{"id",owner},{"bsp",bsp},{"directions",json::array()},{"errors",json::array()}};
                for(unsigned direction=0;direction<directions.size();++direction) {
                    const auto id=owner+":direction"+std::to_string(direction);
                    entry["directions"].push_back(id);
                    need(cases.emplace(id,json{{"id",id},{"input_sha256",input_sha},{"direction",direction},
                        {"status","not-executed"}}).second,"Duplicate all-map renderer direction case");
                }
                try {
                    auto mesh=cache.load_bsp(bsp,true);
                    entry["warnings"]=mesh.material_warnings;
                    const auto lightmaps=cache.load_bsp_lighting(bsp);
                    const bool static_lighting=lightmaps.structure_matches&&lightmaps.bitmap_datum!=0xffffffffu;
                    entry["source_lighting_classification"]=static_lighting?0:3;
                    entry["lightmap_checksum_matches"]=lightmaps.structure_matches;
                    entry["lightmap_bitmap_datum"]=lightmaps.bitmap_datum;
                    need(!mesh.vertices.empty(),"Authored scenario BSP has no rendered geometry");
                    const auto atmosphere=cache.load_bsp_atmosphere(bsp);
                    prepare_bsp_planar_fog(cache,mesh,atmosphere);
                    SourceBspResidentPlan resident(cache,mesh,bsp,static_lighting?
                        SourceBspResidentLighting::static_lightmap:SourceBspResidentLighting::classification3,&texture_camera_binding);
                    entry["loader_warnings"]=entry["warnings"];entry["warnings"]=mesh.material_warnings;
                    bool unresolved=false;
                    for(const auto& warning:mesh.material_warnings) {
                        if(!static_lighting&&warning=="authored lightmap geometry checksum mismatch; static bucket streams not admitted")
                            entry["expected_source_diagnostics"].push_back(warning);
                        else unresolved=true;
                    }
                    for(const auto &material:mesh.materials)if(material.template_name=="shaders\\shader_templates\\texture_camera\\texture_camera" && material.world_pass)
                        entry["texture_camera_default_bindings"].push_back({{"shader",material.source_shader_datum},{"lod",material.selected_shader_lod},{"pass",material.forward_pass_index}});
                    if(unresolved){++errors;entry["errors"].push_back("Authored BSP contains unresolved material/resource diagnostics");}
                    const auto water=cache.load_bsp_water(bsp);
                    std::vector<unsigned> water_material_bases;
                    for(unsigned definition=0;definition<water.definitions.size();++definition) {
                        const auto& authored=water.definitions[definition];
                        if(const auto* tag=cache.query_tag({.handle=TagHandle{authored.shader}}))
                            for(auto& material:mesh.materials)
                                if(material.water_pass && material.name==tag->name)material.water_definition=int(definition);
                        water_material_bases.push_back(unsigned(mesh.materials.size()));
                        for(auto material:authored.geometry.materials) {
                            material.water_definition=int(definition);mesh.materials.push_back(std::move(material));
                        }
                    }
                    BspSkyModels skies(cache,atmosphere,mesh);
                    // As in the ordinary runtime, bind pool record zero only
                    // after resident LODs, independent water and sky roots exist.
                    bind_water_pool_materials(water,mesh.materials);
                    const auto original_key=std::pair{input.key(),bsp};
                    std::optional<std::pair<Vec3,std::string>> paired_eye;
                    if(input.source=="prepared-stock"&&original_eyes.contains(original_key))paired_eye=original_eyes.at(original_key);
                    const auto camera_probe=bsp_camera(cache,mesh,atmosphere,bsp,paired_eye);
                    const auto eye=camera_probe.eye;const auto& eye_source=camera_probe.source;
                    if(!input.prepared())original_eyes[original_key]={eye,eye_source};
                    entry["eye"]={eye.x,eye.y,eye.z};entry["eye_source"]=eye_source;
                    entry["camera_source_leaf"]=camera_probe.location.leaf;entry["camera_source_cluster"]=camera_probe.location.cluster;
                    entry["camera_candidate_queries"]=camera_probe.candidates;
                    if(camera_probe.triangle)entry["camera_authored_triangle"]=*camera_probe.triangle;
                    if(camera_probe.spawn)entry["camera_authored_spawn"]=*camera_probe.spawn;
                    entry["camera_triangle_normal_offset"]=camera_probe.normal_offset;
                    entry["water_definitions"]=water.definitions.size();
                    std::size_t water_vertices{};for(const auto& definition:water.definitions)
                        if(!definition.geometry.materials.empty())water_vertices+=definition.geometry.vertices.size();
                    entry["independent_water_vertices"]=water_vertices;
                    entry["vertices"]=mesh.vertices.size();entry["resident_parts"]=resident.parts().size();entry["resident_material_roots"]=mesh.materials.size();
                    for(unsigned slot=0;slot<mesh.materials.size();++slot)material_rows(matrix,owner,slot,mesh.materials[slot],"root",material_rows_count);
                    std::vector<SourceBspPartBinding> bindings;bindings.reserve(resident.parts().size());
                    for(const auto& part:resident.parts())bindings.push_back({part.original.material,part.uniform_stipple});
                    const auto* shader_override=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
                    std::ostringstream diagnostics;auto* previous=std::cerr.rdbuf(diagnostics.rdbuf());
                    try {
                        Renderer renderer(window,mesh,true,{},&resident,{},nullptr,shader_override?shader_override:std::filesystem::path{});
                        renderer.scene_bsp_atmosphere(atmosphere);
                        const auto device=renderer.device_name();
                        for(const auto* software:{"llvmpipe","lavapipe","SwiftShader","Software Rasterizer"})
                            need(device.find(software)==std::string::npos,"All-map rendering requires a hardware GPU");
                        entry["device"]=renderer.device_name();
                        for(unsigned direction=0;direction<directions.size();++direction) {
                            const auto id=owner+":direction"+std::to_string(direction);
                            json row={{"id",id},{"input_sha256",input_sha},{"direction",direction},{"eye",{eye.x,eye.y,eye.z}},
                                      {"forward",{directions[direction].x,directions[direction].y,directions[direction].z}},
                                      {"status","failed"},{"clock_seconds",0},{"actual_bsp_draws",0},{"uploaded_vertex_instruction_words",0}};
                            bool frame_failure=false;
                            try {
                                auto current=view(eye,directions[direction]);
                                const auto fog=atmosphere.sample(eye,directions[direction]);const auto background=skies.pose(fog,eye);
                                current.atmosphere=fog;current.atmosphere_source=&atmosphere;
                                current.background=background;current.source_bsp_resident_plan=&resident;current.source_bsp_bindings=bindings;
                                std::vector<WaterFrame> water_frames;water_frames.reserve(water.definitions.size());
                                std::vector<Vertex> independent_water;
                                for(unsigned definition=0;definition<water.definitions.size();++definition) {
                                    const auto& authored=water.definitions[definition];
                                    water_frames.push_back(water_frame(authored,eye,directions[direction],
                                        *current.camera.projection_vertical_angle,float(width)/float(height),false));
                                    if(authored.geometry.materials.empty())continue;
                                    for(auto vertex:authored.geometry.vertices) {
                                        vertex.position=water_world_position(water_frames.back(),vertex.position);
                                        vertex.material+=water_material_bases[definition];independent_water.push_back(vertex);
                                    }
                                }
                                current.water=water_frames;current.vertices=independent_water;
                                auto& reflection=current.water_reflection_camera;reflection=current.camera;
                                std::vector<Vertex> reflection_background;
                                if(!water.definitions.empty()) {
                                    reflection.position.z=2.f*water.definitions.front().parameters[33]-eye.z;
                                    reflection.forward.z=-reflection.forward.z;reflection.up.z=-reflection.up.z;
                                    // Mirror the submitting basis, including the two
                                    // vertical diagnostic axes, through the real plane.
                                    current.source_view_shader_constants->reflection=source_view_shader_constants(camera_bank(reflection),1024.f,false);
                                    reflection_background=skies.pose(fog,reflection.position);
                                    current.water_reflection_background=reflection_background;
                                }
                                row["water_frame_count"]=water_frames.size();row["independent_water_vertices"]=independent_water.size();
                                std::set<std::string> shaders;std::map<unsigned,std::size_t> phases;std::set<unsigned> uploaded_slots;PipelineSpecializations specializations;
                                unsigned draw_calls{},words{};
                                current.completed_material_upload_observer=[&](const RendererCompletedMaterialUpload& upload){words+=upload.uploaded_vertex_instruction_words;uploaded_slots.insert(upload.material_slot);};
                                current.completed_bsp_draw_observer=[&](const RendererObservedBspDraw& draw) {
                                    ++draw_calls;++phases[unsigned(draw.phase)];need(draw.pipeline,"Actual BSP draw has no observed pipeline");
                                    for(const auto& shader:draw.pipeline->shaders){shaders.insert(hash(std::as_bytes(std::span(shader.words))));specializations.observe(shader);}
                                };
                                const auto prefix=output/("frame-"+std::to_string(map_count)+"-"+std::to_string(bsp)+"-"+std::to_string(direction));
                                // Warm up lazy targets, then retain/restore the actual postprocess
                                // checkpoint so both submissions observe one identical source frame.
                                const auto warm=prefix.string()+"-warm.ppm";capture(renderer,current,warm);std::filesystem::remove(warm);
                                const auto checkpoint=renderer.bsp_postprocess_state();draw_calls=words=0;phases.clear();uploaded_slots.clear();shaders.clear();specializations.modes.clear();
                                const auto first_path=prefix.string()+".ppm",repeat_path=prefix.string()+"-repeat.ppm";
                                const auto first=capture(renderer,current,first_path);
                                if(checkpoint)renderer.restore_bsp_postprocess_state(*checkpoint);
                                const auto repeat=capture(renderer,current,repeat_path);std::filesystem::remove(repeat_path);
                                need(first.pixels==repeat.pixels,"Identical complete-BSP source frame is not deterministic");
                                need(draw_calls>0,"Complete BSP frame recorded no source-resident draw");
                                if(!reference_mode)need(words==0,"Native full-BSP frame uploaded executable vertex instruction words");
                                row["pixel_sha256"]=first.digest;row["repeat_exact"]=true;row["actual_bsp_draws"]=draw_calls;
                                row["uploaded_vertex_instruction_words"]=words;row["uploaded_material_slots"]=uploaded_slots;
                                row["observed_shader_sha256"]=shaders;row["phase_draws"]=phases;row["observed_pipeline_specializations"]=specializations.report();
                                instruction_words+=words;observed_draws+=draw_calls;++frame_count;
                                const auto key=std::tuple{input.key(),bsp,direction};
                                if(!input.prepared()){original_images[key]=first_path;original_input_hashes[key]=input_sha;}
                                else {
                                    if(input.source=="prepared-stock"&&original_images.contains(key)) {
                                        const auto original=image(original_images.at(key));++asset_pairs;
                                        const bool exact=original.pixels==first.pixels;asset_differences+=!exact;
                                        row["original_asset_pair"]={{"exact",exact},{"source_input_sha256",original_input_hashes.at(key)},
                                            {"source_pixel_sha256",original.digest},{"scope","Different original/prepared asset inputs through the same renderer; separate from conversion equivalence"}};
                                    }else row["original_asset_pair"]={{"scope","No original map/BSP counterpart"}};
                                }
                                if(reference_mode)row["capture"]=std::filesystem::path(first_path).filename().string();
                                if(!reference_path.empty()) {
                                    need(reference_cases.contains(id),"Native renderer case has no baseline counterpart");const auto& baseline=reference_cases.at(id);
                                    need(baseline.at("status")=="passed","Baseline renderer case did not complete");
                                    need(baseline.at("input_sha256")==input_sha&&baseline.at("eye")==row.at("eye")&&baseline.at("forward")==row.at("forward"),"Native/baseline source input or view differs");
                                    ++conversion_pairs;const bool exact=baseline.at("pixel_sha256")==first.digest;conversion_differences+=!exact;
                                    row["baseline_pixel_exact"]=exact;row["baseline_pixel_sha256"]=baseline.at("pixel_sha256");
                                    if(!exact) {
                                        const auto capture=baseline_capture(reference_path,baseline);
                                        if(std::filesystem::is_regular_file(capture))row["baseline_pixel_difference"]=pixel_difference(first,image(capture));
                                        else row["baseline_raw_capture_available"]=false;
                                    }
                                    need(exact,"Native kernel conversion changed the identical-input baseline pixels");
                                }
                                if(input.prepared()&&!reference_mode)std::filesystem::remove(first_path);
                                row["status"]="passed";
                            }catch(const RendererFrameFailure& error){row["error"]=error.what();frame_failure=true;++errors;}
                            catch(const std::exception& error){row["error"]=error.what();++errors;}
                            cases.at(id)=std::move(row);
                            if(frame_failure) {
                                entry["errors"].push_back("Actual draw failed; remaining directions require a fresh Renderer acquisition state");
                                for(unsigned blocked=direction+1;blocked<directions.size();++blocked) {
                                    auto& remaining=cases.at(owner+":direction"+std::to_string(blocked));
                                    remaining["status"]="blocked-by-prior-draw-error";remaining["failed_draw_case"]=id;
                                }
                                break;
                            }
                        }
                    }catch(...){std::cerr.rdbuf(previous);throw;}
                    std::cerr.rdbuf(previous);entry["vulkan_diagnostics"]=diagnostics.str();
                    need(diagnostics.str().empty(),"All-map BSP run produced Vulkan/renderer diagnostics: "+diagnostics.str().substr(0,512));
                    entry["status"]=entry["errors"].empty()?"executed":"failed";
                }catch(const std::exception& error){
                    entry["status"]="failed";entry["errors"].push_back(error.what());++errors;
                    for(const auto& id:entry["directions"])if(cases.at(id.get<std::string>()).at("status")=="not-executed") {
                        auto& row=cases.at(id.get<std::string>());row["status"]="blocked-by-BSP-owner-error";row["error"]=error.what();
                    }
                }
                map["bsps"].push_back(std::move(entry));save(output,report,cases);
            }
            map["status"]="enumerated";
        }catch(const std::exception& error){map["status"]="failed";map["error"]=error.what();++errors;}
        report["maps"].push_back(std::move(map));save(output,report,cases);
    }
    matrix.close();need(bool(matrix),"Complete material-pass matrix write failed");
    if(!reference_path.empty()) {
        std::set<std::string> actual,expected;for(const auto& [id,row]:cases)actual.insert(id);for(const auto& [id,row]:reference_cases)expected.insert(id);
        report["baseline_case_coverage_complete"]=actual==expected;if(actual!=expected){++errors;report["errors"].push_back("Native/reference complete case ledgers differ");}
    }
    report["counts"]={{"map_entries",map_count},{"bsp_entries",bsp_count},{"completed_direction_frames",frame_count},
        {"representative_models",model_count},{"actual_bsp_draws",observed_draws},{"material_pass_rows",material_rows_count},{"uploaded_vertex_instruction_words",instruction_words},
        {"original_prepared_pixel_pairs",asset_pairs},{"original_prepared_pairs_with_differences",asset_differences},
        {"baseline_conversion_pairs",conversion_pairs},{"baseline_conversion_pairs_with_differences",conversion_differences},
        {"required_weather_admission_errors",weather_admission_errors},{"errors",errors}};
    report["material_pass_matrix"]={{"path","material-pass-matrix.csv"},{"sha256",file_hash(output/"material-pass-matrix.csv")},
                                    {"bytes",std::filesystem::file_size(output/"material-pass-matrix.csv")}};
    if(!reference_path.empty()) {
        const bool exact=report.at("reference_material_pass_matrix").at("sha256")==report.at("material_pass_matrix").at("sha256");
        report["baseline_material_pass_matrix_exact"]=exact;
        if(!exact){++errors;report["counts"]["errors"]=errors;report["errors"].push_back("Native/reference complete material-pass ledgers differ");}
    }
    report["passed"]=errors==0&&bsp_count>0&&model_count>0&&frame_count==bsp_count*6&&(!reference_path.empty()||reference_mode)&&(!reference_mode||instruction_words>0);
    report["native_complete_shader_conversion_accepted"]=report["passed"].get<bool>()&&!reference_mode&&filter.empty();
    save(output,report,cases);SDL_DestroyWindow(window);SDL_Quit();
    std::cout<<map_count<<" map entries, "<<bsp_count<<" BSPs, "<<frame_count<<" direction frames, "<<errors<<" errors; complete report: "<<(output/"report.json")<<'\n';
    return report["passed"].get<bool>()?0:1;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';SDL_Quit();return 1;}
