#include "halo2/source_sphere_rectangle.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
template<class T>T read(std::ifstream& f){T value;if(!f.read(reinterpret_cast<char*>(&value),sizeof(value)))throw FormatError("sphere rectangle fixture truncated");return value;}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_sphere_rectangle.original.bin";
    std::ifstream f(path,std::ios::binary);if(!f || read<std::uint32_t>(f)!=0x52533248 || read<std::uint32_t>(f)!=1)throw FormatError("sphere rectangle original fixture schema");const auto count=read<std::uint32_t>(f);unsigned controls=0;
    for(unsigned n=0;n<count;++n){const auto bank=read<SourceViewProjectionBank>(f);const auto eye=read<Vec3>(f);const auto viewport=read<std::array<std::int16_t,4>>(f);const auto center=read<Vec3>(f);const auto radius=read<float>(f);const auto expected=read<std::array<std::uint32_t,4>>(f);const auto actual=source_sphere_rectangle(bank,eye,viewport,center,radius);
        for(unsigned i=0;i<4;++i)if(std::bit_cast<std::uint32_t>(actual[i])!=expected[i])throw FormatError("sphere rectangle case"+std::to_string(n)+" component"+std::to_string(i)+" got"+std::to_string(std::bit_cast<std::uint32_t>(actual[i]))+" expected"+std::to_string(expected[i]));
        const auto changed=source_sphere_rectangle(bank,eye,viewport,{center.x+.5f,center.y,center.z},radius);controls+=std::memcmp(changed.data(),actual.data(),sizeof(actual))!=0;
    }
    if(f.peek()!=std::char_traits<char>::eof() || controls<100)throw FormatError("sphere rectangle fixture coverage/trailing bytes");
    std::cout<<"PASS "<<count<<" whole original30290 sphere rectangles / "<<count*4<<" exact words / "<<controls<<" changed-center controls; floating bounds, no live/device-scissor claim\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
