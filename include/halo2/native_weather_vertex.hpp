#pragma once
#include "halo2/vertex_program.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
namespace native_weather {
// Literal complete source payloads, including the original 2078/count header.
// VS22 SHA256 3f9e4e6030626339db80d3bb5bf9e4103936041f9bdcfd75626f18ca2ff1a1ee.
inline constexpr std::array<std::uint32_t,37> point_program{
    0x00092078,0,0x00c0001b,0x08361800,0x20b08800,
    0,0x00c0201b,0x08363800,0x20b04800,
    0,0x00c0401b,0x08365800,0x20b02800,
    0,0x00c0601b,0x08367800,0x28a01800,
    0,0x0041401a,0xc4355800,0x2090e800,
    0,0x0420021b,0x18001002,0x90a8f818,
    0,0x06000000,0x080013ff,0x10b10000,
    0,0x00400000,0x15fe1000,0x20a08830,
    0,0x0081601a,0xc5ff686a,0xf0b0e801};
// VS21 SHA256 c57b3f2509d664bba750f1b99fd09351efd0ddc4b9c5d2288e7fd2154896799a.
inline constexpr std::array<std::uint32_t,21> plate_program{
    0x00052078,0,0x0020001b,0x0836106c,0x2070f800,
    0,0x0020021b,0x0836106c,0x2070f848,
    0,0x0062a41b,0x0836106c,0x3070c850,
    0,0x0062c61b,0x0836106c,0x3070c858,
    0,0x0062e81b,0x0836106c,0x3070c861};
template<std::size_t N>
inline bool matches(std::span<const std::byte> bytes,
                    const std::array<std::uint32_t,N>& words) {
    static_assert(std::endian::native==std::endian::little);
    const auto original=std::as_bytes(std::span(words));
    return bytes.size()==original.size() && std::equal(bytes.begin(),bytes.end(),original.begin());
}
inline void admit_point(std::span<const std::byte> bytes) {
    if(!matches(bytes,point_program))throw FormatError("weather point/line requires complete original VS22/class0");
}
inline void admit_plate(std::span<const std::byte> bytes) {
    if(!matches(bytes,plate_program))throw FormatError("weather plate requires complete original VS21/class0");
}
inline float stored(float value) {volatile float result=value;return result;}
inline float product(float a,float b) {return a==0.f||b==0.f?0.f:stored(a*b);}
inline float add(float a,float b) {return stored(a+b);}
inline std::array<ShaderVector,16> outputs() {
    std::array<ShaderVector,16> result{};
    for(auto& value:result)value={0,0,0,1};
    return result;
}
// Fixed nine-operation VS22. The CPU submission checks use this direct
// arithmetic too; no shader opcode/register operand is decoded at runtime.
inline std::array<ShaderVector,16> point_vertex(
    const std::array<ShaderVector,12>& camera,const ShaderVector& position,
    std::uint32_t color) {
    auto out=outputs();
    const auto projection=[&](const ShaderVector& row) {
        return add(add(add(product(position[0],row[0]),product(position[1],row[1])),
                       product(position[2],row[2])),row[3]);
    };
    out[0]={projection(camera[0]),projection(camera[1]),projection(camera[2]),projection(camera[3])};
    const float w=out[0][3];
    for(unsigned axis=0;axis<3;++axis)out[0][axis]=product(out[0][axis],camera[10][axis]);
    out[3]={float((color>>16)&255)/255.f,float((color>>8)&255)/255.f,
            float(color&255)/255.f,float(color>>24)/255.f};
    const float reciprocal=stored(1.f/w);
    float clamped=reciprocal;
    if(clamped>0.f || std::bit_cast<std::uint32_t>(clamped)==0u)
        clamped=std::clamp(clamped,std::bit_cast<float>(0x1f800000u),std::bit_cast<float>(0x5f800000u));
    else if(!std::isnan(clamped))
        clamped=std::clamp(clamped,std::bit_cast<float>(0xdf800000u),std::bit_cast<float>(0x9f800000u));
    out[6][0]=product(reciprocal,position[3]);
    for(unsigned axis=0;axis<3;++axis)
        out[0][axis]=add(product(out[0][axis],clamped),camera[11][axis]);
    return out;
}
inline std::array<ShaderVector,16> plate_vertex(
    const std::array<ShaderVector,5>& registers,const std::array<ShaderVector,3>& scroll) {
    auto out=outputs();out[0]=registers[0];out[9]=registers[1];
    for(unsigned layer=0;layer<3;++layer)for(unsigned axis=0;axis<2;++axis)
        out[10+layer][axis]=add(registers[2+layer][axis],scroll[layer][axis]);
    return out;
}
// 3A5C0 mode3 has sixteen finite control tuples. Packed colors remain data;
// exact control identity selects a compiled specialization before submission.
inline std::array<std::uint32_t,60> plate_controls(unsigned power,bool alpha) {
    if(power>7)throw FormatError("weather plate native power outside original0..7");
    std::array<std::uint32_t,60> w{};
    w[1]=0x04209140;w[2]=0x18201140;w[3]=0xdd202df1;
    w[27]=0xc00;w[28]=w[29]=0xd00;
    w[34]=0x08010802;w[35]=0x04201140;w[36]=0x2020;w[37]=0xcc202d20;
    w[45]=0x3045;w[46]=0xc00;w[47]=0x4d00;w[48]=0xc00;
    w[53]=0x11108;w[54]=0x8421;
    w[38]=alpha?0x19011a02:0x09010a02;w[49]=0x3045;
    w[4]=0x1d201d00|(power>3?0x20u:0u);
    w[30]=0xd00|((power>2?0x20u:power>1?0x10u:0u)<<12);
    w[39]=alpha?0x24251b01:0x24250b01;w[50]=0x1045;
    w[5]=0x1d201d00|(power>6?0x20u:0u);
    w[31]=0xd00|((power>5?0x20u:power>4?0x10u:0u)<<12);
    w[40]=0x04250000;w[51]=0xd0;
    w[6]=0x1d120c02;w[7]=0x1d2d0000;
    w[32]=0xd00|((power>0?0x10u:0u)<<12);w[33]=0xd0;
    w[8]=0x1d010000;w[9]=0x1d00;
    return w;
}
inline unsigned plate_variant(std::array<std::uint32_t,60> words) {
    for(unsigned i=10;i<26;++i)words[i]=0;
    words[43]=words[44]=0;
    for(unsigned power=0;power<8;++power)for(unsigned alpha=0;alpha<2;++alpha)
        if(words==plate_controls(power,alpha))return power*2+alpha;
    throw FormatError("weather plate has no exact compiled native control tuple");
}
}
}
