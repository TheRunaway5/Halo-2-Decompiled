#pragma once
#include "halo2/render_program.hpp"
#include <algorithm>
#include <array>
namespace halo2_test {
inline halo2::ShaderVector scalar_combiners(const std::array<std::uint32_t,60> &w,
    const std::array<halo2::ShaderVector,4> &samples,
    halo2::ShaderVector v0={0,0,0,1},halo2::ShaderVector v1={0,0,0,1},
    halo2::ShaderVector fog={0,0,0,0}) {
    using halo2::ShaderVector;using V=std::array<float,3>;
    std::array<ShaderVector,16> r{};
    r[3]=fog;
    const auto unorm=[](std::uint32_t v){return ShaderVector{float((v>>16)&255)/255,float((v>>8)&255)/255,float(v&255)/255,float(v>>24)/255};};
    for(unsigned i=0;i<4;++i)r[8+i]=samples[i];
    r[4]=v0;r[5]=v1;r[12][3]=(w[54]&31)?r[8][3]:1.f;
    auto input=[&](unsigned operand,bool alpha) {
        operand&=255;V v{};
        for(unsigned i=0;i<3;++i) {
            const auto &source=r[operand&15];float s=alpha?source[operand&16?3:2]:source[operand&16?3:i];
            const float p=std::max(0.f,s);
            switch(operand>>5) {
                case 0:s=p;break;case 1:s=1-std::clamp(s,0.f,1.f);break;
                case 2:s=2*p-1;break;case 3:s=1-2*p;break;case 4:s=p-.5f;break;
                case 5:s=.5f-p;break;case 6:break;case 7:s=-s;break;
            }v[i]=s;
        }return v;
    };
    auto dot=[](V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
    auto operation=[&](unsigned operands,unsigned output,bool alpha) {
        const auto a=input(operands>>24,alpha),b=input(operands>>16,alpha),c=input(operands>>8,alpha),d=input(operands,alpha);
        std::array<V,3> v{};
        const bool mux=w[53]&256?r[12][3]>=.5f:(unsigned(r[12][3]*255)&1)!=0;
        for(unsigned i=0;i<3;++i) {
            v[0][i]=!alpha && (output&0x2000)?dot(a,b):a[i]*b[i];
            v[1][i]=!alpha && (output&0x1000)?dot(c,d):c[i]*d[i];
            v[2][i]=output&0x4000?v[mux?1:0][i]:v[0][i]+v[1][i];
        }
        for(auto &group:v)for(float &s:group) {
            switch((output>>15)&7) {case 1:s-=.5f;break;case 2:s*=2;break;case 3:s=(s-.5f)*2;break;case 4:s*=4;break;case 6:s*=.5f;break;}
            s=std::clamp(s,-1.f,1.f);
        }return v;
    };
    auto commit=[&](unsigned out,bool alpha,std::array<V,3> v) {
        const unsigned dest[]{(out>>4)&15,out&15,(out>>8)&15};
        for(unsigned j=0;j<3;++j)if(dest[j]) {
            if(alpha)r[dest[j]][3]=v[j][0];
            else {for(unsigned i=0;i<3;++i)r[dest[j]][i]=v[j][i];
                if(j<2 && (out&(j?0x40000:0x80000)))r[dest[j]][3]=v[j][2];}
        }
    };
    for(unsigned i=0;i<(w[53]&255);++i) {
        r[1]=unorm(w[10+(w[53]&0x1000?i:0)]);r[2]=unorm(w[18+(w[53]&0x10000?i:0)]);
        const auto rgb=operation(w[34+i],w[45+i],false),a=operation(w[i],w[26+i],true);
        commit(w[45+i],false,rgb);commit(w[26+i],true,a);
    }
    r[1]=unorm(w[43]);r[2]=unorm(w[44]);
    for(unsigned i=0;i<3;++i) {
        float a=w[9]&64?1-r[5][i]:r[5][i],b=w[9]&32?1-r[12][i]:r[12][i];
        r[14][i]=w[9]&128?std::clamp(a+b,0.f,1.f):a+b;
    }
    const auto e=input(w[9]>>24,false),f0=input(w[9]>>16,false);
    for(unsigned i=0;i<3;++i)r[15][i]=e[i]*f0[i];
    const auto a=input(w[8]>>24,false),b=input(w[8]>>16,false),c=input(w[8]>>8,false),d=input(w[8],false);
    ShaderVector result{};for(unsigned i=0;i<3;++i)result[i]=std::clamp(a[i]*b[i]+(1-a[i])*c[i]+d[i],0.f,1.f);
    result[3]=std::clamp(input(w[9]>>8,true)[0],0.f,1.f);return result;
}
}
