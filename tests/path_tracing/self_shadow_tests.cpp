#include "halo2/path_tracing/scene.hpp"
#include "halo2/path_tracing/surface_ray.hpp"
#include "halo2/path_tracing/sky_lighting.hpp"
#include "halo2/path_tracing/emission.hpp"
#include "halo2/path_tracing/material_emission.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>

using namespace halo2;
namespace pt=halo2::path_tracing;
namespace {
// Match scene.glsl's current origin displacement, not a proposed fix.
constexpr float epsilon=.0002f;
constexpr float maximum=1.e10f;
constexpr std::array<Vec3,7> samples{{{1.f/3,1.f/3,1.f/3},
    {.8f,.1f,.1f},{.1f,.8f,.1f},{.1f,.1f,.8f},
    {.1f,.45f,.45f},{.45f,.1f,.45f},{.45f,.45f,.1f}}};
void require(bool value,const char* message){if(!value)throw FormatError(message);}
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 mul(Vec3 a,float b){return {a.x*b,a.y*b,a.z*b};}
float dot(Vec3 a,Vec3 b){return (a.x*b.x+a.y*b.y)+a.z*b.z;}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Vec3 normalized(Vec3 a,Vec3 fallback={}){
    const auto squared=dot(a,a);return squared>1.e-18f?mul(a,1.f/std::sqrt(squared)):fallback;
}
Vec3 xyz(const ShaderVector& v){return {v[0],v[1],v[2]};}
Vec3 mix(Vec3 a,Vec3 b,Vec3 c,Vec3 weights){return add(add(mul(a,weights.x),mul(b,weights.y)),mul(c,weights.z));}
nlohmann::json vector(Vec3 v){return nlohmann::json::array({v.x,v.y,v.z});}
struct Double3 {double x,y,z;};
Double3 precise(Vec3 a){return {a.x,a.y,a.z};}
Double3 sub(Double3 a,Double3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Double3 cross(Double3 a,Double3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double dot(Double3 a,Double3 b){return (a.x*b.x+a.y*b.y)+a.z*b.z;}
Double3 normalized(Double3 a){const double length=std::sqrt(dot(a,a));return {a.x/length,a.y/length,a.z/length};}
Vec3 geometric_origin(Vec3 point,Vec3 geometric){
    auto result=add(point,mul(geometric,epsilon));
    // A diagnostic reference only: move every nonzero geometric component
    // one representable step outward after the geometric-normal offset.
    const auto outward=[](float value,float direction){
        return direction==0.f?value:std::nextafter(value,std::copysign(std::numeric_limits<float>::infinity(),direction));
    };
    result.x=outward(result.x,geometric.x);result.y=outward(result.y,geometric.y);result.z=outward(result.z,geometric.z);
    return result;
}
bool same(Vec3 a,Vec3 b){return a.x==b.x && a.y==b.y && a.z==b.z;}
unsigned shared_vertices(const pt::GpuTriangle& a,const pt::GpuTriangle& b){
    unsigned shared{};const std::array other{b.p0,b.p1,b.p2};
    for(const auto* p:{&a.p0,&a.p1,&a.p2})
        if(std::any_of(other.begin(),other.end(),
            [&](const auto& q){return same(xyz(*p),xyz(q));}))++shared;
    return shared;
}
float shared_normal_alignment(const pt::GpuTriangle& a,const pt::GpuTriangle& b){
    const std::array ap{a.p0,a.p1,a.p2},bp{b.p0,b.p1,b.p2};
    const std::array an{a.n0,a.n1,a.n2},bn{b.n0,b.n1,b.n2};float alignment=1.f;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(same(xyz(ap[i]),xyz(bp[j])))
        alignment=std::min(alignment,dot(normalized(xyz(an[i])),normalized(xyz(bn[j]))));
    return alignment;
}
struct Summary {
    std::size_t triangles{},samples{},reversed_winding{},invalid_normal{},below_geometric_sun{},below_shading_sun{};
    std::size_t current_hits{},reference_hits{},current_own_hits{},reference_own_hits{},near_shared_edge_hits{},visibility_differences{};
    float minimum_normal_alignment=1.f;
    nlohmann::json own_examples=nlohmann::json::array(),near_examples=nlohmann::json::array();
    std::size_t terminator_samples{},terminator_current_own{},terminator_current_adjacent{},terminator_current_other{},
        terminator_current_smooth_edge{},terminator_smooth_own{},terminator_smooth_adjacent{},terminator_smooth_other{},
        terminator_cleared{},terminator_unsupported{};
    std::size_t plane_own_ignored{},plane_adjacent_ignored{},plane_other_ignored{},plane_distinct_preserved{};
    float terminator_maximum_displacement{};
    nlohmann::json terminator_examples=nlohmann::json::array();
};
nlohmann::json hit_json(bool found,const pt::Hit& hit,const pt::Scene& scene){
    if(!found)return nullptr;
    return {{"scene_triangle",hit.triangle},{"admitted_triangle",std::bit_cast<std::uint32_t>(scene.triangles[hit.triangle].p0[3])},
        {"material",scene.triangles[hit.triangle].metadata[0]},{"distance",hit.distance},{"barycentric",vector(hit.barycentric)}};
}
nlohmann::json emitter_census(const pt::Scene& scene,std::span<const Material> materials){
    struct Energy {
        std::size_t triangles{},samples{},positive_samples{},sampled_dark_triangles{};
        Vec3 sum{},maximum{};Double3 area_sum{};double area{};
    };
    std::map<std::uint32_t,Energy> energy;
    for(std::uint32_t emitter=0;emitter<scene.emissive_count;++emitter){
        const auto index=scene.pixels.at(scene.texture_texel_count+emitter);
        require(index<scene.triangles.size(),"Emitter index escaped its actual source triangle bank");
        const auto& triangle=scene.triangles[index];const auto slot=triangle.metadata[0];auto& row=energy[slot];++row.triangles;
        const auto area_vector=cross(sub(precise(xyz(triangle.p1)),precise(xyz(triangle.p0))),
            sub(precise(xyz(triangle.p2)),precise(xyz(triangle.p0))));
        const double area=.5*std::sqrt(dot(area_vector,area_vector));row.area+=area;
        bool positive=false;
        for(const auto barycentric:samples){
            const Vec2 uv{(triangle.uv01[0]*barycentric.x+triangle.uv01[2]*barycentric.y)+triangle.uv2[0]*barycentric.z,
                (triangle.uv01[1]*barycentric.x+triangle.uv01[3]*barycentric.y)+triangle.uv2[1]*barycentric.z};
            const auto value=pt::material_emission(scene,slot,uv);
            require(std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z),"Native emitter evaluator returned nonfinite source RGB");
            ++row.samples;const bool emits=value.x>0 || value.y>0 || value.z>0;row.positive_samples+=emits;positive=positive || emits;
            row.sum=add(row.sum,value);row.maximum={std::max(row.maximum.x,value.x),std::max(row.maximum.y,value.y),std::max(row.maximum.z,value.z)};
            row.area_sum.x+=double(value.x)*area/samples.size();row.area_sum.y+=double(value.y)*area/samples.size();row.area_sum.z+=double(value.z)*area/samples.size();
        }
        row.sampled_dark_triangles+=!positive;
    }
    nlohmann::json rows=nlohmann::json::array();std::size_t sampled_dark{};
    for(const auto& [slot,e]:energy){
        const auto& root=materials[slot];nlohmann::json passes=nlohmann::json::array();
        const auto append=[&](const Material& pass){
            if(!pt::supports_emission(pass))return;
            passes.push_back({{"family",unsigned(pt::emission_family(pass))},{"pass",pass.passes[pass.forward_pass_index].name},
                {"constants",nlohmann::json::array({pass.forward_program.words[10],pass.forward_program.words[18],
                    pass.forward_program.words[11],pass.forward_program.words[19],pass.forward_program.words[43],pass.forward_program.words[12]})}});
        };
        if(root.forward_vertex==2)append(root);
        for(const auto& pass:root.world_overlays_after)if(pass)append(*pass);
        sampled_dark+=e.sampled_dark_triangles;
        rows.push_back({{"material_slot",slot},{"name",root.name},{"triangles",e.triangles},{"samples",e.samples},
            {"positive_samples",e.positive_samples},{"sampled_dark_triangles",e.sampled_dark_triangles},
            {"mean_rgb",vector(mul(e.sum,1.f/float(e.samples)))},{"maximum_rgb",vector(e.maximum)},
            {"surface_area",e.area},{"area_weighted_mean_rgb",nlohmann::json::array({e.area_sum.x/e.area,e.area_sum.y/e.area,e.area_sum.z/e.area})},
            {"exact_qualified_passes",std::move(passes)}});
    }
    return {{"emitter_triangles",scene.emissive_count},{"sampled_dark_triangles",sampled_dark},{"materials",std::move(rows)},
        {"scope","Production transported native material_emission evaluator at the same seven interior source-UV barycentric samples per static BSP emitter. Mean/max and area-weighted means describe these samples; sampled-dark does not prove the whole texture is black. Dynamic model/effect emitters in live gameplay are outside this BSP-only census."}};
}
Vec3 smooth_offset(const pt::GpuTriangle& triangle,Vec3 point,Vec3 barycentric,Vec3 geometric,bool& supported){
    // Diagnostic reference to Hanika, Ray Tracing Gems II ch4 (2021):
    // https://jo.dreggn.org/home/2021_terminator.pdf . Project onto the
    // authored vertex tangent planes and barycentrically blend outward
    // corrections. There is no artist-tuned distance/ambient parameter.
    const std::array positions{xyz(triangle.p0),xyz(triangle.p1),xyz(triangle.p2)};
    const std::array normals{normalized(xyz(triangle.n0)),normalized(xyz(triangle.n1)),normalized(xyz(triangle.n2))};
    const std::array weights{barycentric.x,barycentric.y,barycentric.z};Vec3 offset{};
    supported=true;
    for(unsigned n=0;n<3;++n){
        if(dot(normals[n],geometric)<=0.f){supported=false;return {};}
        offset=add(offset,mul(normals[n],-weights[n]*std::min(0.f,dot(sub(point,positions[n]),normals[n]))));
    }
    return offset;
}
void terminator_sample(Summary& summary,const pt::Scene& scene,std::uint32_t index,Vec3 point,Vec3 normal,
    Vec3 geometric,Vec3 barycentric,Vec3 direction,float shading_sun,double geometric_sun,std::size_t source_first,
    const Vertex& source){
    ++summary.terminator_samples;const auto& triangle=scene.triangles[index];pt::Hit current{},smooth{};
    const auto current_origin=add(point,mul(normal,epsilon));
    const bool current_found=pt::intersect(scene,current_origin,direction,maximum,current);
    const bool current_own=current_found && current.triangle==index;
    const bool current_adjacent=current_found && !current_own && shared_vertices(triangle,scene.triangles[current.triangle])>=2;
    summary.terminator_current_own+=current_own;summary.terminator_current_adjacent+=current_adjacent;
    const float normal_alignment=current_adjacent?shared_normal_alignment(triangle,scene.triangles[current.triangle]):0.f;
    summary.terminator_current_smooth_edge+=current_adjacent && normal_alignment>.9999f;
    summary.terminator_current_other+=current_found && !current_own && !current_adjacent;
    const bool plane_ignored=current_found && pt::skip_starting_surface(scene.triangles[current.triangle],current.triangle,
        point,geometric,direction,true);
    summary.plane_own_ignored+=plane_ignored && current_own;
    summary.plane_adjacent_ignored+=plane_ignored && current_adjacent;
    summary.plane_other_ignored+=plane_ignored && !current_own && !current_adjacent;
    summary.plane_distinct_preserved+=current_found && !plane_ignored;
    bool supported;const auto offset=smooth_offset(triangle,point,barycentric,geometric,supported);
    if(!supported){++summary.terminator_unsupported;return;}
    const float displacement=std::sqrt(dot(offset,offset));
    summary.terminator_maximum_displacement=std::max(summary.terminator_maximum_displacement,displacement);
    const auto smooth_origin=add(add(point,offset),mul(normal,epsilon));
    const bool smooth_found=pt::intersect(scene,smooth_origin,direction,maximum,smooth);
    const bool smooth_own=smooth_found && smooth.triangle==index;
    const bool smooth_adjacent=smooth_found && !smooth_own && shared_vertices(triangle,scene.triangles[smooth.triangle])>=2;
    summary.terminator_smooth_own+=smooth_own;summary.terminator_smooth_adjacent+=smooth_adjacent;
    summary.terminator_smooth_other+=smooth_found && !smooth_own && !smooth_adjacent;
    summary.terminator_cleared+=current_found && !smooth_found;
    if(summary.terminator_examples.size()<24 && (current_own || current_adjacent)){
        summary.terminator_examples.push_back({{"scene_triangle",index},{"source_triangle",source_first/3},
            {"source_section",source.source_section},{"source_part",source.source_part},{"bsp_cluster",source.bsp_cluster},
            {"material_slot",triangle.metadata[0]},{"barycentric",vector(barycentric)},
            {"p0",vector(xyz(triangle.p0))},{"p1",vector(xyz(triangle.p1))},{"p2",vector(xyz(triangle.p2))},
            {"n0",vector(xyz(triangle.n0))},{"n1",vector(xyz(triangle.n1))},{"n2",vector(xyz(triangle.n2))},
            {"point",vector(point)},{"shading_normal",vector(normal)},{"geometric_normal",vector(geometric)},
            {"normal_alignment",dot(normal,geometric)},{"shading_sun",shading_sun},{"geometric_sun",geometric_sun},
            {"smooth_offset",vector(offset)},{"displacement",displacement},
            {"current_origin",vector(current_origin)},{"current_hit",hit_json(current_found,current,scene)},
            {"starting_plane_hit_ignored",plane_ignored},
            {"current_hit_shared_normal_alignment",normal_alignment},
            {"smooth_origin",vector(smooth_origin)},{"smooth_hit",hit_json(smooth_found,smooth,scene)}});
    }
}
nlohmann::json smooth_surface_control(){
    // A coarse octant triangle represents a smooth unit sphere. Its vertex
    // normals and analytic sphere normal agree, while this interior face
    // points away from the chosen light. Keep all real ray queries in Scene.
    const std::array<Vec3,3> positions{{{1,0,0},{0,1,0},{0,0,1}}};
    const Vec3 barycentric{.8f,.1f,.1f},point=barycentric,normal=normalized(point),geometric=normalized(Vec3{1,1,1});
    const Vec3 direction=normalized(Vec3{1,-1,-1});
    std::vector<Vertex> vertices;
    for(const auto position:positions){Vertex v;v.position=position;v.normal=position;v.color={1,1,1};vertices.push_back(v);}
    Material material;material.diffuse={1,1,11,{std::byte{255},std::byte{255},std::byte{255},std::byte{255}}};
    const std::array materials{material,material};
    const auto coarse=pt::build_scene(vertices,materials);require(coarse.triangles.size()==1,"Smooth-surface control lost its coarse sphere face");
    require(dot(normal,direction)>0.f && dot(geometric,direction)<0.f,"Smooth-surface control no longer isolates a geometric terminator");
    bool supported;const auto offset=smooth_offset(coarse.triangles[0],point,barycentric,geometric,supported);
    require(supported && dot(offset,geometric)>0.f,"Vertex tangent-plane projection did not lift the convex control");
    const auto current_origin=add(point,mul(normal,epsilon)),smooth_origin=add(add(point,offset),mul(normal,epsilon));pt::Hit raw{},smooth{};
    const bool raw_hit=pt::intersect(coarse,current_origin,direction,maximum,raw);
    const bool smooth_hit=pt::intersect(coarse,smooth_origin,direction,maximum,smooth);
    require(raw_hit && !smooth_hit,"Coarse-sphere direct-shadow terminator control did not distinguish flat and smooth origins");
    require(pt::skip_starting_surface(coarse.triangles[0],0,point,geometric,direction,true),
        "Starting-plane classifier retained its own smooth-terminator face");
    require(!pt::skip_starting_surface(coarse.triangles[0],0,point,geometric,direction,false,0),
        "Starting-plane classifier changed a direction without a smooth terminator");
    auto displaced=coarse.triangles[0];
    for(auto* p:{&displaced.p0,&displaced.p1,&displaced.p2})for(unsigned axis=0;axis<3;++axis)
        (*p)[axis]+=std::array{geometric.x,geometric.y,geometric.z}[axis]*(epsilon*4.f);
    require(!pt::skip_starting_surface(displaced,1,point,geometric,direction,true),
        "Starting-plane classifier ignored a separate parallel plane four epsilons away");
    auto tilted=coarse.triangles[0];tilted.p0[0]+=geometric.x*.1f;tilted.p0[1]+=geometric.y*.1f;tilted.p0[2]+=geometric.z*.1f;
    require(!pt::skip_starting_surface(tilted,1,point,geometric,direction,true),
        "Starting-plane classifier ignored a distinct tilted plane");
    auto flat=coarse.triangles[0];flat.n0=flat.n1=flat.n2={geometric.x,geometric.y,geometric.z,0};
    const auto flat_offset=smooth_offset(flat,point,barycentric,geometric,supported);
    require(supported && dot(flat_offset,flat_offset)<1.e-12f,"Tangent-plane correction displaced a flat-normal control");
    auto concave=coarse.triangles[0];
    for(auto* n:{&concave.n0,&concave.n1,&concave.n2})for(unsigned a=0;a<3;++a)(*n)[a]=-(*n)[a];
    const auto concave_offset=smooth_offset(concave,point,barycentric,mul(geometric,-1.f),supported);
    require(supported && dot(concave_offset,concave_offset)==0.f,"Tangent-plane correction moved a concave control inside its surface");
    // A separate blocker beyond the implied smooth sphere must still shadow
    // the receiver. It is not part of the self/adjacent primitive exclusion.
    const auto center=add(smooth_origin,mul(direction,2.f));
    const auto tangent=normalized(cross(direction,Vec3{0,0,1})),bitangent=cross(direction,tangent);
    for(const auto position:std::array{add(add(center,mul(tangent,-2)),mul(bitangent,-2)),
        add(add(center,mul(tangent,2)),mul(bitangent,-2)),add(center,mul(bitangent,2))}){
        Vertex v;v.position=position;v.normal=mul(direction,-1);v.color={1,1,1};v.material=1;vertices.push_back(v);
    }
    const auto blocked=pt::build_scene(vertices,materials);pt::Hit blocker{};
    const bool blocker_hit=pt::intersect(blocked,smooth_origin,direction,maximum,blocker);
    require(blocker_hit && blocked.triangles[blocker.triangle].metadata[0]==1,"Smooth origin leaked light through its separate blocker control");
    return {{"raw_self_shadow",raw_hit},{"smooth_reference_self_shadow",smooth_hit},{"separate_blocker_preserved",blocker_hit},
        {"starting_plane_own_ignored",true},{"starting_plane_parallel_blocker_preserved",true},
        {"starting_plane_tilted_blocker_preserved",true},{"ordinary_direction_preserved",true},
        {"source_point",vector(point)},{"analytic_sphere_normal",vector(normal)},{"light_direction",vector(direction)},
        {"shading_light_cosine",dot(normal,direction)},{"geometric_light_cosine",dot(geometric,direction)},
        {"smooth_offset",vector(offset)},{"displacement",std::sqrt(dot(offset,offset))},
        {"flat_normal_displacement",std::sqrt(dot(flat_offset,flat_offset))},{"concave_displacement",0},
        {"scope","Diagnostic coarse-sphere origin comparison through production CPU Scene/intersect. Analytic smooth sphere is sun-facing while its coarse face is geometrically back-facing. The projected origin is not integrated into production shading; this control does not prove every nearby blocker is preserved."}};
}
}
int main(int argc,char** argv)try {
    require(argc==3,"Supply prepared MCC Coagulation map and report path");
    if(!std::filesystem::is_regular_file(argv[1])){std::cout<<"SKIP missing prepared Coagulation map: "<<argv[1]<<'\n';return 77;}
    Cache cache(argv[1]);require(cache.is_mcc(),"Self-shadow probe requires the launcher's prepared MCC map");
    const auto mesh=cache.load_bsp(0);
    pt::SkyLighting skies;skies.set_active(true);const auto* sky=skies.select(cache,0);
    require(sky,"Actual Coagulation sky was not admitted");
    const auto frame=object_sky_light_frame(*sky);require(frame.test_collision,"Actual Coagulation sky lacks its source directional query");
    const auto direction=normalized(frame.incident);require(dot(direction,direction)>.99f,"Source sun direction is zero");
    pt::Light sun;sun.direction=frame.incident;sun.color=frame.radiosity;
    const auto scene=pt::build_scene(mesh.vertices,mesh.materials,std::span(&sun,1));
    require(!scene.triangles.empty(),"Actual Coagulation BSP admitted no Path Tracing geometry");
    // p0.w preserves the pre-BVH admission ordinal. Recover its original
    // expanded source triangle without relying on the reordered BVH index.
    std::vector<std::size_t> source_first;
    for(std::size_t first=0;first<mesh.vertices.size();first+=3){
        const auto& a=mesh.vertices[first];const auto& b=mesh.vertices[first+1];const auto& c=mesh.vertices[first+2];
        if((scene.materials[a.material].texture[3]&16u) || !a.source_stipple_level)continue;
        const auto geometric=cross(sub(b.position,a.position),sub(c.position,a.position));
        if(dot(geometric,geometric)<1.e-18f)continue;
        source_first.push_back(first);
    }
    require(source_first.size()==scene.triangles.size(),"BSP source-to-admitted triangle census differs");
    std::map<std::string,Summary> summaries;
    for(std::uint32_t index=0;index<scene.triangles.size();++index){
        const auto& triangle=scene.triangles[index];const auto slot=triangle.metadata[0];const auto& material=mesh.materials[slot];
        if(material.name.find("coag_ground")==std::string::npos && material.name.find("blood_cliffs")==std::string::npos)continue;
        auto& summary=summaries[material.name];++summary.triangles;
        const auto admitted=std::bit_cast<std::uint32_t>(triangle.p0[3]);
        require(admitted<source_first.size(),"BVH triangle lost its source admission ordinal");
        const auto first=source_first[admitted];const auto& source=mesh.vertices[first];
        const auto p0=xyz(triangle.p0),p1=xyz(triangle.p1),p2=xyz(triangle.p2);
        require(same(p0,source.position) && same(p1,mesh.vertices[first+1].position) && same(p2,mesh.vertices[first+2].position),
            "BVH source admission ordinal identifies different source vertices");
        require(same(xyz(triangle.n0),source.normal) && same(xyz(triangle.n1),mesh.vertices[first+1].normal) &&
            same(xyz(triangle.n2),mesh.vertices[first+2].normal),"BVH changed the actual BSP source normals");
        const auto raw_geometric=normalized(cross(sub(p1,p0),sub(p2,p0)));
        const auto exact_raw=normalized(cross(sub(precise(p1),precise(p0)),sub(precise(p2),precise(p0))));
        for(const auto barycentric:samples){
            const auto point=mix(p0,p1,p2,barycentric);
            const auto normal=normalized(mix(xyz(triangle.n0),xyz(triangle.n1),xyz(triangle.n2),barycentric));
            if(dot(normal,normal)<.99f){++summary.invalid_normal;continue;}
            // Both-sided source surfaces orient the geometric hemisphere to
            // the authored smooth normal. Reject physical terminator cases:
            // a light below the actual face is allowed to be occluded.
            const bool reversed=dot(raw_geometric,normal)<0.f;
            const auto geometric=reversed?mul(raw_geometric,-1.f):raw_geometric;
            const Double3 exact_geometric=reversed?Double3{-exact_raw.x,-exact_raw.y,-exact_raw.z}:exact_raw;
            summary.reversed_winding+=reversed;
            summary.minimum_normal_alignment=std::min(summary.minimum_normal_alignment,dot(geometric,normal));
            const double geometric_sun=dot(exact_geometric,precise(direction));
            const auto shading_sun=dot(normal,direction);
            if(!(geometric_sun>0.)){
                ++summary.below_geometric_sun;
                if(shading_sun>0.f)terminator_sample(summary,scene,index,point,normal,geometric,barycentric,direction,
                    shading_sun,geometric_sun,first,source);
                continue;
            }
            if(!(shading_sun>0.f)){++summary.below_shading_sun;continue;}
            ++summary.samples;
            const auto current_origin=add(point,mul(normal,epsilon));
            const auto reference_origin=geometric_origin(point,geometric);
            pt::Hit current{},reference{};
            const bool current_found=pt::intersect(scene,current_origin,direction,maximum,current);
            const bool reference_found=pt::intersect(scene,reference_origin,direction,maximum,reference);
            summary.current_hits+=current_found;summary.reference_hits+=reference_found;
            const bool own=current_found && current.triangle==index;
            const bool reference_own=reference_found && reference.triangle==index;
            summary.current_own_hits+=own;summary.reference_own_hits+=reference_own;
            summary.visibility_differences+=(current_found!=reference_found);
            const bool near=current_found && !own && current.distance<=epsilon*16.f &&
                shared_vertices(triangle,scene.triangles[current.triangle])>=2;
            summary.near_shared_edge_hits+=near;
            if((own && summary.own_examples.size()<32) || (near && summary.near_examples.size()<16)){
                const auto plane_distance=[&](Vec3 p){return dot(exact_geometric,sub(precise(p),precise(p0)));};
                nlohmann::json row{{"scene_triangle",index},{"admitted_triangle",admitted},{"source_triangle",first/3},
                    {"source_section",source.source_section},{"source_part",source.source_part},{"bsp_cluster",source.bsp_cluster},
                    {"material_slot",slot},{"lightmap",source.lightmap},{"barycentric",vector(barycentric)},
                    {"p0",vector(p0)},{"p1",vector(p1)},{"p2",vector(p2)},
                    {"n0",vector(xyz(triangle.n0))},{"n1",vector(xyz(triangle.n1))},{"n2",vector(xyz(triangle.n2))},
                    {"point",vector(point)},{"shading_normal",vector(normal)},{"geometric_normal",vector(geometric)},
                    {"normal_alignment",dot(normal,geometric)},{"shading_sun",shading_sun},{"geometric_sun",geometric_sun},
                    {"point_plane_distance",plane_distance(point)},{"current_origin",vector(current_origin)},
                    {"current_origin_plane_distance",plane_distance(current_origin)},{"reference_origin",vector(reference_origin)},
                    {"reference_origin_plane_distance",plane_distance(reference_origin)},
                    {"current_hit",hit_json(current_found,current,scene)},{"reference_hit",hit_json(reference_found,reference,scene)}};
                if(own)summary.own_examples.push_back(row);else summary.near_examples.push_back(row);
            }
        }
    }
    nlohmann::json rows=nlohmann::json::array();bool passed=true;std::size_t tested{},own_hits{};
    for(const auto& [name,s]:summaries){
        passed=passed && s.samples && !s.current_own_hits && !s.reference_own_hits && !s.invalid_normal &&
            s.plane_own_ignored==s.terminator_current_own;
        tested+=s.samples;own_hits+=s.current_own_hits;
        rows.push_back({{"name",name},{"triangles",s.triangles},{"samples",s.samples},{"reversed_winding_samples",s.reversed_winding},
            {"invalid_normal_samples",s.invalid_normal},{"below_geometric_sun_samples",s.below_geometric_sun},
            {"below_shading_sun_samples",s.below_shading_sun},{"minimum_normal_alignment",s.minimum_normal_alignment},
            {"current_hits",s.current_hits},{"reference_hits",s.reference_hits},{"current_own_hits",s.current_own_hits},
            {"reference_own_hits",s.reference_own_hits},{"near_shared_edge_hits",s.near_shared_edge_hits},
            {"visibility_differences",s.visibility_differences},{"own_examples",s.own_examples},{"near_examples",s.near_examples},
            {"geometric_terminator",{{"samples",s.terminator_samples},{"unsupported_vertex_hemispheres",s.terminator_unsupported},
                {"current_own_hits",s.terminator_current_own},{"current_adjacent_hits",s.terminator_current_adjacent},
                {"current_smooth_edge_hits",s.terminator_current_smooth_edge},
                {"current_other_hits",s.terminator_current_other},{"smooth_own_hits",s.terminator_smooth_own},
                {"starting_plane_own_ignored",s.plane_own_ignored},{"starting_plane_adjacent_ignored",s.plane_adjacent_ignored},
                {"starting_plane_other_coplanar_ignored",s.plane_other_ignored},{"starting_plane_distinct_hits_preserved",s.plane_distinct_preserved},
                {"smooth_adjacent_hits",s.terminator_smooth_adjacent},{"smooth_other_hits",s.terminator_smooth_other},
                {"cleared_hits",s.terminator_cleared},{"maximum_smooth_displacement",s.terminator_maximum_displacement},
                {"examples",s.terminator_examples}}}});
    }
    passed=passed && summaries.size()>=2 && tested;
    nlohmann::json report{{"passed",passed},{"map",argv[1]},{"mcc",true},{"sky",sky->name},
        {"starting_surface_classifier","Exact candidate plane and triangle interior at the source point; candidate geometric back hemisphere; existing epsilon plus coordinate ULP roundoff"},
        {"source_sun_incident",vector(frame.incident)},{"sun_direction",vector(direction)},{"source_sun_radiosity",vector(frame.radiosity)},
        {"bsp_triangles",mesh.vertices.size()/3},{"admitted_triangles",scene.triangles.size()},
        {"emitter_energy_census",emitter_census(scene,mesh.materials)},
        {"smooth_surface_control",smooth_surface_control()},
        {"epsilon",epsilon},{"material_cohorts",std::move(rows)},
        {"scope","Actual prepared Coagulation BSP centroid/interior barycentric source-normal direct-sun self-hit probe. Uses production CPU Scene/intersect and current shading-normal offset. Requires both smooth and true geometric normals to face the actual source sun. Only same-source-triangle hits fail; adjacent-edge and visibility differences are diagnostic, since real nearby geometry may occlude. Geometric plus one-ULP offset is a comparison, not an accepted production fix. No GPU or user-screenshot reproduction claim."}};
    std::ofstream output(argv[2]);require(bool(output),"Cannot save actual BSP self-shadow report");output<<report.dump(2)<<'\n';
    std::cout<<(passed?"PASS":"FAIL")<<" actual Coagulation source self-shadow: samples="<<tested<<" own_hits="<<own_hits<<'\n';
    return passed?0:1;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
