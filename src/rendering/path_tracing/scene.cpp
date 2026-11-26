#include "halo2/path_tracing/scene.hpp"
#include "material_atlas.hpp"
#include "halo2/path_tracing/material_residency.hpp"
#include "halo2/path_tracing/material_emission.hpp"
#include "halo2/path_tracing/emission.hpp"
#include "halo2/path_tracing/emitter_sampling.hpp"
#include "halo2/path_tracing/surface.hpp"
#include "halo2/path_tracing/surface_material.hpp"
#include "halo2/path_tracing/chamber_reflection.hpp"
#include "halo2/path_tracing/material_surface.hpp"
#include "halo2/path_tracing/color_guide.hpp"
#include "halo2/path_tracing/glow.hpp"
#include "halo2/path_tracing/source_ambient.hpp"
#include "halo2/pstencil/environment.hpp"
#include "halo2/pstencil/combined_environment.hpp"
#include "halo2/source_texture_filtering.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

namespace halo2::path_tracing {
namespace {
// Glow layers are drawn additively over a dark, LDR frame; Halo 2 baked them
// as radiosity emitters well above that display value. Four times the layer's
// drawn colour reads as a lit fixture without overexposing nearby walls.
constexpr float glow_gain=4.f;
constexpr std::size_t maximum_triangles=2'000'000,maximum_texels=material_atlas::maximum_words;
// Source vertex IDs and GPU triangle material slots are uint32. Preserve that
// identity namespace, including preloaded slots activated by later geometry.
// VkPhysicalDeviceLimits::maxStorageBufferRange is a uint32 byte limit; the
// upload owner additionally checks the selected device's actual lower limit
// before replacing any buffer. A 16-bit slot cap has no corresponding owner.
constexpr std::size_t maximum_materials=
    std::numeric_limits<std::uint32_t>::max()/sizeof(GpuMaterial);
// An emission.w pointer is stored as uintBitsToFloat(offset+1), so zero still
// represents the legacy constant-only material interface. Record offsets are
// relative to their own record, allowing a complete material allocation to move.
constexpr std::uint32_t emission_magic=0x45525448u,emission_record_words=48;
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 mul(Vec3 a,float b){return {a.x*b,a.y*b,a.z*b};}
float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
bool finite(Vec3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
ShaderVector vec(Vec3 p){return {p.x,p.y,p.z,0};}
float unit(float p){return std::isfinite(p)?std::clamp(p,0.f,1.f):0.f;}
float positive(float p){return std::isfinite(p)?std::max(0.f,p):0.f;}
void bounds(ShaderVector &a,ShaderVector &b,const GpuTriangle &t) {
    for(unsigned n=0;n<3;++n)for(const auto *p:{&t.p0,&t.p1,&t.p2}) {
        a[n]=std::min(a[n],(*p)[n]);b[n]=std::max(b[n],(*p)[n]);
    }
}
std::uint32_t build_node(Scene &scene,std::uint32_t first,std::uint32_t count,unsigned depth) {
    const auto index=std::uint32_t(scene.nodes.size());scene.nodes.emplace_back();
    ShaderVector low{INFINITY,INFINITY,INFINITY,0},high{-INFINITY,-INFINITY,-INFINITY,0};
    for(std::uint32_t n=0;n<count;++n)bounds(low,high,scene.triangles[first+n]);
    scene.nodes[index].minimum=low;scene.nodes[index].maximum=high;
    if(count<=4 || depth>=30) {scene.nodes[index].children={first,count,1,0};return index;}
    // Large BSP triangles overlap badly under a count-median split. Minimize
    // child surface area times primitive count, while retaining the bounded
    // depth required by the GPU traversal stack.
    constexpr unsigned bin_count=12;
    struct Bin {
        ShaderVector low{INFINITY,INFINITY,INFINITY,0},high{-INFINITY,-INFINITY,-INFINITY,0};
        std::uint32_t count{};
        void merge(const Bin& other) {
            if(!other.count)return;
            for(unsigned a=0;a<3;++a){low[a]=std::min(low[a],other.low[a]);high[a]=std::max(high[a],other.high[a]);}
            count+=other.count;
        }
        double area()const {
            const double x=double(high[0])-low[0],y=double(high[1])-low[1],z=double(high[2])-low[2];
            return 2.0*(x*y+x*z+y*z);
        }
    };
    const auto center=[](const GpuTriangle& t,unsigned axis) {
        return (double(t.p0[axis])+t.p1[axis]+t.p2[axis])/3.0;
    };
    std::array<double,3> center_low{INFINITY,INFINITY,INFINITY},center_high{-INFINITY,-INFINITY,-INFINITY};
    for(std::uint32_t n=0;n<count;++n)for(unsigned a=0;a<3;++a) {
        const auto value=center(scene.triangles[first+n],a);
        center_low[a]=std::min(center_low[a],value);center_high[a]=std::max(center_high[a],value);
    }
    double best_cost=INFINITY;
    unsigned best_axis=3,best_bin{};
    // Every selected child must still fit into the remaining depth. A median
    // fallback can always satisfy this invariant when the parent does.
    const auto child_capacity=std::uint64_t(4)<<(29-depth);
    for(unsigned a=0;a<3;++a) {
        const auto extent=center_high[a]-center_low[a];
        if(!(extent>0))continue;
        std::array<Bin,bin_count> bins;
        const auto bin=[&](const GpuTriangle& t) {
            return std::min(bin_count-1,unsigned((center(t,a)-center_low[a])*double(bin_count)/extent));
        };
        for(std::uint32_t n=0;n<count;++n) {
            const auto& triangle=scene.triangles[first+n];auto& target=bins[bin(triangle)];
            bounds(target.low,target.high,triangle);++target.count;
        }
        std::array<Bin,bin_count> prefix,suffix;
        Bin accumulated;
        for(unsigned n=0;n<bin_count;++n){accumulated.merge(bins[n]);prefix[n]=accumulated;}
        accumulated=Bin{};
        for(unsigned n=bin_count;n-->0;){accumulated.merge(bins[n]);suffix[n]=accumulated;}
        for(unsigned n=0;n+1<bin_count;++n) {
            const auto& left=prefix[n];const auto& right=suffix[n+1];
            if(!left.count || !right.count || left.count>child_capacity || right.count>child_capacity)continue;
            const auto cost=left.area()*left.count+right.area()*right.count;
            if(cost<best_cost){best_cost=cost;best_axis=a;best_bin=n;}
        }
    }
    std::uint32_t middle{};
    if(best_axis<3) {
        const auto extent=center_high[best_axis]-center_low[best_axis];
        const auto at=std::partition(scene.triangles.begin()+first,scene.triangles.begin()+first+count,
            [&](const auto& triangle) {
                const auto bin=std::min(bin_count-1,unsigned((center(triangle,best_axis)-center_low[best_axis])*double(bin_count)/extent));
                return bin<=best_bin;
            });
        middle=std::uint32_t(at-scene.triangles.begin());
    } else {
        unsigned axis=0;for(unsigned a=1;a<3;++a)
            if(center_high[a]-center_low[a]>center_high[axis]-center_low[axis])axis=a;
        middle=first+count/2;
        std::nth_element(scene.triangles.begin()+first,scene.triangles.begin()+middle,
            scene.triangles.begin()+first+count,[&](const auto& a,const auto& b){return center(a,axis)<center(b,axis);});
    }
    const auto left=build_node(scene,first,middle-first,depth+1);
    const auto right=build_node(scene,middle,first+count-middle,depth+1);
    scene.nodes[index].children={left,right,0,0};return index;
}
double hierarchy_cost(const Scene& scene) {
    const auto area=[](const GpuNode& node) {
        const double x=double(node.maximum[0])-node.minimum[0];
        const double y=double(node.maximum[1])-node.minimum[1];
        const double z=double(node.maximum[2])-node.minimum[2];
        return 2.0*(x*y+x*z+y*z);
    };
    if(scene.nodes.empty())return 0;
    const auto root_area=area(scene.nodes[0]);
    if(!(root_area>0) || !std::isfinite(root_area))return INFINITY;
    double cost=0;
    for(const auto& node:scene.nodes)
        cost+=area(node)*(node.children[2]?node.children[1]:1u);
    return cost/root_area;
}
bool reusable_hierarchy(const Scene& scene,std::vector<std::uint32_t>& permutation) {
    const auto count=scene.triangles.size();
    if(!count || count>maximum_triangles || scene.nodes.empty() || scene.nodes.size()>count*2-1 ||
       !(scene.nodes[0].minimum[3]>0) || !std::isfinite(scene.nodes[0].minimum[3]))return false;
    // The spare p0.w lane carries the admitted source ordinal, not a numeric
    // position component. All CPU/GPU geometry consumers use p0.xyz only.
    permutation.resize(count);
    for(std::size_t n=0;n<count;++n) {
        const auto ordinal=std::bit_cast<std::uint32_t>(scene.triangles[n].p0[3]);
        if(ordinal>=count)return false;
        permutation[n]=ordinal;
    }
    // The high bit is available because the admitted bank is bounded to2M.
    // Use it as an in-place uniqueness mark instead of a second triangle map.
    constexpr std::uint32_t marked=0x80000000u;
    for(std::size_t n=0;n<count;++n) {
        const auto ordinal=permutation[n]&~marked;
        if(permutation[ordinal]&marked)return false;
        permutation[ordinal]|=marked;
    }
    for(auto& ordinal:permutation)ordinal&=~marked;
    // Validate all links before refitting in reverse order: each node is
    // reached exactly once, children follow their parent, and leaves cover
    // every reordered primitive exactly once within the GPU stack depth cap.
    std::vector<std::uint8_t> depths(scene.nodes.size(),255);depths[0]=0;
    std::size_t covered=0;
    for(std::size_t n=0;n<scene.nodes.size();++n) {
        const auto& child=scene.nodes[n].children;
        if(depths[n]>30 || child[2]>1)return false;
        if(child[2]) {
            if(child[0]!=covered || !child[1] || child[1]>count-covered ||
               (child[1]>4 && depths[n]!=30))return false;
            covered+=child[1];
        } else {
            if(depths[n]>=30 || child[0]<=n || child[1]<=n || child[0]==child[1] ||
               child[0]>=scene.nodes.size() || child[1]>=scene.nodes.size() ||
               depths[child[0]]!=255 || depths[child[1]]!=255)return false;
            depths[child[0]]=depths[child[1]]=depths[n]+1;
        }
    }
    return covered==count;
}
void reorder_for_refit(Scene& scene,std::vector<std::uint32_t>& permutation) {
    // destination[n] takes raw source[permutation[n]]. Resolve each cycle in
    // place so animated payloads never need a duplicate144-byte triangle bank.
    for(std::uint32_t first=0;first<permutation.size();++first) {
        if(permutation[first]==first)continue;
        auto saved=scene.triangles[first];auto at=first;
        while(permutation[at]!=first) {
            const auto next=permutation[at];scene.triangles[at]=scene.triangles[next];
            permutation[at]=at;at=next;
        }
        scene.triangles[at]=saved;permutation[at]=at;
    }
}
void refit_hierarchy(Scene& scene) {
    for(std::size_t n=scene.nodes.size();n-->0;) {
        auto& node=scene.nodes[n];
        node.minimum={INFINITY,INFINITY,INFINITY,0};node.maximum={-INFINITY,-INFINITY,-INFINITY,0};
        if(node.children[2]) {
            for(std::uint32_t t=0;t<node.children[1];++t)
                bounds(node.minimum,node.maximum,scene.triangles[node.children[0]+t]);
        } else for(unsigned a=0;a<3;++a) {
            node.minimum[a]=std::min(scene.nodes[node.children[0]].minimum[a],scene.nodes[node.children[1]].minimum[a]);
            node.maximum[a]=std::max(scene.nodes[node.children[0]].maximum[a],scene.nodes[node.children[1]].maximum[a]);
        }
    }
}
void append_texture(Scene &scene,GpuMaterial &target,const Texture &source,unsigned minimum_level=0) {
    if(!source.width || !source.height || source.pixels.empty() || source.layers!=1 || source.depth) {
        target.texture[0]=std::uint32_t(scene.pixels.size());target.texture[1]=target.texture[2]=1;
        scene.pixels.push_back(0xffffffffu);return;
    }
    // Primary rays use the resident full-resolution image. Secondary rays use
    // an existing authored mip <=128 where available, bounding the scene atlas.
    Texture image=source;
    if(image.mips.empty()) {
        const auto bytes=image.format==11?std::size_t(image.width)*image.height*4:
            std::size_t((image.width+3)/4)*((image.height+3)/4)*(image.format==14?8:16);
        image.mips.push_back({image.width,image.height,0,bytes});
    }
    if(image.format>=14 && image.format<=16)image=prepare_xemu_bc_texture(image);
    if(image.format!=11)throw FormatError("Path Tracing unsupported authored base texture format");
    unsigned level=std::min(minimum_level,unsigned(image.mips.size()-1));
    while(level+1<image.mips.size() && (image.mips[level].width>128 || image.mips[level].height>128))++level;
    const auto &mip=image.mips[level];
    const std::size_t count=std::size_t(mip.width)*mip.height;
    if(!count || count>maximum_texels || scene.pixels.size()>maximum_texels-count ||
       mip.offset>image.pixels.size() || count*4>image.pixels.size()-mip.offset)
        throw FormatError("Path Tracing authored base texture exceeds bounded scene storage");
    target.texture[0]=std::uint32_t(scene.pixels.size());target.texture[1]=mip.width;target.texture[2]=mip.height;
    const Bytes bytes(image.pixels);
    for(std::size_t p=0;p<count;++p)scene.pixels.push_back(bytes.u32(mip.offset+p*4));
}
// A surface's normal map, for primary pixels: its mip chain from the largest
// level no wider than 256 texels down to 4, as RGBA8. Record: the level count,
// then per level its offset from the record, width and height; then texels.
// Returns the record's offset in `image`, or none when it cannot be decoded.
constexpr std::uint32_t no_normal_map=~0u,normal_map_levels=7;
std::uint32_t append_normal_map(Scene& image,const Texture& source) {
    if(!source.width || !source.height || source.pixels.empty() || source.layers!=1 || source.depth)return no_normal_map;
    Texture decoded=source;
    if(decoded.mips.empty()) {
        const auto bytes=decoded.format==11?std::size_t(decoded.width)*decoded.height*4:
            std::size_t((decoded.width+3)/4)*((decoded.height+3)/4)*(decoded.format==14?8:16);
        decoded.mips.push_back({decoded.width,decoded.height,0,bytes});
    }
    try {
        if(decoded.format>=14 && decoded.format<=16)decoded=prepare_xemu_bc_texture(decoded);
    } catch(const FormatError&) {return no_normal_map;}
    if(decoded.format!=11)return no_normal_map;
    std::size_t first=0;
    while(first+1<decoded.mips.size() && (decoded.mips[first].width>256 || decoded.mips[first].height>256))++first;
    std::vector<std::size_t> levels;
    for(auto level=first;level<decoded.mips.size() && levels.size()<normal_map_levels;++level) {
        const auto& mip=decoded.mips[level];
        if(mip.width<4 || mip.height<4)break;
        if(mip.offset>decoded.pixels.size() || std::size_t(mip.width)*mip.height*4>decoded.pixels.size()-mip.offset)break;
        levels.push_back(level);
    }
    if(levels.empty())return no_normal_map;
    const auto record=std::uint32_t(image.pixels.size());
    image.pixels.push_back(std::uint32_t(levels.size()));
    const auto headers=image.pixels.size();
    image.pixels.resize(headers+levels.size()*3);
    const Bytes bytes(decoded.pixels);
    for(std::size_t n=0;n<levels.size();++n) {
        const auto& mip=decoded.mips[levels[n]];
        image.pixels[headers+n*3]=std::uint32_t(image.pixels.size())-record;
        image.pixels[headers+n*3+1]=mip.width;image.pixels[headers+n*3+2]=mip.height;
        for(std::size_t texel=0;texel<std::size_t(mip.width)*mip.height;++texel)image.pixels.push_back(bytes.u32(mip.offset+texel*4));
    }
    return record;
}
// The authored reflection cube has a bounded base mip and, when present,
// the next coarser source mip. Each starts with the face size followed by
// RGBA8 faces +X -X +Y -Y +Z -Z.
constexpr std::uint32_t no_environment_map=~0u;
constexpr unsigned environment_face_limit=64;
std::optional<std::array<TextureMip,6>> environment_map_faces(const Texture& source,unsigned coarser=0) {
    if(source.layers!=6 || source.depth || !source.width || source.width!=source.height ||
       source.mips.empty() || source.mips.size()%6 ||
       (source.format!=11 && (source.format<14 || source.format>16)))return {};
    const std::size_t levels=source.mips.size()/6;
    if(levels>std::size_t(std::bit_width(source.width)))return {};
    std::size_t level=0;
    while(level+1<levels && (source.width>>level)>environment_face_limit)++level;
    level+=coarser;
    if(level>=levels)return {};
    const auto size=source.width>>level;
    if(size>environment_face_limit)return {};
    const auto blocks=(size+3)/4;
    const std::size_t bytes=source.format==11?size*size*4:blocks*blocks*(source.format==14?8:16);
    std::array<TextureMip,6> faces;
    for(std::size_t face=0;face<faces.size();++face) {
        const auto& mip=source.mips[face*levels+level];
        if(mip.width!=size || mip.height!=size || mip.depth!=1 || mip.size!=bytes ||
           mip.offset>source.pixels.size() || bytes>source.pixels.size()-mip.offset)return {};
        faces[face]=mip;
    }
    return faces;
}
bool append_environment_level(Scene& image,const Texture& source,const std::array<TextureMip,6>& faces) {
    const auto size=faces.front().width;
    const auto count=std::size_t(size)*size;
    Texture decoded{size,size,source.format,source.pixels,{faces.begin(),faces.end()},6};
    try {
        if(source.format!=11)decoded=prepare_xemu_bc_texture(decoded);
    } catch(const FormatError&) {return false;}
    image.pixels.push_back(size);
    const Bytes bytes(decoded.pixels);
    for(const auto& mip:decoded.mips)
        for(std::size_t texel=0;texel<count;++texel)image.pixels.push_back(bytes.u32(mip.offset+texel*4));
    return true;
}
std::uint32_t append_environment_map(Scene& image,const Texture& source,GpuMaterial& target) {
    const auto faces=environment_map_faces(source);
    const auto record=std::uint32_t(image.pixels.size());
    if(!faces || !append_environment_level(image,source,*faces))return no_environment_map;
    if(const auto coarse=environment_map_faces(source,1);coarse && append_environment_level(image,source,*coarse))
        target.texture[3]|=environment_mip_flag;
    return record;
}
material_atlas::ImageKey environment_image_key(const Texture& image) {
    std::uint64_t hash=14695981039346656037ull;
    const auto add=[&](std::uint64_t word){hash^=word;hash*=1099511628211ull;};
    add(image.width);add(image.height);add(image.format);add(image.layers);add(image.depth);
    for(const auto& mip:image.mips) {add(mip.width);add(mip.height);add(mip.depth);add(mip.offset);add(mip.size);}
    return {image.pixels.fingerprint(),hash};
}
const Texture &current_base_texture(const Material &material) {
    if(material.animated_textures[0].empty())return material.diffuse;
    const auto frame=material.texture_frames[0];
    if(frame>=material.animated_textures[0].size())
        throw FormatError("Path Tracing base texture frame escaped its authored bank");
    return material.animated_textures[0][frame];
}
const Texture &emission_texture(const Material& material,unsigned stage) {
    if(!material.animated_textures[stage].empty())return material.animated_textures[stage].at(material.texture_frames[stage]);
    const Texture* images[]{&material.diffuse,&material.detail,&material.bump,&material.auxiliary};
    return *images[stage];
}
std::vector<const Material*> emission_passes(const Material& material) {
    std::vector<const Material*> passes;
    const auto append=[&](const Material& pass){if(supports_emission(pass))passes.push_back(&pass);};
    if(material.forward_vertex==2)append(material);
    for(const auto& pass:material.world_overlays_after)if(pass)append(*pass);
    return passes;
}
void append_emission(Scene& image,GpuMaterial& target,const Material& material) {
    const auto passes=emission_passes(material);if(passes.empty())return;
    if(passes.size()>64 || image.pixels.size()>maximum_texels-2-passes.size()*emission_record_words)
        throw FormatError("Path Tracing emissive pass bank exceeds bounded scene storage");
    const auto start=std::uint32_t(image.pixels.size());
    image.pixels.push_back(emission_magic);image.pixels.push_back(std::uint32_t(passes.size()));
    image.pixels.resize(image.pixels.size()+passes.size()*emission_record_words);
    for(unsigned n=0;n<passes.size();++n) {
        const auto& pass=*passes[n];const auto record=start+2+n*emission_record_words;
        image.pixels[record]=unsigned(emission_family(pass));image.pixels[record+1]=pass.forward_program.words[54];
        constexpr unsigned constants[]{10,18,11,19,43,12};
        for(unsigned c=0;c<6;++c)image.pixels[record+2+c]=pass.forward_program.words[constants[c]];
        const auto& base=pass.forward_transforms[0];
        for(unsigned stage=0;stage<4;++stage) {
            if(!((pass.forward_program.words[54]>>(stage*5))&31u))continue;
            const auto& sampler=pass.texture_states[stage];
            if(sampler.address_u<1 || sampler.address_u>5 || sampler.address_v<1 || sampler.address_v>5 ||
               sampler.color_key_op || sampler.color_sign)
                throw FormatError("Path Tracing emissive texture lacks its source sampler interface");
            auto texture=emission_texture(pass,stage);
            if(texture.mips.empty() && texture.format>=14 && texture.format<=16) {
                const auto bytes=std::size_t((texture.width+3)/4)*((texture.height+3)/4)*(texture.format==14?8:16);
                texture.mips.push_back({texture.width,texture.height,0,bytes});
            }
            if(texture.format>=14 && texture.format<=16)texture=prepare_xemu_bc_texture(texture);
            if(texture.format!=11 || !texture.width || !texture.height || texture.layers!=1 || texture.depth)
                throw FormatError("Path Tracing emissive texture lacks its authored RGBA8 image");
            const auto level=texture.mips.empty()?0u:std::min(sampler.max_mip,unsigned(texture.mips.size()-1));
            const unsigned width=texture.mips.empty()?texture.width:texture.mips[level].width;
            const unsigned height=texture.mips.empty()?texture.height:texture.mips[level].height;
            const std::size_t offset=texture.mips.empty()?0:texture.mips[level].offset;
            const std::size_t count=std::size_t(width)*height;
            if(!count || count>maximum_texels || image.pixels.size()>maximum_texels-count ||
               offset>texture.pixels.size() || count*4>texture.pixels.size()-offset)
                throw FormatError("Path Tracing emissive image exceeds bounded scene storage");
            const auto at=record+8+stage*10;const auto& transform=pass.forward_transforms[stage+1];
            image.pixels[at]=std::uint32_t(image.pixels.size())-record;
            image.pixels[at+1]=width;image.pixels[at+2]=height;
            image.pixels[at+3]=sampler.address_u|(sampler.address_v<<4)|
                (source_texture_filtering(sampler).mag_filter<<8);
            const float affine[]{base[0]*transform[0],base[1]*transform[1],
                base[2]*transform[0]+transform[2],base[3]*transform[1]+transform[3]};
            for(unsigned a=0;a<4;++a) {
                if(!std::isfinite(affine[a]))throw FormatError("Path Tracing emissive UV transform is nonfinite");
                image.pixels[at+4+a]=std::bit_cast<std::uint32_t>(affine[a]);
            }
            image.pixels[at+8]=sampler.border_color;image.pixels[at+9]=sampler.alpha_kill;
            const Bytes bytes(texture.pixels);
            for(std::size_t p=0;p<count;++p)image.pixels.push_back(bytes.u32(offset+p*4));
        }
    }
    target.emission[3]=std::bit_cast<float>(start+1);
}
void append_surface(Scene& image,GpuMaterial& target,const Material& material) {
    const auto source=qualified_surface(material);if(!source)return;
    if(image.pixels.size()>maximum_texels-surface_record_words)
        throw FormatError("Path Tracing source surface exceeds bounded scene storage");
    const auto record=std::uint32_t(image.pixels.size());image.pixels.resize(record+surface_record_words);
    image.pixels[record]=surface_magic;image.pixels[record+1]=unsigned(source->family);
    image.pixels[record+2]=material.forward_program.words[54];
    const auto colors=surface_constants(material,source->family);
    for(unsigned c=0;c<4;++c)image.pixels[record+3+c]=colors[c];
    image.pixels[record+7]=source->stages;
    for(unsigned c=0;c<4;++c)image.pixels[record+8+c]=std::bit_cast<std::uint32_t>(source->uv.base[c]);
    target.texture[3]&=~8u;
    for(unsigned stage=0;stage<4;++stage) {
        if(!(source->stages&(1u<<stage)))continue;
        const auto& sampler=material.texture_states[stage];
        GpuMaterial temporary;
        if(stage==0 && sampler.max_mip==0)
            // The standard ray material samples this same source mip. Keep
            // the sampler and UVs independent, and share the decoded texels.
            temporary.texture=target.texture;
        else append_texture(image,temporary,emission_texture(material,stage),sampler.max_mip);
        const auto at=record+12+stage*10;
        image.pixels[at]=temporary.texture[0]-record;
        image.pixels[at+1]=temporary.texture[1];image.pixels[at+2]=temporary.texture[2];
        image.pixels[at+3]=sampler.address_u|(sampler.address_v<<4)|(source_texture_filtering(sampler).mag_filter<<8);
        for(unsigned c=0;c<4;++c)image.pixels[at+4+c]=std::bit_cast<std::uint32_t>(source->uv.stages[stage][c]);
        image.pixels[at+8]=sampler.border_color;image.pixels[at+9]=sampler.alpha_kill;
        if(sampler.alpha_kill&4u)target.texture[3]|=8u;
    }
    target.texture[3]|=surface_flag;target.base[3]=std::bit_cast<float>(record+1);
}
std::array<std::uint64_t,2> material_image_key(const Material &material) {
    if(!ray_material(material))return {};
    const auto &texture=current_base_texture(material);
    std::uint64_t metadata=14695981039346656037ull;
    const auto add=[&](std::uint64_t value){metadata^=value;metadata*=1099511628211ull;};
    add(texture.width);add(texture.height);add(texture.format);add(texture.layers);add(texture.depth);
    add(material.texture_frames[0]);
    for(const auto &mip:texture.mips) {add(mip.width);add(mip.height);add(mip.depth);add(mip.offset);add(mip.size);}
    const auto surface=qualified_surface(material);add(surface?unsigned(surface->family):0u);
    if(surface) {
        for(float value:surface->uv.base)add(std::bit_cast<std::uint32_t>(value));
        for(const auto& transform:surface->uv.stages)for(float value:transform)add(std::bit_cast<std::uint32_t>(value));
        const auto colors=surface_constants(material,surface->family);
        const unsigned first=surface->family==SurfaceFamily::TerrainBlend ||
            surface->family==SurfaceFamily::TerrainBlendThreeStage?2:0;
        add(colors[first]);add(colors[first+1]);
        if(surface->family==SurfaceFamily::TextureFourChangeColor){add(colors[2]);add(colors[3]);}
        for(unsigned stage=0;stage<4;++stage) {
            if(!(surface->stages&(1u<<stage)))continue;
            const auto& image=emission_texture(material,stage);const auto& sampler=material.texture_states[stage];
            add(image.width);add(image.height);add(image.format);add(image.layers);add(image.depth);
            add(image.pixels.fingerprint());add(material.texture_frames[stage]);
            for(const auto& mip:image.mips){add(mip.width);add(mip.height);add(mip.offset);add(mip.size);}
            add(sampler.address_u);add(sampler.address_v);add(sampler.mag_filter);add(sampler.min_filter);
            add(sampler.anisotropy);add(sampler.max_mip);add(sampler.border_color);add(sampler.alpha_kill);
        }
    }
    const auto passes=emission_passes(material);add(passes.size());
    for(const auto* pass:passes) {
        add(unsigned(emission_family(*pass)));
        for(const auto word:pass->forward_program.words)add(word);
        for(const auto& transform:pass->forward_transforms)for(const auto value:transform)add(std::bit_cast<std::uint32_t>(value));
        for(unsigned stage=0;stage<4;++stage) {
            if(!((pass->forward_program.words[54]>>(stage*5))&31u))continue;
            const auto& image=emission_texture(*pass,stage);const auto& sampler=pass->texture_states[stage];
            add(image.width);add(image.height);add(image.format);add(image.layers);add(image.depth);
            add(image.pixels.fingerprint());add(pass->texture_frames[stage]);
            for(const auto& mip:image.mips){add(mip.width);add(mip.height);add(mip.offset);add(mip.size);}
            add(sampler.address_u);add(sampler.address_v);add(sampler.mag_filter);add(sampler.min_filter);
            add(sampler.anisotropy);add(sampler.max_mip);add(sampler.border_color);add(sampler.alpha_kill);
        }
    }
    return {metadata,texture.pixels.fingerprint()};
}
float triangle_area(const GpuTriangle& triangle) {
    const auto& a=triangle.p0;const auto& b=triangle.p1;const auto& c=triangle.p2;
    const float x=(b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]);
    const float y=(b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]);
    const float z=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
    return .5f*std::sqrt(x*x+y*y+z*z);
}
// The emitter table light sampling reads (scene.glsl traceDirect): every
// emissive triangle, then the running sum of their power, so a shadow ray
// goes to a source in proportion to the light it gives off rather than to
// its triangle count. A source whose sampled colour is black keeps a small
// share: its texture may still emit away from the sampled point.
// The complete pixel programs of a surface's world_overlays_after layers
// (Scene::material_layer_programs), when one of them is an additive
// self-illumination layer that glow_layer.frag draws. Returns their offset
// in `image`.
std::uint32_t append_layer_programs(Scene& image,const Material& material) {
    const bool glowing=std::ranges::any_of(material.world_overlays_after,[](const auto& layer) {
        return layer && layer->forward_vertex==2 && layer->forward_blend==3;
    });
    if(!glowing)return no_layer_programs;
    const auto at=std::uint32_t(image.pixels.size());
    for(const auto& layer:material.world_overlays_after) {
        const auto words=layer?layer->forward_program.words:std::array<std::uint32_t,layer_program_words>{};
        image.pixels.insert(image.pixels.end(),words.begin(),words.end());
    }
    return at;
}
// Additive self-illumination layers drawn over a traced surface (door and
// console lights, the training device's lamps) whose program has no exact
// emission record (emission_passes). Each emits its average drawn colour.
std::vector<const Material*> self_illumination_layers(const Material& material) {
    std::vector<const Material*> layers;
    for(const auto& pass:material.world_overlays_after)
        if(pass && pass->forward_pass && pass->forward_vertex==2 && !supports_emission(*pass))layers.push_back(pass.get());
    return layers;
}
void refresh_emitters(Scene &scene) {
    scene.pixels.resize(scene.texture_texel_count);scene.emissive_count=0;
    struct Emitter {std::uint32_t entry;float power;};
    std::array<std::vector<Emitter>,2> emitters;
    for(std::uint32_t n=0;n<scene.triangles.size();++n) {
        const auto& triangle=scene.triangles[n];
        const auto& material=scene.materials[triangle.metadata[0]];
        const auto& emission=material.emission;
        if(!std::bit_cast<std::uint32_t>(emission[3]) && !(emission[0]>0) && !(emission[1]>0) && !(emission[2]>0))continue;
        // A surface that carries baked light (a lightmap or static vertex
        // radiance) was part of the bake its own emission lit.
        const bool baked=(material.texture[3]&baked_emission_flag) && std::bit_cast<std::uint32_t>(triangle.color_guide[3])!=0;
        const Vec2 centre{(triangle.uv01[0]+triangle.uv01[2]+triangle.uv2[0])/3.f,
                          (triangle.uv01[1]+triangle.uv01[3]+triangle.uv2[1])/3.f};
        const auto colour=material_emission(scene,triangle.metadata[0],centre);
        const float luminance=.2126f*colour.x+.7152f*colour.y+.0722f*colour.z;
        const float area=triangle_area(triangle);
        const float value=area*std::max(std::isfinite(luminance)?luminance:0.f,1.e-3f);
        emitters[baked].push_back({n|(baked?emitter_baked_bit:0u),std::isfinite(value) && value>0?value:1.e-12f});
    }
    // A receiver with baked lighting can sample the unbaked prefix of the
    // same cumulative distribution without spending rays on baked emitters.
    const auto unbaked=std::uint32_t(emitters[0].size());
    scene.emissive_count=unbaked+std::uint32_t(emitters[1].size());
    for(const auto& bank:emitters)for(const auto& emitter:bank)scene.pixels.push_back(emitter.entry);
    float running=0;
    for(const auto& bank:emitters)for(const auto& emitter:bank) {
        running+=emitter.power;scene.pixels.push_back(std::bit_cast<std::uint32_t>(running));
    }
    append_emitter_groups(scene,unbaked);
}
bool alpha_pass(const Scene &scene,const GpuTriangle &triangle,float u,float v) {
    const auto &m=scene.materials[triangle.metadata[0]];
    if(!(m.texture[3]&9u))return true;
    const float s=(triangle.uv01[0]*(1-u-v)+triangle.uv01[2]*u+triangle.uv2[0]*v)*m.transform[0]+m.transform[2];
    const float t=(triangle.uv01[1]*(1-u-v)+triangle.uv01[3]*u+triangle.uv2[1]*v)*m.transform[1]+m.transform[3];
    const auto coordinate=[](float a,unsigned extent,bool clamp) {
        if(clamp)a=std::clamp(a,0.f,1.f);else a-=std::floor(a);
        return std::min(extent-1,unsigned(a*float(extent)));
    };
    const auto x=coordinate(s,m.texture[1],m.texture[3]&2u),y=coordinate(t,m.texture[2],m.texture[3]&4u);
    unsigned alpha=scene.pixels[m.texture[0]+y*m.texture[1]+x]>>24;
    if(m.texture[3]&surface_flag) {
        const Vec2 raw{triangle.uv01[0]*(1-u-v)+triangle.uv01[2]*u+triangle.uv2[0]*v,
                       triangle.uv01[1]*(1-u-v)+triangle.uv01[3]*u+triangle.uv2[1]*v};
        const auto source=material_surface(scene,triangle.metadata[0],raw);if(!source)return false;
        alpha=unsigned(std::round(std::clamp((*source)[3],0.f,1.f)*255.f));
    } else if((m.texture[3]&8u) && !alpha)return false;
    if(!(m.texture[3]&1u))return true;
    const auto reference=unsigned(std::round(m.response[3]*255));
    switch((m.texture[3]>>8)&7u) {
    case 0:return false;case 1:return alpha<reference;case 2:return alpha==reference;
    case 3:return alpha<=reference;case 4:return alpha>reference;case 5:return alpha!=reference;
    case 6:return alpha>=reference;default:return true;
    }
}
bool node_hit(const GpuNode &node,Vec3 origin,Vec3 direction,float maximum) {
    float near=0,far=maximum;const auto o=vec(origin),d=vec(direction);
    for(unsigned a=0;a<3;++a) {
        if(std::abs(d[a])<1.e-20f) {if(o[a]<node.minimum[a] || o[a]>node.maximum[a])return false;continue;}
        auto p=(node.minimum[a]-o[a])/d[a],q=(node.maximum[a]-o[a])/d[a];if(p>q)std::swap(p,q);
        near=std::max(near,p);far=std::min(far,q);if(far<near)return false;
    }
    return true;
}
}
namespace alias_detail {
std::array<std::uint32_t,3> append_image(Scene& scene,const Texture& image,unsigned minimum_level) {
    GpuMaterial temporary;append_texture(scene,temporary,image,minimum_level);
    scene.texture_texel_count=std::uint32_t(scene.pixels.size());
    return {temporary.texture[0],temporary.texture[1],temporary.texture[2]};
}
}
const Material* reflection_pass(const Material &material) {
    const Material *environment=material.environment_pass.get();
    if(environment && environment->forward_vertex!=4)environment=nullptr;
    if(material.forward_vertex==4)environment=&material;
    const auto find_environment=[&](const auto &passes) {
        for(const auto &pass:passes)if(!environment && pass && pass->forward_vertex==4)environment=pass.get();
    };
    find_environment(material.world_overlays_before);find_environment(material.world_overlays_after);
    if(!environment)environment=pstencil::combined_visor_environment(material);
    const auto find_combined=[&](const Material* pass) {
        if(!environment && pass && pstencil::supports_combined_environment_program(*pass))environment=pass;
    };
    find_combined(&material);find_combined(material.environment_pass.get());
    for(const auto& pass:material.world_overlays_before)find_combined(pass.get());
    for(const auto& pass:material.world_overlays_after)find_combined(pass.get());
    if(!environment)environment=chamber_reflection_pass(material);
    return environment;
}
GpuMaterial material_response(const Material &material) {
    GpuMaterial result;
    const Material *environment=reflection_pass(material);
    const bool chamber=environment && chamber_reflection_pass(material)==environment;
    // Halo's legacy response has no physical roughness. This optional policy
    // deliberately broadens weak reflections instead of making every surface
    // metallic. Authored facing/glancing strengths remain independently bounded.
    float facing=0,glancing=0;
    // Path Tracing follows the current Original material. The May2003 lookup
    // belongs to pStencil presentation and must not override retail colors.
    if(chamber) {
        const auto parameters=chamber_reflection_parameters(environment->forward_program.words);
        facing=parameters.facing[3];glancing=parameters.glancing[3];
        for(unsigned n=0;n<3;++n) {
            result.base[n]=parameters.facing[n];result.glancing_tint[n]=parameters.glancing[n];
        }
        result.glancing_tint[3]=parameters.baked_weight;
    }else if(environment && environment->forward_vertex==6) {
        const auto& words=environment->forward_program.words;
        facing=float(words[17]>>24)/255.f;glancing=float(words[25]>>24)/255.f;
        const auto facing_color=surface_detail::argb(words[15]),glancing_color=surface_detail::argb(words[23]);
        for(unsigned n=0;n<3;++n){result.base[n]=facing_color[n];result.glancing_tint[n]=glancing_color[n];}
    }else if(environment) {
        facing=unit(environment->forward_diffuse[3]);glancing=unit(environment->forward_specular[3]);
        for(unsigned n=0;n<3;++n) {
            result.base[n]=unit(environment->forward_diffuse[n]);
            result.glancing_tint[n]=unit(environment->forward_specular[n]);
        }
    }else if(material.specular_brightness>0) {
        facing=unit(material.specular_brightness)*.04f;glancing=facing;
    }
    const float strength=std::max(facing,glancing);
    result.response={std::clamp(.85f-.75f*std::sqrt(strength),.12f,.85f),facing,glancing,
        float(material.forward_raster.alpha_reference&255u)/255.f};
    auto flags=material.forward_raster.alpha_test?1u:0u;
    if(material.texture_states[0].address_u==3)flags|=2u;
    if(material.texture_states[0].address_v==3)flags|=4u;
    if(material.texture_states[0].alpha_kill&4u)flags|=8u;
    if(glow_material(material)) {
        // Light-only emitter: no albedo (flag 32), no vertex tint (128); its
        // radiance is the authored texture times the pass's diffuse tint.
        flags|=light_only_flag|32u|128u;
    } else if(!traces_material(material))flags|=16u;
    if(material.forward_vertex==2)flags|=32u; // Primary root emits; child passes retain their source draw.
    if(qualified_unlit_white_tint(material) || (material.forward_vertex==2 && supports_emission(material)))flags|=128u;
    if(const auto* ambient=source_ambient_owner(material)) {
        flags|=source_ambient_flag;
        if(source_ambient_color_word(*ambient)==16u)flags|=source_flat_ambient_flag;
    }
    if(chamber)flags|=chamber_baked_reflection_flag;
    const auto surface=qualified_surface(material);
    if(!(flags&(16u|32u)))
        apply_surface_material_response(material,surface?surface->family:SurfaceFamily{},result.detail,result.response);
    if(chamber || (environment && environment->forward_vertex==4 && environment->forward_blend==8 && surface &&
       ((unsigned(surface->family)>=8u && unsigned(surface->family)<=10u) ||
        surface->family==SurfaceFamily::DynamicDetail || surface->family==SurfaceFamily::TextureHonorGuard)))
        // These complete native programs preserve the destination alpha
        // consumed by the environment pass, including C24.a for dynamic
        // detail and T0.a in the chamber's combined environment program.
        flags|=2048u;
    // The environment pass blends its cube map in proportion to the
    // destination alpha the texture pass left: the base map's alpha, its
    // specular mask, unless alpha is the surface's coverage.
    if(environment && environment->forward_blend==8 && !(flags&2048u) && !material.forward_raster.alpha_test)
        flags|=environment_mask_flag;
    if(!(flags&(16u|32u)) && environment && heretic_nearby_reflector(material)) {
        const bool tangent=(flags&(2048u|environment_mask_flag)) && environment->forward_vertex==4 &&
            environment->forward_pass_index<environment->passes.size() &&
            environment->passes[environment->forward_pass_index].name==
                "shaders\\shader_passes\\environment_mapping\\default_environment_map_tangent_space";
        const bool combined=material.source_surface_material==SourceSurfaceMaterial::Polished?
            pstencil::supports_combined_environment_program(*environment):
            pstencil::supports_combined_visor_environment(*environment);
        if(tangent || combined) {
            flags|=nearby_scene_reflection_flag;
            if(combined && material.source_surface_material==SourceSurfaceMaterial::Lens)
                flags|=visor_scene_reflection_flag;
        }
    }
    flags|=(material.forward_raster.alpha_function&7u)<<8;
    // Authored surface detail for the full-resolution resolve (world.frag):
    // the static lightmap pass binds diffuse/detail/bump at stages 0/1/2
    // (lightmap_tex has no detail and its bump on stage 1), and an
    // unqualified forward texture pass is evaluated with its own combiner.
    const auto real=[](const Texture& texture){return texture.width>4 && texture.height>4;};
    if(material.static_lightmap_pass && !material.passes.empty()) {
        const auto& name=material.passes[0].name;
        const auto leaf=std::string_view(name).substr(name.find_last_of('\\')+1);
        if(leaf=="lightmap_tex_detail") {
            if(real(material.detail))flags|=surface_detail_stage1_flag;
            if(real(material.bump))flags|=surface_bump_stage2_flag;
        } else if(leaf=="lightmap_tex" && real(material.detail))flags|=surface_bump_stage1_flag;
    }
    if(material.forward_pass && material.forward_vertex==3 && !surface)flags|=surface_forward_combiner_flag;
    result.texture[3]=flags;
    result.transform=material.texture_transforms[0];
    // These complete native vertex identities use c18 for base UVs even
    // when their pixel family falls back to the ordinary source texture.
    if(const auto uv=qualified_unlit_uv(material))result.transform=uv->base;
    const auto emission=[&](const Material &pass) {
        if(pass.forward_vertex!=2)return;
        for(unsigned n=0;n<3;++n)result.emission[n]+=positive(pass.forward_diffuse[n]);
    };
    emission(material);for(const auto &pass:material.world_overlays_after)if(pass)emission(*pass);
    if(surface && surface->default_object_ambient)
        result.color_guide={source_default_ambient_kind,0u,
            source_default_ambient_rgb(material.forward_program.words,surface->family),0u};
    else if(const auto fixed=surface?source_fixed_accumulated_ambient(material,surface->family):std::nullopt) {
        result.color_guide={source_default_ambient_kind,0u,*fixed,0u};
        result.texture[3]|=source_flat_ambient_flag;
    }else if(chamber && environment->per_vertex_lighting)result.color_guide={2,0,0,0};
    else result.color_guide=color_guide_material(material);
    if(glow_material(material)) {
        // Until its images are read (refresh_materials_impl replaces this
        // with the layer's average drawn colour), a glow emits its tint.
        const auto tint=material.forward_diffuse;
        const bool tinted=tint[0]>0 || tint[1]>0 || tint[2]>0;
        for(unsigned n=0;n<3;++n)result.emission[n]=glow_gain*(tinted?positive(tint[n]):1.f);
    }
    return result;
}
std::vector<const Texture*> find_environment_maps(std::span<const Material> materials) {
    std::vector<const Texture*> result(materials.size());
    for(std::size_t n=0;n<materials.size();++n) {
        if(!traces_material(materials[n]))continue;
        const auto* pass=reflection_pass(materials[n]);
        if(!pass || pass->forward_pass_index>=pass->passes.size())continue;
        const auto& bindings=pass->passes[pass->forward_pass_index].textures;
        const Texture* reflection=nullptr;
        for(const auto& binding:bindings) {
            if(binding.stage>=4 || binding.external || binding.parameter==255)continue;
            // NV2A mode 12 samples the reflected direction at stage 3.
            // Fast and combined passes supply their direction in mode 3.
            // The normalization cube is an external binding, including
            // stage 3 of the combined visor pass, and never supplies shine.
            const auto mode=(pass->forward_program.words[54]>>(5u*binding.stage))&31u;
            if(mode!=3u && (mode!=12u || binding.stage!=3u))continue;
            const auto& image=emission_texture(*pass,binding.stage);
            if(!environment_map_faces(image))continue;
            if(reflection && reflection!=&image) {
                reflection=nullptr;
                break;
            }
            reflection=&image;
        }
        result[n]=reflection;
    }
    return result;
}
std::vector<const Texture*> find_normal_maps(std::span<const Material> materials) {
    // A model shader's normal map is bound by its lighting pass (the
    // lightmap passes put it on stage 2), a separate material of the same
    // shader from the one the path tracer draws. Accept an image only if it
    // averages to an outward tangent-space normal.
    const auto normal_map=[](const Texture& image) {
        if(image.width<8 || image.height<8 || image.layers!=1 || image.depth)return false;
        const auto mean=average_image_rgba(image);
        if(!mean)return false;
        const float x=2.f*(*mean)[0]-1.f,y=2.f*(*mean)[1]-1.f,z=2.f*(*mean)[2]-1.f;
        return z>.5f && std::abs(x)<.35f && std::abs(y)<.35f;
    };
    std::unordered_map<std::uint32_t,const Texture*> by_shader;
    for(const auto& material:materials) {
        if(material.source_shader_datum==0xffffffffu || material.forward_pass_index>=material.passes.size())continue;
        if(material.passes[material.forward_pass_index].name.find("\\lightmap\\")==std::string::npos)continue;
        auto& found=by_shader[material.source_shader_datum];
        if(!found && normal_map(material.bump))found=&material.bump;
    }
    std::vector<const Texture*> result(materials.size());
    for(std::size_t n=0;n<materials.size();++n) {
        const auto& material=materials[n];
        if(!traces_material(material))continue;
        if(const auto* chamber=chamber_reflection_pass(material)) {
            const auto& bump=emission_texture(*chamber,2);
            if(normal_map(bump))result[n]=&bump;
            continue;
        }
        // Static lightmap passes already bind their own bump (source_detail.glsl).
        if(material.static_lightmap_pass)continue;
        if(const auto found=by_shader.find(material.source_shader_datum);found!=by_shader.end())result[n]=found->second;
    }
    return result;
}
GpuSceneUniform Scene::uniform(Vec3 environment) const {
    if(!finite(environment) || environment.x<0 || environment.y<0 || environment.z<0)
        throw FormatError("Path Tracing source environment is non-finite or negative");
    return {{std::uint32_t(nodes.size()),std::uint32_t(triangles.size()),std::uint32_t(lights.size()),emissive_count},
        {environment.x,environment.y,environment.z,std::bit_cast<float>(texture_texel_count)},{1,1,0,1}};
}
bool glow_material(const Material &material) {
    // This stand-alone additive layer draws the volume of the Council's
    // light beams. The triangles represent scattered light along the beam.
    // Treating their entire area as an emitter lights the chamber twice.
    if(material.name=="scenarios\\shaders\\solo\\highcharity\\long_volume" && material.forward_blend==3 &&
       material.template_name=="shaders\\shader_templates\\transparent\\one_add_illum_detail" &&
       material.forward_pass_index<material.passes.size() &&
       material.passes[material.forward_pass_index].name=="shaders\\shader_passes\\transparent\\one_add_illum_detail")
        return false;
    // Additive and self-illuminated panels can be sources beyond the light
    // mapped field. UI, water and the other presentation passes stay raster.
    const bool additive=material.forward_blend==3 || material.forward_blend==18;
    const bool illuminated=material.forward_blend==2 && material.template_name.find("illum")!=std::string::npos;
    return material.forward_pass && (additive || illuminated) &&
        (material.forward_vertex==1 || material.forward_vertex==2 || material.forward_vertex==3) &&
        !material.hud_pass && !material.light_volume_pass && !material.water_pass && !material.vista_loading &&
        !material.ui_emblem_pass && !material.ui_full_surface && !material.ui_widescreen_background &&
        !material.ui_frame_center && !material.camouflage_group_pass && !material.camouflage_scene_texture &&
        !material.diffuse.pixels.empty();
}
bool traces_material(const Material &material) {
    return !(material.forward_pass && material.forward_vertex==2 && !supports_emission(material)) &&
        !material.hud_pass && !material.light_volume_pass && !material.water_pass &&
        !material.vista_loading && !material.ui_emblem_pass &&
        !material.ui_full_surface && !material.ui_widescreen_background && !material.ui_frame_center &&
        !material.camouflage_group_pass && !material.camouflage_scene_texture &&
        material.forward_blend==0 && material.forward_vertex!=4 && material.forward_vertex!=5 &&
        material.forward_vertex!=7 && material.forward_vertex!=8 && material.forward_vertex!=10 &&
        material.forward_vertex!=11;
}
namespace material_atlas {
void relocate(GpuMaterial& material,std::uint32_t destination) {
    const auto previous=material.texture[0];
    material.texture[0]=destination;
    const auto rebase=[&](std::uint32_t offset){return destination+(offset-previous);};
    if(const auto pointer=std::bit_cast<std::uint32_t>(material.emission[3]))
        material.emission[3]=std::bit_cast<float>(rebase(pointer));
    if(material.texture[3]&surface_flag)
        material.base[3]=std::bit_cast<float>(rebase(std::bit_cast<std::uint32_t>(material.base[3])));
    if(material.detail[0])material.detail[0]=rebase(material.detail[0]);
}
bool compact(Scene& scene,std::size_t required_words,std::uint32_t replaced,
             std::span<const ImageKey> live_images,const ImageKey* pending_cube) {
    enum class Kind {Root,Image,Cube};
    struct Allocation {std::uint32_t start,size;std::size_t slot;Kind kind;};
    std::vector<Allocation> keep;
    std::unordered_set<std::uint32_t> live_cubes;
    const auto placeholder=scene.material_alias_prefix_words;
    const auto live_root=[&](std::size_t slot) {
        return slot!=replaced && (scene.live_material_roots.empty() || scene.live_material_roots[slot]);
    };
    for(std::size_t slot=0;slot<scene.materials.size();++slot) {
        const auto size=scene.material_texel_capacities[slot];
        if(!size || !live_root(slot))continue;
        const auto& material=scene.materials[slot];
        keep.push_back({material.texture[0],size,slot,Kind::Root});
        if(material.detail[1])live_cubes.insert(material.detail[1]);
    }
    for(std::size_t slot=0;slot<scene.material_alias_images.size();++slot) {
        const auto& image=scene.material_alias_images[slot];
        if(std::ranges::binary_search(live_images,image.key))
            keep.push_back({image.image[0],image.image[1]*image.image[2],slot,Kind::Image});
    }
    for(std::size_t slot=0;slot<scene.material_environment_images.size();++slot) {
        const auto& cube=scene.material_environment_images[slot];
        if((pending_cube && cube.key==*pending_cube) || live_cubes.contains(cube.offset))
            keep.push_back({cube.offset,cube.words,slot,Kind::Cube});
    }
    std::ranges::sort(keep,{},&Allocation::start);
    auto previous_end=placeholder+1u;
    std::size_t words=previous_end;
    // A genuinely over-budget live set leaves the old allocation bank intact
    // so a caller can retry another frame without stale or displaced records.
    for(const auto& allocation:keep) {
        if(allocation.start<previous_end || allocation.start>scene.texture_texel_count ||
           allocation.size>scene.texture_texel_count-allocation.start)
            throw FormatError("Path Tracing atlas compaction lost a live allocation");
        previous_end=allocation.start+allocation.size;
        words+=allocation.size;
    }
    if(required_words>maximum_words || words>maximum_words-required_words)return false;

    using Relocation=std::pair<std::uint32_t,std::uint32_t>;
    std::vector<Relocation> cube_moves;
    std::vector<MaterialAliasImage> images;
    std::vector<MaterialEnvironmentImage> cubes;
    cube_moves.reserve(scene.material_environment_images.size());
    images.reserve(scene.material_alias_images.size());
    cubes.reserve(scene.material_environment_images.size());
    auto destination=placeholder+1u;
    for(const auto& allocation:keep) {
        if(allocation.start!=destination)
            std::memmove(scene.pixels.data()+destination,scene.pixels.data()+allocation.start,
                std::size_t(allocation.size)*sizeof(std::uint32_t));
        switch(allocation.kind) {
        case Kind::Root:
            relocate(scene.materials[allocation.slot],destination);
            break;
        case Kind::Image:
            images.push_back(scene.material_alias_images[allocation.slot]);images.back().image[0]=destination;
            break;
        case Kind::Cube:
            cubes.push_back(scene.material_environment_images[allocation.slot]);cubes.back().offset=destination;
            if(allocation.start!=destination)cube_moves.emplace_back(allocation.start,destination);
            break;
        }
        destination+=allocation.size;
    }
    for(std::size_t slot=0;slot<scene.materials.size();++slot) {
        auto& material=scene.materials[slot];
        if(!live_root(slot) && scene.material_texel_capacities[slot]) {
            const auto flags=material.texture[3]&16u;
            scene.material_texel_capacities[slot]=0;scene.material_image_keys[slot]={};scene.material_glow[slot]={};
            scene.material_layer_programs[slot]=no_layer_programs;
            material={};material.texture={placeholder,1u,1u,flags};
        } else if(material.detail[1]) {
            const auto move=std::ranges::lower_bound(cube_moves,material.detail[1],{},&Relocation::first);
            if(move!=cube_moves.end() && move->first==material.detail[1])material.detail[1]=move->second;
        }
    }
    scene.material_alias_images=std::move(images);
    scene.material_environment_images=std::move(cubes);
    scene.texture_texel_count=destination;
    scene.pixels.resize(destination);
    return true;
}
}
Scene build_scene(std::span<const Vertex> vertices,std::span<const Material> materials,std::span<const Light> lights) {
    if(vertices.size()%3 || vertices.size()/3>maximum_triangles || materials.size()>maximum_materials || lights.size()>65536)
        throw FormatError("Path Tracing scene exceeds finite geometry/material/light limits");
    Scene scene;scene.materials.reserve(materials.size());
    // Unreferenced slots share one white placeholder. In particular, the
    // source bank also contains UI images/fonts that have no WORLD triangles.
    scene.pixels.push_back(0xffffffffu);scene.texture_texel_count=1;
    for(const auto &material:materials) {
        auto response=material_response(material);
        response.texture[0]=0;response.texture[1]=response.texture[2]=1;
        scene.materials.push_back(response);
        scene.material_image_keys.push_back({});scene.material_texel_capacities.push_back(0);
    }
    refresh_materials_for_geometry(scene,materials,vertices);
    update_geometry(scene,vertices,lights);return scene;
}
namespace {
void refresh_materials_impl(Scene &scene,std::span<const Material> materials,std::span<const std::uint8_t> active) {
    if(materials.size()!=scene.materials.size() || materials.size()!=scene.material_image_keys.size() ||
       materials.size()!=scene.material_texel_capacities.size() || (!active.empty() && active.size()!=materials.size()))
        throw FormatError("Path Tracing material refresh changed its resident slot owner");
    scene.pixels.resize(scene.texture_texel_count);
    scene.material_glow.resize(materials.size());
    scene.material_layer_programs.resize(materials.size(),no_layer_programs);
    for(std::size_t n=0;n<materials.size();++n) {
        // Refresh unreferenced materials when they return to the ray scene.
        if(!active.empty() && !active[n])continue;
        auto response=material_response(materials[n]);
        const auto authored_flags=response.texture[3];
        response.texture[3]|=scene.materials[n].texture[3]&environment_mip_flag;
        if(scene.materials[n].texture[3]&surface_flag) {
            response.texture[3]=(response.texture[3]&~8u)|surface_flag|(scene.materials[n].texture[3]&8u);
            response.base[3]=scene.materials[n].base[3];
        }
        const bool glow=glow_material(materials[n]);
        const auto previous=scene.materials[n].texture;
        response.emission[3]=scene.materials[n].emission[3];
        response.detail[0]=scene.materials[n].detail[0];response.detail[1]=scene.materials[n].detail[1];
        response.texture[0]=previous[0];response.texture[1]=previous[1];response.texture[2]=previous[2];
        const bool resident=scene.material_texel_capacities[n]!=0;
        const bool requested=!active.empty() && active[n] && ray_material(materials[n]);
        if(!resident && !requested) {scene.materials[n]=response;continue;}
        const auto key=material_image_key(materials[n]);
        if(!resident || key!=scene.material_image_keys[n]) {
            Scene image;response.emission[3]=0;response.base[3]=1.f;response.texture[3]=authored_flags;
            response.detail[0]=response.detail[1]=0;
            append_texture(image,response,ray_material(materials[n])?current_base_texture(materials[n]):Texture{});
            append_emission(image,response,materials[n]);
            append_surface(image,response,materials[n]);
            const auto layer_programs=append_layer_programs(image,materials[n]);
            const auto* normal_map=n<scene.material_normal_maps.size()?scene.material_normal_maps[n]:nullptr;
            const auto normal_record=normal_map?append_normal_map(image,*normal_map):no_normal_map;
            const auto* environment_map=n<scene.material_environment_maps.size()?scene.material_environment_maps[n]:nullptr;
            const auto cube_key=environment_map?environment_image_key(*environment_map):material_atlas::ImageKey{};
            const auto find_cube=[&] {return std::ranges::find(scene.material_environment_images,cube_key,&MaterialEnvironmentImage::key);};
            Scene cube_image;
            if(environment_map && find_cube()==scene.material_environment_images.end())
                append_environment_map(cube_image,*environment_map,response);
            const bool grow=image.pixels.size()>scene.material_texel_capacities[n];
            const auto required=(grow?image.pixels.size():0)+cube_image.pixels.size();
            if(required>maximum_texels)throw FormatError("Path Tracing material exceeds finite atlas storage");
            if(scene.texture_texel_count>maximum_texels-required &&
               !material_atlas::compact(scene,required,grow?std::uint32_t(n):~0u,{},environment_map?&cube_key:nullptr))
                throw FormatError("Path Tracing live materials exceed finite atlas storage");
            const auto count=std::uint32_t(image.pixels.size());
            std::uint32_t offset=scene.materials[n].texture[0];
            if(grow) {
                offset=scene.texture_texel_count;scene.texture_texel_count+=count;
                scene.pixels.resize(scene.texture_texel_count);scene.material_texel_capacities[n]=count;
            }
            std::copy(image.pixels.begin(),image.pixels.end(),scene.pixels.begin()+offset);
            scene.material_layer_programs[n]=layer_programs;
            if(const auto pointer=std::bit_cast<std::uint32_t>(response.emission[3]))
                response.emission[3]=std::bit_cast<float>(offset+pointer);
            if(response.texture[3]&surface_flag)
                response.base[3]=std::bit_cast<float>(offset+std::bit_cast<std::uint32_t>(response.base[3]));
            response.texture[0]=offset;scene.material_image_keys[n]=key;
            if(normal_record!=no_normal_map)response.detail[0]=offset+normal_record;
            if(!cube_image.pixels.empty()) {
                const auto start=scene.texture_texel_count;
                const auto size=std::uint32_t(cube_image.pixels.size());
                scene.pixels.insert(scene.pixels.end(),cube_image.pixels.begin(),cube_image.pixels.end());
                scene.material_environment_images.push_back({cube_key,start,size,response.texture[3]&environment_mip_flag});
                scene.texture_texel_count+=size;
            }
            if(environment_map)if(const auto cube=find_cube();cube!=scene.material_environment_images.end()) {
                response.detail[1]=cube->offset;
                response.texture[3]|=cube->flags;
            }
            if(glow) {
                const auto color=average_glow_color(materials[n]);
                scene.material_glow[n]=color?ShaderVector{color->x,color->y,color->z,1.f}:ShaderVector{};
            } else if(!std::bit_cast<std::uint32_t>(response.emission[3])) {
                // Sum the drawn colour of the layers that emission_passes
                // could not describe exactly.
                ShaderVector sum{};
                for(const auto* layer:self_illumination_layers(materials[n]))
                    if(const auto color=average_glow_color(*layer)) {
                        sum[0]+=color->x;sum[1]+=color->y;sum[2]+=color->z;sum[3]=1.f;
                    }
                scene.material_glow[n]=sum;
            }
        }
        // Light sources, by what is known about them. A glow layer and a
        // surface's self-illumination emit the colour they are drawn with; a
        // surface whose shader lit the lightmap bake emits the power it gave
        // the bake instead, since that is the light the map was authored with.
        // A pure emissive pass keeps its exact record: it is also its own
        // visible colour (world.frag).
        const auto lightmap=n<scene.material_lightmap_emission.size()?scene.material_lightmap_emission[n]:Vec3{};
        const bool baked=!glow && !(response.texture[3]&32u) && (lightmap.x>0 || lightmap.y>0 || lightmap.z>0);
        if(baked) {
            response.emission={lightmap.x,lightmap.y,lightmap.z,0.f};
            response.texture[3]|=uniform_emission_flag|baked_emission_flag;
        } else if(scene.material_glow[n][3]>0) {
            for(unsigned c=0;c<3;++c)response.emission[c]=glow_gain*scene.material_glow[n][c];
            if(!glow)response.texture[3]|=uniform_emission_flag;
        }
        scene.materials[n]=response;
    }
    refresh_emitters(scene);
}
}
void refresh_materials(Scene &scene,std::span<const Material> materials) {
    scene.live_material_roots.clear();
    refresh_materials_impl(scene,materials,{});
}
void refresh_materials_for_geometry(Scene &scene,std::span<const Material> materials,std::span<const Vertex> vertices) {
    if(vertices.size()%3)throw FormatError("Path Tracing material residency requires complete source triangles");
    auto& active=scene.live_material_roots;
    active.assign(materials.size(),0);
    for(std::size_t first=0;first<vertices.size();first+=3) {
        for(unsigned n=0;n<3;++n)if(vertices[first+n].material>=materials.size())
            throw FormatError("Path Tracing material residency escaped its source slot bank");
        // Vulkan's default first triangle-list vertex supplies the
        // primary shader's flat stipple level. Level0 has no visible samples.
        if(vertices[first].source_stipple_level==0)continue;
        for(unsigned n=0;n<3;++n)active[vertices[first+n].material]=1;
    }
    refresh_materials_impl(scene,materials,active);
}
namespace {
// Converts source triangles to GPU records, appended after scene.triangles.
// Returns false if no triangle was admitted.
void append_triangles(Scene &scene,std::span<const Vertex> vertices) {
    for(std::size_t first=0;first<vertices.size();first+=3) {
        const auto &a=vertices[first],&b=vertices[first+1],&c=vertices[first+2];
        if(a.material>=scene.materials.size() || a.material!=b.material || a.material!=c.material ||
           !finite(a.position) || !finite(b.position) || !finite(c.position) ||
           !finite(a.normal) || !finite(b.normal) || !finite(c.normal) ||
           !finite(a.color) || !finite(b.color) || !finite(c.color) ||
           !std::isfinite(a.uv.x) || !std::isfinite(a.uv.y) || !std::isfinite(b.uv.x) ||
           !std::isfinite(b.uv.y) || !std::isfinite(c.uv.x) || !std::isfinite(c.uv.y))
            throw FormatError("Path Tracing triangle lacks coherent finite source geometry");
        if(scene.materials[a.material].texture[3]&16u)continue;
        if(a.source_stipple_level==0)continue;
        if(!scene.material_texel_capacities[a.material])
            throw FormatError("Path Tracing source material image was not admitted before geometry");
        const auto normal=cross(sub(b.position,a.position),sub(c.position,a.position));
        if(dot(normal,normal)<1.e-18f)continue;
        GpuTriangle triangle;triangle.p0=vec(a.position);triangle.p1=vec(b.position);triangle.p2=vec(c.position);
        triangle.p0[3]=std::bit_cast<float>(std::uint32_t(scene.triangles.size()));
        triangle.n0=vec(a.normal);triangle.n1=vec(b.normal);triangle.n2=vec(c.normal);
        triangle.uv01={a.uv.x,a.uv.y,b.uv.x,b.uv.y};triangle.uv2={c.uv.x,c.uv.y,0,0};
        triangle.color_guide=color_guide_triangle(a,b,c,scene.materials[a.material]);
        const bool unlit_white=(scene.materials[a.material].texture[3]&128u)!=0u;
        const auto color=[&](const Vertex &v) {
            const auto c=unlit_white || v.lightmap!=0 || v.bsp_cluster!=-1?Vec3{1,1,1}:v.color;
            return std::uint32_t(std::round(unit(c.x)*255))|
            (std::uint32_t(std::round(unit(c.y)*255))<<8)|(std::uint32_t(std::round(unit(c.z)*255))<<16)|0xff000000u;};
        triangle.metadata={a.material,color(a),color(b),color(c)};scene.triangles.push_back(triangle);
    }
}
void set_lights(Scene &scene,std::span<const Light> lights) {
    scene.lights.clear();
    for(const auto &light:lights) {
        if(!finite(light.position) || !finite(light.color) || !std::isfinite(light.radius) ||
           !std::isfinite(light.intensity) || light.radius<0 || light.intensity<0 ||
           light.color.x<0 || light.color.y<0 || light.color.z<0)
            throw FormatError("Path Tracing light lacks finite authored source values");
        for(const auto &plane:light.planes)for(const auto value:plane)
            if(!std::isfinite(value))throw FormatError("Path Tracing influence plane is non-finite");
        if(light.intensity<=0)continue;
        auto position=light.position;float radius=light.radius;
        if(light.direction) {
            const auto direction=*light.direction;const auto squared=dot(direction,direction);
            if(!finite(direction) || !std::isfinite(squared) || squared<=1.e-18f)
                throw FormatError("Path Tracing distant light lacks a finite nonzero source direction");
            position=mul(direction,1.f/std::sqrt(squared));radius=-1;
        } else if(radius<=0)continue;
        scene.lights.push_back({{position.x,position.y,position.z,radius},
            {light.color.x,light.color.y,light.color.z,light.intensity},light.planes});
    }
}
}
void update_geometry(Scene &scene,std::span<const Vertex> vertices,std::span<const Light> lights,std::size_t static_vertices) {
    if(vertices.size()%3 || vertices.size()/3>maximum_triangles || lights.size()>65536 ||
       static_vertices%3 || static_vertices>vertices.size())
        throw FormatError("Path Tracing scene exceeds finite geometry/light limits");
    if(scene.material_texel_capacities.size()!=scene.materials.size())
        throw FormatError("Path Tracing geometry lacks its material residency owner");
    std::vector<std::uint32_t> permutation;
    const bool reusable=scene.cpu_hierarchy && reusable_hierarchy(scene,permutation);
    // Keep old nodes out of the live Scene while admitting new geometry. An
    // invalid source triangle must not leave an old tree addressing a partial
    // replacement bank if the caller catches the validation error.
    auto previous_nodes=std::move(scene.nodes);
    scene.triangles.clear();
    scene.pixels.resize(scene.texture_texel_count);
    scene.triangles.reserve(vertices.size()/3);
    append_triangles(scene,vertices.first(static_vertices));
    scene.static_triangles=std::uint32_t(scene.triangles.size());
    append_triangles(scene,vertices.subspan(static_vertices));
    scene.static_reused=false;
    scene.nodes=std::move(previous_nodes);
    bool refitted=false;
    if(!scene.cpu_hierarchy) {
        // Hardware traversal: no CPU BVH, and triangles stay in admission
        // order so ordinal==index for temporal/acceleration bookkeeping.
        scene.nodes.clear();
        refitted=true;
    }
    if(reusable && permutation.size()==scene.triangles.size()) {
        const auto baseline=scene.nodes[0].minimum[3];
        reorder_for_refit(scene,permutation);refit_hierarchy(scene);
        const auto cost=hierarchy_cost(scene);
        // Preserve the original baseline so many small motions cannot hide a
        // progressively degraded tree. Large pose/layout changes rebuild SAH.
        if(std::isfinite(cost) && cost<=double(baseline)*1.25) {
            scene.nodes[0].minimum[3]=baseline;refitted=true;
        }
    }
    if(!refitted) {
        scene.nodes.clear();scene.nodes.reserve(scene.triangles.size());
        if(!scene.triangles.empty()) {
            build_node(scene,0,std::uint32_t(scene.triangles.size()),0);
            // Spare root minimum.w stores the normalized build quality only;
            // bounds/traversal continue to consume the three spatial lanes.
            scene.nodes[0].minimum[3]=float(hierarchy_cost(scene));
        }
    }
    refresh_emitters(scene);
    set_lights(scene,lights);
}
void update_dynamic_geometry(Scene &scene,std::span<const Vertex> vertices,std::span<const Light> lights) {
    if(scene.cpu_hierarchy || scene.static_triangles>scene.triangles.size())
        throw FormatError("Path Tracing static geometry reuse requires hardware traversal and a static prefix");
    if(vertices.size()%3 || (vertices.size()/3+scene.static_triangles)>maximum_triangles || lights.size()>65536)
        throw FormatError("Path Tracing scene exceeds finite geometry/light limits");
    if(scene.material_texel_capacities.size()!=scene.materials.size())
        throw FormatError("Path Tracing geometry lacks its material residency owner");
    scene.triangles.resize(scene.static_triangles);
    scene.pixels.resize(scene.texture_texel_count);
    append_triangles(scene,vertices);
    scene.static_reused=true;
    refresh_emitters(scene);
    set_lights(scene,lights);
}
std::optional<ShaderVector> material_surface(const Scene& scene,std::uint32_t material,Vec2 uv) {
    const auto& target=scene.materials.at(material);
    const auto word=[&](std::size_t offset){return scene.pixels.at(offset);};
    const auto rgba=[](std::uint32_t value) {
        return ShaderVector{float(value&255u)/255.f,float((value>>8)&255u)/255.f,
            float((value>>16)&255u)/255.f,float(value>>24)/255.f};
    };
    if(!(target.texture[3]&surface_flag)) {
        float x=uv.x*target.transform[0]+target.transform[2],y=uv.y*target.transform[1]+target.transform[3];
        x=target.texture[3]&2u?std::clamp(x,0.f,1.f):x-std::floor(x);
        y=target.texture[3]&4u?std::clamp(y,0.f,1.f):y-std::floor(y);
        const auto ix=std::min(target.texture[1]-1,unsigned(x*target.texture[1]));
        const auto iy=std::min(target.texture[2]-1,unsigned(y*target.texture[2]));
        const auto color=rgba(word(target.texture[0]+iy*target.texture[1]+ix));
        if((target.texture[3]&8u) && color[3]==0.f)return {};
        return color;
    }
    const auto pointer=std::bit_cast<std::uint32_t>(target.base[3]);
    if(!pointer || pointer-1>scene.pixels.size() || scene.pixels.size()-(pointer-1)<surface_record_words)
        throw FormatError("Path Tracing source surface lost its bounded record");
    const auto record=pointer-1;
    if(word(record)!=surface_magic || (word(record+7)!=3 && word(record+7)!=7 && word(record+7)!=11 && word(record+7)!=15))
        throw FormatError("Path Tracing source surface lost its native family record");
    const Vec2 base{uv.x*std::bit_cast<float>(word(record+8))+std::bit_cast<float>(word(record+10)),
                    uv.y*std::bit_cast<float>(word(record+9))+std::bit_cast<float>(word(record+11))};
    const auto address=[](int coordinate,unsigned extent,unsigned mode)->int {
        if(mode==3)return std::clamp(coordinate,0,int(extent)-1);
        if(mode==4)return coordinate<0 || coordinate>=int(extent)?-1:coordinate;
        if(mode==5)return std::min(coordinate<0?-coordinate-1:coordinate,int(extent)-1);
        const int period=int(extent)*(mode==2?2:1);int at=coordinate%period;if(at<0)at+=period;
        return mode==2 && at>=int(extent)?period-at-1:at;
    };
    std::array<ShaderVector,4> samples{};
    for(unsigned stage=0;stage<4;++stage) {
        if(!(word(record+7)&(1u<<stage)))continue;
        const auto at=record+12+stage*10,width=word(at+1),height=word(at+2),sampler=word(at+3);
        if(!width || !height)throw FormatError("Path Tracing source surface has an empty stage");
        const float u=base.x*std::bit_cast<float>(word(at+4))+std::bit_cast<float>(word(at+6));
        const float v=base.y*std::bit_cast<float>(word(at+5))+std::bit_cast<float>(word(at+7));
        const auto fetch=[&](int x,int y) {
            x=address(x,width,sampler&15u);y=address(y,height,(sampler>>4)&15u);
            if(x<0 || y<0)return surface_detail::argb(word(at+8));
            // Relative offsets can address the base image before this header.
            const std::uint32_t image=record+word(at);
            return rgba(word(std::size_t(image)+unsigned(y)*std::size_t(width)+unsigned(x)));
        };
        if(((sampler>>8)&255u)==1)samples[stage]=fetch(int(std::floor(u*width)),int(std::floor(v*height)));
        else {
            const float x=u*width-.5f,y=v*height-.5f;const int left=int(std::floor(x)),top=int(std::floor(y));
            const float fx=x-left,fy=y-top;const auto a=fetch(left,top),b=fetch(left+1,top),c=fetch(left,top+1),d=fetch(left+1,top+1);
            for(unsigned channel=0;channel<4;++channel)
                samples[stage][channel]=(a[channel]*(1-fx)+b[channel]*fx)*(1-fy)+(c[channel]*(1-fx)+d[channel]*fx)*fy;
        }
        if((word(at+9)&4u) && samples[stage][3]==0.f)return {};
    }
    const std::array<std::uint32_t,4> colors{word(record+3),word(record+4),word(record+5),word(record+6)};
    return surface_color(SurfaceFamily(word(record+1)),colors,samples);
}
Vec3 material_emission(const Scene& scene,std::uint32_t material,Vec2 uv) {
    const auto& target=scene.materials.at(material);Vec3 result{};
    const auto pointer=std::bit_cast<std::uint32_t>(target.emission[3]);
    const auto word=[&](std::size_t offset){return scene.pixels.at(offset);};
    const auto rgba=[](std::uint32_t value) {
        return ShaderVector{float(value&255u)/255.f,float((value>>8)&255u)/255.f,
            float((value>>16)&255u)/255.f,float(value>>24)/255.f};
    };
    if(!pointer && (target.texture[3]&(light_only_flag|uniform_emission_flag)))
        return {target.emission[0],target.emission[1],target.emission[2]}; // Emitted evenly.
    if(!pointer) {
        auto x=uv.x*target.transform[0]+target.transform[2],y=uv.y*target.transform[1]+target.transform[3];
        x=target.texture[3]&2u?std::clamp(x,0.f,1.f):x-std::floor(x);
        y=target.texture[3]&4u?std::clamp(y,0.f,1.f):y-std::floor(y);
        const auto ix=std::min(target.texture[1]-1,unsigned(x*target.texture[1]));
        const auto iy=std::min(target.texture[2]-1,unsigned(y*target.texture[2]));
        const auto color=rgba(word(target.texture[0]+iy*target.texture[1]+ix));
        return {target.emission[0]*color[0],target.emission[1]*color[1],target.emission[2]*color[2]};
    }
    const auto header=pointer-1;
    if(word(header)!=emission_magic || word(header+1)>64)
        throw FormatError("Path Tracing emissive payload lost its bounded source owner");
    const auto address=[](int coordinate,unsigned extent,unsigned mode)->int {
        if(mode==3)return std::clamp(coordinate,0,int(extent)-1);
        if(mode==4)return coordinate<0 || coordinate>=int(extent)?-1:coordinate;
        if(mode==5)return std::min(coordinate<0?-coordinate-1:coordinate,int(extent)-1);
        const int period=int(extent)*(mode==2?2:1);
        int at=coordinate%period;if(at<0)at+=period;
        return mode==2 && at>=int(extent)?period-at-1:at;
    };
    for(unsigned pass=0;pass<word(header+1);++pass) {
        const auto record=header+2+pass*emission_record_words;
        std::array<ShaderVector,4> samples{};bool killed=false;
        for(unsigned stage=0;stage<4;++stage) {
            if(!((word(record+1)>>(stage*5))&31u))continue;
            const auto at=record+8+stage*10,width=word(at+1),height=word(at+2),sampler=word(at+3);
            const float u=uv.x*std::bit_cast<float>(word(at+4))+std::bit_cast<float>(word(at+6));
            const float v=uv.y*std::bit_cast<float>(word(at+5))+std::bit_cast<float>(word(at+7));
            const auto fetch=[&](int x,int y) {
                x=address(x,width,sampler&15u);y=address(y,height,(sampler>>4)&15u);
                if(x<0 || y<0)return emission_detail::color(word(at+8));
                return rgba(word(record+word(at)+unsigned(y)*width+unsigned(x)));
            };
            if(((sampler>>8)&255u)==1)samples[stage]=fetch(int(std::floor(u*width)),int(std::floor(v*height)));
            else {
                const float x=u*width-.5f,y=v*height-.5f;const int left=int(std::floor(x)),top=int(std::floor(y));
                const float fx=x-left,fy=y-top;const auto a=fetch(left,top),b=fetch(left+1,top),c=fetch(left,top+1),d=fetch(left+1,top+1);
                for(unsigned channel=0;channel<4;++channel)
                    samples[stage][channel]=(a[channel]*(1-fx)+b[channel]*fx)*(1-fy)+(c[channel]*(1-fx)+d[channel]*fx)*fy;
            }
            killed=killed || ((word(at+9)&4u) && samples[stage][3]==0.f);
        }
        if(killed)continue;
        ShaderVector value{};
        const auto color=[&](unsigned at){return emission_detail::color(word(record+at));};
        if(word(record)==unsigned(EmissionFamily::Default)) {
            const auto tint=color(2);for(unsigned c=0;c<3;++c)value[c]=unit(positive(samples[0][c])*tint[c]);
        } else if(word(record)==unsigned(EmissionFamily::Multichannel)) {
            auto texture=samples[0];for(auto& c:texture)c=positive(c);
            const float first=unit(emission_detail::dot_rgb(color(2),texture)),second=unit(emission_detail::dot_rgb(color(3),texture));
            const auto a=color(4),b=color(5),final=color(6);const auto blue=unit(samples[0][2]);
            for(unsigned c=0;c<3;++c)value[c]=unit(unit(first*a[c]+second*b[c])+blue*(final[c]*final[3]));
        } else if(word(record)==unsigned(EmissionFamily::Detail)) {
            const auto tint=color(7);
            for(unsigned c=0;c<3;++c)value[c]=unit(unit(unit(positive(samples[0][c])+positive(samples[1][c]))*positive(samples[2][c]))*tint[c]);
        } else throw FormatError("Path Tracing emissive payload has an unknown native family");
        result.x+=value[0];result.y+=value[1];result.z+=value[2];
    }
    return result;
}
bool intersect(const Scene &scene,Vec3 origin,Vec3 direction,float maximum,Hit &hit) {
    if(!finite(origin) || !finite(direction) || !std::isfinite(maximum) || maximum<=0 || scene.nodes.empty())return false;
    // Same ray-aligned float32 edge evaluation as scene.glsl. Native BSPs
    // contain needle triangles whose long-edge scalar triple products can
    // report a positive self-hit from an origin already above the face.
    const std::array d{direction.x,direction.y,direction.z};
    unsigned kz=std::abs(d[0])>std::abs(d[1])?0u:1u;
    if(std::abs(d[2])>std::abs(d[kz]))kz=2;
    if(d[kz]==0.f)return false;
    unsigned kx=(kz+1)%3,ky=(kx+1)%3;if(d[kz]<0.f)std::swap(kx,ky);
    const float inverse_z=1.f/d[kz],sx=d[kx]*inverse_z,sy=d[ky]*inverse_z;
    const auto project=[&](const ShaderVector& p){
        const std::array v{p[0]-origin.x,p[1]-origin.y,p[2]-origin.z};
        return std::array{v[kx]-sx*v[kz],v[ky]-sy*v[kz],v[kz]*inverse_z};
    };
    std::array<std::uint32_t,64> stack{};unsigned size=1;stack[0]=0;bool found=false;float closest=maximum;
    while(size) {
        const auto &node=scene.nodes[stack[--size]];if(!node_hit(node,origin,direction,closest))continue;
        if(!node.children[2]) {stack[size++]=node.children[0];stack[size++]=node.children[1];continue;}
        for(unsigned n=0;n<node.children[1];++n) {
            const auto index=node.children[0]+n;const auto &triangle=scene.triangles[index];
            const auto a=project(triangle.p0),b=project(triangle.p1),c=project(triangle.p2);
            const float u=c[0]*b[1]-c[1]*b[0],v=a[0]*c[1]-a[1]*c[0],w=b[0]*a[1]-b[1]*a[0];
            if(std::min({u,v,w})<0.f && std::max({u,v,w})>0.f)continue;
            const float determinant=(u+v)+w;if(determinant==0.f)continue;
            const float distance=a[2]+(v*(b[2]-a[2])+w*(c[2]-a[2]))/determinant;
            const Vec3 barycentric{u/determinant,v/determinant,w/determinant};
            if(!(distance>1.e-4f && distance<closest) || !alpha_pass(scene,triangle,barycentric.y,barycentric.z))continue;
            closest=distance;hit={distance,index,barycentric};found=true;
        }
    }
    return found;
}
}
