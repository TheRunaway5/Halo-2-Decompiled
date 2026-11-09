#include "halo2/native_tu15_cluster_frustum_value.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <random>
namespace reference {using namespace halo2;
#include "native_tu15_cluster_frustum_value_reference.inc"
}
using namespace halo2;
int main()try{
    std::mt19937 random(0x167ef0);unsigned comparisons{},failures{};
    for(unsigned n=0;n<512;++n){
        SourceClusterFrustumSet set;set.group_count=std::array<std::int16_t,9>{-2,0,1,2,3,4,5,6,7}[n%9];set.clusters.resize(3);set.frustums.resize(12);
        for(auto&range:set.clusters)for(unsigned group=0;group<6;++group){range.counts[group]=std::int16_t(int(random()%5)-1);range.first_indices[group]=std::int16_t(random()%8);}
        for(unsigned index=0;index<set.frustums.size();++index){auto&f=set.frustums[index];f.bounds={-10,10,-10,10,-10,10};for(unsigned plane=0;plane<6;++plane){std::array<float,4>row{0,0,0,std::array{-2.f,0.f,9.f}[index%3]};row[plane/2]=plane%2?-1.f:1.f;std::memcpy(&f.planes[plane],row.data(),16);}}
        const Vec3 center{float(int(random()%31)-15),float(int(random()%31)-15),float(int(random()%31)-15)};const auto section=n%3;const auto radius=std::array{0.f,1.f,2.5f,10.f}[n%4];
        if(n%13==0)set.clusters[section].first_indices[0]=-1;
        if(n%17==0)set.clusters[section].first_indices[0]=100;
        if(n%19==0)set.frustums[0].planes[5][0]=std::bit_cast<float>(0x7fc12345u);
        auto seed=native_raw_x87_detail::capture();seed.word(0,std::array<std::uint16_t,4>{0x27f,0x37f,0x77f,0xf7f}[n%4]);seed.mxcsr=0x1f80|((n%4)<<13);native_raw_x87_detail::restore(seed);
        bool original_failed{};SourceClusterFrustumResult original;try{original=reference::source_cluster_frustum_visibility_reference(set,section,center,radius);}catch(const FormatError&){original_failed=true;}
        const auto original_fp=native_raw_x87_detail::capture();native_raw_x87_detail::restore(seed);
        bool shared_failed{};SourceClusterFrustumResult shared;try{shared=native_cluster_frustum_value_detail::visible(set,section,center,radius);}catch(const FormatError&){shared_failed=true;}
        const auto shared_fp=native_raw_x87_detail::capture();
        if(original_failed!=shared_failed||(!original_failed&&(original.visible!=shared.visible||original.contained!=shared.contained))||!native_raw_x87_detail::equivalent(original_fp,shared_fp))throw std::runtime_error("Shared cluster value result/exception/FP changed at"+std::to_string(n));
        failures+=original_failed;++comparisons;
    }
    std::cout<<nlohmann::json{{"passed",true},{"public_value_comparisons",comparisons},{"genuine_format_errors",failures}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
