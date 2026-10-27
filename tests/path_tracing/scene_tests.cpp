#include "halo2/path_tracing/scene.hpp"
#include "halo2/path_tracing/material_residency.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace halo2;
namespace pt=halo2::path_tracing;
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F&& f,const char* message){try{f();}catch(const FormatError&){return;}throw std::runtime_error(message);}
Material white(unsigned alpha=255) {
    Material m;m.diffuse={1,1,11,{std::byte{255},std::byte{255},std::byte{255},std::byte(alpha)}};return m;
}
void triangle(std::vector<Vertex> &vertices,float x,float z,unsigned material=0) {
    for(Vec3 p:std::array<Vec3,3>{{{x,0,z},{x+1,0,z},{x,1,z}}}) {
        Vertex v;v.position=p;v.normal={0,0,1};v.color={1,1,1};v.uv={.25f,.25f};v.material=material;vertices.push_back(v);
    }
}
std::vector<std::uint32_t> permutation(const pt::Scene& scene) {
    std::vector<std::uint32_t> result;
    for(const auto& t:scene.triangles)result.push_back(std::bit_cast<std::uint32_t>(t.p0[3]));
    return result;
}
auto topology(const pt::Scene& scene) {
    std::vector<std::array<std::uint32_t,4>> result;
    for(const auto& node:scene.nodes)result.push_back(node.children);
    return result;
}
void valid_hierarchy(const pt::Scene& scene) {
    if(scene.triangles.empty()){require(scene.nodes.empty(),"Empty geometry retained a BVH root");return;}
    require(!scene.nodes.empty(),"Admitted geometry lost its BVH root");
    std::vector<unsigned> visited(scene.nodes.size()),covered(scene.triangles.size());
    const auto visit=[&](auto&& self,std::uint32_t index,unsigned depth)->void {
        require(index<scene.nodes.size() && depth<=30,"Refreshed BVH escaped its node/depth bounds");
        require(!visited[index]++,"Refreshed BVH reused a node or introduced a cycle");
        const auto& node=scene.nodes[index];
        for(unsigned a=0;a<3;++a)require(std::isfinite(node.minimum[a]) &&
            std::isfinite(node.maximum[a]) && node.minimum[a]<=node.maximum[a],
            "Refreshed BVH retained invalid spatial bounds");
        ShaderVector exact_low{INFINITY,INFINITY,INFINITY,0},exact_high{-INFINITY,-INFINITY,-INFINITY,0};
        const auto include=[&](const auto& low,const auto& high) {
            for(unsigned a=0;a<3;++a) {
                exact_low[a]=std::min(exact_low[a],low[a]);exact_high[a]=std::max(exact_high[a],high[a]);
            }
        };
        if(node.children[2]==1) {
            const auto first=node.children[0],count=node.children[1];
            require(count && first<covered.size() && count<=covered.size()-first,
                "Refreshed BVH leaf escaped its triangle range");
            for(std::uint32_t t=first;t<first+count;++t) {
                require(!covered[t]++,"Refreshed BVH leaves overlap");
                const auto& value=scene.triangles[t];
                for(const auto* p:{&value.p0,&value.p1,&value.p2})include(*p,*p);
            }
        }else {
            require(node.children[2]==0,"Refreshed BVH has an unknown node kind");
            for(unsigned child=0;child<2;++child) {
                const auto next=node.children[child];
                require(next>index && next<scene.nodes.size(),"Refreshed BVH lost preorder child ownership");
                include(scene.nodes[next].minimum,scene.nodes[next].maximum);
                self(self,next,depth+1);
            }
        }
        for(unsigned a=0;a<3;++a)require(node.minimum[a]==exact_low[a] && node.maximum[a]==exact_high[a],
            "Refreshed BVH bounds differ from exact current leaf/child unions");
    };
    visit(visit,0,0);
    require(std::all_of(visited.begin(),visited.end(),[](auto n){return n==1;}) &&
        std::all_of(covered.begin(),covered.end(),[](auto n){return n==1;}),
        "Refreshed BVH omitted nodes or admitted triangles");
}
void same_payload(const pt::Scene& actual,const pt::Scene& expected) {
    valid_hierarchy(actual);valid_hierarchy(expected);
    auto left=actual.triangles,right=expected.triangles;
    const auto ordinal=[](const auto& a,const auto& b) {
        return std::bit_cast<std::uint32_t>(a.p0[3])<std::bit_cast<std::uint32_t>(b.p0[3]);
    };
    std::sort(left.begin(),left.end(),ordinal);std::sort(right.begin(),right.end(),ordinal);
    require(left.size()==right.size(),"Refit changed source triangle admission");
    for(std::size_t n=0;n<left.size();++n) {
        const auto& a=left[n];const auto& b=right[n];
        require(a.p0==b.p0 && a.p1==b.p1 && a.p2==b.p2 && a.n0==b.n0 &&
            a.n1==b.n1 && a.n2==b.n2 && a.uv01==b.uv01 && a.uv2==b.uv2 && a.metadata==b.metadata,
            "Refit retained stale position/normal/UV/color/material payload");
    }
    require(actual.texture_texel_count==expected.texture_texel_count &&
        std::equal(actual.pixels.begin(),actual.pixels.begin()+actual.texture_texel_count,expected.pixels.begin()),
        "Refit changed current source texture payload");
    const auto emitter_ordinals=[](const pt::Scene& scene) {
        std::vector<std::uint32_t> result;
        for(unsigned n=0;n<scene.emissive_count;++n) {
            const auto index=scene.pixels.at(scene.texture_texel_count+n);
            result.push_back(std::bit_cast<std::uint32_t>(scene.triangles.at(index).p0[3]));
        }
        std::sort(result.begin(),result.end());return result;
    };
    require(emitter_ordinals(actual)==emitter_ordinals(expected),"Refit retained stale emitter indices");
}
void same_hits(const pt::Scene& actual,const pt::Scene& expected,std::span<const Vertex> vertices) {
    for(std::size_t first=0;first<vertices.size();first+=3) {
        const auto& a=vertices[first].position;const auto& b=vertices[first+1].position;
        const auto& c=vertices[first+2].position;
        const Vec3 origin{(a.x+b.x+c.x)/3,(a.y+b.y+c.y)/3,(a.z+b.z+c.z)/3+4};
        pt::Hit hit,reference;const auto got=pt::intersect(actual,origin,{0,0,-1},16,hit);
        require(got==pt::intersect(expected,origin,{0,0,-1},16,reference) &&
            (!got || (std::abs(hit.distance-reference.distance)<1.e-5f &&
                std::bit_cast<std::uint32_t>(actual.triangles[hit.triangle].p0[3])==
                std::bit_cast<std::uint32_t>(expected.triangles[reference.triangle].p0[3]))),
            "Refreshed BVH intersection differs from a fresh source build");
    }
}
void refit_regressions() {
    std::vector<Material> materials{white(),white()};
    materials[1].forward_vertex=2;materials[1].forward_diffuse={.08f,.04f,.02f,1};
    std::vector<Vertex> original;
    for(unsigned n=0;n<128;++n)triangle(original,float((n*37)%128)*3,0,n%2);
    auto scene=pt::build_scene(original,materials);
    const auto original_topology=topology(scene);
    const auto original_permutation=permutation(scene);
    require(!std::is_sorted(original_permutation.begin(),original_permutation.end()),
        "Refit fixture did not exercise a nontrivial source permutation");
    const float baseline=scene.nodes[0].minimum[3];
    require(baseline>0 && std::isfinite(baseline),"Fresh BVH lacks a finite quality baseline");
    pt::update_geometry(scene,original);
    require(topology(scene)==original_topology && permutation(scene)==original_permutation &&
        scene.nodes[0].minimum[3]==baseline,"Stable geometry did not retain its BVH topology/permutation");
    auto moved=original;
    for(auto& v:moved){v.position.x+=7;v.position.y-=2;v.position.z-=3;}
    pt::update_geometry(scene,moved);
    require(topology(scene)==original_topology && permutation(scene)==original_permutation,
        "Rigid translation rebuilt an unchanged BVH partition");
    same_payload(scene,pt::build_scene(moved,materials));same_hits(scene,pt::build_scene(moved,materials),moved);
    pt::Hit hit;
    require(pt::intersect(scene,{7.2f,-1.8f,2},{0,0,-1},8,hit) && hit.distance==5,
        "Refit retained stale translated bounds or hit distance");

    auto changed=moved;
    for(std::size_t n=0;n<changed.size();++n) {
        auto& v=changed[n];v.material=1-v.material;v.normal={.2f,.3f,.9f};
        v.color={.15f+float(n%3)*.1f,.35f,.7f};v.uv={float(n%7)*.125f,float(n%5)*.2f};
    }
    materials[0].forward_raster.alpha_test=1;materials[0].forward_raster.alpha_function=0x204;
    materials[0].forward_raster.alpha_reference=128;
    materials[0].animated_textures[0]={white(0).diffuse,white(255).diffuse};
    for(unsigned frame=0;frame<2;++frame) {
        materials[0].texture_frames[0]=frame;
        pt::refresh_materials_for_geometry(scene,materials,changed);pt::update_geometry(scene,changed);
        const auto fresh=pt::build_scene(changed,materials);same_payload(scene,fresh);same_hits(scene,fresh,changed);
        require(topology(scene)==original_topology && permutation(scene)==original_permutation,
            "Current material payload unnecessarily changed spatial BVH ownership");
        const auto& cutout=changed[3].position;
        require(pt::intersect(scene,{cutout.x+.2f,cutout.y+.2f,2},{0,0,-1},8,hit)==bool(frame),
            "Refitted geometry ignored the selected animated alpha frame");
    }

    auto replacement=original;
    for(unsigned n=0;n<128;++n)for(unsigned corner=0;corner<3;++corner) {
        auto& v=replacement[n*3+corner];v.position.x=float((n*53)%128)*3+11+(corner==1?1:0);
        v.position.y+=float(n%3)*2;v.position.z=-float(n%7)*.25f;
        v.normal={.3f,.4f,.8f};v.color={.2f,.5f,.8f};v.uv={.1f*corner,.2f*corner};
    }
    pt::update_geometry(scene,replacement);
    auto fresh=pt::build_scene(replacement,materials);same_payload(scene,fresh);same_hits(scene,fresh,replacement);
    triangle(replacement,500,-1,1);pt::update_geometry(scene,replacement);
    fresh=pt::build_scene(replacement,materials);same_payload(scene,fresh);same_hits(scene,fresh,replacement);
    replacement[0].source_stipple_level=0;pt::update_geometry(scene,replacement);
    fresh=pt::build_scene(replacement,materials);same_payload(scene,fresh);same_hits(scene,fresh,replacement);
    replacement[0].source_stipple_level=15;replacement[3].source_stipple_level=0;
    pt::update_geometry(scene,replacement);fresh=pt::build_scene(replacement,materials);
    same_payload(scene,fresh);same_hits(scene,fresh,replacement);
    replacement[6].position=replacement[7].position;pt::update_geometry(scene,replacement);
    fresh=pt::build_scene(replacement,materials);same_payload(scene,fresh);same_hits(scene,fresh,replacement);

    const auto clean=pt::build_scene(original,materials);
    const auto recovers=[&](auto corrupt) {
        auto damaged=clean;corrupt(damaged);pt::update_geometry(damaged,original);
        same_payload(damaged,clean);same_hits(damaged,clean,original);
        require(topology(damaged)==topology(clean) && permutation(damaged)==permutation(clean),
            "Corrupt refit metadata did not fall back to a fresh BVH build");
    };
    recovers([](auto& s){s.triangles[0].p0[3]=std::bit_cast<float>(0xffffffffu);});
    recovers([](auto& s){s.triangles[0].p0[3]=s.triangles[1].p0[3];});
    recovers([](auto& s){s.nodes[0].children[0]=0;});
    recovers([](auto& s){for(auto& n:s.nodes)if(n.children[2]==1){n.children[1]=std::uint32_t(s.triangles.size()+1);break;}});
    recovers([](auto& s){s.nodes[0].minimum[3]=std::numeric_limits<float>::quiet_NaN();});

    const auto recovers_source_error=[&](auto invalidate) {
        auto damaged=clean;auto invalid=original;invalidate(invalid);
        rejects([&]{pt::update_geometry(damaged,invalid);},"Invalid replacement geometry was accepted during refit");
        require(damaged.nodes.empty(),"Caught geometry error left an old BVH addressing partial replacement triangles");
        pt::update_geometry(damaged,original);same_payload(damaged,clean);same_hits(damaged,clean,original);
        require(topology(damaged)==topology(clean) && permutation(damaged)==permutation(clean),
            "Valid geometry did not rebuild after a caught admission error");
    };
    recovers_source_error([](auto& v){v[15].position.x=std::numeric_limits<float>::quiet_NaN();});
    recovers_source_error([](auto& v){v[15].material=1-v[15].material;});

    auto scattered=original;
    for(unsigned n=0;n<128;++n)for(unsigned corner=0;corner<3;++corner)
        scattered[original_permutation[n]*3+corner].position.x=float((n%2)*64+n/2)*3+(corner==1?1:0);
    scene=clean;pt::update_geometry(scene,scattered);fresh=pt::build_scene(scattered,materials);
    require(permutation(scene)!=permutation(clean),"Severe refit overlap did not trigger a new SAH partition");
    same_payload(scene,fresh);same_hits(scene,fresh,scattered);
}
}
int main()try {
    std::vector<Material> materials{white()};std::vector<Vertex> vertices;
    for(unsigned n=0;n<24;++n)triangle(vertices,float(n)*2,0);
    auto scene=pt::build_scene(vertices,materials);pt::Hit hit;
    require(scene.nodes.size()>1,"Scene did not build an acceleration hierarchy");
    for(unsigned n=0;n<24;++n) {
        require(pt::intersect(scene,{float(n)*2+.2f,.2f,2},{0,0,-1},3,hit) && hit.distance==2,
            "BVH traversal lost a source triangle after partitioning");
        require(!pt::intersect(scene,{float(n)*2+1.5f,.2f,2},{0,0,-1},3,hit),"BVH admitted an empty-space hit");
    }
    require(!pt::intersect(scene,{.2f,.2f,2},{0,0,-1},1,hit),"Shadow ray ignored its finite light distance");
    require(pt::intersect(scene,{.2f,.2f,2},{0,0,-1},3,hit),"Parallel AABB axes rejected an inside ray");
    const auto pixels=scene.pixels;const auto offset=scene.materials[0].texture[0];
    vertices.clear();triangle(vertices,0,-3);pt::update_geometry(scene,vertices);
    require(scene.pixels==pixels && scene.materials[0].texture[0]==offset,"Dynamic update redecoded or shifted source textures");
    require(pt::intersect(scene,{.2f,.2f,2},{0,0,-1},8,hit) && hit.distance==5,"BVH retained stale posed geometry");
    vertices.clear();triangle(vertices,0,1,0);triangle(vertices,0,0,1);
    materials={white(0),white()};materials[0].forward_raster.alpha_test=1;
    materials[0].forward_raster.alpha_function=0x204;materials[0].forward_raster.alpha_reference=128;
    scene=pt::build_scene(vertices,materials);
    require(pt::intersect(scene,{.2f,.2f,2},{0,0,-1},3,hit) && hit.distance==2,
        "Transparent cutout blocked a shadow/reflection ray before the solid triangle");
    const bool comparisons[]{false,false,true,true,false,false,true,true};
    for(unsigned mode=0;mode<8;++mode) {
        materials[0]=white(128);materials[0].forward_raster.alpha_test=1;
        materials[0].forward_raster.alpha_function=0x200+mode;materials[0].forward_raster.alpha_reference=128;
        scene=pt::build_scene(std::span(vertices).first(3),materials);
        require(pt::intersect(scene,{.2f,.2f,2},{0,0,-1},3,hit)==comparisons[mode],"Authored alpha comparison changed during any-hit traversal");
    }
    materials[0]=white();materials[0].forward_blend=1;scene=pt::build_scene(vertices,materials);
    require(scene.triangles.size()==1,"Transparent original pass became an opaque ray blocker");
    materials[0]=white();materials[0].water_pass=true;scene=pt::build_scene(vertices,materials);
    require(scene.triangles.size()==1,"Water original pass became an opaque ray blocker");
    Material matte=white();const auto rough=pt::material_response(matte);
    require(rough.response[1]==0 && rough.response[2]==0 && rough.response[0]>=.8f,
        "Ordinary diffuse material acquired an unsolicited shiny coat");
    Material visor=white();visor.name="objects\\characters\\masterchief\\shaders\\masterchief_visor";
    visor.environment_pass=std::make_shared<Material>();visor.environment_pass->forward_vertex=4;
    const auto polished=pt::material_response(visor);
    require(std::abs(polished.response[1]-.4f)<1.e-6f && std::abs(polished.response[2]-.2f)<1.e-6f,
        "Visor lost the authenticated source facing/glancing reflection strengths");
    require(polished.response[0]<rough.response[0] && polished.base[0]>polished.base[2],
        "Visor lost its independent roughness/tinted reflection response");
    materials={white(),white()};materials[1].forward_vertex=2;materials[1].forward_diffuse={.5f,.1f,.02f,1};
    scene=pt::build_scene(vertices,materials);
    require(scene.emissive_count==1 && scene.pixels.size()==scene.texture_texel_count+1,
        "Authored emissive geometry was omitted from direct-light sampling");
    auto uniform=scene.uniform();require(uniform.environment[0]==0 && uniform.environment[1]==0 && uniform.environment[2]==0,
        "Scene added an invented sky light");
    require(std::bit_cast<std::uint32_t>(uniform.environment[3])==scene.texture_texel_count,
        "GPU emissive index offset lost exact integer transport");
    require(scene.triangles[scene.pixels[scene.texture_texel_count]].metadata[0]==1,"Emissive index no longer follows BVH reorder");
    const auto texels=std::vector(scene.pixels.begin(),scene.pixels.begin()+scene.texture_texel_count);
    pt::update_geometry(scene,std::span(vertices).first(3));
    require(scene.emissive_count==0 && scene.pixels==texels,"Frame-local rebuild retained a deleted emitter");
    const pt::Light light{{0,0,2},4,{1,.5f,.25f},2};pt::update_geometry(scene,std::span(vertices).first(3),std::span(&light,1));
    require(scene.lights.size()==1 && scene.lights[0].position_radius[3]==4 && scene.lights[0].color_intensity[3]==2,
        "Source light radius/color/intensity changed at the GPU boundary");
    auto directional=light;directional.radius=0;directional.direction=Vec3{0,0,2};
    pt::update_geometry(scene,std::span(vertices).first(3),std::span(&directional,1));
    require(scene.lights.size()==1 && scene.lights[0].position_radius==ShaderVector{0,0,1,-1} &&
        scene.lights[0].color_intensity==ShaderVector{1,.5f,.25f,2},
        "Source sky direction/energy lost its distinct normalized GPU representation");
    directional.direction=Vec3{};
    rejects([&]{pt::update_geometry(scene,vertices,std::span(&directional,1));},"Zero source sky direction was accepted");
    directional.direction=Vec3{std::numeric_limits<float>::quiet_NaN(),0,1};
    rejects([&]{pt::update_geometry(scene,vertices,std::span(&directional,1));},"Nonfinite source sky direction was accepted");
    auto invalid=vertices;invalid[0].material=2;
    rejects([&]{pt::build_scene(invalid,materials);},"Unknown material was accepted");
    invalid=vertices;invalid[1].material=1;
    rejects([&]{pt::build_scene(invalid,materials);},"Mixed material triangle was accepted");
    invalid=vertices;invalid[0].position.x=std::numeric_limits<float>::quiet_NaN();
    rejects([&]{pt::build_scene(invalid,materials);},"Non-finite source triangle was accepted");
    rejects([&]{pt::build_scene(std::span(vertices).first(2),materials);},"Partial triangle was accepted");
    rejects([&]{scene.uniform({-1,0,0});},"Negative source environment was accepted");
    materials={white()};vertices.clear();triangle(vertices,0,0);
    for(auto &vertex:vertices){vertex.color={.05f,.1f,.15f};vertex.lightmap=1;}
    scene=pt::build_scene(vertices,materials);
    require(scene.triangles[0].metadata[1]==0xffffffffu,"Baked per-vertex BSP radiance survived as secondary albedo");
    for(auto &vertex:vertices){vertex.lightmap=0;vertex.bsp_cluster=0;}
    pt::update_geometry(scene,vertices);
    require(scene.triangles[0].metadata[1]==0xffffffffu,"Untextured BSP radiance survived as secondary albedo");
    for(auto &vertex:vertices)vertex.bsp_cluster=-1;
    pt::update_geometry(scene,vertices);
    require(scene.triangles[0].metadata[1]!=0xffffffffu,"Actual object vertex tint was discarded with baked lighting");
    materials={white(0),white()};materials[0].forward_raster.alpha_test=1;
    materials[0].forward_raster.alpha_function=0x204;materials[0].forward_raster.alpha_reference=128;
    vertices.clear();triangle(vertices,0,1,0);triangle(vertices,0,0,1);scene=pt::build_scene(vertices,materials);
    require(pt::intersect(scene,{.2f,.2f,2},{0,0,-1},3,hit) && hit.distance==2,"Initial animated alpha frame was not transparent");
    const auto atlas_size=scene.pixels.size();const auto atlas_offset=scene.materials[0].texture[0];
    materials[0].diffuse.pixels[3]=std::byte{255};++materials[0].texture_frames[0];
    pt::refresh_materials(scene,materials);
    require(scene.pixels.size()==atlas_size && scene.materials[0].texture[0]==atlas_offset,
        "Same-size animated source image did not reuse its atlas allocation");
    require(pt::intersect(scene,{.2f,.2f,2},{0,0,-1},3,hit) && hit.distance==1,
        "Secondary rays retained an old animated alpha frame");
    materials[0].animated_textures[0]={white(0).diffuse,white(255).diffuse};materials[0].texture_frames[0]=0;
    pt::refresh_materials(scene,materials);
    require(pt::intersect(scene,{.2f,.2f,2},{0,0,-1},3,hit) && hit.distance==2,
        "Secondary albedo ignored the selected animated image and reused Material.diffuse");
    materials[0].texture_frames[0]=1;pt::refresh_materials(scene,materials);
    require(pt::intersect(scene,{.2f,.2f,2},{0,0,-1},3,hit) && hit.distance==1,
        "A frame-index-only animation did not refresh secondary albedo");
    materials[0].texture_frames[0]=2;
    rejects([&]{pt::refresh_materials(scene,materials);},"Out-of-range authored texture frame was accepted");
    materials[0].texture_frames[0]=1;
    materials[0].forward_vertex=2;materials[0].forward_diffuse={.3f,.1f,.05f,1};pt::refresh_materials(scene,materials);
    require(scene.emissive_count==1,"Current authored emissive constants did not refresh");
    const auto refreshed_pixels=scene.pixels;pt::refresh_materials(scene,materials);
    require(scene.pixels==refreshed_pixels,"Unchanged material refresh changed the immutable atlas");
    materials={white(),white(),white()};
    materials[2].name="font/unused-ui-atlas";materials[2].diffuse.width=materials[2].diffuse.height=8192;
    vertices.clear();triangle(vertices,0,0,0);
    scene=pt::build_scene(vertices,materials);
    require(scene.materials.size()==3 && scene.material_texel_capacities[0]!=0 &&
        scene.material_texel_capacities[1]==0 && scene.material_texel_capacities[2]==0 && scene.texture_texel_count==2,
        "Unreferenced UI images were decoded or source material slot identities changed");
    pt::refresh_materials(scene,materials);
    require(scene.texture_texel_count==2 && scene.material_texel_capacities[2]==0,
        "Legacy current-frame refresh activated an unreferenced oversized UI image");
    triangle(vertices,3,0,1);
    rejects([&]{pt::update_geometry(scene,vertices);},"New dynamic geometry silently used an unadmitted material placeholder");
    pt::refresh_materials_for_geometry(scene,materials,vertices);pt::update_geometry(scene,vertices);
    require(scene.material_texel_capacities[1]!=0 && scene.material_texel_capacities[2]==0 &&
        scene.triangles[0].metadata[0]!=scene.triangles[1].metadata[0] && scene.texture_texel_count==3,
        "New dynamic geometry did not activate its actual root material slot");
    triangle(vertices,6,0,2);
    rejects([&]{pt::refresh_materials_for_geometry(scene,materials,vertices);},
        "An actually used malformed source image was silently replaced with a placeholder");
    auto empty=pt::build_scene({},materials);
    require(empty.materials.size()==3 && empty.texture_texel_count==1 && empty.pixels.size()==1 &&
        std::all_of(empty.material_texel_capacities.begin(),empty.material_texel_capacities.end(),[](auto count){return count==0;}),
        "Empty ray geometry eagerly decoded the resident UI/material bank");
    materials={white()};vertices.clear();triangle(vertices,0,0);
    scene=pt::build_scene(vertices,materials);
    require(pt::intersect(scene,{.2f,.2f,2},{0,0,-1},3,hit),"Visible source primitive did not enter the BVH");
    vertices[0].source_stipple_level=0;
    pt::update_geometry(scene,vertices);
    require(scene.triangles.empty() && !pt::intersect(scene,{.2f,.2f,2},{0,0,-1},3,hit),
        "Fully faded source primitive remained an invisible ray shadow/reflection blocker");
    auto faded=pt::build_scene(vertices,materials);
    require(faded.triangles.empty() && faded.material_texel_capacities[0]==0 && faded.texture_texel_count==1,
        "Fully faded geometry unnecessarily admitted its material image");
    for(unsigned level:std::array<unsigned,3>{1,8,15}) {
        vertices[0].source_stipple_level=level;
        pt::refresh_materials_for_geometry(scene,materials,vertices);pt::update_geometry(scene,vertices);
        require(pt::intersect(scene,{.2f,.2f,2},{0,0,-1},3,hit),
            "Partially stippled source primitive lost its documented solid secondary-ray coverage");
    }
    refit_regressions();
    std::cout<<"PASS Path Tracing BVH/refit, finite shadows, alpha any-hit, posed refresh, authored matte/visor response and emitters\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
