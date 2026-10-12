#include "halo2/object_light_query.hpp"
#include "halo2/math.hpp"
#include <bit>
#include <iostream>
#include <fstream>
using namespace halo2;
namespace {
void require(bool b,const std::string& message){if(!b)throw std::runtime_error(message);}
std::vector<std::filesystem::path> fixtures(const std::filesystem::path& maps) {
    if(maps.extension()==".map")return {maps};
    std::vector<std::filesystem::path> out;
    constexpr const char* base[]{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar"};
    for(auto name:base)out.push_back(maps/(std::string(name)+".map"));
    const auto pack=maps.parent_path().parent_path()/"map-pack/maps";
    if(std::filesystem::exists(pack)) {
        constexpr const char* extra[]{"backwash","containment","deltatap","dune","elongation","gemini","triplicate","turf","warlock"};
        for(auto name:extra)out.push_back(pack/(std::string(name)+".map"));
    }
    return out;
}
}
int main(int argc,char** argv) {try {
    if(argc==3 && std::string(argv[1])=="--instance-vector") {
        const Cache cache(argv[2]);const auto bank=cache.load_bsp_surface_lighting();
        unsigned which;Vec3 origin,ray;float maximum;
        while(std::cin>>which>>origin.x>>origin.y>>origin.z>>ray.x>>ray.y>>ray.z>>maximum) {
            const auto result=original_instance_vector_contact(bank,std::uint16_t(which),origin,ray,maximum);
            std::cout<<bool(result);
            if(result) {
                const auto& h=result->contact;
                std::cout<<' '<<std::bit_cast<std::uint32_t>(h.fraction)<<' '<<h.leaf<<' '<<h.surface
                    <<' '<<h.oriented_plane<<' '<<unsigned(h.flags)<<' '<<unsigned(h.breakable)<<' '<<h.material;
            }
            std::cout<<'\n';
        }
        return 0;
    }
    if(argc==3 && std::string(argv[1])=="--vector") {
        const Cache cache(argv[2]);const auto bank=cache.load_bsp_surface_lighting();
        int which;Vec3 origin,ray;float maximum;unsigned flags;
        while(std::cin>>which>>origin.x>>origin.y>>origin.z>>ray.x>>ray.y>>ray.z>>maximum>>flags) {
            const auto& collision=which>=0?bank.collision.at(which):bank.definition_collision.at(-which-2);
            const auto result=original_bsp_vector_query(collision,origin,ray,maximum,flags);
            std::cout<<bool(result.contact);
            if(result.contact) {
                const auto& h=*result.contact;
                std::cout<<' '<<std::bit_cast<std::uint32_t>(h.fraction)<<' '<<h.leaf<<' '<<h.surface<<' '<<h.plane
                    <<' '<<h.oriented_plane<<' '<<unsigned(h.flags)<<' '<<unsigned(h.breakable)<<' '<<h.material;
            }
            std::cout<<' '<<result.leaves.size();for(auto l:result.leaves)std::cout<<' '<<l;std::cout<<'\n';
        }
        return 0;
    }
    if(argc==2 && std::string(argv[1])=="--barycentric") {
        Vec3 a,b,c,p;
        while(std::cin>>a.x>>a.y>>a.z>>b.x>>b.y>>b.z>>c.x>>c.y>>c.z>>p.x>>p.y>>p.z) {
            float w_b=77,w_c=88;
            const auto hit=original_render_barycentrics(a,b,c,p,w_b,w_c);
            std::cout<<hit<<' '<<std::bit_cast<std::uint32_t>(w_b)<<' '<<std::bit_cast<std::uint32_t>(w_c)<<'\n';
        }
        return 0;
    }
    if(argc==2 && std::string(argv[1])=="--cpu-incident") {
        std::uint32_t packed;
        while(std::cin>>packed) {
            const auto v=decode_cpu_lightmap_incident(packed);
            std::cout<<std::bit_cast<std::uint32_t>(v.x)<<' '<<std::bit_cast<std::uint32_t>(v.y)<<' '<<std::bit_cast<std::uint32_t>(v.z)<<'\n';
        }
        return 0;
    }
    if(argc==2 && std::string(argv[1])=="--cpu-incident-interpolation") {
        BspSurfaceLighting bank;
        bank.lighting.structure_matches=true;
        bank.shaders={0};bank.clusters.resize(1);
        auto& geometry=bank.clusters.front();
        geometry.diffuse_uv.resize(3);geometry.indices={0,1,2};
        geometry.incident.resize(3);geometry.parts.push_back({0,3,0});
        RenderSurfaceHit hit;hit.cluster=hit.geometry=hit.part=0;
        while(std::cin>>geometry.incident[0]>>geometry.incident[1]>>geometry.incident[2]
                      >>hit.weight_b>>hit.weight_c) {
            const auto sample=bank.interpolate(hit);
            require(bool(sample),"original incident interpolation fixture was rejected");
            const auto v=sample->incident;
            std::cout<<std::bit_cast<std::uint32_t>(v.x)<<' '<<std::bit_cast<std::uint32_t>(v.y)
                     <<' '<<std::bit_cast<std::uint32_t>(v.z)<<'\n';
        }
        return 0;
    }
    require(argc==2,"source map or maps directory required");
    std::size_t total_attribution=0,total_spawn=0,total_resolved=0,total_unresolved=0;
    for(const auto& path:fixtures(argv[1])) {
        const Cache cache(path);const auto bank=cache.load_bsp_surface_lighting();
        auto content=cache.load_bsp_content(0);
        require(!content.collision.empty(),cache.name()+": no source collision fixture");
        std::size_t attributed=0,unresolved=0,sampled=0;
        for(std::size_t i=0;i<content.collision.size();i+=53) {
            const auto& t=content.collision[i];
            require(t.source.bsp==0 && t.source.surface>=0 && t.source.record>=0,cache.name()+": collision identity lost");
            const auto p=(t.a+t.b+t.c)/3.f;
            if(const auto hit=bank.attribute(t.source,p)) {
                ++attributed;
                if(const auto s=bank.sample(*hit)) {
                    require(std::isfinite(s->incident.x)&&std::isfinite(s->incident.y)&&std::isfinite(s->incident.z),"CPU incident sample nonfinite");
                    const auto& g=(hit->instance<0?bank.clusters:bank.definitions).at(hit->geometry);
                    const auto type=g.parts.at(hit->part).type;
                    if(type==4||type==5)require(s->diffuse_rgb==0xff404040u&&s->lightmap_rgb==0xff404040u&&
                        s->incident.x==0&&s->incident.y==0&&s->incident.z==-1,"source special-part initial sample changed");
                    else require(!(s->diffuse_rgb&0xff000000) && !(s->lightmap_rgb&0xff000000),"sample RGB layout changed");
                    ++sampled;
                }
            } else ++unresolved;
        }
        require(attributed>0,cache.name()+": no unambiguous source attribution");
        CollisionWorld world(content.collision);
        std::array<std::size_t,5> status{};
        std::size_t spawn_index=0;
        for(const auto& spawn:cache.spawns()) {
          for(unsigned mode:{0u,1u}) {
            const auto result=query_object_lightmap(bank,world,spawn.position,mode);
            ++status[unsigned(result.status)];++total_spawn;
            require(result.rays<=unsigned(mode?5:1),"source ray bank overrun");
            require(bool(result.sample)==(result.status==ObjectLightQueryStatus::resolved),"failed query replaced source sample");
            const bool authored_gap=cache.name()=="triplicate" && spawn_index==116 && mode==0;
            require(result.status==(authored_gap?ObjectLightQueryStatus::miss:ObjectLightQueryStatus::resolved),
                cache.name()+": source spawn query outcome changed");
            if(result.sample)++total_resolved;
            if(result.attribution_failures)++total_unresolved;
            if(result.status==ObjectLightQueryStatus::attribution_unresolved)
                std::cout<<cache.name()<<": unresolved source query at spawn "<<spawn_index<<", mode "<<mode
                    <<", origin "<<spawn.position.x<<' '<<spawn.position.y<<' '<<spawn.position.z<<'\n';
          }
          ++spawn_index;
        }
        total_attribution+=attributed;
        std::cout<<cache.name()<<": centroid attribution="<<attributed<<", sample="<<sampled<<", unresolved="<<unresolved
            <<"; spawn query resolved="<<status[0]<<", unavailable="<<status[1]<<", miss="<<status[2]<<", generated="<<status[3]<<", attribution-unresolved="<<status[4]<<'\n';
    }
    std::cout<<total_attribution<<" source render attributions; "<<total_resolved<<'/'<<total_spawn<<" authored spawn queries resolved; "<<total_unresolved<<" contain rejected attribution attempts\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
