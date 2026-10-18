#include "halo2/pstencil/projected_light_material.hpp"
#include <fstream>
#include <iostream>
#include <bit>

int main(int argc,char**argv) {try {
    if(argc!=3)throw halo2::FormatError("Supply the unchanged TU1.5 selection and feature fixtures");
    std::ifstream input(argv[1]);
    if(!input)throw halo2::FormatError("TU1.5 selection fixture is unavailable");
    unsigned flags,mask,material,layers,implementation,specular,cases=0;
    while(input>>flags>>mask>>material>>layers>>implementation>>specular) {
        if(halo2::pstencil::projected_light_implementation(flags,mask,material)!=implementation ||
           halo2::pstencil::projected_light_specular_mask(flags,mask,layers)!=(specular!=0))
            throw halo2::FormatError("Projected material differs from unchanged TU1.5 selection output");
        ++cases;
    }
    if(!input.eof() || cases!=720)
        throw halo2::FormatError("TU1.5 projected material fixture is incomplete");
    std::cout<<"PASS "<<cases<<" unchanged TU1.5 projected implementation/LOD-specular-mask selections\n";
    std::ifstream features(argv[2]);
    unsigned r,g,b,epsilon,shadow,volume,kind,expected,feature_cases=0;
    while(features>>r>>g>>b>>epsilon>>shadow>>volume>>kind>>expected) {
        const auto value=[](unsigned bits){return std::bit_cast<float>(bits);};
        if(halo2::pstencil::projected_light_feature_mask({value(r),value(g),value(b)},value(epsilon),
            shadow!=0,value(volume),std::uint16_t(kind))!=expected)
            throw halo2::FormatError("Projected features differ from unchanged TU1.5 prefix output");
        ++feature_cases;
    }
    if(!features.eof() || feature_cases!=736)
        throw halo2::FormatError("TU1.5 projected feature fixture is incomplete");
    std::cout<<"PASS "<<feature_cases<<" unchanged TU1.5 projected feature cases\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
