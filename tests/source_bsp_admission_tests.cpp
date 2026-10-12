#include "halo2/source_bsp_admission.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void require(bool v,const char *m){if(!v)throw FormatError(m);}
std::vector<std::uint32_t> words(const std::string &line){std::istringstream s(line);std::vector<std::uint32_t> out;std::uint32_t w;while(s>>std::hex>>w)out.push_back(w);return out;}
std::vector<std::uint32_t> probe(const std::vector<std::uint32_t> &input) {
    std::size_t at=0;auto take=[&](){require(at<input.size(),"truncated BSP admission fixture");return input[at++];};
    auto scalar=[&](){return std::bit_cast<float>(take());};
    SourceBspAdmissionInput context;context.cluster_capacity=take();context.instance_capacity=take();context.flags=take();context.context_2a78=take();
    for(auto &v:context.clipping_plane)v=scalar();
    SourcePortalVisibilitySet visible;visible.groups.resize(take());const auto nc=take();
    for(unsigned i=0;i<nc;++i){SourceClusterFrustumRanges c;c.source_cluster=std::int16_t(take());for(auto &v:c.counts)v=std::int16_t(take());for(auto &v:c.first_indices)v=std::int16_t(take());visible.clusters.push_back(c);}
    const auto nf=take();for(unsigned i=0;i<nf;++i){SourceCameraFrustum f;for(auto &v:f.bounds)v=scalar();for(auto &p:f.planes)for(auto &v:p)v=scalar();visible.frustums.push_back(f);}
    SourceBspAdmissionBsp bsp;const auto nd=take();for(unsigned i=0;i<nd;++i)bsp.definitions.push_back({std::uint16_t(take())});
    const auto ni=take();for(unsigned i=0;i<ni;++i)bsp.instances.push_back({std::int16_t(take()),{scalar(),scalar(),scalar()},scalar()});
    const auto n=take();std::vector<std::uint8_t> fog;for(unsigned i=0;i<n;++i){SourceBspAdmissionCluster c;c.geometry_count=std::uint16_t(take());for(auto &v:c.bounds)v=scalar();
        const auto ids=take();for(unsigned j=0;j<ids;++j)c.instances.push_back(std::uint16_t(take()));bsp.clusters.push_back(std::move(c));fog.push_back(std::uint8_t(take()));}
    SourceBspAdmission auxiliary;
    if(at<input.size()){
        require(take()==0x314c5053,"invalid sphere-mode BSP fixture extension");
        context.mode=take();context.sphere_center={scalar(),scalar(),scalar()};context.sphere_radius=scalar();
        if(take()){
            const auto clusters=take();for(unsigned i=0;i<clusters;++i)auxiliary.clusters.push_back({take(),0,0xffff,0});
            const auto instances=take();for(unsigned i=0;i<instances;++i)auxiliary.instances.push_back({take(),0,0xffff,0});
            context.auxiliary=&auxiliary;
        }
    }
    require(at==input.size(),"trailing BSP admission fixture words");context.containing_fog=fog;
    const auto out=source_bsp_admission(bsp,visible,context);std::vector<std::uint32_t> result;
    for(unsigned i=0;i<512;i+=4)result.push_back(unsigned(out.cluster_entries[i])|(unsigned(out.cluster_entries[i+1])<<8)|(unsigned(out.cluster_entries[i+2])<<16)|(unsigned(out.cluster_entries[i+3])<<24));
    result.insert(result.end(),out.marked_clusters.begin(),out.marked_clusters.end());
    for(const auto &queue:{out.clusters,out.instances}){result.push_back(unsigned(queue.size()));for(const auto &r:queue)result.insert(result.end(),{r.id,r.flags,r.other,r.association});}
    result.push_back(unsigned(out.associations.size()));for(const auto &mask:out.associations)result.insert(result.end(),mask.begin(),mask.end());
    // Public consumers must use admitted records, not merely marked clusters.
    for(unsigned i=0;i<bsp.clusters.size();++i){const auto found=std::find_if(out.clusters.begin(),out.clusters.end(),[&](const auto &r){return r.id==i;});
        require(out.geometry_visible(int(i))==(found!=out.clusters.end()),"BSP geometry consumer differs from admitted queue");
        require(out.geometry_contains_fog(int(i))==(found!=out.clusters.end()&&bool(found->flags&0x400)),"BSP fog consumer differs from source flags");}
    for(unsigned i=0;i<bsp.instances.size();++i){const auto *r=out.instance(i);require(out.geometry_visible(-2-int(i))==(r!=nullptr),"instance geometry consumer differs from admitted queue");require(out.geometry_contains_fog(-2-int(i))==(r&&bool(r->flags&0x400)),"instance fog consumer differs from source flags");}
    require(out.geometry_visible(-1)&&!out.geometry_contains_fog(-1),"unowned geometry boundary changed");return result;
}
std::uint64_t hash(std::span<const std::byte> bytes){std::uint64_t h=14695981039346656037ull;for(auto b:bytes)h=(h^std::to_integer<unsigned>(b))*1099511628211ull;return h;}
std::vector<std::uint32_t> assets(const std::filesystem::path &path) {
    Cache cache(path);const auto b=cache.load_source_bsp_admission(0);std::vector<std::byte> clusters,instances,defs;
    auto put=[](auto &out,const auto &v){const auto bytes=std::as_bytes(std::span(&v,1));out.insert(out.end(),bytes.begin(),bytes.end());};
    unsigned memberships=0;for(const auto &c:b.clusters){put(clusters,c.geometry_count);for(auto v:c.bounds)put(clusters,v);const auto n=unsigned(c.instances.size());memberships+=n;put(clusters,n);for(auto v:c.instances)put(clusters,v);}
    for(const auto &i:b.instances){put(instances,i.definition);put(instances,i.center);put(instances,i.radius);}
    for(const auto &d:b.definitions)put(defs,d.geometry_count);
    const auto ch=hash(clusters),ih=hash(instances),dh=hash(defs);
    return {unsigned(b.clusters.size()),unsigned(b.instances.size()),unsigned(b.definitions.size()),memberships,unsigned(ch),unsigned(ch>>32),unsigned(ih),unsigned(ih>>32),unsigned(dh),unsigned(dh>>32)};
}
void print(const auto &v){for(auto w:v)std::cout<<std::hex<<w<<' ';std::cout<<'\n';}
std::vector<std::uint32_t> fog_probe(const std::vector<std::uint32_t> &input) {
    std::size_t at=0;auto take=[&](){require(at<input.size(),"truncated16E210 fixture");return input[at++];};
    SourceBspFogAdmissionInput context;context.scenario_loaded=take()!=0;context.bsp_loaded=take()!=0;context.datum=take();context.flags_4e64c8=take();
    BspAtmosphere atmosphere;const auto nc=take();for(unsigned i=0;i<nc;++i){BspAtmosphere::Cluster c;c.planar_designator=std::int8_t(take());atmosphere.clusters.push_back(c);}
    const auto nf=take();for(unsigned i=0;i<nf;++i){BspAtmosphere::FogPlane p;p.palette=std::int16_t(take());atmosphere.fog_planes.push_back(p);}
    const auto np=take();for(unsigned i=0;i<np;++i){BspAtmosphere::FogPalette p;p.datum=take();atmosphere.fog_palette.push_back(p);}
    require(at==input.size(),"trailing16E210 fixture words");const auto out=source_bsp_fog_admission(atmosphere,context);std::vector<std::uint32_t> result(out.begin(),out.end());
    result.push_back(source_bsp_cluster_contains_fog(atmosphere,-1,context));return result;
}
std::vector<std::uint32_t> fog_assets(const std::filesystem::path &path) {
    Cache cache(path);const auto b=cache.load_bsp_atmosphere(0);std::vector<std::uint32_t> result{unsigned(b.clusters.size())};
    for(const auto &c:b.clusters)result.push_back(unsigned(c.planar_designator)&255);
    result.push_back(unsigned(b.fog_planes.size()));for(const auto &p:b.fog_planes)result.push_back(unsigned(p.palette)&65535);
    result.push_back(unsigned(b.fog_palette.size()));for(const auto &p:b.fog_palette)result.push_back(p.datum);
    return result;
}
}
int main(int argc,char **argv){try {
    if(argc==4&&std::string_view(argv[1])=="--sphere-clusters"){
        const auto spatial=Cache(argv[2]).load_object_light_spatial_bsp(0);
        std::ifstream fixture(argv[3]);require(bool(fixture),"sphere cluster fixture missing");unsigned cases=0;
        for(std::string line;std::getline(fixture,line);){if(line.empty()||line[0]=='#')continue;
            const auto split=line.find('|');require(split!=std::string::npos,"sphere cluster fixture malformed");
            const auto input=words(line.substr(0,split)),expected=words(line.substr(split+1));require(input.size()==5,"sphere cluster input length");
            const auto f=[&](unsigned i){return std::bit_cast<float>(input[i]);};
            const auto out=source_bsp_sphere_clusters(spatial,std::int16_t(input[0]),{f(1),f(2),f(3)},f(4));
            std::vector<std::uint32_t> actual{unsigned(out.clusters.size())};
            for(auto id:out.clusters)actual.push_back(std::uint32_t(id));actual.insert(actual.end(),out.sorted_indices.begin(),out.sorted_indices.end());
            if(actual!=expected)throw FormatError("original sphere-cluster mismatch case "+std::to_string(cases));++cases;
        }
        require(cases>100,"sphere-cluster fixture lacks coverage");std::cout<<"PASS "<<cases<<" whole-original sphere-cluster calls\n";return 0;
    }
    if(argc==2&&std::string_view(argv[1])=="--capacities"){print(std::array{source_bsp_main_cluster_capacity,source_bsp_main_instance_capacity});return 0;}
    if(argc==2&&std::string_view(argv[1])=="--probe"){for(std::string line;std::getline(std::cin,line);)if(!line.empty())print(probe(words(line)));return 0;}
    if(argc==2&&std::string_view(argv[1])=="--assets"){for(std::string line;std::getline(std::cin,line);)if(!line.empty())print(assets(line));return 0;}
    if(argc==2&&std::string_view(argv[1])=="--fog-probe"){for(std::string line;std::getline(std::cin,line);)if(!line.empty())print(fog_probe(words(line)));return 0;}
    if(argc==2&&std::string_view(argv[1])=="--fog-assets"){for(std::string line;std::getline(std::cin,line);)if(!line.empty())print(fog_assets(line));return 0;}
    if(argc==4&&std::string_view(argv[1])=="--fog"){std::ifstream fixture(argv[2]);require(bool(fixture),"source16E210 fixture missing");unsigned cases=0;
        for(std::string line;std::getline(fixture,line);){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');require(split!=std::string::npos,"source16E210 fixture malformed");require(fog_probe(words(line.substr(0,split)))==words(line.substr(split+1)),"source16E210 membership differs");++cases;}
        require(cases>=128,"source16E210 fixture incomplete");std::ifstream asset(argv[3]);require(bool(asset),"source16E210 asset fixture missing");unsigned maps=0;
        for(std::string line;std::getline(asset,line);){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');require(split!=std::string::npos,"source16E210 asset fixture malformed");auto path=line.substr(0,split);while(!path.empty()&&path.back()==' ')path.pop_back();require(fog_assets(path)==words(line.substr(split+1)),"source16E210 authored atmosphere fields differ");++maps;}
        require(maps==21,"source16E210 map coverage incomplete");std::cout<<cases<<" untouched16E210 banks and all21 atmosphere metadata banks match\n";return 0;}
    require(source_bsp_main_cluster_capacity==128&&source_bsp_main_instance_capacity==384,"original131FF0 main queue initializer changed");
    require(argc==3,"original BSP admission and asset fixtures required");std::ifstream fixture(argv[1]);require(bool(fixture),"BSP admission fixture missing");unsigned cases=0;
    for(std::string line;std::getline(fixture,line);){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');require(split!=std::string::npos,"BSP admission fixture malformed");
        if(probe(words(line.substr(0,split)))!=words(line.substr(split+1)))throw FormatError("original BSP admission differs at case "+std::to_string(cases));++cases;}
    require(cases>=128,"BSP admission original fixture incomplete");std::ifstream asset(argv[2]);require(bool(asset),"BSP admission asset fixture missing");unsigned maps=0;
    for(std::string line;std::getline(asset,line);){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');require(split!=std::string::npos,"BSP admission assets malformed");auto path=line.substr(0,split);while(!path.empty()&&path.back()==' ')path.pop_back();require(assets(path)==words(line.substr(split+1)),"BSP admission authored cache bytes differ");++maps;}
    require(maps==21,"BSP admission map coverage incomplete");std::cout<<cases<<" original134390 admission cases and all21 authored banks match\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
