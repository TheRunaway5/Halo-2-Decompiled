#include "halo2/renderer.hpp"
#include "halo2/effect_light.hpp"
#include "halo2/world_material.hpp"
#include "halo2/frontend_material.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool b,const char *message){if(!b)throw FormatError(message);}
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image read(const std::filesystem::path &path){
    std::ifstream f(path,std::ios::binary);Image out;std::string magic;unsigned maximum;
    f>>magic>>out.width>>out.height>>maximum;f.get();require(magic=="P6" && maximum==255,"Invalid light capture");
    out.pixels.resize(std::size_t(out.width)*out.height*3);f.read(reinterpret_cast<char*>(out.pixels.data()),out.pixels.size());
    require(bool(f),"Truncated light capture");return out;
}
Image capture(Renderer &r,std::span<const ModelView> views,const std::filesystem::path &path){
    for(unsigned attempt=0;attempt<5;++attempt)if(r.draw(views[0].camera,path,{},false,{},{},{},views))return read(path);
    throw FormatError("Light capture did not submit");
}
std::vector<Vertex> quad(unsigned material,float aspect){
    std::vector<Vertex> out;
    for(auto uv:std::array{Vec2{0,0},Vec2{1,1},Vec2{1,0},Vec2{0,0},Vec2{0,1},Vec2{1,1}}){
        Vertex v;v.material=material;v.position={2,(1-uv.x*2)*2*aspect,(1-uv.y*2)*2};v.uv=uv;
        v.normal={-1,0,0};v.tangent={0,-1,0};v.binormal={0,0,-1};v.color={1,1,1};out.push_back(v);
    }
    return out;
}
std::array<ShaderVector,7> camera_bank(unsigned width,unsigned height,unsigned view,unsigned count){
    const unsigned columns=count==1?1:2,rows=count==1?1:2;
    const unsigned left=(view%columns)*width/columns,right=(view%columns+1)*width/columns;
    const unsigned top=(view/columns)*height/rows,bottom=(view/columns+1)*height/rows;
    SourceFrustumProjectionInput p;p.projection_angle=1.5707963267948966f;
    p.outer=p.safe={std::int16_t(top),std::int16_t(left),std::int16_t(bottom),std::int16_t(right)};
    SourceViewProjectionInput c;c.forward={1,0,0};c.up={0,0,1};c.near_distance=.015625f;c.far_distance=1024;
    c.projection_program=source_frustum_projection(p).projection_program;
    return source_view_shader_constants(source_view_projection(c),1024,false);
}
struct View {
    std::vector<Vertex> scene,receiver;
    std::array<EffectLightDraw,2> lights;
};
}
int verify(int argc,char**argv){
    const std::filesystem::path source=HALO2_SOURCE_DIR;
    if(!std::filesystem::exists(source/"assets/disc/maps/lockout.map"))return 77;
    const bool authored_colors=argc==3 && std::string_view(argv[1])=="--authored-colors";
    require(argc==2 || authored_colors,"Supply light GPU evidence directory, optionally --authored-colors");
    const std::filesystem::path output=argv[authored_colors?2:1];std::filesystem::create_directories(output);
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());struct Sdl{~Sdl(){SDL_Quit();}} sdl;
    const auto original=authored_colors?OriginalData::load_xbe(source/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03"):nullptr;
    Cache cache(source/"assets/disc/maps/lockout.map",source/"assets/disc/maps",original);
    const auto find=[&](const char* group,const char* name){const auto it=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto&t){return t.group==group && t.name==name;});require(it!=cache.tags().end(),"Missing authored light GPU fixture tag");return it->datum;};
    unsigned total_layouts=0;
    for(const char *shader:{"scenarios\\shaders\\multi\\lockout\\trim_vents",
                           "scenarios\\shaders\\multi\\lockout\\composite_block_snow"}) {
    const std::array<const char*,2> light_names{
        "effects\\objects\\weapons\\grenade\\frag_grenade\\grenade_frag_explosion",
        "effects\\objects\\weapons\\grenade\\plasma_grenade\\plasma_grenade_explosion"};
    for(unsigned grenade=0;grenade<(authored_colors?2u:1u);++grenade) {
    const auto authored=cache.load_material(find("shad",shader));
    const auto fixture_name=(authored_colors?std::string(grenade?"plasma-":"frag-"):std::string{})+
        std::string(shader).substr(std::string(shader).find_last_of('\\')+1);
    const auto definition=load_effect_light(cache,find("ligh",light_names[grenade]));
    EffectLightState state;state.size=1;state.brightness=1;state.diffuse={.5f,.5f,.5f};
    if(authored_colors)state=source_effect_light_state(definition,1,0,30,original->fade_curve);
    EffectLightViewFades fades;fades.diffuse=fades.specular=fades.secondary=fades.overall=fades.volume=1;
    const auto uniforms=source_effect_light_view_uniforms(definition,state,{1.5f,0,0},{1,0,0},{0,0,1},fades,1,{});
    auto light=prepare_effect_light_diffuse_material(cache,authored,definition,uniforms);require(light.size()==1,"Authored light pass cardinality differs");
    {
        const auto &m=light[0];VertexProgramState v;v.attributes=m.program_constants[0];
        const auto camera=camera_bank(640,480,0,1);std::copy(camera.begin(),camera.end(),v.constants.begin());
        v.constants[7]={0,0,0,767.8125f};const auto viewport=viewport_constants({0,0,640,480});
        std::copy(viewport.begin(),viewport.end(),v.constants.begin()+8);
        std::copy(m.program_constants[1].begin(),m.program_constants[1].end(),v.constants.begin()+18);
        std::copy(m.vertex_bank->constants.begin(),m.vertex_bank->constants.end(),v.constants.begin()+m.vertex_bank->first_constant);
        v.constants[172]={1,1,0,0};
        require(bool(m.vertex_declaration),"Sphere diffuse source declaration missing");
        for(unsigned reg=0;reg<m.vertex_declaration->semantics.size();++reg)switch(m.vertex_declaration->semantics[reg]){
        case 0:v.attributes[reg]={2,0,0,1};break;case 3:v.attributes[reg]={.5f,.5f,0,1};break;
        case 4:v.attributes[reg]={-1,0,0,1};break;case 5:v.attributes[reg]={0,-1,0,1};break;case 6:v.attributes[reg]={0,0,-1,1};break;
        default:throw FormatError("Unresolved sphere GPU fixture semantic");}
        execute_vertex_program(decode_vertex_program(m.vertex_program),v);
        std::cout<<"source sphere texture modes="<<std::hex<<m.forward_program.words[54]<<std::dec<<" declaration";
        for(auto semantic:m.vertex_declaration->semantics)std::cout<<' '<<semantic;
        for(unsigned output:{3u,4u,9u,10u,11u,12u}){std::cout<<" output"<<output;for(float x:v.outputs[output])std::cout<<' '<<x;}
        std::cout<<'\n';
    }
    auto base=authored;require(prepare_dynamic_material(cache,base,{},false),"Controlled base source VP unavailable");
    base.forward_program=scope_copy_frame({}).program;
    base.diffuse={1,1,11,std::vector<std::byte>(4,std::byte{24}),{}};base.diffuse.pixels[3]=std::byte{255};
    // The ordinary sphere variant consumes the original texture-mapping
    // target. Retain its authored dynamic-lighting child so the renderer's
    // existing base/lighting schedule populates that target before the light.
    if(light[0].accumulation_texture)require(bool(base.environment_pass),"Ordinary light fixture needs original texture-mapping producer");
    else base.environment_pass.reset();
    base.world_overlays_before.clear();base.world_overlays_after.clear();base.world_bloom_passes.clear();base.world_planar_passes={};base.planar_fog_pass.reset();base.accumulation_texture=false;
    base.forward_blend=0;base.forward_raster.depth_enable=base.forward_raster.depth_write=1;base.forward_raster.depth_function=0x203;base.forward_raster.cull=0;
    for(auto &frames:base.animated_textures)frames.clear();
    Mesh mesh;mesh.materials={base,light[0]};mesh.vertices.resize(3);
    auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(SDL_CreateWindow("Original sphere diffuse GPU services",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
    require(bool(window),SDL_GetError());const auto atmosphere=cache.load_bsp_atmosphere(0);
    Renderer renderer(window.get(),mesh,true);
    // This optional fixture measures layer12's authored colors and additive
    // blend directly. BSP bloom is nonlinear above its threshold and is a
    // separate service; comparing its final output to a linear blend equation
    // would conflate two different boundaries. Keep W-depth/view banks below.
    renderer.scene_bsp_atmosphere(atmosphere);
    unsigned layouts=0;
    for(auto dimensions:std::array{std::array{640,480},std::array{1280,720},std::array{853,479},std::array{640,480}})for(unsigned count:{1u,4u}){
        const auto [width,height]=dimensions;SDL_SetWindowSize(window.get(),width,height);SDL_PumpEvents();renderer.resized();
        std::array<View,4> fixture;std::array<ModelView,4> views;
        for(unsigned i=0;i<count;++i){auto &v=fixture[i];auto &model=views[i];const unsigned columns=count==1?1:2,rows=count==1?1:2;
            model.left=float(i%columns)*640/columns;model.top=float(i/columns)*480/rows;model.width=640.f/columns;model.height=480.f/rows;model.full_surface=true;
            model.camera.oriented=true;model.camera.forward={1,0,0};model.camera.up={0,0,1};model.camera.field_of_view=90;
            model.atmosphere=BspFogSample{};
            if(authored_colors)model.atmosphere->bloom={1,.9f,0,0};
            model.source_view_shader_constants.emplace();model.source_view_shader_constants->world=camera_bank(width,height,i,count);
            model.source_view_shader_constants->reflection=model.source_view_shader_constants->world;
            v.scene=quad(0,float(width)/height);v.receiver=quad(1,float(width)/height);model.vertices=v.scene;
            v.lights[0]=source_effect_light_draw(1,light[0],v.receiver);v.lights[1]=v.lights[0];
        }
        const auto stem=fixture_name+"-"+std::to_string(width)+"x"+std::to_string(height)+"-views"+std::to_string(count)+"-"+std::to_string(layouts++);
        const auto active=std::span(views).first(count);
        const auto baseline=capture(renderer,active,output/(stem+"-base.ppm"));
        for(unsigned i=0;i<count;++i)views[i].effect_lights={fixture[i].lights.data(),1};
        const auto once=capture(renderer,active,output/(stem+"-once.ppm"));
        for(unsigned i=0;i<count;++i)views[i].effect_lights=fixture[i].lights;
        const auto twice=capture(renderer,active,output/(stem+"-twice.ppm"));
        std::size_t lit=0;unsigned error=0;
        for(std::size_t p=0;p<baseline.pixels.size();++p){const int delta=int(once.pixels[p])-baseline.pixels[p];lit+=delta>2;
            const int expected=std::clamp(int(baseline.pixels[p])+2*delta,0,255);error=std::max(error,unsigned(std::abs(int(twice.pixels[p])-expected)));
        }
        require(lit>100,"Authored sphere diffuse GPU draw produced no light");require(error<=3,"Sphere diffuse repeated-draw blend differs");
        // A zero bank for only the final view must leave every other view's
        // independently queued program/constants intact.
        auto dark=state;dark.diffuse={};const auto zero_uniforms=source_effect_light_view_uniforms(definition,dark,{1.5f,0,0},{1,0,0},{0,0,1},fades,1,{});
        const auto zero=prepare_effect_light_diffuse_material(cache,authored,definition,zero_uniforms);
        fixture[count-1].lights[0]=source_effect_light_draw(1,zero[0],fixture[count-1].receiver);
        for(unsigned i=0;i<count;++i)views[i].effect_lights={fixture[i].lights.data(),1};
        const auto isolated=capture(renderer,active,output/(stem+"-isolated.ppm"));
        unsigned isolation_error=0;
        for(unsigned y=0;y<unsigned(height);++y)for(unsigned x=0;x<unsigned(width);++x){const bool last=count==1 || (x>=unsigned(width)/2 && y>=unsigned(height)/2);const auto &expected=last?baseline:once;
            for(unsigned c=0;c<3;++c){const auto at=(std::size_t(y)*width+x)*3+c;isolation_error=std::max(isolation_error,unsigned(std::abs(int(isolated.pixels[at])-expected.pixels[at])));}
        }
        require(isolation_error<=1,"Sphere diffuse constants leaked between views");
        std::cout<<stem<<" lit_channels="<<lit<<" blend_error="<<error<<" isolation_error="<<isolation_error<<'\n';
    }
    total_layouts+=layouts;
    }
    }
    std::cout<<"PASS "<<total_layouts<<" original sphere diffuse GPU layouts, textured/ordinary programs; service fixture, not original gameplay appearance\n";
    return 0;
}
int main(int argc,char**argv){
    std::ostringstream validation;auto*previous=std::cerr.rdbuf(validation.rdbuf());
    try {
        const auto result=verify(argc,argv);std::cerr.rdbuf(previous);
        require(validation.str().find("Vulkan validation:")==std::string::npos,
            "Sphere light Vulkan/synchronization diagnostics");
        return result;
    }catch(const std::exception&e){std::cerr.rdbuf(previous);std::cerr<<validation.str()<<e.what()<<'\n';return 1;}
}
