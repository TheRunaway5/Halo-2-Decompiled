#include "halo2/source_portal_visibility.hpp"
#include "halo2/source_shadow_view.hpp"
#include <bit>
#include <cstring>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw FormatError(why);}
std::vector<std::uint32_t> probe(const std::string& line) {
    std::istringstream stream(line);std::vector<std::uint32_t> input;
    for(std::uint32_t word;stream>>std::hex>>word;)input.push_back(word);
    std::size_t at{};
    const auto take=[&]{require(at<input.size(),"Truncated cube portal input");return input[at++];};
    const auto scalar=[&]{return std::bit_cast<float>(take());};
    require(take()==6,"Large sphere requires six original C1D90 cameras");
    std::array<SourceFrustumCamera,6> groups;
    for(auto& group:groups) {
        std::array<std::uint32_t,sizeof(SourceFrustumCamera)/4> words{};
        for(auto& word:words)word=take();group=std::bit_cast<SourceFrustumCamera>(words);
    }
    const auto native=source_effect_light_cameras(groups[0].camera_to_world.position,groups[0].far_distance);
    for(unsigned i=0;i<6;++i)
        require(std::memcmp(&native[i].camera,&groups[i],sizeof(SourceFrustumCamera))==0,
            "Native cube camera differs from whole original C1D90");
    const int containing=std::bit_cast<std::int32_t>(take());
    std::array<std::uint32_t,16> mask;for(auto& word:mask)word=take();
    SourcePortalBsp bsp;
    const auto clusters=take();for(unsigned i=0;i<clusters;++i) {
        const auto count=take();auto& row=bsp.cluster_portals.emplace_back();
        for(unsigned j=0;j<count;++j)row.push_back(std::uint16_t(take()));
    }
    const auto planes=take();for(unsigned i=0;i<planes;++i)bsp.planes.push_back({scalar(),scalar(),scalar(),scalar()});
    const auto portals=take();for(unsigned i=0;i<portals;++i) {
        SourcePortalGeometry p;p.back=std::int16_t(take());p.front=std::int16_t(take());
        p.plane_index=take();p.flags=take();const auto count=take();
        for(unsigned j=0;j<count;++j)p.vertices.push_back({scalar(),scalar(),scalar()});
        bsp.portals.push_back(std::move(p));
    }
    require(at==input.size(),"Trailing cube portal input");
    const auto out=source_portal_visibility(bsp,groups,containing,mask);
    std::vector<std::uint32_t> result{unsigned(out.clusters.size())};
    for(const auto& cluster:out.clusters) {
        result.push_back(std::uint16_t(cluster.source_cluster));
        for(const auto count:cluster.counts)result.push_back(std::uint16_t(count));
        for(const auto first:cluster.first_indices)result.push_back(std::uint16_t(first));
    }
    result.insert(result.end(),out.sorted_cluster_indices.begin(),out.sorted_cluster_indices.end());
    result.push_back(unsigned(out.frustums.size()));
    for(const auto& frustum:out.frustums) {
        const auto words=std::bit_cast<std::array<std::uint32_t,sizeof(SourceCameraFrustum)/4>>(frustum);
        for(unsigned i=0;i<words.size();++i)if(i!=3)result.push_back(words[i]);
    }
    const auto spheres=out.sphere_bank();for(unsigned i=0;i<out.clusters.size();++i) {
        const auto found=source_cluster_frustum_visibility(spheres,i,groups[0].camera_to_world.position,.01f);
        result.push_back(found.visible);result.push_back(found.contained);
    }
    return result;
}
}
int main(int argc,char** argv)try {
    require(argc==2&&std::string_view(argv[1])=="--probe","Supply --probe with declared original cube inputs");
    const std::uint16_t control=0x27f;const std::uint32_t mxcsr=0x1f80;
    asm volatile("fldcw %0"::"m"(control));asm volatile("ldmxcsr %0"::"m"(mxcsr));
    for(std::string line;std::getline(std::cin,line);)if(!line.empty()) {
        for(const auto word:probe(line))std::cout<<std::hex<<word<<' ';
        std::cout<<'\n';
    }
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
