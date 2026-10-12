#include "halo2/source_weather_initialization.hpp"
#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> b,std::size_t p) {
    std::uint32_t v;std::memcpy(&v,b.data()+p,4);return v;
}
float scalar(std::span<const std::byte> b,std::size_t p) {
    return std::bit_cast<float>(word(b,p));
}
void store(std::span<std::byte> b,std::size_t p,std::uint32_t v) {
    std::memcpy(b.data()+p,&v,4);
}
void put(std::span<std::byte> b,std::size_t p,float v) {
    store(b,p,std::bit_cast<std::uint32_t>(v));
}
}
bool source_weather_initialize_particles(std::span<std::byte> bank,
    std::int32_t count,std::span<std::byte> render,
    std::span<std::byte> state,std::uint32_t &auxiliary) {
    if(bank.size()!=0x8c)throw std::invalid_argument("weather definition must be 140 bytes");
    const bool admitted=std::bit_cast<std::int32_t>(word(bank,0x64))>0;
    if(admitted && count>0 && (std::uint64_t(count)*20>render.size() ||
                               std::uint64_t(count)*32>state.size()))
        throw std::invalid_argument("weather particle resource exceeds owned arrays");
    put(bank,0x80,0.f);bank[0x86]=std::byte{0};
    if(admitted) {
        auto random=[&] {
            auxiliary=auxiliary*0x19660du+0x3c6ef35fu;
            // Original FLD/FMULT uses the stored float reciprocal in x87.
            return double(auxiliary>>16)*std::bit_cast<float>(0x37800080u);
        };
        for(std::int32_t i=0;i<count;++i) {
            const auto r=std::size_t(i)*20,s=std::size_t(i)*32;
            store(render,r+16,0xffffffffu);
            if((.0001f>scalar(bank,0x1c) && .0001f>scalar(bank,0x20)) ||
                scalar(bank,0x1c)>scalar(bank,0x20)) {
                put(bank,0x1c,.1f);put(bank,0x20,.8f);
            }
            const double minimum=scalar(bank,0x1c);
            put(state,s+24,float((double(scalar(bank,0x20))-minimum)*random()+minimum));
            put(state,s,float(random()*scalar(bank,0x30)));
            put(state,s+4,float(random()*scalar(bank,0x34)));
            // FST stores z but retains its unrounded x87 value for the norm.
            const double z=random()*scalar(bank,0x38);put(state,s+8,float(z));
            const double x=scalar(state,s),y=scalar(state,s+4);
            const double norm=std::sqrt((x*x+y*y)+z*z);
            const float magnitude=float(norm);
            if(!(.0001f>std::abs(norm))) {
                const float inverse=1.f/magnitude;
                put(state,s,scalar(state,s)*inverse);
                put(state,s+4,scalar(state,s+4)*inverse);
                put(state,s+8,scalar(state,s+8)*inverse);
            }
            const double size_unit=random();
            put(render,r+12,float(size_unit*(double(scalar(bank,0x28))-
                scalar(bank,0x24))+scalar(bank,0x24)));
            put(state,s+28,0.f);
            for(auto p:{8u,12u,16u})if(.0001f>scalar(bank,p))put(bank,p,1.f);
            for(unsigned axis=0;axis<3;++axis) {
                const double unit=random();
                const unsigned extent=axis==0?16:axis==1?8:12;
                put(render,r+axis*4,float(((unit+unit)-1.)*scalar(bank,extent)));
            }
            for(unsigned axis=0;axis<3;++axis) {
                const double unit=random();put(state,s+12+axis*4,float((unit+unit)-1.));
            }
        }
        bank[0x86]=std::byte{1};
    }
    put(bank,0x88,1.f);return true;
}
void source_weather_initialize_plate(std::span<std::byte> bank,std::uint32_t &auxiliary) {
    if(bank.size()!=0x3a8)throw std::invalid_argument("weather plate must be 936 bytes");
    for(unsigned view=0;view<4;++view) {
        const auto p=0xc8+view*0xb8;
        for(unsigned j=0;j<18;++j)store(bank,p+j*4,0);
        for(unsigned j=0;j<3;++j) {
            store(bank,p+0x3c+j*4,0);store(bank,p+0x48+j*4,0);
        }
        put(bank,p+0x80,1.f);put(bank,p+0x84,1.f);put(bank,p+0x78,0.f);
        for(unsigned layer=0;layer<3;++layer) {
            const auto v=0x24+layer*12;
            const double x=scalar(bank,v),y=scalar(bank,v+4),z=scalar(bank,v+8);
            put(bank,p+0xac+layer*4,float(std::sqrt((x*x+y*y)+z*z)));
        }
        for(unsigned j=0;j<9;++j) {
            auxiliary=auxiliary*0x19660du+0x3c6ef35fu;
            const double unit=double(auxiliary>>16)*std::bit_cast<float>(0x37800080u);
            put(bank,p+0x88+j*4,float((unit+unit)-1.));
        }
    }
}
}
