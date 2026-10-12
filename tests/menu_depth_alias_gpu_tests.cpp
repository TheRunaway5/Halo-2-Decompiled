#include "gpu_compute.hpp"
#include <array>
#include <cstdint>
#include <iostream>

int main() {
    try {
        // Four thousand workgroups stay below Vulkan's minimum dispatch
        // limit; each checks a disjoint range of the entire integer D24 bank.
        using Range=std::array<std::uint32_t,4>;
        std::vector<Range> ranges;
        for(std::uint32_t first=0;first<(1u<<24);first+=4096)
            ranges.push_back({first,first+4096,0,0});
        halo2_test::Compute<Range,1> gpu(HALO2_MENU_DEPTH_ALIAS_SHADER);
        const auto result=gpu.run(ranges);
        std::uint64_t checked{};
        for(const auto &row:result) {
            if(row[0]!=0)throw std::runtime_error("fog alias differs after original W quantization at "+
                std::to_string(std::uint32_t(row[1]))+", decoded "+std::to_string(std::uint32_t(row[2])));
            checked+=std::uint32_t(row[3]);
        }
        if(checked!=(1u<<24))throw std::runtime_error("incomplete W alias domain");
        VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(gpu.physical,&properties);
        std::cout<<properties.deviceName<<": "<<checked<<" integer W values retain exact fog alias bytes through production GLSL. "
            "Compute arithmetic only; attachment conversion, rasterization and live fog remain separate checks.\n";
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
