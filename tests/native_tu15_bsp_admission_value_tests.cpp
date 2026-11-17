// Retain the existing whole predecessor numeric/capacity/error corpus, then
// add the full controller's auxiliary/duplicate/ordinal ownership families.
#define main geometry_predecessor_main
#include "native_tu15_geometry_visibility_value_tests.cpp"
#undef main
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
template<class Function>auto fp_outcome(Function&&function,unsigned variant){
    const std::uint16_t control=std::array<std::uint16_t,3>{0x7f,0x27f,0x37f}[variant%3]|((variant%4)<<10u);
    asm volatile("fninit;fldcw %0"::"m"(control):"memory");
    _mm_setcsr(0x1f80u|((variant%4)<<13u)|std::array<unsigned,4>{0,0x40,0x8000,0x8040}[(variant/4)%4]);
    const auto value=outcome(std::forward<Function>(function));std::uint16_t cw,sw;
    asm volatile("fnstcw %0;fnstsw %1":"=m"(cw),"=m"(sw)::"memory");
    return std::pair{value,std::array<unsigned,3>{cw,sw,_mm_getcsr()}};
}
#endif
int main()try{
    if(geometry_predecessor_main())return 1;
    const auto baseline=comparisons;
    for(unsigned option=0;option<256;++option)for(unsigned mode=0;mode<2;++mode){
        SourceBspAdmissionBsp bsp;bsp.clusters.resize(2);bsp.definitions={{1},{0}};
        bsp.clusters[0].geometry_count=option&1u?1:0;bsp.clusters[0].instances={0,1,0};
        bsp.clusters[1].geometry_count=1;bsp.clusters[1].instances={0,2,1};
        for(auto&c:bsp.clusters)c.bounds={-1,1,-1,1,-1,1};
        bsp.instances={{0,{0,0,0},1},{0,{2,0,0},3},{std::int16_t(option&2u?1:0),{0,0,0},2}};
        auto bank=visible(option&4u?0:3);
        for(unsigned n=0;n<bank.clusters.size();++n)bank.clusters[n].source_cluster=n%2;
        if(option&8u&&!bank.clusters.empty())bank.clusters.back().source_cluster=-1;
        SourceBspAdmission auxiliary;
        if(option&16u)auxiliary.clusters.push_back({0,0x3000,0xffff,0});
        if(option&32u)auxiliary.clusters.push_back({1,0x3000,0xffff,0});
        if(option&64u)auxiliary.instances.push_back({0,0x3000,0xffff,0});
        std::array<std::uint8_t,2>fog{std::uint8_t(option%2),std::uint8_t((option>>1u)%2)};
        SourceBspAdmissionInput input;input.mode=mode;input.flags=1u|((option&128u)?0x80u:0u);input.auxiliary=&auxiliary;input.containing_fog=fog;input.sphere_radius=1;
        input.cluster_capacity=(option%3)+1;input.instance_capacity=(option%5)+1;
        compare([&]{return source_bsp_admission(bsp,bank,input);},[&]{return association_reference::bsp(bsp,bank,input);});
    }
    unsigned fp_cases{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    for(unsigned variant=0;variant<144;++variant)for(unsigned capacity:{1u,8u}){
        SourceBspAdmissionBsp bsp;bsp.clusters.resize(1);bsp.clusters[0].geometry_count=1;bsp.clusters[0].instances={0,0};
        bsp.definitions={{1}};const auto [center,radius]=sphere(variant);bsp.instances={{0,center,radius}};
        bsp.clusters[0].bounds={center.x,radius,0,0,0,0};auto bank=visible(2);const std::array<std::uint8_t,1>fog{1};
        SourceBspAdmissionInput input;input.containing_fog=fog;input.mode=variant%2;input.sphere_radius=1;input.cluster_capacity=capacity;input.instance_capacity=capacity;input.flags=0x200;input.clipping_plane={1,0,0,0};
        const auto native=fp_outcome([&]{return source_bsp_admission(bsp,bank,input);},variant);
        const auto reference=fp_outcome([&]{return association_reference::bsp(bsp,bank,input);},variant);
        if(native!=reference)throw FormatError("public BSP controller result/FP effect differs at "+std::to_string(variant)+" capacity "+std::to_string(capacity));
        ++fp_cases;
    }
    asm volatile("fninit":::"memory");_mm_setcsr(0x1f80);
#endif
    std::cout<<"{\"comparisons\":"<<comparisons<<",\"exact_errors\":"<<errors<<",\"saturation_witnesses\":"<<saturations<<",\"additional_controller_cases\":"<<comparisons-baseline<<",\"public_fp_effect_cases\":"<<fp_cases<<"}\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
