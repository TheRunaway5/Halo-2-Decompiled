#include "halo2/source_bsp_admission.hpp"
#include "halo2/source_object_admission.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <stdexcept>
namespace halo2 {
#include "bsp_queue_find_value_reference.inc"
}
int main()try{
    using namespace halo2;std::mt19937 random(0x166c00);unsigned comparisons{};
    for(unsigned count: {0u,1u,2u,3u,17u,128u,512u,1024u})for(unsigned variant=0;variant<32;++variant){
        SourceBspAdmission bsp;SourceObjectAdmission object;
        for(unsigned index=0;index<count;++index)bsp.instances.push_back({std::uint32_t(random()),std::uint16_t(random()),std::uint16_t(random()),std::uint32_t(random())});
        if(count)bsp.instances.front().id=0xffffffffu;
        if(count>1)bsp.instances.back().id=0;
        if(count>3){bsp.instances[1].id=0x80000000u;bsp.instances[count-2].id=0x80000000u;}
        object.objects=bsp.instances;
        std::vector<std::size_t>keys{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu};
        if constexpr(sizeof(std::size_t)>4){keys.push_back(std::size_t(0x100000000ull));keys.push_back(std::size_t(0x180000000ull));}
        for(unsigned n=0;n<16;++n)keys.push_back(random());
        for(const auto&row:bsp.instances)keys.push_back(row.id);
        for(auto key:keys){
            if(bsp.instance(key)!=reference_instance(bsp.instances,key))throw std::runtime_error("Actual BSP ID-search first match changed");
            const auto full=std::uint32_t(key);
            if(object.object(full)!=reference_object(object.objects,full))throw std::runtime_error("Actual object ID-search first match changed");
            comparisons+=2;
        }
    }
    std::cout<<"{\"passed\":true,\"actual_public_api_comparisons\":"<<comparisons<<"}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
