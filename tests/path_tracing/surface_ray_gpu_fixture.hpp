#pragma once
#include "halo2/path_tracing/sky_lighting.hpp"

namespace surface_ray_gpu_detail {
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 scale(Vec3 a,float b){return {a.x*b,a.y*b,a.z*b};}
float dot(Vec3 a,Vec3 b){return (a.x*b.x+a.y*b.y)+a.z*b.z;}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Vec3 unit(Vec3 a){return scale(a,1.f/std::sqrt(dot(a,a)));}
struct Metric {double reference{},candidate{},absolute_error{};unsigned pixels{};};
Metric compare(const std::vector<std::uint8_t>& reference,const std::vector<std::uint8_t>& candidate,VkExtent2D extent) {
    require(reference.size()==candidate.size() && reference.size()==std::size_t(extent.width)*extent.height*4,
        "Surface ray fixture frame sizes differ");Metric result;
    for(unsigned y=extent.height/8;y<extent.height*7/8;++y)for(unsigned x=extent.width/8;x<extent.width*7/8;++x) {
        const auto at=(std::size_t(y)*extent.width+x)*4;
        if(reference[at]<8)continue;
        result.reference+=reference[at];result.candidate+=candidate[at];
        result.absolute_error+=std::abs(int(reference[at])-int(candidate[at]));++result.pixels;
    }
    require(result.pixels>=128,"Surface ray fixture has no substantial lit receiver coverage");
    result.reference/=result.pixels;result.candidate/=result.pixels;result.absolute_error/=result.pixels;return result;
}
void print(const char* name,const Metric& metric) {
    std::cout<<"\""<<name<<"\":{\"reference\":"<<metric.reference<<",\"candidate\":"<<metric.candidate
        <<",\"absolute_error\":"<<metric.absolute_error<<",\"pixels\":"<<metric.pixels<<'}';
}
Camera face_camera(std::span<const Vertex> face) {
    const auto origin=face[0].position;
    const auto right=unit(sub(face[1].position,origin));
    auto normal=unit(cross(sub(face[1].position,origin),sub(face[2].position,origin)));
    if(dot(normal,face[0].normal)<0)normal=scale(normal,-1);
    const auto up=unit(cross(normal,right));
    float x0=0,x1=0,y0=0,y1=0;
    for(const auto& vertex:face) {
        const auto relative=sub(vertex.position,origin);const auto x=dot(relative,right),y=dot(relative,up);
        x0=std::min(x0,x);x1=std::max(x1,x);y0=std::min(y0,y);y1=std::max(y1,y);
    }
    const float sx=1.6f/(x1-x0),sy=1.6f/(y1-y0);
    Camera camera;camera.matrix={};camera.matrix[15]=1;
    const std::array<Vec3,3> axes{scale(right,sx),scale(up,sy),scale(normal,.01f)};
    for(unsigned row=0;row<3;++row) {
        camera.matrix[row]=axes[row].x;camera.matrix[4+row]=axes[row].y;camera.matrix[8+row]=axes[row].z;
        camera.matrix[12+row]=-dot(axes[row],origin)+(row==0?-sx*(x0+x1)*.5f:row==1?-sy*(y0+y1)*.5f:.5f);
    }
    const auto center=scale(add(add(face[0].position,face[1].position),face[2].position),1.f/3);
    const auto eye=add(center,scale(normal,5));camera.eye={eye.x,eye.y,eye.z,1};return camera;
}
}

// Uses the real production world.frag, its derivatives and its BVH traversal.
// The empty-BVH controls retain precisely the same primary raster surface.
int smooth_terminator_gpu(const std::filesystem::path& shaders,const char* source_map=nullptr,VkExtent2D extent={width,height}) {
    using namespace surface_ray_gpu_detail;
    Fixture gpu;gpu.initialize(shaders,extent);gpu.renderer->set_quality(1,1);
    const auto measure=[&](const auto& reference,const auto& candidate){return compare(reference,candidate,extent);};
    auto blocker=white_material();blocker.diffuse=solid_texture({0,0,0,255});
    const std::array materials{white_material(),blocker};
    const Vec3 shading{.8f,0,-.6f};
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,shading,0);
    pt::Light sun;sun.direction=Vec3{.979795897f,0,.2f};sun.color={.8f,.8f,.8f};
    const auto direct_reference=gpu.draw(pt::build_scene({},materials,std::span(&sun,1)),primary);
    const auto direct_same=gpu.draw(pt::build_scene(primary,materials,std::span(&sun,1)),primary);
    auto external=primary;quad(external,-4,-4,4,4,.55f,{0,0,-1},1);
    const auto direct_external=gpu.draw(pt::build_scene(external,materials,std::span(&sun,1)),primary);
    auto nearby=primary;quad(nearby,-4,-4,4,4,.5008f,{0,0,-1},1);
    const auto direct_nearby=gpu.draw(pt::build_scene(nearby,materials,std::span(&sun,1)),primary);
    auto ordinary=primary;for(auto& vertex:ordinary)vertex.normal={0,0,-1};
    auto ordinary_sun=sun;ordinary_sun.direction=Vec3{0,0,-1};
    const auto ordinary_reference=gpu.draw(pt::build_scene({},materials,std::span(&ordinary_sun,1)),ordinary);
    const auto ordinary_same=gpu.draw(pt::build_scene(ordinary,materials,std::span(&ordinary_sun,1)),ordinary);
    auto ordinary_blocked=ordinary;quad(ordinary_blocked,-4,-4,4,4,.45f,{0,0,-1},1);
    const auto ordinary_external=gpu.draw(pt::build_scene(ordinary_blocked,materials,std::span(&ordinary_sun,1)),ordinary);
    const Vec3 white_sky{.3f,.3f,.3f};
    const auto diffuse_reference=gpu.draw(pt::build_scene({},materials),primary,white_sky);
    const auto diffuse_same=gpu.draw(pt::build_scene(primary,materials),primary,white_sky);
    const auto diffuse_external=gpu.draw(pt::build_scene(external,materials),primary,white_sky);
    const auto direct=measure(direct_reference,direct_same),shadow=measure(direct_reference,direct_external),near=measure(direct_reference,direct_nearby);
    const auto front=measure(ordinary_reference,ordinary_same),front_shadow=measure(ordinary_reference,ordinary_external);
    const auto diffuse=measure(diffuse_reference,diffuse_same),diffuse_shadow=measure(diffuse_reference,diffuse_external);
    std::optional<Metric> source;std::size_t source_first{};
    if(source_map) {
        Cache cache(source_map);require(cache.is_mcc(),"Terminator source control requires prepared MCC");const auto mesh=cache.load_bsp(0);
        pt::SkyLighting skies;skies.set_active(true);const auto* sky=skies.select(cache,0);require(sky,"Terminator source control has no authored sky");
        const auto frame=object_sky_light_frame(*sky);const auto direction=unit(frame.incident);
        std::vector<Vertex> selected;
        for(std::size_t first=0;first<mesh.vertices.size();first+=3) {
            const auto& name=mesh.materials[mesh.vertices[first].material].name;
            if(name.find("blood_cliffs")==std::string::npos && name.find("coag_ground")==std::string::npos)continue;
            const auto a=mesh.vertices[first],b=mesh.vertices[first+1],c=mesh.vertices[first+2];
            auto ng=unit(cross(sub(b.position,a.position),sub(c.position,a.position)));
            const auto ns=unit(add(add(a.normal,b.normal),c.normal));if(dot(ng,ns)<0)ng=scale(ng,-1);
            if(dot(ng,direction)>=-.01f || dot(ns,direction)<.1f)continue;
            const auto center=scale(add(add(a.position,b.position),c.position),1.f/3);
            std::array<Vertex,3> opaque_face{a,b,c};
            for(auto& vertex:opaque_face){vertex.material=0;vertex.color={1,1,1};}
            const auto face=pt::build_scene(opaque_face,materials);pt::Hit hit;
            if(!pt::intersect(face,add(center,scale(ns,.0002f)),direction,1.e10f,hit))continue;
            selected.assign(opaque_face.begin(),opaque_face.end());
            source_first=first;break;
        }
        require(!selected.empty(),"Actual source control contains no reproducible smooth-normal own-face occlusion");
        const auto camera=face_camera(selected);auto source_sun=sun;source_sun.direction=direction;
        const auto reference=gpu.draw(pt::build_scene({},materials,std::span(&source_sun,1)),selected,{},camera);
        const auto same=gpu.draw(pt::build_scene(selected,materials,std::span(&source_sun,1)),selected,{},camera);
        source=measure(reference,same);
    }
    std::cout<<"Smooth surface rays: {\"width\":"<<extent.width<<",\"height\":"<<extent.height<<',';
    print("direct_same_face",direct);std::cout<<',';print("direct_external_blocker",shadow);
    std::cout<<',';print("direct_separate_near_plane",near);std::cout<<',';print("ordinary_same_face",front);
    std::cout<<',';print("ordinary_external_blocker",front_shadow);std::cout<<',';print("diffuse_same_face",diffuse);
    std::cout<<',';print("diffuse_external_blocker",diffuse_shadow);
    if(source){std::cout<<',';print("actual_source_face",*source);std::cout<<",\"source_first_vertex\":"<<source_first;}
    std::cout<<",\"validation_errors\":"<<gpu.validation_errors.load()<<'}'<<std::endl;
    require(direct.reference>100 && direct.absolute_error<2,"Smooth source face shadowed its own sun-facing shading normal");
    require(shadow.candidate<3 && near.candidate<3,"Surface plane exclusion leaked a separate opaque direct-light blocker");
    require(front.absolute_error<2 && front_shadow.candidate<3,"Surface plane exclusion changed ordinary front-facing shadow visibility");
    require(diffuse.reference>70 && diffuse.absolute_error<1,"Smooth source face blocked its own constant-sky diffuse continuation");
    require(diffuse_shadow.candidate<diffuse.reference-5,"Surface plane exclusion leaked a real diffuse-continuation blocker");
    if(source)require(source->absolute_error<3,"Actual Coagulation face retained geometric-terminator self shadow");
    require(gpu.validation_errors.load()==0,"Surface ray fixture reported Vulkan errors");return 0;
}
