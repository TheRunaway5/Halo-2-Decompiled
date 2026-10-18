#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/bsp_lighting.hpp"
#include "halo2/original_data.hpp"
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using namespace halo2;
void require(bool value,const std::string& message) {
    if(!value)throw std::runtime_error(message);
}
bool same(float a,float b) {
    return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);
}
bool same(Vec2 a,Vec2 b) {return same(a.x,b.x)&&same(a.y,b.y);}
bool same(Vec3 a,Vec3 b) {return same(a.x,b.x)&&same(a.y,b.y)&&same(a.z,b.z);}
struct Counts {std::size_t geometries{},vertices{},lightmap_components{},incident_words{},bucket_words{},expanded_vertices{};};
void compare_buckets(const BspLighting& a,const BspLighting& b,Counts& counts) {
    require(a.buckets.size()==b.buckets.size(),"static lightmap bucket count differs");
    for(std::size_t i=0;i<a.buckets.size();++i) {
        const auto& x=a.buckets[i];const auto& y=b.buckets[i];
        require(bsp_incident_count(x)==y.incident.size(),"static lightmap incident count differs");
        require(y.incident_vectors.empty(),"original fixture has PC incident vectors");
        require(x.radiance==y.radiance,"static lightmap radiance bytes differ");
        for(std::size_t v=0;v<y.incident.size();++v) {
            const auto word=x.incident_vectors.empty()?x.incident[v]:pack_bsp_incident_tu15(x.incident_vectors[v]);
            require(word==y.incident[v],"static lightmap incident code differs");
            ++counts.bucket_words;
        }
    }
}
void compare_mesh(const Mesh& a,const Mesh& b) {
    require(a.vertices.size()==b.vertices.size(),"expanded source vertex count differs");
    for(std::size_t i=0;i<a.vertices.size();++i) {
        const auto& x=a.vertices[i];const auto& y=b.vertices[i];
        const auto prefix="expanded vertex "+std::to_string(i)+": ";
        require(same(x.position,y.position),prefix+"position bits differ");
        require(same(x.uv,y.uv),prefix+"diffuse coordinate bits differ");
        require(same(x.normal,y.normal),prefix+"normal bits differ");
        require(same(x.tangent,y.tangent),prefix+"tangent bits differ");
        require(same(x.binormal,y.binormal),prefix+"binormal bits differ");
        require(same(x.lightmap_uv,y.lightmap_uv),prefix+"GPU lightmap coordinate bits differ");
        require(same(x.incident,y.incident),prefix+"GPU incident bits differ");
        require(x.source_section==y.source_section&&x.source_part==y.source_part&&
            x.source_part_type==y.source_part_type&&x.source_part_flags==y.source_part_flags,
            prefix+"source part attribution differs");
    }
}
void compare(const std::vector<BspSurfaceLighting::Geometry>& converted,
    const std::vector<BspSurfaceLighting::Geometry>& reference,Counts& counts) {
    require(converted.size()==reference.size(),"source geometry section count differs");
    for(std::size_t section=0;section<converted.size();++section) {
        const auto& a=converted[section];const auto& b=reference[section];
        const auto prefix="section "+std::to_string(section)+": ";
        require(a.positions.size()==b.positions.size(),prefix+"position count differs");
        require(a.diffuse_uv.size()==b.diffuse_uv.size(),prefix+"diffuse coordinate count differs");
        require(a.lightmap_uv.size()==b.lightmap_uv.size(),prefix+"lightmap coordinate count differs");
        require(a.indices==b.indices,prefix+"source triangle indices differ");
        require(a.incident.size()==b.incident.size(),prefix+"CPU-selected incident count differs");
        for(std::size_t i=0;i<a.incident.size();++i)
            require(a.incident[i]==b.incident[i],prefix+"CPU incident vertex "+std::to_string(i)+
                " differs: MCC="+std::to_string(a.incident[i])+", Xbox="+std::to_string(b.incident[i]));
        require(a.parts.size()==b.parts.size(),prefix+"part count differs");
        for(std::size_t i=0;i<a.parts.size();++i) {
            const auto& x=a.parts[i];const auto& y=b.parts[i];
            require(x.first==y.first&&x.count==y.count&&x.material==y.material&&x.type==y.type,
                prefix+"CPU part attribution differs");
        }
        for(std::size_t i=0;i<a.positions.size();++i)
            require(same(a.positions[i],b.positions[i]),prefix+"source position bits differ");
        for(std::size_t i=0;i<a.diffuse_uv.size();++i)
            require(same(a.diffuse_uv[i],b.diffuse_uv[i]),prefix+"diffuse coordinate bits differ");
        for(std::size_t i=0;i<a.lightmap_uv.size();++i)
            require(same(a.lightmap_uv[i],b.lightmap_uv[i]),prefix+"CPU lightmap coordinate bits differ");
        ++counts.geometries;counts.vertices+=a.positions.size();
        counts.lightmap_components+=a.lightmap_uv.size()*2;counts.incident_words+=a.incident.size();
    }
}
}
int main(int argc,char** argv)try {
    if(argc==1 || (argc==3 && (!*argv[1] || !*argv[2])))return 77;
    if(argc==4 && std::string_view(argv[1])=="--mcc-gpu-only") {
        const Cache cache(argv[3]);require(cache.is_mcc(),"fixture is not an MCC map");
        const auto bsp=std::stoul(argv[2]);require(bsp<cache.bsps().size(),"selected BSP is outside the map");
        const auto plain=cache.load_bsp(bsp,false),textured=cache.load_bsp(bsp,true);
        require(!plain.vertices.empty()&&plain.vertices.size()==textured.vertices.size(),"incomplete MCC GPU geometry");
        for(const auto& v:textured.vertices)
            require(std::isfinite(v.position.x)&&std::isfinite(v.position.y)&&std::isfinite(v.position.z)&&
                std::isfinite(v.lightmap_uv.x)&&std::isfinite(v.lightmap_uv.y),"nonfinite MCC GPU inputs");
        std::cout<<"PASS MCC-only textured/untextured GPU input loading: "<<plain.vertices.size()<<" expanded vertices\n";
        return 0;
    }
    const bool selected=argc==5 && std::string_view(argv[1])=="--bsp";
    require(argc==3 || selected,"Supply matching MCC and original Xbox map fixtures, optionally --bsp INDEX");
    if(std::string_view(argv[1])=="--mcc-only") {
        const Cache cache(argv[2]);require(cache.is_mcc(),"fixture is not an MCC map");
        Counts counts;
        for(std::size_t i=0;i<cache.bsps().size();++i) {
            const auto bank=cache.load_bsp_surface_lighting(i);
            for(const auto* group:{&bank.clusters,&bank.definitions})for(const auto& geometry:*group) {
                const auto count=geometry.positions.size();
                require(geometry.diffuse_uv.size()==count,"incomplete diffuse coordinate stream");
                require(geometry.lightmap_uv.empty()||geometry.lightmap_uv.size()==count,
                    "incomplete canonical lightmap coordinate stream");
                require(geometry.incident.empty()||geometry.incident.size()==count,
                    "incomplete CPU-selected incident stream");
                for(const auto uv:geometry.diffuse_uv)
                    require(std::isfinite(uv.x)&&std::isfinite(uv.y),"nonfinite diffuse coordinates");
                for(const auto uv:geometry.lightmap_uv)
                    require(std::isfinite(uv.x)&&std::isfinite(uv.y),"nonfinite canonical lightmap coordinates");
                ++counts.geometries;counts.vertices+=count;
                counts.lightmap_components+=geometry.lightmap_uv.size()*2;
                counts.incident_words+=geometry.incident.size();
            }
        }
        require(counts.vertices>0,"fixture has no render vertices");
        std::cout<<"PASS MCC-only "<<counts.geometries<<" geometry sections, "<<counts.vertices
            <<" vertices, "<<counts.lightmap_components<<" canonical lightmap components, "
            <<counts.incident_words<<" CPU-selected incident words\n";
        return 0;
    }
    const Cache converted(argv[selected?3:1]),reference(argv[selected?4:2],{},OriginalData::native_tu15());
    require(converted.is_mcc()&&!reference.is_mcc()&&!reference.is_vista(),"fixture formats differ");
    const auto* a_scenario=converted.find_tag(converted.scenario_tag());
    const auto* b_scenario=reference.find_tag(reference.scenario_tag());
    require(a_scenario&&b_scenario&&a_scenario->name==b_scenario->name,"source scenario identity differs");
    require(converted.bsps().size()==reference.bsps().size(),"source BSP count differs");
    const auto selected_bsp=selected?std::stoul(argv[2]):0;
    require(!selected || selected_bsp<converted.bsps().size(),"selected BSP is outside the map");
    Counts counts;
    for(std::size_t i=0;i<converted.bsps().size();++i) {
      if(selected && i!=selected_bsp)continue;
      try {
        const auto a=converted.load_bsp_surface_lighting(i),b=reference.load_bsp_surface_lighting(i);
        compare(a.clusters,b.clusters,counts);compare(a.definitions,b.definitions,counts);
        compare_buckets(a.lighting,b.lighting,counts);
        const auto ma=converted.load_bsp(i,false),mb=reference.load_bsp(i,false);
        compare_mesh(ma,mb);counts.expanded_vertices+=ma.vertices.size();
        compare_mesh(converted.load_bsp(i,true),reference.load_bsp(i,true));
      }catch(const std::exception& error) {
        throw std::runtime_error("BSP "+std::to_string(i)+": "+error.what());
      }
    }
    require(counts.geometries&&counts.vertices&&counts.lightmap_components&&counts.incident_words,
        "paired fixture has no resident lighting streams");
    std::cout<<"PASS "<<counts.geometries<<" source geometry sections, "<<counts.vertices
        <<" vertices, "<<counts.lightmap_components<<" exact CPU lightmap components, "
        <<counts.incident_words<<" exact CPU-selected packed incident words, "
        <<counts.bucket_words<<" exact static bucket incident words, "<<counts.expanded_vertices
        <<" exact expanded GPU vertices\n";
    return 0;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
