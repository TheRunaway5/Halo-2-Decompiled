#include "halo2/atmosphere.hpp"
#include <cstring>
#include <fstream>
#include <iostream>

int main(int argc,char **argv) {
    try {
        const bool direct=argc==4 && (std::string_view(argv[1])=="--direct" || std::string_view(argv[1])=="--compare-direct");
        const bool compare=argc==4 && (std::string_view(argv[1])=="--compare" || std::string_view(argv[1])=="--compare-direct");
        if(argc!=3 && !compare && !direct)throw halo2::FormatError("haze quad input/output or comparison arguments required");
        const unsigned first=argc==4?2:1;
        if(compare && (!std::filesystem::exists(argv[first]) || !std::filesystem::exists(argv[first+1])))return 77;
        std::ifstream input(argv[first],std::ios::binary),expected;
        std::ofstream output;
        if(compare)expected.open(argv[first+1],std::ios::binary);else output.open(argv[first+1],std::ios::binary);
        if(!input || (compare?!expected:!output))throw halo2::FormatError("haze quad fixture missing");
        unsigned count{};
        if(!input.read(reinterpret_cast<char*>(&count),4) || !count || count>65536)
            throw halo2::FormatError("invalid haze quad fixture count");
        for(unsigned n=0;n<count;++n) {
            std::array<float,37> values;
            if(!input.read(reinterpret_cast<char*>(values.data()),sizeof(values)))throw halo2::FormatError("short haze quad input");
            halo2::PatchyFogFrame frame;
            std::memcpy(frame.rows.data(),values.data(),sizeof(frame.rows));frame.vertical=values[32];
            halo2::PatchyFog fog;fog.distance={values[0],values[1]};
            const auto native=direct?halo2::patchy_fog_direct_quad(fog,values[33],values[34],values[2],values[3],values[35],values[36]):
                halo2::patchy_fog_noise_quad(frame,values[33],values[34],values[35],values[36]);
            if(compare) {
                std::array<halo2::ShaderVector,20> original;
                if(!expected.read(reinterpret_cast<char*>(original.data()),sizeof(original)))throw halo2::FormatError("short original haze quad");
                if(std::memcmp(native.attributes.data(),original.data(),sizeof(original)))throw halo2::FormatError("haze quad differs at fixture "+std::to_string(n));
            }else if(!output.write(reinterpret_cast<const char*>(native.attributes.data()),sizeof(native.attributes)))
                throw halo2::FormatError("cannot save haze quad");
        }
        if(input.peek()!=std::char_traits<char>::eof() || (compare && expected.peek()!=std::char_traits<char>::eof()))
            throw halo2::FormatError("extra haze quad records");
        std::cout<<count<<" source haze quads: 20 float4 attributes each\n";
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
