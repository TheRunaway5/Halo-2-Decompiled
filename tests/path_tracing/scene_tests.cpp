#include "halo2/path_tracing/scene.hpp"
#include "halo2/path_tracing/material_residency.hpp"
#include "halo2/path_tracing/material_surface.hpp"
#include "halo2/path_tracing/emitter_sampling.hpp"
#include "halo2/path_tracing/surface.hpp"
#include "halo2/path_tracing/surface_material.hpp"
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
Texture environment_cube(unsigned side,unsigned levels,unsigned red) {
    Texture cube{side,side,11,{},{},6};
    std::vector<std::byte> pixels;
    for(unsigned face=0;face<6;++face)for(unsigned mip=0;mip<levels;++mip) {
        const auto size=side>>mip;
        const auto offset=pixels.size(),bytes=std::size_t(size)*size*4;
        cube.mips.push_back({size,size,offset,bytes});
        pixels.resize(offset+bytes);
        for(std::size_t i=offset;i<pixels.size();i+=4) {
            pixels[i]=std::byte(red+face+mip*8);
            pixels[i+3]=std::byte{255};
        }
    }
    cube.pixels=std::move(pixels);
    return cube;
}
void authored_surface_response() {
    using Class=SourceSurfaceMaterial;
    require(pt::surface_material_class("tough_terrain_sand")==Class::Sand &&
        pt::surface_material_class("hard_metal_thick_for")==Class::Metal &&
        pt::surface_material_class("painted_metal_plate")==Class::CoatedMetal &&
        pt::surface_material_class("visor_gold")==Class::Lens &&
        pt::surface_material_class("cloth_uniform")==Class::Fabric &&
        pt::surface_material_class("skin_elite")==Class::Skin &&
        pt::surface_material_tag_class("scenarios\\bitmaps\\nature\\ground\\sand_grey")==Class::Sand &&
        pt::terrain_bitmap_class("scenarios\\bitmaps\\detail_maps\\nature\\grass\\lush")==Class::Grass &&
        pt::terrain_bitmap_class("scenarios\\bitmaps\\detail_maps\\concrete\\poured")==Class::Concrete &&
        pt::surface_material_tag_class("scenarios\\bitmaps\\garden\\weeds")==Class::Grass &&
        pt::surface_material_tag_class("metal\\sandblasted_paint")==Class::Unknown,
        "Physical categories used a directory or partial token as a source identity");
    auto material=white(64);
    material.specular_brightness=.64f;
    const auto legacy=pt::material_response(material);
    require(legacy.detail[2]==0 && legacy.detail[3]==0,"Unknown material changed its legacy response");
    material.source_surface_material=Class::Sand;
    const auto sand=pt::material_response(material);
    require(sand.detail[2]==0xc9c9c9c9u && sand.response[0]==.89f &&
        std::bit_cast<float>(sand.detail[3])==sand.response[0] &&
        sand.response[1]==legacy.response[1] && sand.response[2]==legacy.response[2] &&
        sand.response[3]==legacy.response[3] && sand.base==legacy.base && sand.emission==legacy.emission,
        "Authored sand absorption modified strength, appearance, or coverage");
    material.source_surface_material=Class::Polished;
    const auto polished=pt::material_response(material);
    require(polished.detail[2]==0 && polished.response[0]==.26f &&
        polished.response[1]==legacy.response[1] && polished.response[2]==legacy.response[2],
        "Polished material introduced diffuse absorption or new reflective strength");
    material.water_pass=true;material.source_surface_material=Class::Sand;
    const auto water=pt::material_response(material);
    require(water.detail[2]==0 && water.detail[3]==0,"Water was assigned a solid surface response");

    auto strong=white();
    strong.environment_pass=std::make_shared<Material>();
    strong.environment_pass->forward_vertex=4;
    strong.environment_pass->forward_diffuse={.5f,.3f,.2f,.8f};
    strong.environment_pass->forward_specular={.7f,.5f,.1f,.4f};
    const auto unclassified=pt::material_response(strong);
    require(unclassified.response[0]>=.60f && unclassified.response[1]==.8f && unclassified.response[2]==.4f,
        "Strong authored gain inferred an unqualified mirror finish");
    strong.source_surface_material=Class::CoatedMetal;
    const auto painted=pt::material_response(strong);
    require(painted.response[0]>.48f && std::abs(painted.response[1]-.28f)<1.e-6f &&
        std::abs(painted.response[2]-.14f)<1.e-6f && painted.base==unclassified.base &&
        painted.glancing_tint==unclassified.glancing_tint,
        "Paint lost its broad reflection, authored facing balance or tint");
    strong.source_surface_material=Class::Lens;
    const auto lens=pt::material_response(strong);
    require(lens.response[0]<painted.response[0] && lens.response[1]==.8f && lens.response[2]==.4f,
        "Gold lens lost its intentionally strong reflection");

    material.water_pass=false;
    material.forward_pass=true;material.forward_vertex=3;material.forward_pass_index=0;
    auto& pass=material.passes.emplace_back();
    pass.name="shaders\\shader_passes\\texture_mapping\\detail_blend_texture_zwrite";
    pass.vertex_shader="rasterizer\\vertex_shaders_xbox\\stencil_texture";
    for(const auto word:pt::unlit_uv_detail::texture_words)
        for(unsigned byte=0;byte<4;++byte)material.vertex_program.push_back(std::byte((word>>(byte*8))&255));
    pass.vertex_code=material.vertex_program;
    material.forward_program.words=pt::surface_detail::terrain_three_stage;
    material.detail=material.bump=material.diffuse;
    for(auto& sampler:material.texture_states) {
        sampler.address_u=sampler.address_v=1;
        sampler.mag_filter=sampler.min_filter=1;
    }
    const auto surface=pt::qualified_surface(material);
    require(surface && surface->family==pt::SurfaceFamily::TerrainBlendThreeStage,"Invalid terrain response fixture");
    material.source_terrain_materials={Class::Grass,Class::Sand};
    auto materials=std::span(&material,1);std::vector<Vertex> vertices;triangle(vertices,0,0);
    auto scene=pt::build_scene(vertices,materials);
    require(scene.materials[0].detail[2]==0xc9c9ffedu && scene.materials[0].response[0]==.89f,
        "Terrain atlas creation lost T1 grass/T2 sand packed in authored order");
    material.source_terrain_materials={Class::Soil,Class::Unknown};
    pt::refresh_materials_for_geometry(scene,materials,vertices);
    require(scene.materials[0].detail[2]==0xffffdedeu && scene.materials[0].response[0]==.84f,
        "Terrain refresh overwrote a current response or classified an unresolved bitmap");
    material.source_terrain_materials={};
    pt::refresh_materials_for_geometry(scene,materials,vertices);
    require(scene.materials[0].detail[2]==0 && scene.materials[0].response[0]==.89f,
        "When terrain layers are unknown the authored root should only supply the broad lobe");
}
void authored_environment() {
    auto material=white(64);
    auto& pass=*(material.environment_pass=std::make_shared<Material>());
    pass.forward_pass=true;pass.forward_vertex=4;pass.forward_blend=8;
    pass.forward_pass_index=0;
    auto& selected=pass.passes.emplace_back();
    // The independent tangent-space program puts the auxiliary vector cube
    // in stage 2 and the bump-dependent reflected environment in stage 3.
    pass.forward_program.words[54]=0x62e21u;
    selected.textures={{0,1,2,0},{1,0,3,0}};
    pass.bump=environment_cube(4,1,8);
    pass.auxiliary=environment_cube(128,2,24);
    auto materials=std::span(&material,1);
    const auto maps=pt::find_environment_maps(materials);
    require(maps[0]==&pass.auxiliary,"Authored reflection selected a normalization cube instead of stage 3");
    auto scene=pt::build_scene({},materials);
    scene.material_environment_maps=maps;
    std::vector<Vertex> vertices;triangle(vertices,0,0);
    pt::refresh_materials_for_geometry(scene,materials,vertices);
    const auto& gpu=scene.materials[0];
    const auto record=gpu.detail[1];
    require((gpu.texture[3]&pt::environment_mask_flag)!=0,
        "Independent environment lost the texture pass's destination-alpha mask");
    require(record && scene.pixels.at(record)==64,"Environment atlas did not select the bounded mip");
    for(unsigned face=0;face<6;++face) {
        const auto begin=scene.pixels.begin()+record+1+face*64*64;
        require(std::all_of(begin,begin+64*64,[face](auto pixel){return pixel==(0xff000020u+face);}),
            "Environment atlas did not preserve face-major mip order and RGBA");
    }
    require(!(gpu.texture[3]&pt::environment_mip_flag),"One resident cube mip gained a coarse level");
    pass.auxiliary.mips[3].size-=4;
    require(!pt::find_environment_maps(materials)[0],"Environment admission read past the selected mip");
    pass.auxiliary=environment_cube(128,1,24);
    require(!pt::find_environment_maps(materials)[0],"Environment admission exceeded the bounded face size");
    pass.auxiliary=environment_cube(128,3,24);
    auto pyramid=pt::build_scene({},materials);
    pyramid.material_environment_maps=pt::find_environment_maps(materials);
    pt::refresh_materials_for_geometry(pyramid,materials,vertices);
    const auto mip=pyramid.materials[0].detail[1]+1+6*64*64;
    require((pyramid.materials[0].texture[3]&pt::environment_mip_flag) &&
        pyramid.pixels.at(mip)==32 && pyramid.pixels.at(mip+1+5*32*32)==0xff00002du,
        "Environment atlas lost the next authored mip or its face order");
    pt::refresh_materials_for_geometry(pyramid,materials,vertices);
    require(pyramid.materials[0].texture[3]&pt::environment_mip_flag,
        "Material refresh dropped the resident environment mip");
    pass.auxiliary=environment_cube(4,1,24);
    material.forward_raster.alpha_test=1;
    require(!(pt::material_response(material).texture[3]&pt::environment_mask_flag),
        "An opaque-material destination alpha fallback was applied to alpha coverage");

    // A native direct cube lookup can put the authored cube in stage 1
    // while stage 3 independently holds the externally bound vector cube.
    pass.forward_program.words[54]=0x18461u;
    selected.textures={{0,1,3,0},{1,0,1,0}};
    pass.detail=environment_cube(4,1,32);
    require(pt::find_environment_maps(materials)[0]==&pass.detail,
        "Direct environment selection ignored authored and external texture bindings");
    selected.textures.erase(selected.textures.begin()+1);
    require(!pt::find_environment_maps(materials)[0],"An inactive bound texture supplied authored shine");
}
void atlas_pressure_compacts() {
    const auto retired_cube=environment_cube(4,1,7);
    const auto cube=environment_cube(8,2,23);
    const auto duplicate=cube;
    auto normal=white().diffuse;
    normal.width=normal.height=4;
    normal.pixels=std::vector<std::byte>(64,std::byte{128});
    auto surface=white();
    surface.forward_pass=true;surface.forward_vertex=3;surface.forward_pass_index=0;
    auto& pass=surface.passes.emplace_back();
    pass.name="shaders\\shader_passes\\texture_mapping\\detail_blend_texture_zwrite";
    pass.vertex_shader="rasterizer\\vertex_shaders_xbox\\stencil_texture";
    for(const auto word:pt::unlit_uv_detail::texture_words)
        for(unsigned byte=0;byte<4;++byte)surface.vertex_program.push_back(std::byte((word>>(byte*8))&255));
    pass.vertex_code=surface.vertex_program;
    surface.forward_program.words=pt::surface_detail::terrain_three_stage;
    surface.detail=surface.bump=surface.diffuse;
    for(auto& sampler:surface.texture_states) {
        sampler.address_u=sampler.address_v=1;
        sampler.mag_filter=sampler.min_filter=1;
    }
    std::array materials{white(),surface,white(),white()};
    auto scene=pt::build_scene({},materials);
    scene.material_environment_maps={&retired_cube,&cube,&duplicate,nullptr};
    scene.material_normal_maps={nullptr,&normal,nullptr,nullptr};
    std::vector<Vertex> before;triangle(before,0,0,0);triangle(before,2,0,1);triangle(before,4,0,2);
    pt::refresh_materials_for_geometry(scene,materials,before);pt::update_geometry(scene,before);
    const auto cube_before=scene.materials[1].detail[1];
    require(scene.material_environment_images.size()==2 && cube_before && scene.materials[2].detail[1]==cube_before,
        "Identical authored cubes consumed duplicate atlas allocations");
    const auto root_color=pt::material_surface(scene,1,{.25f,.25f});
    const auto normal_before=scene.materials[1].detail[0];

    // Simulate the gaps left by earlier growing animation frames without
    // constructing or decompressing artificial high resolution textures.
    scene.pixels.resize(16'777'216);scene.texture_texel_count=std::uint32_t(scene.pixels.size());
    const auto third_capacity=scene.material_texel_capacities[2];
    scene.material_texel_capacities[2]=scene.texture_texel_count-scene.materials[2].texture[0];
    auto over_budget=before;triangle(over_budget,6,0,3);
    rejects([&]{pt::refresh_materials_for_geometry(scene,materials,over_budget);},
        "Genuinely over-budget live atlas was accepted");
    require(scene.materials[1].detail[1]==cube_before && scene.material_texel_capacities[0]!=0 &&
        scene.texture_texel_count==16'777'216 && pt::material_surface(scene,1,{.25f,.25f})==root_color,
        "Over-budget admission damaged the prior valid root or shared cube");
    scene.material_texel_capacities[2]=third_capacity;
    std::vector<Vertex> after;triangle(after,2,0,1);triangle(after,4,0,2);triangle(after,6,0,3);
    pt::refresh_materials_for_geometry(scene,materials,after);pt::update_geometry(scene,after);
    require(scene.texture_texel_count<1024 && scene.material_texel_capacities[0]==0 &&
        scene.material_environment_images.size()==1 && scene.triangles.size()==3,
        "A cut freed neither retired textures nor preceding shot gaps");
    const auto cube_after=scene.materials[1].detail[1];
    require(cube_after!=cube_before && cube_after==scene.materials[2].detail[1] &&
        scene.pixels.at(cube_after)==8 && scene.pixels.at(cube_after+1)==0xff000017u &&
        scene.pixels.at(cube_after+1+6*8*8)==4 && (scene.materials[2].texture[3]&pt::environment_mip_flag),
        "Compaction did not preserve both shared cube levels and referenced offsets");
    require(scene.materials[1].detail[0]!=normal_before && scene.pixels.at(scene.materials[1].detail[0])==1 &&
        pt::material_surface(scene,1,{.25f,.25f})==root_color,
        "Compaction left a stale normal map or a relative material texture address");
    pt::refresh_materials_for_geometry(scene,materials,before);pt::update_geometry(scene,before);
    require(scene.material_texel_capacities[0]!=0 && scene.material_environment_images.size()==2 &&
        scene.pixels.at(scene.materials[0].detail[1]+1)==0xff000007u,
        "A material reappearing after a cut did not recover its original texture");
}
void large_source_material_bank() {
    // Actual four-view Coagulation preload size from ECR-006. IDs remain the
    // source renderer's uint32 slots; only geometry-referenced images reside.
    constexpr std::uint32_t count=77668;
    std::vector<Material> materials(count);
    auto scene=pt::build_scene({},materials);
    require(scene.materials.size()==count && scene.texture_texel_count==1 &&
        std::all_of(scene.material_texel_capacities.begin(),scene.material_texel_capacities.end(),
            [](auto capacity){return capacity==0;}),
        "Large source namespace lost preloaded slots or decoded unused images");
    const std::array<std::uint32_t,3> slots{65535,65536,count-1};
    std::vector<Vertex> vertices;
    for(unsigned n=0;n<slots.size();++n){
        materials[slots[n]]=white();triangle(vertices,float(n)*3,0,slots[n]);
    }
    // An unreferenced malformed UI image must remain untouched, including
    // during later refresh of a material above the old 16-bit boundary.
    materials[3].diffuse={65536,65536,11,{}};
    pt::refresh_materials_for_geometry(scene,materials,vertices);pt::update_geometry(scene,vertices);
    require(scene.triangles.size()==3 && scene.material_texel_capacities[3]==0,
        "Large source namespace admitted an unrelated image or lost geometry");
    for(unsigned n=0;n<slots.size();++n){
        pt::Hit hit;require(pt::intersect(scene,{float(n)*3+.2f,.2f,1},{0,0,-1},2,hit) &&
            scene.triangles.at(hit.triangle).metadata[0]==slots[n],
            "Large source material ID was truncated or remapped");
    }
    const auto last=slots.back(),offset=scene.materials[last].texture[0];
    materials[last].diffuse.pixels={std::byte{17},std::byte{34},std::byte{51},std::byte{255}};
    pt::refresh_materials_for_geometry(scene,materials,vertices);pt::update_geometry(scene,vertices);
    require(scene.materials.size()==count && scene.materials[last].texture[0]==offset &&
        scene.pixels.at(offset)==0xff332211u && scene.material_texel_capacities[3]==0,
        "Late high-ID material refresh changed namespace/residency or retained stale image bytes");
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
            const auto index=scene.pixels.at(scene.texture_texel_count+n)&~pt::emitter_baked_bit;
            result.push_back(std::bit_cast<std::uint32_t>(scene.triangles.at(index).p0[3]));
        }
        std::sort(result.begin(),result.end());return result;
    };
    require(emitter_ordinals(actual)==emitter_ordinals(expected),"Refit retained stale emitter indices");
}
void native_needle_self_shadow() {
    // Actual prepared Coagulation blood_cliffs: expanded triangle7813,
    // admitted triangle6031. The old scalar triple-product intersection
    // returned three positive hits for these outward shadow origins even
    // though their double-precision plane distances are all positive.
    const std::array<Vec3,3> positions{{
        {27.594724655151367f,-116.19940948486328f,12.582358360290527f},
        {28.29535675048828f,-120.41195678710938f,18.776273727416992f},
        {28.295442581176758f,-120.41218566894531f,18.776273727416992f}}};
    const std::array<Vec3,4> origins{{
        {28.062026977539062f,-119.00779724121094f,16.711681365966797f},
        {27.980289459228516f,-118.51634979248047f,15.989065170288086f},
        {27.98025894165039f,-118.51626586914062f,15.98906421661377f},
        {27.980297088623047f,-118.51634216308594f,15.989039421081543f}}};
    const Vec3 direction{.879587709903717f,.0769539475440979f,.4694715738296509f};
    const Vec3 reverse{-direction.x,-direction.y,-direction.z};
    const Material material=white();std::vector<Vertex> vertices;
    for(auto position:positions){Vertex v;v.position=position;v.color={1,1,1};v.normal={0,0,1};vertices.push_back(v);}
    for(bool reversed:{false,true}) {
        if(reversed)std::swap(vertices[1],vertices[2]);
        const auto scene=pt::build_scene(vertices,std::span(&material,1));pt::Hit hit;
        require(scene.triangles.size()==1,"Actual native needle triangle was discarded");
        for(const auto origin:origins) {
            require(!pt::intersect(scene,origin,direction,1,hit),"Outward source sun ray falsely hit its native needle triangle");
            require(pt::intersect(scene,origin,reverse,1,hit) && hit.distance>1.e-4f && hit.distance<.001f,
                "Precision correction discarded a real inward hit on the same native needle triangle");
            require(!pt::intersect(scene,origin,reverse,hit.distance*.5f,hit),
                "Native needle intersection ignored the finite ray segment");
        }
    }
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
    large_source_material_bank();
    native_needle_self_shadow();
    std::vector<Material> materials{white()};std::vector<Vertex> vertices;
    for(unsigned n=0;n<24;++n)triangle(vertices,float(n)*2,0);
    auto scene=pt::build_scene(vertices,materials);pt::Hit hit;
    require(scene.nodes.size()>1,"Scene did not build an acceleration hierarchy");
    require(!pt::intersect(scene,{.2f,.2f,0},{0,0,std::numeric_limits<float>::denorm_min()},3,hit),
        "Subnormal coplanar ray produced a nonfinite triangle hit");
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
    visor.source_surface_material=SourceSurfaceMaterial::Lens;
    visor.environment_pass=std::make_shared<Material>();visor.environment_pass->forward_vertex=4;
    visor.environment_pass->forward_diffuse={.9f,.65f,.2f,.7f};
    visor.environment_pass->forward_specular={.7f,.3f,.1f,.9f};
    const auto polished=pt::material_response(visor);
    require(std::abs(polished.response[1]-.7f)<1.e-6f && std::abs(polished.response[2]-.9f)<1.e-6f,
        "Path Tracing replaced current vanilla reflection strengths with the pStencil 2003 style");
    require(std::abs(polished.base[0]-.9f)<1.e-6f && std::abs(polished.base[1]-.65f)<1.e-6f &&
        std::abs(polished.base[2]-.2f)<1.e-6f,
        "Path Tracing replaced the current vanilla reflection tint");
    require(polished.glancing_tint[0]==.7f && polished.glancing_tint[1]==.3f && polished.glancing_tint[2]==.1f,
        "Path Tracing dropped the independent authored glancing reflection tint");
    require(polished.response[0]<rough.response[0] && polished.base[0]>polished.base[2],
        "Visor lost its independent roughness/tinted reflection response");
    materials={white(),white()};materials[1].forward_vertex=2;materials[1].forward_diffuse={.5f,.1f,.02f,1};
    scene=pt::build_scene(vertices,materials);
    // The emitter table holds an index and a running power per emitter.
    const auto emitter_groups=scene.texture_texel_count+2*scene.emissive_count;
    const auto group=emitter_groups+pt::emitter_group_header_words;
    const auto bounds=group+pt::emitter_group_words;
    require(scene.emissive_count==1 && scene.pixels.size()==bounds+pt::emitter_bounds_words,
        "Authored emissive geometry was omitted from direct-light sampling");
    require(scene.pixels[emitter_groups]==pt::emitter_group_magic && scene.pixels[emitter_groups+1]==1 &&
        std::bit_cast<float>(scene.pixels[emitter_groups+2])>0,
        "Emissive continuation lost its bounded source-group proposal metadata");
    require(scene.pixels[group]==1 && scene.pixels[group+1]==1 &&
        std::bit_cast<float>(scene.pixels[group+3])>0 && std::bit_cast<float>(scene.pixels[group+7])>0,
        "Emitter proposal changed its source material, triangle count or finite geometric bounds");
    const auto& emitter=scene.triangles[scene.pixels[scene.texture_texel_count]];
    for(unsigned axis=0;axis<3;++axis) {
        require(std::bit_cast<float>(scene.pixels[bounds+axis])==std::min({emitter.p0[axis],emitter.p1[axis],emitter.p2[axis]}) &&
            std::bit_cast<float>(scene.pixels[bounds+3+axis])==std::max({emitter.p0[axis],emitter.p1[axis],emitter.p2[axis]}),
            "Live emitter bounds do not enclose the complete shadow target");
    }
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
    authored_surface_response();
    authored_environment();
    atlas_pressure_compacts();
    std::cout<<"PASS Path Tracing BVH/refit, finite shadows, alpha any-hit, posed refresh, authored matte/visor response and emitters\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
