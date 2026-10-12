#include "halo2/source_object_admission.hpp"
#include "halo2/source_object_spatial_pool.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void require(bool test,const char *message){if(!test)throw FormatError(message);}
std::vector<std::uint32_t> words(const std::string &line){std::istringstream s(line);std::vector<std::uint32_t> out;std::uint32_t v;while(s>>std::hex>>v)out.push_back(v);return out;}
std::vector<std::uint32_t> probe(const std::vector<std::uint32_t> &input,bool raw_pool=false) {
    std::size_t at=0;auto take=[&](){require(at<input.size(),"truncated source object admission fixture");return input[at++];};
    auto scalar=[&](){return std::bit_cast<float>(take());};
    SourceObjectAdmissionInput in;in.capacity=take();in.flags=take();for(auto &v:in.clipping_plane)v=scalar();in.sky_cluster=std::int16_t(take());
    SourcePortalVisibilitySet visible;visible.groups.resize(take());const auto entries=take();
    for(unsigned i=0;i<entries;++i){SourceClusterFrustumRanges c;c.source_cluster=std::int16_t(take());for(auto &v:c.counts)v=std::int16_t(take());for(auto &v:c.first_indices)v=std::int16_t(take());visible.clusters.push_back(c);}
    const auto frustums=take();for(unsigned i=0;i<frustums;++i){SourceCameraFrustum f;for(auto &v:f.bounds)v=scalar();for(auto &p:f.planes)for(auto &v:p)v=scalar();visible.frustums.push_back(f);}
    const auto clusters=take();std::vector<std::uint8_t> fog(clusters);for(auto &v:fog)v=std::uint8_t(take());in.containing_fog=fog;
    ObjectLightSpatialLists collidable(clusters),noncollidable(clusters);
    SourceObjectSpatialPool pool_collidable(clusters),pool_noncollidable(clusters,false);
    std::array<unsigned,2> inserted{};
    std::vector<SourceObjectAdmissionObject> objects;
    const auto owners=take();for(unsigned i=0;i<owners;++i){SourceObjectAdmissionObject o;o.full_handle=take();o.flags=take();o.type=std::int8_t(take());o.rendering_sphere={{scalar(),scalar(),scalar()},scalar()};
        const auto kind=take(),count=take();std::vector<std::int16_t> memberships;for(unsigned j=0;j<count;++j)memberships.push_back(std::int16_t(take()));
        const ObjectLightSpatialPrimitive primitive{o.type,0,o.rendering_sphere.center,o.rendering_sphere.radius};
        (kind?collidable:noncollidable).insert(o.full_handle,primitive,memberships);
        std::uint32_t reference_head=0xffffffffu;
        (kind?pool_collidable:pool_noncollidable).insert(o.full_handle,reference_head,primitive,memberships);
        inserted[bool(kind)]+=count;objects.push_back(o);}
    const auto prior_count=take();std::vector<std::array<std::uint32_t,4>> prior(prior_count);for(auto &mask:prior)for(auto &v:mask)v=take();
    require(at==input.size(),"trailing source object admission fixture words");
    require(inserted[0]==pool_noncollidable.entries.active_count &&
        inserted[1]==pool_collidable.entries.active_count,
        "successful-only original admission fixture exceeds actual spatial pool capacity");
    const auto out=raw_pool?source_object_admission(visible,pool_collidable,pool_noncollidable,objects,in,prior):
        source_object_admission(visible,collidable,noncollidable,objects,in,prior);
    std::vector<std::uint32_t> result{unsigned(out.objects.size())};
    for(const auto &r:out.objects){result.insert(result.end(),{r.id,r.flags,r.other,r.association});require(out.object(r.id)==&r,"source object consumer lost full handle");}
    result.push_back(unsigned(out.associations.size()));for(const auto &mask:out.associations)result.insert(result.end(),mask.begin(),mask.end());
    return result;
}
std::vector<std::uint32_t> bounds(const std::vector<std::uint32_t> &input) {
    require(input.size()==14,"invalid cold registration sphere fixture");
    const auto f=[&](unsigned i){return std::bit_cast<float>(input.at(i));};
    const ObjectLightSpatialDefinition d{std::uint16_t(input[0]),f(1),{f(2),f(3),f(4)}};
    const ObjectLightSpatialSphere query{{f(5),f(6),f(7)},f(8)},previous{{f(10),f(11),f(12)},f(13)};
    const auto out=source_object_light_spatial_bounds(d,query,f(9));std::vector<std::uint32_t> result;
    for(const auto &s:{out.dynamic,out.primitive})for(float v:{s.center.x,s.center.y,s.center.z,s.radius})result.push_back(std::bit_cast<std::uint32_t>(v));
    result.push_back(source_object_light_spatial_bounds_changed(previous,out.primitive,false));return result;
}
void print(const auto &v){for(auto word:v)std::cout<<std::hex<<word<<' ';std::cout<<'\n';}
std::vector<std::uint32_t> spatial_probe(const std::vector<std::uint32_t> &input,
    const ObjectLightSpatialBsp *authored=nullptr) {
    std::size_t at=0;auto take=[&](){require(at<input.size(),"truncated source membership fixture");return input[at++];};
    auto scalar=[&](){return std::bit_cast<float>(take());};ObjectLightSpatialBsp bank;
    if(!authored){
        const auto planes=take();for(unsigned i=0;i<planes;++i)bank.planes.push_back({scalar(),scalar(),scalar(),scalar()});
        const auto clusters=take();for(unsigned i=0;i<clusters;++i){std::vector<std::uint16_t> ids;const auto count=take();for(unsigned j=0;j<count;++j)ids.push_back(std::uint16_t(take()));bank.clusters.push_back(std::move(ids));}
        const auto portals=take();for(unsigned i=0;i<portals;++i){ObjectLightSpatialBsp::Portal p;p.back=std::int16_t(take());p.front=std::int16_t(take());p.plane=take();p.center={scalar(),scalar(),scalar()};p.radius=scalar();const auto count=take();for(unsigned j=0;j<count;++j)p.vertices.push_back({scalar(),scalar(),scalar()});bank.portals.push_back(std::move(p));}
    }
    const auto &source=authored?*authored:bank;const auto count=take();std::vector<std::uint32_t> result;
    for(unsigned i=0;i<count;++i){const auto kind=take(),index=take();const Vec3 center{scalar(),scalar(),scalar()};const float radius=scalar();
        if(kind==0)result.push_back(source_object_light_portal_intersects(source,std::uint16_t(index),center,radius));
        else {require(kind==1,"unknown source membership query");const auto count=take();std::vector<std::uint32_t> forced(count);for(auto &v:forced)v=take();const auto out=source_object_light_spatial_membership(source,std::int16_t(index),center,radius,forced);
            result.insert(result.end(),{out.total,unsigned(out.truncated),unsigned(out.clusters.size())});for(const auto v:out.clusters)result.push_back(unsigned(v)&65535);}}
    require(at==input.size(),"trailing source membership fixture words");return result;
}
unsigned spatial_fixture(const std::filesystem::path &path) {
    std::ifstream file(path);require(bool(file),"source membership fixture unavailable");std::string line;unsigned count=0;
    std::vector<std::pair<std::filesystem::path,ObjectLightSpatialBsp>> banks;
    while(std::getline(file,line)){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');require(split!=std::string::npos,"missing original membership words");std::istringstream source(line.substr(0,split));std::string kind;source>>kind;const ObjectLightSpatialBsp *bank=nullptr;
        if(kind=="asset"){std::filesystem::path asset;source>>asset;auto found=std::find_if(banks.begin(),banks.end(),[&](const auto &v){return v.first==asset;});if(found==banks.end()){banks.emplace_back(asset,Cache(asset).load_object_light_spatial_bsp());found=std::prev(banks.end());}bank=&found->second;}
        else require(kind=="synthetic","unknown source membership fixture bank");
        std::string input;std::getline(source,input);const auto actual=spatial_probe(words(input),bank),expected=words(line.substr(split+1));
        if(actual!=expected){std::cerr<<path<<" membership fixture "<<count<<" differs\n";print(actual);throw FormatError("native sphere membership differs from untouched original");}++count;}
    require(count!=0,"empty original membership fixture");return count;
}
unsigned fixture(const std::filesystem::path &path,bool cold) {
    std::ifstream file(path);require(bool(file),"source object admission fixture unavailable");std::string line;unsigned count=0;
    while(std::getline(file,line)){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');require(split!=std::string::npos,"missing original object expected words");
        const auto expected=words(line.substr(split+1)),actual=cold?bounds(words(line.substr(0,split))):probe(words(line.substr(0,split)));
        if(actual!=expected){std::cerr<<path<<" fixture "<<count<<" differs\n";print(actual);throw FormatError("native object admission differs from untouched original");}
        if(!cold)require(probe(words(line.substr(0,split)),true)==expected,
            "literal spatial-pool admission differs from untouched original queue");
        ++count;}
    require(count!=0,"empty original object admission fixture");return count;
}
}
int main(int argc,char **argv){try{
    if(argc==2 && std::string(argv[1])=="--probe"){std::string line;while(std::getline(std::cin,line))if(!line.empty())print(probe(words(line)));return 0;}
    if(argc==2 && std::string(argv[1])=="--spatial-probe"){std::string line;while(std::getline(std::cin,line))if(!line.empty())print(spatial_probe(words(line)));return 0;}
    if(argc==3 && std::string(argv[1])=="--spatial"){const auto count=spatial_fixture(argv[2]);std::cout<<count<<" untouched source portal/membership fixtures exact\n";return 0;}
    require(argc==3,"expected source object admission and cold fixture paths");
    const auto count=fixture(argv[1],false),cold=fixture(argv[2],true);
    std::cout<<count<<" original object queue cases exact through lists and literal spatial pools; "<<cold<<" cold registration spheres exact\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
