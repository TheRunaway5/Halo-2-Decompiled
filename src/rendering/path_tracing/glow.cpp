#include "halo2/path_tracing/glow.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <array>
#include <cmath>

// CPU evaluation of a glow layer's retail pixel program, used once per image
// change to give the path tracer the colour that layer is actually drawn with.
// The arithmetic mirrors shaders/combiner.glsl line for line; read that file
// for the NV2A register-combiner semantics.
namespace halo2::path_tracing {
namespace {
using Color=std::array<float,4>;
constexpr unsigned grid=32; // Evaluation points per side.

// One stage's authored image, decoded to floating-point RGBA at the smallest
// mip still covering the evaluation grid.
struct StageImage {
    unsigned width{},height{};
    std::vector<Color> texels;
    Color at(float u,float v) const {
        const auto x=std::min(width-1,unsigned(u*float(width)));
        const auto y=std::min(height-1,unsigned(v*float(height)));
        return texels[std::size_t(y)*width+x];
    }
};
// Volume stages are position dependent; for an average they contribute the
// mean colour of every slice. A cube stage is an environment reflection of
// the surroundings, not emitted light (the path tracer reflects the actual
// scene), so it contributes black: window glass is not a lamp.
std::optional<StageImage> mean_image(const Texture& source) {
    if(source.layers==6)return StageImage{1,1,{Color{0,0,0,0}}};
    Texture image=source;
    if(image.format>=14 && image.format<=16) {
        if(image.depth || image.mips.empty())return {};
        image=prepare_xemu_bc_texture(image);
    }
    const bool floats=image.format==native_rgba32f;
    if(image.format!=11 && !floats)return {};
    const std::size_t stride=floats?16:4,count=image.pixels.size()/stride;
    if(!count)return {};
    const Bytes bytes(image.pixels);
    double sum[4]{};
    for(std::size_t n=0;n<count;++n) {
        const auto at=n*stride;
        if(floats)for(unsigned c=0;c<4;++c)sum[c]+=bytes.f32(at+c*4);
        else {
            const auto word=bytes.u32(at);
            for(unsigned c=0;c<4;++c)sum[c]+=double((word>>(8*c))&255u)/255.0;
        }
    }
    StageImage result{1,1,{}};
    result.texels.push_back({float(sum[0]/double(count)),float(sum[1]/double(count)),
                             float(sum[2]/double(count)),float(sum[3]/double(count))});
    return result;
}
std::optional<StageImage> stage_image(const Texture& source) {
    if(!source.width || !source.height || source.pixels.empty())return {};
    if(source.layers!=1 || source.depth)return mean_image(source);
    Texture image=source;
    if(image.mips.empty()) {
        const auto bytes=image.format==native_rgba32f?std::size_t(image.width)*image.height*16:
            image.format==11?std::size_t(image.width)*image.height*4:
            std::size_t((image.width+3)/4)*((image.height+3)/4)*(image.format==14?8:16);
        image.mips.push_back({image.width,image.height,0,bytes});
    }
    if(image.format>=14 && image.format<=16)image=prepare_xemu_bc_texture(image);
    const bool floats=image.format==native_rgba32f;
    if(image.format!=11 && !floats)return {};
    std::size_t level=0;
    while(level+1<image.mips.size() && image.mips[level+1].width>=grid && image.mips[level+1].height>=grid)++level;
    const auto& mip=image.mips[level];
    const std::size_t count=std::size_t(mip.width)*mip.height,stride=floats?16:4;
    if(!count || mip.offset>image.pixels.size() || count*stride>image.pixels.size()-mip.offset)return {};
    const Bytes bytes(image.pixels);
    StageImage result{mip.width,mip.height,{}};
    result.texels.reserve(count);
    for(std::size_t n=0;n<count;++n) {
        const auto at=mip.offset+n*stride;
        if(floats)result.texels.push_back({bytes.f32(at),bytes.f32(at+4),bytes.f32(at+8),bytes.f32(at+12)});
        else {
            const auto word=bytes.u32(at); // RGBA8, red in the low byte.
            result.texels.push_back({float(word&255u)/255.f,float((word>>8)&255u)/255.f,
                                     float((word>>16)&255u)/255.f,float(word>>24)/255.f});
        }
    }
    return result;
}

// NV2A texture-stage modes (D3DPIXELSHADERDEF PSTextureModes, 5 bits per stage).
enum : unsigned {mode_none=0,mode_passthrough=4,mode_clip_plane=5,mode_dot_zw=10,mode_dot_product=17};
bool fetches_texture(unsigned mode) {
    return mode!=mode_none && mode!=mode_passthrough && mode!=mode_clip_plane &&
        mode!=mode_dot_zw && mode!=mode_dot_product;
}

class Combiner {
public:
    explicit Combiner(const PixelProgram& program):words_(program.words){}
    Color run(const std::array<Color,4>& textures,Color v0,Color v1) {
        registers_={};
        registers_[4]=v0;registers_[5]=v1;
        for(unsigned i=0;i<4;++i)registers_[8+i]=textures[i];
        registers_[12][3]=(word(54)&31u)?textures[0][3]:1.f;
        const auto stages=std::min(word(53)&255u,8u);
        for(unsigned stage=0;stage<stages;++stage) {
            registers_[1]=constant(10+((word(53)&0x1000u)?stage:0));
            registers_[2]=constant(18+((word(53)&0x10000u)?stage:0));
            const auto rgb=combine(word(34+stage),word(45+stage),false);
            const auto alpha=combine(word(stage),word(26+stage),true);
            commit(word(45+stage),false,rgb);
            commit(word(26+stage),true,alpha);
        }
        const auto abcd=word(8),efg=word(9);
        if(!abcd && !efg)return clamp(registers_[12],0.f,1.f);
        registers_[1]=constant(43);registers_[2]=constant(44);
        Rgb sum{};
        for(unsigned c=0;c<3;++c) {
            sum[c]=((efg&64u)?1.f-registers_[5][c]:registers_[5][c])+((efg&32u)?1.f-registers_[12][c]:registers_[12][c]);
            registers_[14][c]=(efg&128u)?std::clamp(sum[c],0.f,1.f):sum[c];
        }
        registers_[14][3]=0;
        const auto e=input(efg>>24,false),f=input(efg>>16,false);
        for(unsigned c=0;c<3;++c)registers_[15][c]=e[c]*f[c];
        registers_[15][3]=0;
        const auto a=input(abcd>>24,false),b=input(abcd>>16,false),c=input(abcd>>8,false),d=input(abcd,false);
        Color result{};
        for(unsigned n=0;n<3;++n)result[n]=a[n]*b[n]+(1.f-a[n])*c[n]+d[n];
        result[3]=input(efg>>8,true)[0];
        return clamp(result,0.f,1.f);
    }
private:
    using Rgb=std::array<float,3>;
    struct Products {Rgb ab,cd,sum;};
    std::uint32_t word(unsigned index) const {return words_[index];}
    static Color argb(std::uint32_t color) {
        return {float((color>>16)&255u)/255.f,float((color>>8)&255u)/255.f,float(color&255u)/255.f,float(color>>24)/255.f};
    }
    Color constant(unsigned index) const {return argb(word(index));}
    static Color clamp(Color value,float low,float high) {
        for(auto& c:value)c=std::clamp(c,low,high);
        return value;
    }
    Rgb input(std::uint32_t operand,bool alpha) const {
        const auto& value=registers_[operand&15u];
        const bool alpha_channel=(operand&16u)!=0;
        Rgb selected;
        if(alpha)selected.fill(alpha_channel?value[3]:value[2]);
        else selected=alpha_channel?Rgb{value[3],value[3],value[3]}:Rgb{value[0],value[1],value[2]};
        for(auto& x:selected) {
            switch((operand>>5)&7u) {
            case 0:x=std::max(x,0.f);break;
            case 1:x=1.f-std::clamp(x,0.f,1.f);break;
            case 2:x=2.f*std::max(x,0.f)-1.f;break;
            case 3:x=1.f-2.f*std::max(x,0.f);break;
            case 4:x=std::max(x,0.f)-.5f;break;
            case 5:x=.5f-std::max(x,0.f);break;
            case 6:break;
            default:x=-x;break;
            }
        }
        return selected;
    }
    static Rgb output(Rgb value,std::uint32_t operation) {
        for(auto& x:value) {
            switch((operation>>15)&7u) {
            case 1:x-=.5f;break;
            case 2:x*=2.f;break;
            case 3:x=(x-.5f)*2.f;break;
            case 4:x*=4.f;break;
            case 6:x*=.5f;break;
            default:break;
            }
            x=std::clamp(x,-1.f,1.f);
        }
        return value;
    }
    static float dot(const Rgb& a,const Rgb& b) {return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
    // Both RGB and alpha read the register state before either writes.
    Products combine(std::uint32_t inputs,std::uint32_t operation,bool alpha) const {
        const auto a=input(inputs>>24,alpha),b=input(inputs>>16,alpha);
        const auto c=input(inputs>>8,alpha),d=input(inputs,alpha);
        Products p;
        for(unsigned n=0;n<3;++n){p.ab[n]=a[n]*b[n];p.cd[n]=c[n]*d[n];}
        if(!alpha && (operation&0x2000u))p.ab.fill(dot(a,b));
        if(!alpha && (operation&0x1000u))p.cd.fill(dot(c,d));
        const bool mux=(word(53)&0x100u)?registers_[12][3]>=.5f:(unsigned(registers_[12][3]*255.f)&1u)!=0;
        for(unsigned n=0;n<3;++n)p.sum[n]=(operation&0x4000u)?(mux?p.cd[n]:p.ab[n]):p.ab[n]+p.cd[n];
        p.ab=output(p.ab,operation);p.cd=output(p.cd,operation);p.sum=output(p.sum,operation);
        return p;
    }
    void store(unsigned destination,const Rgb& value,bool alpha,bool blue) {
        if(!destination)return;
        auto& target=registers_[destination];
        if(alpha){target[3]=value[0];return;}
        target[0]=value[0];target[1]=value[1];target[2]=value[2];
        if(blue)target[3]=value[2];
    }
    void commit(std::uint32_t operation,bool alpha,const Products& p) {
        store((operation>>4)&15u,p.ab,alpha,(operation&0x80000u)!=0);
        store(operation&15u,p.cd,alpha,(operation&0x40000u)!=0);
        store((operation>>8)&15u,p.sum,alpha,false);
    }
    const std::array<std::uint32_t,60>& words_;
    std::array<Color,16> registers_{};
};
}

std::optional<Vec3> average_glow_color(const Material& material) {
    if(!material.forward_pass)return {};
    const auto& program=material.forward_program;
    std::array<std::optional<StageImage>,4> images;
    const Texture* sources[]{&material.diffuse,&material.detail,&material.bump,&material.auxiliary};
    for(unsigned stage=0;stage<4;++stage) {
        const auto mode=(program.words[54]>>(5*stage))&31u;
        if(!fetches_texture(mode))continue;
        const auto& frames=material.animated_textures[stage];
        const auto& source=frames.empty()?*sources[stage]:frames.at(std::min<std::size_t>(material.texture_frames[stage],frames.size()-1));
        images[stage]=stage_image(source);
        if(!images[stage])return {};
    }
    // The transparent vertex programs pass the layer's diffuse colour as v0.
    const Color v0{material.forward_diffuse[0],material.forward_diffuse[1],material.forward_diffuse[2],material.forward_diffuse[3]};
    const Color v1{0,0,0,1};
    Combiner combiner(program);
    double sum[3]{};
    for(unsigned y=0;y<grid;++y)for(unsigned x=0;x<grid;++x) {
        const float u=(float(x)+.5f)/float(grid),v=(float(y)+.5f)/float(grid);
        std::array<Color,4> textures{};
        for(unsigned stage=0;stage<4;++stage) {
            const auto mode=(program.words[54]>>(5*stage))&31u;
            if(images[stage])textures[stage]=images[stage]->at(u,v);
            else if(mode==mode_passthrough)textures[stage]={u,v,0,1};
        }
        const auto color=combiner.run(textures,v0,v1);
        // Light added to the frame: ONE/ONE and premultiplied layers add
        // their RGB, SRC_ALPHA/ONE adds RGB scaled by alpha.
        const float weight=material.forward_blend==18?color[3]:1.f;
        for(unsigned c=0;c<3;++c)sum[c]+=double(color[c]*weight);
    }
    const double count=double(grid*grid);
    return Vec3{float(sum[0]/count),float(sum[1]/count),float(sum[2]/count)};
}
std::optional<ShaderVector> average_image_rgba(const Texture& texture) {
    const auto image=stage_image(texture);
    if(!image || image->texels.empty())return {};
    double sum[4]{};
    for(const auto& texel:image->texels)for(unsigned c=0;c<4;++c)sum[c]+=texel[c];
    const double count=double(image->texels.size());
    return ShaderVector{float(sum[0]/count),float(sum[1]/count),float(sum[2]/count),float(sum[3]/count)};
}
}
