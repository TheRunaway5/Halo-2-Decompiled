#include "halo2/renderer.hpp"
#include "halo2/debug_panel.hpp"
#include "halo2/effect_light.hpp"
#include "halo2/world_material.hpp"
#include "halo2/frontend_material.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/menu_world_depth.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
using namespace halo2;
namespace {
void require(bool value,const std::string& why){if(!value)throw FormatError(why);}
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 mul(Vec3 a,float s){return {a.x*s,a.y*s,a.z*s};}
Vec3 norm(Vec3 a){const auto n=std::sqrt(dot(a,a));require(n>0,"raw fixture zero basis");return mul(a,1/n);}
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image image(const std::filesystem::path& path){std::ifstream f(path,std::ios::binary);std::string magic;unsigned maximum;Image r;f>>magic>>r.width>>r.height>>maximum;f.get();require(magic=="P6"&&maximum==255,"raw light capture schema");r.pixels.resize(std::size_t(r.width)*r.height*3);f.read(reinterpret_cast<char*>(r.pixels.data()),r.pixels.size());require(bool(f),"raw light capture truncated");return r;}
Image capture(Renderer& renderer,std::span<ModelView> views,const std::filesystem::path& path){for(unsigned i=0;i<5;++i)if(renderer.draw(views[0].camera,path,{},false,{},{},{},views))return image(path);throw FormatError("raw light capture did not submit");}
struct Row {unsigned command,kind,section,instance,part,shader,lod,implementation,vertex,classification;std::string key;};
}
int run(int argc,char** argv){
    require(argc==3,"raw renderer test needs original binding directory and fresh evidence directory");
    const std::filesystem::path root=HALO2_SOURCE_DIR,fixtures=argv[1],output=argv[2];
    if(!std::filesystem::exists(root/"assets/disc/maps/lockout.map"))return 77;
    require(!std::filesystem::exists(output),"raw light evidence directory exists");std::filesystem::create_directories(output);
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());struct Sdl{~Sdl(){SDL_Quit();}} sdl;
    Cache cache(root/"assets/disc/maps/lockout.map");const auto bsp=cache.load_bsp(0);
    const auto tag=std::ranges::find_if(cache.tags(),[](const auto& t){return t.group=="ligh" && t.name=="effects\\objects\\weapons\\grenade\\frag_grenade\\grenade_frag_explosion";});
    require(tag!=cache.tags().end(),"raw light authored frag definition missing");const auto definition=load_effect_light(cache,tag->datum);
    std::ifstream manifest(fixtures/"manifest.tsv");std::string line;require(bool(std::getline(manifest,line))&&line=="H2_SPHERE_RAW_BINDINGS_V1","raw light original selection schema");
    std::vector<Row> rows;std::set<std::string> keys;
    while(std::getline(manifest,line)){Row r;std::istringstream f(line);require(bool(f>>r.command>>r.kind>>r.section>>r.instance>>r.part>>r.shader>>r.lod>>r.implementation>>r.vertex>>r.classification>>r.key),"raw light original selection row");if(keys.insert(r.key).second)rows.push_back(r);}
    require(rows.size()==3,"raw renderer needs both source classes and texture families");
    std::ofstream report(output/"measurements.tsv");report<<"program\twidth\theight\tviews\tlit_channels\tblend_error\tisolation_error\traw_packed_different_channels\n";
    unsigned layouts=0;std::size_t shadowed_total=0;
    for(const auto& row:rows){
        std::vector<Vertex> scene;
        for(auto v:bsp.vertices)if(v.bsp_cluster==(row.kind?-2-int(row.instance):int(row.section))&&v.source_section==row.section&&v.source_part==row.part){v.material=0;v.lightmap=0;v.bsp_cluster=-1;scene.push_back(v);}
        require(scene.size()>=3,"raw renderer actual source part has no expanded reference triangles");
        Vec3 center{},normal{};bool found=false;
        for(unsigned t=0;t<scene.size();t+=3){const auto n=cross(sub(scene[t+1].position,scene[t].position),sub(scene[t+2].position,scene[t].position));if(dot(n,n)>1.e-8f){center=mul(add(add(scene[t].position,scene[t+1].position),scene[t+2].position),1.f/3);normal=norm(n);if(dot(normal,scene[t].normal)<0)normal=mul(normal,-1);found=true;break;}}
        require(found,"raw renderer actual source part is degenerate");
        const auto eye=add(center,mul(normal,.8f)),forward=mul(normal,-1);
        const auto up=std::abs(normal.z)>.9f?Vec3{0,1,0}:Vec3{0,0,1};
        const auto position=add(center,mul(normal,.3f));
        EffectLightState light;light.size=1;light.diffuse={.4f,.25f,.125f};light.brightness=1;
        EffectLightViewFades fades;fades.diffuse=fades.specular=fades.secondary=fades.overall=fades.volume=1;
        const auto uniforms=source_effect_light_view_uniforms(definition,light,position,{1,0,0},{0,0,1},fades,1,{});
        auto authored=cache.load_material(row.shader);authored.selected_shader_lod=row.lod;
        const auto prepared=prepare_effect_light_diffuse_material(cache,authored,definition,uniforms);require(prepared.size()==1,"raw source selected pass cardinality");
        const auto& material=prepared[0];const auto pass=load_menu_world_source_pass(cache,row.shader,{row.lod,12,0,1});
        auto base=authored;require(prepare_dynamic_material(cache,base,{},false),"raw controlled base pass missing");
        base.forward_program=scope_copy_frame({}).program;base.diffuse={1,1,11,std::vector<std::byte>(4,std::byte{24}),{}};base.diffuse.pixels[3]=std::byte{255};
        if(!material.accumulation_texture)base.environment_pass.reset();
        base.world_overlays_before.clear();base.world_overlays_after.clear();base.world_bloom_passes.clear();base.world_planar_passes={};base.planar_fog_pass.reset();base.accumulation_texture=false;
        base.forward_blend=0;base.forward_raster.depth_enable=base.forward_raster.depth_write=1;base.forward_raster.depth_function=0x203;
        for(auto& frames:base.animated_textures)frames.clear();
        Mesh mesh;mesh.materials={base,material};mesh.vertices.resize(3);
        auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(SDL_CreateWindow("Original raw sphere Vulkan draw",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);require(bool(window),SDL_GetError());
        Renderer renderer(window.get(),mesh,true);const auto atmosphere=cache.load_bsp_atmosphere(0);renderer.scene_bsp_atmosphere(atmosphere);
        const auto geometry=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,row.kind?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster,row.section));
        MenuWorldSourceOwner owner;MenuWorldSourceDraw source;
        if(row.kind){const auto instance=load_menu_world_source_bsp_owner(cache,0,row.instance);owner={MenuWorldSourceOwnerKind::bsp_instance,instance.bsp_datum,row.instance,{}};source=menu_world_source_draw(geometry,row.part,MenuWorldSourceTopology::triangle_list,std::span(&instance.rows,1));}
        else{owner={MenuWorldSourceOwnerKind::bsp_cluster,geometry->owner_datum(),row.section,{}};source=menu_world_source_cluster_draw(geometry,row.part,MenuWorldSourceTopology::triangle_list);}
        Image single_base,single_lit;
        for(const auto layout:std::array<std::array<unsigned,4>,3>{{{640,480,1,0},{853,479,4,0},{640,480,1,1}}}){
            const auto [width,height,count,aa]=layout;SDL_SetWindowSize(window.get(),width,height);SDL_PumpEvents();renderer.resized();
            auto display=renderer.debug_panel().display_settings();display.anti_aliasing=aa;renderer.debug_panel().display_settings(display);
            std::array<ModelView,4> views;std::array<std::vector<EffectLightRawDraw>,4> draws;std::array<std::vector<Vertex>,4> packed;
            std::array<EffectLightDraw,4> packed_draws;
            for(unsigned i=0;i<count;++i){auto& v=views[i];const unsigned cols=count==1?1:2;
                const unsigned left=i%cols*width/cols,right=(i%cols+1)*width/cols,top=i/cols*height/cols,bottom=(i/cols+1)*height/cols;
                v.left=left*640.f/width;v.top=top*480.f/height;v.width=(right-left)*640.f/width;v.height=(bottom-top)*480.f/height;v.full_surface=true;
                v.camera.position=eye;v.camera.oriented=true;v.camera.forward=forward;v.camera.up=up;v.camera.field_of_view=70;v.vertices=scene;
                v.atmosphere=BspFogSample{};v.atmosphere->bloom={1,.9f,0,0};
                SourceFrustumProjectionInput pi;pi.projection_angle=1.2217305f;pi.outer=pi.safe={std::int16_t(top),std::int16_t(left),std::int16_t(bottom),std::int16_t(right)};
                const auto camera=source_view_projection({eye,forward,up,.015625f,1024.f,{},source_frustum_projection(pi).projection_program});
                const auto bank=source_view_shader_bank(camera,1024.f,eye,{float(left),float(top),float(right-left),float(bottom-top)},false);
                v.source_view_shader_constants.emplace();std::copy_n(bank.begin(),7,v.source_view_shader_constants->world.begin());v.source_view_shader_constants->reflection=v.source_view_shader_constants->world;
                MenuWorldSourceConstants previous{};std::copy(bank.begin(),bank.end(),previous.begin());const auto constants=source_effect_light_raw_constants(previous,material);
                const auto binding=menu_world_source_binding_class(cache,owner,source,row.vertex,row.classification,constants);
                MenuWorldRasterContext raster;raster.viewport_reset({width,height});raster.game_initialize_offset();raster.layer_begin(MenuWorldRasterLayer::sphere_diffuse_12);raster.entry_begin({false,false,false});
                const auto state=raster.snapshot({MenuWorldRasterPrimitive::ordinary_triangles,MenuWorldRasterPolygon::fill,std::array<std::int32_t,2>{1,1},true});raster.entry_end();
                const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,false});
                draws[i].push_back(source_effect_light_raw_draw(1,material,pass,binding,state,{0,16777215.f,depth.minimum_w,depth.maximum_w},{1,1}));draws[i][0].point_light_position=position;draws[i].push_back(draws[i][0]);
                packed[i]=scene;for(auto& vertex:packed[i])vertex.material=1;packed_draws[i]=source_effect_light_draw(1,material,packed[i]);
            }
            const auto stem=row.key+"-"+std::to_string(width)+"x"+std::to_string(height)+"-views"+std::to_string(count)+"-"+std::to_string(layouts++);auto active=std::span(views).first(count);
            const auto baseline=capture(renderer,active,output/(stem+"-base.ppm"));
            for(unsigned i=0;i<count;++i)views[i].raw_effect_lights=std::span(draws[i]).first(1);
            const auto once=capture(renderer,active,output/(stem+"-raw.ppm"));
            if(count==1 && !aa){single_base=baseline;single_lit=once;}
            if(aa) {
                const auto centroid=[](const Image& a,const Image& b) {
                    std::array<double,3> sum{};
                    for(unsigned y=0;y<a.height;++y)for(unsigned x=0;x<a.width;++x) {
                        const auto p=(std::size_t(y)*a.width+x)*3;double weight{};
                        for(unsigned c=0;c<3;++c)weight+=std::max(0,int(b.pixels[p+c])-int(a.pixels[p+c]));
                        sum[0]+=x*weight;sum[1]+=y*weight;sum[2]+=weight;
                    }
                    require(sum[2]>100,"AA illumination centroid has no positive light");
                    return std::array{sum[0]/sum[2],sum[1]/sum[2]};
                };
                const auto normal=centroid(single_base,single_lit),sampled=centroid(baseline,once);
                require(std::abs(normal[0]-sampled[0])<2 && std::abs(normal[1]-sampled[1])<2,
                    "SSAA moved raw point-light illumination in the output image");
            }
            auto graphics=renderer.debug_panel().display_settings();
            graphics.graphics_mode=pstencil::GraphicsMode::pStencil;renderer.debug_panel().display_settings(graphics);
            const auto stencil=capture(renderer,active,output/(stem+"-pstencil.ppm"));
            std::size_t shadowed=0;
            for(std::size_t p=0;p<once.pixels.size();++p) {
                require(int(stencil.pixels[p])<=int(once.pixels[p])+1,"pStencil introduced illumination");
                shadowed+=int(once.pixels[p])-int(stencil.pixels[p])>1;
            }
            shadowed_total+=shadowed;
            graphics.graphics_mode=pstencil::GraphicsMode::Original;renderer.debug_panel().display_settings(graphics);
            const auto restored=capture(renderer,active,output/(stem+"-restored.ppm"));
            require(restored.pixels==once.pixels,"Original pixels changed after live pStencil round trip");
            std::cout<<stem<<" pStencil_shadowed_channels="<<shadowed<<" Original_roundtrip=exact\n";
            for(unsigned i=0;i<count;++i)views[i].raw_effect_lights=draws[i];
            const auto twice=capture(renderer,active,output/(stem+"-twice.ppm"));
            unsigned blend_error=0;std::size_t lit=0;
            for(std::size_t p=0;p<once.pixels.size();++p){const int delta=int(once.pixels[p])-baseline.pixels[p];lit+=delta>2;blend_error=std::max(blend_error,unsigned(std::abs(int(twice.pixels[p])-std::clamp(int(baseline.pixels[p])+2*delta,0,255))));}
            require(lit>100,"actual raw source sphere draw produced no illumination");require(blend_error<=3,"raw source sphere additive blend differs");
            auto dark=light;dark.diffuse={};const auto dark_uniforms=source_effect_light_view_uniforms(definition,dark,position,{1,0,0},{0,0,1},fades,1,{});const auto dark_material=prepare_effect_light_diffuse_material(cache,authored,definition,dark_uniforms).at(0);
            const auto dark_constants=source_effect_light_raw_constants(draws[count-1][0].source.constants(),dark_material);
            const auto dark_binding=menu_world_source_binding_class(cache,owner,source,row.vertex,row.classification,dark_constants);
            draws[count-1][0]=source_effect_light_raw_draw(1,dark_material,pass,dark_binding,draws[count-1][0].raster,draws[count-1][0].clip_range,{1,1});
            for(unsigned i=0;i<count;++i)views[i].raw_effect_lights=std::span(draws[i]).first(1);
            const auto isolated=capture(renderer,active,output/(stem+"-isolated.ppm"));unsigned isolation_error=0;
            for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){const bool last=count==1||(x>=width/2&&y>=height/2);const auto& expected=last?baseline:once;for(unsigned c=0;c<3;++c){const auto p=(std::size_t(y)*width+x)*3+c;isolation_error=std::max(isolation_error,unsigned(std::abs(int(isolated.pixels[p])-expected.pixels[p])));}}
            require(isolation_error<=1,"raw source sphere constants leaked between views");
            for(unsigned i=0;i<count;++i){views[i].raw_effect_lights={};views[i].effect_lights=std::span(&packed_draws[i],1);}
            const auto expanded=capture(renderer,active,output/(stem+"-packed-control.ppm"));std::size_t differences=0;for(std::size_t p=0;p<once.pixels.size();++p)differences+=once.pixels[p]!=expanded.pixels[p];
            report<<row.key<<'\t'<<width<<'\t'<<height<<'\t'<<count<<'\t'<<lit<<'\t'<<blend_error<<'\t'<<isolation_error<<'\t'<<differences<<'\n';report.flush();
            std::cout<<stem<<" lit_channels="<<lit<<" blend_error="<<blend_error<<" isolation_error="<<isolation_error<<" raw_packed_differences="<<differences<<'\n';
        }
    }
    require(layouts==9,"pStencil real geometry layout coverage changed");std::cout<<"PASS 9 pStencil real Cache layouts including SSAA with live Original round trips; actual Cache class0/1 geometry, volume and live texture target, additive blend, per-view isolation and resize; controlled service fixture, no original gameplay frame\n";return 0;
}
int main(int argc,char** argv){std::ostringstream validation;auto* old=std::cerr.rdbuf(validation.rdbuf());try{const auto result=run(argc,argv);std::cerr.rdbuf(old);require(validation.str().find("Vulkan validation:")==std::string::npos,"raw source sphere Vulkan validation diagnostics");return result;}catch(const std::exception& e){std::cerr.rdbuf(old);std::cerr<<validation.str()<<e.what()<<'\n';return 1;}}
