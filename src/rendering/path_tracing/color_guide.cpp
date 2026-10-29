#include "halo2/path_tracing/color_guide.hpp"
#include "halo2/path_tracing/unlit_uv.hpp"
#include "halo2/source_texture_filtering.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2::path_tracing {
namespace {
constexpr std::size_t maximum_words=16'777'216;
constexpr std::uint32_t vertex_kind=0xffffffffu;
float brightness(Vec3 c){return (c.x*.299f+c.y*.587f)+c.z*.114f;}
bool finite(Vec3 c){return std::isfinite(c.x)&&std::isfinite(c.y)&&std::isfinite(c.z);}
std::uint32_t rgb(Vec3 c){
    return std::uint32_t(std::round(std::clamp(c.x,0.f,1.f)*255))|
        (std::uint32_t(std::round(std::clamp(c.y,0.f,1.f)*255))<<8)|
        (std::uint32_t(std::round(std::clamp(c.z,0.f,1.f)*255))<<16);
}
Vec3 rgb(std::uint32_t c){return {float(c&255)/255,float((c>>8)&255)/255,float((c>>16)&255)/255};}
std::uint32_t uv(Vec2 c){
    const auto axis=[](float v){if(!std::isfinite(v))throw FormatError("RT color guide has nonfinite source UV");
        return std::uint16_t(std::int16_t(std::round(std::clamp(v,-1.f,1.f)*32767.f)));};
    return axis(c.x)|(std::uint32_t(axis(c.y))<<16);
}
Vec2 uv(std::uint32_t c){return {float(std::int16_t(c&65535))/32767,float(std::int16_t(c>>16))/32767};}
std::optional<Vec3> valid(Vec3 c){return finite(c)&&brightness(c)>0.f?std::optional(c):std::nullopt;}
}
std::array<std::uint32_t,4> color_guide_material(const Material& m){
    const auto qualified=qualified_unlit_uv(m);
    if(m.world_pass && m.per_vertex_lighting && qualified && qualified->family==UnlitUvFamily::LightmapDetail)
        return {2,0,0,0}; // Proven source static vertex radiance, never arbitrary model tint.
    const Material* owner=m.baked_lightmap_texture?&m:nullptr;
    if(!owner && m.environment_pass && m.environment_pass->baked_lightmap_texture)owner=m.environment_pass.get();
    if(!owner || !owner->world_pass || owner->per_vertex_lighting || !owner->forward_pass ||
       owner->forward_pass_index>=owner->passes.size())return {};
    const auto& pass=owner->passes[owner->forward_pass_index];
    if(pass.vertex_shader.find("\\lightmap_perpixel")==std::string::npos ||
       !std::any_of(pass.textures.begin(),pass.textures.end(),[](const auto& b){return b.external==11 && b.stage==3;}))return {};
    const auto& sampler=owner->texture_states[3];
    if(sampler.address_u<1 || sampler.address_u>5 || sampler.address_v<1 || sampler.address_v>5 ||
       sampler.color_sign || sampler.color_key_op || sampler.max_mip)return {};
    return {1,sampler.address_u|(sampler.address_v<<4)|(source_texture_filtering(sampler).mag_filter<<8),sampler.border_color,0};
}
ShaderVector color_guide_triangle(const Vertex& a,const Vertex& b,const Vertex& c,const GpuMaterial& m){
    if(m.color_guide[0]==1 && a.lightmap && a.lightmap==b.lightmap && a.lightmap==c.lightmap)
        return {std::bit_cast<float>(uv(a.lightmap_uv)),std::bit_cast<float>(uv(b.lightmap_uv)),
            std::bit_cast<float>(uv(c.lightmap_uv)),std::bit_cast<float>(a.lightmap)};
    if(m.color_guide[0]==2)
        return {std::bit_cast<float>(rgb(a.color)),std::bit_cast<float>(rgb(b.color)),
            std::bit_cast<float>(rgb(c.color)),std::bit_cast<float>(vertex_kind)};
    return {};
}
void refresh_color_guides(Scene& scene,std::span<const Texture> images){
    if(images.size()>65535)throw FormatError("RT color guide image bank exceeds source limits");
    std::vector<std::array<std::uint64_t,2>> keys;keys.reserve(images.size());
    for(const auto& image:images)keys.push_back({image.pixels.fingerprint(),
        std::uint64_t(image.width)|(std::uint64_t(image.height)<<16)|(std::uint64_t(image.format)<<48)});
    if(keys==scene.color_guide_image_keys && !scene.color_guide_pixels.empty())return;
    std::vector<std::uint32_t> words(4*(images.size()+1));words[0]=std::uint32_t(images.size());
    for(std::size_t n=0;n<images.size();++n){
        auto image=images[n];if(image.format>=14 && image.format<=16)image=prepare_xemu_bc_texture(image);
        if(image.format!=11 || !image.width || !image.height || image.layers!=1 || image.depth)
            throw FormatError("RT color guide requires an authored two-dimensional source lightmap");
        const auto offset=image.mips.empty()?0:image.mips.front().offset;
        const auto count=std::size_t(image.width)*image.height;
        if(count>maximum_words || words.size()>maximum_words-count || offset>image.pixels.size() || count*4>image.pixels.size()-offset)
            throw FormatError("RT color guide atlas exceeds bounded source storage");
        const auto record=(n+1)*4;words[record]=std::uint32_t(words.size());words[record+1]=image.width;words[record+2]=image.height;
        const Bytes bytes(image.pixels);for(std::size_t p=0;p<count;++p)words.push_back(bytes.u32(offset+p*4));
    }
    scene.color_guide_image_keys=std::move(keys);scene.color_guide_pixels=std::move(words);
}
std::optional<Vec3> sample_color_guide(const Scene& scene,const GpuMaterial& m,std::uint32_t image,Vec2 coordinate,Vec3 vertex_radiance){
    if(m.color_guide[0]==2)return valid(vertex_radiance);
    const auto& data=scene.color_guide_pixels;
    if(m.color_guide[0]!=1 || data.size()<4 || !image || image>data[0] || image*4+3>=data.size())return {};
    const auto record=image*4,width=data[record+1],height=data[record+2],sampler=m.color_guide[1];
    const auto address=[](int p,unsigned extent,unsigned mode){
        if(mode==3)return std::clamp(p,0,int(extent)-1);
        if(mode==4)return p<0 || p>=int(extent)?-1:p;
        if(mode==5)return std::min(p<0?-p-1:p,int(extent)-1);
        const int period=int(extent)*(mode==2?2:1);int at=p%period;if(at<0)at+=period;
        return mode==2 && at>=int(extent)?period-at-1:at;
    };
    const auto fetch=[&](int x,int y){x=address(x,width,sampler&15u);y=address(y,height,(sampler>>4)&15u);
        if(x<0 || y<0){const auto c=m.color_guide[2];return Vec3{float((c>>16)&255)/255,float((c>>8)&255)/255,float(c&255)/255};}
        return rgb(data.at(data[record]+unsigned(y)*width+unsigned(x)));};
    if(((sampler>>8)&255u)==1)return valid(fetch(int(std::floor(coordinate.x*width)),int(std::floor(coordinate.y*height))));
    const float x=coordinate.x*width-.5f,y=coordinate.y*height-.5f;const int left=int(std::floor(x)),top=int(std::floor(y));
    const float fx=x-left,fy=y-top;const auto a=fetch(left,top),b=fetch(left+1,top),c=fetch(left,top+1),d=fetch(left+1,top+1);
    return valid({(a.x*(1-fx)+b.x*fx)*(1-fy)+(c.x*(1-fx)+d.x*fx)*fy,
        (a.y*(1-fx)+b.y*fx)*(1-fy)+(c.y*(1-fx)+d.y*fx)*fy,
        (a.z*(1-fx)+b.z*fx)*(1-fy)+(c.z*(1-fx)+d.z*fx)*fy});
}
std::optional<Vec3> sample_color_guide(const Scene& scene,const GpuTriangle& triangle,Vec3 weights){
    const auto image=std::bit_cast<std::uint32_t>(triangle.color_guide[3]);if(!image)return {};
    const auto& m=scene.materials.at(triangle.metadata[0]);
    if(image==vertex_kind){
        const auto a=rgb(std::bit_cast<std::uint32_t>(triangle.color_guide[0])),b=rgb(std::bit_cast<std::uint32_t>(triangle.color_guide[1])),
            c=rgb(std::bit_cast<std::uint32_t>(triangle.color_guide[2]));
        return sample_color_guide(scene,m,0,{},Vec3{(a.x*weights.x+b.x*weights.y)+c.x*weights.z,
            (a.y*weights.x+b.y*weights.y)+c.y*weights.z,(a.z*weights.x+b.z*weights.y)+c.z*weights.z});
    }
    const auto a=uv(std::bit_cast<std::uint32_t>(triangle.color_guide[0])),b=uv(std::bit_cast<std::uint32_t>(triangle.color_guide[1])),
        c=uv(std::bit_cast<std::uint32_t>(triangle.color_guide[2]));
    return sample_color_guide(scene,m,image,{(a.x*weights.x+b.x*weights.y)+c.x*weights.z,(a.y*weights.x+b.y*weights.y)+c.y*weights.z});
}
Vec3 guide_static_color(Vec3 incoming,const std::optional<Vec3>& guide){
    if(!guide || !valid(*guide))return incoming;
    const float scale=brightness(incoming)/brightness(*guide);return {guide->x*scale,guide->y*scale,guide->z*scale};
}
}
