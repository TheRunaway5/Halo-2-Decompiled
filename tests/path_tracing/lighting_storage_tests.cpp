#include "halo2/path_tracing/lighting_storage.hpp"
#include <iostream>
#include <stdexcept>
using namespace halo2::path_tracing;
int main()try {
    constexpr std::uint64_t mib=1024*1024;
    constexpr auto native=lighting_storage_plan(3840,2160,512*mib);
    static_assert(native.scale==2 && native.cells==2073600 && native.current_bytes==497664032 && native.previous_bytes==364953824);
    constexpr auto minimum=lighting_storage_plan(3840,2160,128*mib);
    static_assert(minimum.scale==5 && minimum.cells==331776 && minimum.current_bytes==107495456 && minimum.previous_bytes==58392800);
    static_assert(lighting_storage_plan(7680,4320,128*mib).scale==71);
    static_assert(lighting_storage_plan(8192,8192,128*mib).scale==0);
    static_assert(lighting_storage_plan(0,1080,128*mib).scale==0);
    static_assert(lighting_storage_plan(~0u,~0u,~std::uint64_t{}).scale==0);
    constexpr auto thin=lighting_storage_plan(~0u,1,32+std::uint64_t(~0u)*4+224);
    static_assert(thin.scale==~0u && thin.cells==1);
    unsigned cases{};
    for(const auto width:{1u,63u,1280u,1920u,2561u,3840u,4095u,7680u})
        for(const auto height:{1u,65u,721u,1080u,1441u,2160u,4320u})
            for(const auto limit:{128*mib,256*mib,512*mib}) {
                const auto plan=lighting_storage_plan(width,height,limit);
                if(!plan.scale)continue;
                if(plan.current_bytes>limit || plan.previous_bytes>limit)throw std::runtime_error("lighting storage exceeds device range");
                const auto representative=[&](unsigned cell,unsigned size) {
                    return std::min(cell*plan.scale+plan.scale/2,size-1);
                };
                for(const auto size:{width,height}) {
                    const auto cells=(size+plan.scale-1)/plan.scale;
                    for(unsigned cell=0;cell<cells;++cell) {
                        const auto pixel=representative(cell,size);
                        if(pixel>=size || pixel/plan.scale!=cell)throw std::runtime_error("odd scale representative escaped its grid cell");
                        // Shader previous-grid base and four-candidate search
                        // must include this representative, including a partial
                        // final cell clamped to the image's final pixel.
                        const double coordinate=(double(pixel)-double(plan.scale/2))/plan.scale;
                        const auto base=coordinate<0?-1:int(coordinate);
                        if(int(cell)!=base && int(cell)!=base+1)throw std::runtime_error("odd scale reprojection missed its own representative");
                    }
                }
                ++cases;
            }
    std::cout<<"PASS Path Tracing lighting storage: "<<cases<<" extents/ranges; UHD128MiB scale="<<minimum.scale<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
