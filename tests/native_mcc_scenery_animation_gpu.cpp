#define SDL_MAIN_HANDLED
#include "native_mcc_scenery_animation_gpu.hpp"
#include "halo2/renderer.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/source_campaign_lighting_tu15.hpp"
#include "halo2/source_campaign_screen_tu15.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/shader_animation.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

namespace {
using namespace halo2;
void check(bool value,const std::string& message){if(!value)throw FormatError(message);}
unsigned word(const SourceRuntimeMemory& m,unsigned at){return Bytes(m.view(at,4)).u32(0);}
struct Image {std::vector<unsigned char> rgb;};
Image read_image(const std::filesystem::path& path){
    std::ifstream input(path,std::ios::binary);std::string magic;unsigned width{},height{},maximum{};
    input>>magic>>width>>height>>maximum;input.get();
    check(magic=="P6"&&width==640&&height==480&&maximum==255,"Scenery GPU capture extent/format differs");
    Image result;result.rgb.resize(width*height*3);input.read(reinterpret_cast<char*>(result.rgb.data()),result.rgb.size());
    check(bool(input),"Scenery GPU capture is truncated");return result;
}
unsigned difference(const Image& a,const Image& b){
    check(a.rgb.size()==b.rgb.size(),"Scenery GPU control extents differ");unsigned result{};
    for(unsigned n=0;n<a.rgb.size();n+=3)if(a.rgb[n]!=b.rgb[n]||a.rgb[n+1]!=b.rgb[n+1]||a.rgb[n+2]!=b.rgb[n+2])++result;
    return result;
}
ObjectLightingState authored_lighting(const LevelContent& content,const ScenarioObject& placement,
    MultiplayerObjectRuntime& objects,unsigned object){
    auto& world=objects.tu15_world();const auto& p=content.scripts;
    const auto node=[&](unsigned datum)->const ScriptNode&{
        check(datum!=null_datum&&(datum&65535)<p.nodes.size(),"Scenery lighting escapes syntax");
        const auto& n=p.nodes[datum&65535];check(n.salt==datum>>16,"Scenery lighting has stale syntax salt");return n;
    };
    const auto text=[&](unsigned at){check(at<p.strings.size(),"Scenery lighting name escapes strings");
        const auto end=p.strings.find('\0',at);check(end!=std::string::npos,"Scenery lighting name is unterminated");return p.strings.substr(at,end-at);};
    struct Call {std::string name;std::vector<ScriptNode> arguments;};
    std::vector<Call> selected;
    for(const auto& script:p.scripts){
        const auto& root=node(script.root);if(root.flags&3)continue;
        const auto& begin=node(root.value);if(text(begin.string_offset)!="begin")continue;
        std::vector<Call> calls;bool owns=false;std::set<unsigned> seen;
        for(auto next=begin.next;next!=null_datum;next=node(next).next){
            check(seen.insert(next).second,"Scenery lighting sibling cycle");const auto& expression=node(next);
            if(expression.flags&3)continue;const auto& function=node(expression.value);
            Call call{text(function.string_offset),{}};
            if(call.name!="object_uses_cinematic_lighting"&&call.name!="cinematic_lighting_set_primary_light"&&
                call.name!="cinematic_lighting_set_secondary_light"&&call.name!="cinematic_lighting_set_ambient_light")continue;
            std::set<unsigned> arguments;
            for(auto arg=function.next;arg!=null_datum;arg=node(arg).next){
                check(arguments.insert(arg).second&&arguments.size()<=8,"Scenery lighting argument cycle/budget");call.arguments.push_back(node(arg));
            }
            if(call.name=="object_uses_cinematic_lighting"&&call.arguments.size()==2){
                const auto& target=call.arguments[0];const auto& active=call.arguments[1];
                owns|=(target.flags&1)&&!(target.flags&4)&&std::int16_t(target.value)==placement.name&&
                    active.opcode==5&&(active.value&255);
            }
            calls.push_back(std::move(call));
        }
        if(owns){check(selected.empty(),"Scenery has ambiguous authored cinematic lighting");selected=std::move(calls);}
    }
    check(!selected.empty(),"Scenery GPU consumer lacks its actual authored cinematic lighting script");
    unsigned lights{};
    for(const auto& call:selected){
        const bool primary=call.name=="cinematic_lighting_set_primary_light",secondary=call.name=="cinematic_lighting_set_secondary_light";
        if(!primary&&!secondary&&call.name!="cinematic_lighting_set_ambient_light")continue;
        check(call.arguments.size()==(primary||secondary?5u:3u),"Authored cinematic light arity differs");
        for(const auto& arg:call.arguments)check(arg.opcode==6&&(arg.flags&1)&&!(arg.flags&4),"Authored cinematic light needs literal floats");
        const auto f=[&](unsigned n){return std::bit_cast<float>(call.arguments[n].value);};
        if(primary)source_campaign_primary_light_tu15(world,f(0),f(1),{f(2),f(3),f(4)});
        else if(secondary)source_campaign_secondary_light_tu15(world,f(0),f(1),{f(2),f(3),f(4)});
        else source_campaign_ambient_light_tu15(world,{f(0),f(1),f(2)});
        ++lights;
    }
    check(lights==3,"Scenery GPU consumer lost an authored cinematic light");
    source_campaign_object_lighting_tu15(world,object,true);
    ObjectLightingState result;const auto at=word(world.memory(),0x4eabc4u);
    std::memcpy(&result,world.memory().view(at+0x1cu,sizeof(result)).data(),sizeof(result));return result;
}
}

struct SceneryAnimationGpuConsumer::Impl {
    const Cache& cache;MultiplayerObjectRuntime& objects;unsigned object,motion_tick;
    SourceCampaignModel& model;Mesh mesh;ObjectLightingState lighting;ObjectColors colors;
    std::filesystem::path output;std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window{nullptr,&SDL_DestroyWindow};
    std::unique_ptr<Renderer> renderer;std::ostringstream validation;std::streambuf* previous{};
    std::vector<Transform> initial_nodes;std::map<std::pair<unsigned,int>,Image> frames;
    std::optional<Image> empty_reference;unsigned submissions{},uploads{},instruction_words{},draws{};
    bool motion_checked{},finished{};
    Impl(const Cache& c,const LevelContent& content,const ScenarioObject& placement,MultiplayerObjectRuntime& o,
        unsigned id,unsigned motion,SourceCampaignModel& m):cache(c),objects(o),object(id),motion_tick(motion),model(m),
        lighting(authored_lighting(content,placement,o,id)){
        const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch&&*scratch,"Scenery GPU captures require owned scratch");
        output=std::filesystem::path(scratch)/"scenery-gpu";std::filesystem::create_directories(output);
        for(const auto* name:{"HALO2_PIPELINE_CACHE_DIRECTORY","XDG_CACHE_HOME","MESA_SHADER_CACHE_DIR","__GL_SHADER_DISK_CACHE_PATH"}){
            const auto path=output/name;std::filesystem::create_directories(path);
            check(SDL_setenv(name,path.c_str(),1)==0,"Cannot bind scenery GPU cache to its owned test storage");
        }
        model.prepare_materials(cache,mesh);check(!mesh.materials.empty(),"Scenery GPU material preparation is empty");
        const auto selected=model.variant_name(objects.object_model_regions(object).selection.variant_index);
        colors=source_object_initial_colors(cache.load_source_object_color_definition(placement.definition),placement.position,selected);
        check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
        window.reset(SDL_CreateWindow("Native MCC scenery animation",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN));
        check(bool(window),SDL_GetError());previous=std::cerr.rdbuf(validation.rdbuf());
        try{renderer.reset(new Renderer(window.get(),mesh,true,{},nullptr,{},nullptr,HALO2_SHADER_DIR));}
        catch(...){restore();throw;}
    }
    void restore(){if(previous){std::cerr.rdbuf(previous);previous=nullptr;}}
    ~Impl(){renderer.reset();restore();if(!finished&&!validation.str().empty())std::cerr<<validation.str();window.reset();SDL_Quit();}
    ModelView view(SourceModelShaderView& shader){
        auto& world=objects.tu15_world();const auto out=world.prepare_source_call().gp[4]+0x100;
        const std::array<std::byte,56> clear{};world.write_source_call_input(out,clear);
        auto abi=world.prepare_source_call(std::array{out,out+52});world.run_actual(0x16fe00u,abi);
        check(abi.gp[0]&255,"Actual native animated camera sample is absent");Transform camera;
        std::memcpy(&camera,world.memory().view(out,sizeof(camera)).data(),sizeof(camera));
        const auto director=word(world.memory(),0x51df6cu);const float fov=Bytes(world.memory().view(director+0x10,4)).f32(0);
        const auto angle=source_camera_angles(fov).projection_angle;const auto planes=source_campaign_clip_planes_tu15(world);
        SourceFrustumProjectionInput input;input.projection_angle=angle;input.outer=input.safe={0,0,480,640};input.scenario_present=true;input.scenario_type=0;
        const auto projection=source_frustum_projection(input);
        const auto bank=source_view_projection({camera.position,camera.basis[0],camera.basis[2],planes.x,planes.y,{},projection.projection_program});
        shader={source_shader_coverage_camera(bank),source_shader_lod_view(0,480,false,false),
            source_geometry_lod_policy(1).coverage_multiplier,source_shader_lod_player_multiplier(1),projection.view_modifier};
        ModelView result;result.width=640;result.height=480;result.full_surface=true;result.camera.position=camera.position;
        result.camera.oriented=true;result.camera.forward=camera.basis[0];result.camera.up=camera.basis[2];result.camera.projection_vertical_angle=angle;
        const auto clip=source_camera_projection(angle,{0,0,640,480},{0,0,640,480},false);
        result.camera.projection_clip_scale=clip.clip_scale;result.camera.projection_clip_offset=clip.clip_offset;
        result.source_view_shader_constants.emplace();result.source_view_shader_constants->world=source_view_shader_constants(bank,planes.y,false);
        result.source_view_shader_constants->reflection=result.source_view_shader_constants->world;
        const auto fp=source_view_shader_constants(bank,planes.y,true);std::copy_n(fp.begin(),4,result.source_view_shader_constants->foreground.begin());
        result.projection_clip_planes=planes;result.source_w_distance=planes.y;
        result.source_render_policy=source_geometry_lod_policy(1);
        result.completed_material_upload_observer=[this](const auto& u){++uploads;instruction_words+=u.uploaded_vertex_instruction_words;};
        result.completed_object_draw_observer=[this](const auto& d){check(d.object_index==0&&d.owner_constants&&d.count,"Scenery GPU draw lost its independent live owner");++draws;};
        return result;
    }
    Image capture(const ModelView& view,const std::string& name){
        const auto path=output/(name+".ppm");
        for(unsigned n=0;n<5;++n)if(renderer->draw(view.camera,path,{},false,{},{},{},{&view,1})){
            ++submissions;auto image=read_image(path);std::filesystem::remove(path);return image;
        }
        throw FormatError("Scenery GPU frame was not submitted");
    }
};

SceneryAnimationGpuConsumer::SceneryAnimationGpuConsumer(const halo2::Cache& c,const halo2::LevelContent& content,
    const halo2::ScenarioObject& p,halo2::MultiplayerObjectRuntime& o,unsigned object,unsigned tick,halo2::SourceCampaignModel& model)
    :impl_(std::make_unique<Impl>(c,content,p,o,object,tick,model)){}
SceneryAnimationGpuConsumer::~SceneryAnimationGpuConsumer()=default;
void SceneryAnimationGpuConsumer::sample(const halo2::SourceObjectDrawCollector& collector,std::span<const halo2::Transform> nodes){
    auto& s=*impl_;auto& world=s.objects.tu15_world();const auto tick=word(world.memory(),word(world.memory(),0x51df54u)+8);
    SourceModelShaderView shader;auto view=s.view(shader);const auto sphere=s.objects.object_sphere(s.object);
    const auto coverage=source_model_shader_collector(sphere.center,sphere.radius,shader);
    auto vertices=s.model.draw(s.object,collector,nodes,coverage,shader);
    check(!vertices.empty(),"Actual authored scenery shader selection emitted no triangles");
    if(s.initial_nodes.empty())s.initial_nodes.assign(nodes.begin(),nodes.end());
    const ShaderInput input=[&](unsigned name){return s.objects.object_shader_input(s.object,name);};
    for(auto& material:s.mesh.materials)animate_forward_material(material,float(tick)/30.f,input);
    s.renderer->update_material_constants(s.mesh.materials);
    ObjectLightingState lighting;
    std::memcpy(&lighting,world.memory().view(word(world.memory(),0x4eabc4u)+0x1cu,sizeof(lighting)).data(),sizeof(lighting));
    ModelObjectDraw object{vertices,lighting,s.colors,input};object.source_object=s.object;
    view.objects={&object,1};view.effect_time=float(tick)/30.f;
    const auto image=s.capture(view,"tick-"+std::to_string(tick));
    const auto key=std::pair{tick,int(source_campaign_cinematic_animation_time_tu15(world,s.object,false))};
    if(const auto found=s.frames.find(key);found!=s.frames.end())check(difference(image,found->second)==0,"Scenery GPU image differs after native checkpoint replay");
    else s.frames.emplace(key,image);
    if(tick==s.motion_tick&&!s.motion_checked){
        const auto frozen_vertices=s.model.draw(s.object,collector,s.initial_nodes,coverage,shader);
        object.vertices=frozen_vertices;const auto frozen=s.capture(view,"frozen-geometry");
        view.objects={};const auto empty=s.capture(view,"empty-owner");
        s.empty_reference=empty;
        const auto motion=difference(image,frozen),visible=difference(image,empty);
        check(motion>100&&visible>100,"Scenery GPU motion/visible-owner control produced too few changed pixels: "+std::to_string(motion)+"/"+std::to_string(visible));
        s.motion_checked=true;std::cout<<"PASS actual scenery GPU pixels: tick="<<tick<<" motion="<<motion<<" visible_owner="<<visible<<'\n';
    }
}
void SceneryAnimationGpuConsumer::finish(){
    auto& s=*impl_;check(s.motion_checked&&s.submissions>=9&&s.uploads&&s.draws&&s.instruction_words==0,
        "Scenery GPU consumer lacks motion/owner/draw/upload/native-program evidence");
    SourceModelShaderView shader;const auto empty=s.capture(s.view(shader),"retired-owner");
    check(s.empty_reference&&difference(empty,*s.empty_reference)==0,"Scenery GPU retained stale geometry after native retirement");
    s.renderer.reset();s.restore();check(s.validation.str().empty(),"Scenery Vulkan/renderer diagnostics: "+s.validation.str().substr(0,2048));
    s.finished=true;std::cout<<"PASS native authored scenery GPU consumer: submissions="<<s.submissions<<" uploads="<<s.uploads
        <<" draws="<<s.draws<<" executable_vertex_words="<<s.instruction_words<<"; raw captures removed\n";
}
