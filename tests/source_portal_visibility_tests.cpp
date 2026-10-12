#include "halo2/source_portal_visibility.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
std::vector<std::uint32_t> words(const std::string &line) {
    std::istringstream stream(line);std::vector<std::uint32_t> out;std::uint32_t word;
    while(stream>>std::hex>>word)out.push_back(word);return out;
}
std::uint64_t hash(std::span<const std::byte> bytes){std::uint64_t h=14695981039346656037ull;for(const auto b:bytes)h=(h^std::to_integer<unsigned>(b))*1099511628211ull;return h;}
std::vector<std::uint32_t> assets(const std::filesystem::path &path) {
    Cache c(path);const auto b=c.load_source_portal_bsp(0);std::vector<std::byte> portals,clusters;
    auto append=[](auto &out,const auto &value){const auto bytes=std::as_bytes(std::span(&value,1));out.insert(out.end(),bytes.begin(),bytes.end());};
    for(const auto &p:b.portals){append(portals,p.back);append(portals,p.front);append(portals,p.plane_index);append(portals,p.flags);
        const auto n=std::uint32_t(p.vertices.size());append(portals,n);for(const auto v:p.vertices)append(portals,v);}
    for(const auto &v:b.cluster_portals){const auto n=std::uint32_t(v.size());append(clusters,n);for(const auto p:v)append(clusters,p);}
    const auto a=hash(std::as_bytes(std::span(b.planes))),p=hash(portals),cl=hash(clusters);
    return {unsigned(b.planes.size()),unsigned(b.portals.size()),unsigned(b.cluster_portals.size()),
        unsigned(a),unsigned(a>>32),unsigned(p),unsigned(p>>32),unsigned(cl),unsigned(cl>>32)};
}
std::vector<std::uint32_t> probe(const std::vector<std::uint32_t>&input) {
    std::size_t at=0;auto take=[&](){require(at<input.size(),"truncated source portal visibility input");return input[at++];};
    auto scalar=[&](){return std::bit_cast<float>(take());};
    const auto ng=take();std::vector<SourceFrustumCamera> groups;
    for(unsigned i=0;i<ng;++i){std::array<std::uint32_t,sizeof(SourceFrustumCamera)/4> w{};for(auto &v:w)v=take();groups.push_back(std::bit_cast<SourceFrustumCamera>(w));}
    const int cluster=std::int32_t(take());std::array<std::uint32_t,16> open{};for(auto &v:open)v=take();
    SourcePortalBsp bsp;const auto nc=take();for(unsigned i=0;i<nc;++i){const auto n=take();std::vector<std::uint16_t> p;for(unsigned j=0;j<n;++j)p.push_back(std::uint16_t(take()));bsp.cluster_portals.push_back(std::move(p));}
    const auto np=take();for(unsigned i=0;i<np;++i)bsp.planes.push_back({scalar(),scalar(),scalar(),scalar()});
    const auto nportal=take();for(unsigned i=0;i<nportal;++i){SourcePortalGeometry p;p.back=std::int16_t(take());p.front=std::int16_t(take());p.plane_index=take();p.flags=take();
        const auto n=take();for(unsigned j=0;j<n;++j)p.vertices.push_back({scalar(),scalar(),scalar()});bsp.portals.push_back(std::move(p));}
    require(at==input.size(),"trailing source portal visibility input");const auto out=source_portal_visibility(bsp,groups,cluster,open);
    std::vector<std::uint32_t> result{unsigned(out.clusters.size())};
    for(const auto &c:out.clusters){result.push_back(std::uint16_t(c.source_cluster));for(const auto v:c.counts)result.push_back(std::uint16_t(v));for(const auto v:c.first_indices)result.push_back(std::uint16_t(v));}
    result.insert(result.end(),out.sorted_cluster_indices.begin(),out.sorted_cluster_indices.end());result.push_back(out.frustums.size());
    for(const auto &f:out.frustums){const auto w=std::bit_cast<std::array<std::uint32_t,sizeof(f)/4>>(f);
        for(unsigned i=0;i<w.size();++i)if(i!=3)result.push_back(w[i]);}
    // Exercise the actual165010 consumer using the resulting bank as well.
    const auto spheres=out.sphere_bank();for(std::size_t i=0;i<out.clusters.size();++i){
        const auto found=source_cluster_frustum_visibility(spheres,i,groups[0].camera_to_world.position,.01f);
        result.push_back(found.visible);result.push_back(found.contained);}
    return result;
}
void print(const auto &v){for(auto word:v)std::cout<<std::hex<<word<<' ';std::cout<<'\n';}
}
int main(int argc,char **argv){try {
    if(argc==2 && std::string_view(argv[1])=="--probe"){for(std::string line;std::getline(std::cin,line);)if(!line.empty())print(probe(words(line)));return 0;}
    if(argc==2 && std::string_view(argv[1])=="--assets"){for(std::string line;std::getline(std::cin,line);)if(!line.empty())print(assets(line));return 0;}
    require(argc==3,"source portal visibility math and asset fixtures required");std::ifstream input(argv[1]);require(bool(input),"source portal visibility fixture missing");unsigned cases=0;
    for(std::string line;std::getline(input,line);){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');require(split!=std::string::npos,"source portal visibility fixture malformed");
        if(probe(words(line.substr(0,split)))!=words(line.substr(split+1)))throw FormatError("source portal traversal differs at case "+std::to_string(cases));++cases;}
    require(cases>=64,"source portal traversal fixture incomplete");std::ifstream asset(argv[2]);require(bool(asset),"source portal assets fixture missing");unsigned maps=0;
    for(std::string line;std::getline(asset,line);){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');require(split!=std::string::npos,"source portal assets fixture malformed");
        auto path=line.substr(0,split);while(!path.empty()&&path.back()==' ')path.pop_back();require(assets(path)==words(line.substr(split+1)),"original source portal asset bytes differ");++maps;}
    require(maps==21,"source portal map coverage incomplete");std::cout<<cases<<" original traversal/consumer cases and all21 authored portal banks match\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
