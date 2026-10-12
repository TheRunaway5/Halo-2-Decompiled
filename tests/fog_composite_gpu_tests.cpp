#include "halo2/atmosphere.hpp"
#include "gpu_compute.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
namespace {
using namespace halo2;
struct Fog {std::array<std::uint32_t,60> program;std::array<ShaderVector,2> lookup;};
struct Fixture {Fog material;ShaderVector depth;std::array<ShaderVector,256> gradient;};
static_assert(sizeof(Fog)==272 && sizeof(Fixture)==4384);
// Independent scalar NV2A register arithmetic for checking the actual GLSL
// fog lookup and combiner path. It reads the captured retail program words.
ShaderVector reference(const Fixture &f) {
    using V=std::array<float,3>;
    std::array<ShaderVector,16> r{};
    const auto &w=f.material.program;
    const auto unorm=[](std::uint32_t v){return ShaderVector{float((v>>16)&255)/255,float((v>>8)&255)/255,float(v&255)/255,float(v>>24)/255};};
    const auto z=std::uint32_t(std::round(std::clamp(f.depth[0],0.f,1.f)*16777215.f));
    r[8]={float((z>>8)&255)/255,float(z&255)/255,0,float(z>>16)/255};
    if(w[54]==0x2621) {
        const float hi=float(z>>8)/65535,lo=float((z&255)<<8)/65535;
        auto coord=[&](unsigned i){return f.material.lookup[i][0]*hi+f.material.lookup[i][1]*lo+f.material.lookup[i][2];};
        const float x=std::clamp(coord(0),0.f,1.f)*16-.5f,y=std::clamp(coord(1),0.f,1.f)*16-.5f;
        const int ax=int(std::floor(x)),ay=int(std::floor(y));
        const float fx=x-ax,fy=y-ay;
        auto texel=[&](int x,int y){return f.gradient[std::clamp(y,0,15)*16+std::clamp(x,0,15)];};
        const auto a=texel(ax,ay),b=texel(ax+1,ay),c=texel(ax,ay+1),d=texel(ax+1,ay+1);
        for(unsigned i=0;i<4;++i)r[10][i]=(a[i]*(1-fx)+b[i]*fx)*(1-fy)+(c[i]*(1-fx)+d[i]*fx)*fy;
    }
    r[4][3]=r[5][3]=1;r[12][3]=r[8][3];
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
int main(int argc,char **argv) {
 try {
    if(argc!=2)throw std::runtime_error("maps directory required");
    const auto fog=halo2::Cache(std::filesystem::path(argv[1])/"mainmenu.map").load_menu_atmosphere();
    std::vector<Fixture> fixtures;std::vector<ShaderVector> expected;
    for(float height:{210.f,215.828f,218.f,221.82756f,240.f})
    for(unsigned depth=0;depth<=1024;depth+=2) {
        const auto program=menu_fog_composite(fog,{0,0,height});
        Fixture f{{program.program.words,program.lookup_coordinates},{float(depth)/1024,0,0,0},{}};
        for(unsigned i=0;i<256;++i)for(unsigned c=0;c<4;++c)f.gradient[i][c]=float(std::to_integer<unsigned>(fog.gradient.pixels[i*4+c]))/255;
        expected.push_back(reference(f));fixtures.push_back(f);
    }
    halo2_test::Compute<Fixture,1> compute(HALO2_FOG_COMPOSITE_TEST_SHADER);
    const auto pixels=compute.run(fixtures);float maximum=0;
    for(unsigned i=0;i<fixtures.size();++i)for(unsigned c=0;c<4;++c) {
        const float error=std::abs(pixels[i][c]-expected[i][c]);maximum=std::max(maximum,error);
        if(!std::isfinite(pixels[i][c]) || error>2e-5f)throw std::runtime_error("fog composite GPU mismatch at "+std::to_string(i)+", component "+std::to_string(c));
    }
    if(expected[512][3]<=.4f || expected[512][0]<=.1f)throw std::runtime_error("fog fixtures do not produce atmospheric color");
    std::cout<<fixtures.size()<<" fog depth/lookup/combiner GPU fixtures passed; maximum error "<<maximum<<'\n';
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
