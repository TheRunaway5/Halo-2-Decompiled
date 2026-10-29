#include "halo2/path_tracing/scene.hpp"
#include "halo2/path_tracing/material_residency.hpp"
#include "halo2/path_tracing/material_emission.hpp"
#include "halo2/path_tracing/emission.hpp"
#include "halo2/path_tracing/emitter_sampling.hpp"
#include "halo2/path_tracing/surface.hpp"
#include "halo2/path_tracing/material_surface.hpp"
#include "halo2/path_tracing/color_guide.hpp"
#include "halo2/pstencil/environment.hpp"
#include "halo2/pstencil/combined_environment.hpp"
#include "halo2/source_texture_filtering.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <numeric>

namespace halo2::path_tracing {
namespace {
constexpr std::size_t maximum_triangles=2'000'000,maximum_texels=16'777'216;
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
        GpuMaterial temporary;append_texture(image,temporary,emission_texture(material,stage),sampler.max_mip);
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
    if(!traces_material(material))return {};
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
        const unsigned first=surface->family==SurfaceFamily::TerrainBlend?2:0;
        add(colors[first]);add(colors[first+1]);
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
void refresh_emitters(Scene &scene) {
    scene.pixels.resize(scene.texture_texel_count);scene.emissive_count=0;
    for(std::uint32_t n=0;n<scene.triangles.size();++n) {
        const auto &emission=scene.materials[scene.triangles[n].metadata[0]].emission;
        if(std::bit_cast<std::uint32_t>(emission[3]) || emission[0]>0 || emission[1]>0 || emission[2]>0) {scene.pixels.push_back(n);++scene.emissive_count;}
    }
    append_emitter_groups(scene);
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
GpuMaterial material_response(const Material &material) {
    GpuMaterial result;
    const Material *environment=material.environment_pass.get();
    if(environment && environment->forward_vertex!=4)environment=nullptr;
    if(material.forward_vertex==4)environment=&material;
    const auto find_environment=[&](const auto &passes) {
        for(const auto &pass:passes)if(!environment && pass && pass->forward_vertex==4)environment=pass.get();
    };
    find_environment(material.world_overlays_before);find_environment(material.world_overlays_after);
    if(!environment)environment=pstencil::combined_visor_environment(material);
    const auto *reference=pstencil::reference_environment_parameters(material.name);
    // Halo's legacy response has no physical roughness. This optional policy
    // deliberately broadens weak reflections instead of making every surface
    // metallic. Authored facing/glancing strengths remain independently bounded.
    float facing=0,glancing=0;
    if(reference && (environment || pstencil::supports_combined_environment_strength(material))) {
        facing=unit(reference->facing[3]);glancing=unit(reference->glancing[3]);
        for(unsigned n=0;n<3;++n)result.base[n]=unit(reference->facing[n]);
    }else if(environment) {
        facing=unit(environment->forward_diffuse[3]);glancing=unit(environment->forward_specular[3]);
        for(unsigned n=0;n<3;++n)result.base[n]=unit(environment->forward_diffuse[n]);
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
    if(!traces_material(material))flags|=16u;
    if(material.forward_vertex==2)flags|=32u; // Primary root emits; child passes retain their source draw.
    if(qualified_unlit_white_tint(material) || (material.forward_vertex==2 && supports_emission(material)))flags|=128u;
    flags|=(material.forward_raster.alpha_function&7u)<<8;result.texture[3]=flags;
    result.transform=material.texture_transforms[0];
    // These complete native vertex identities use c18 for base UVs even
    // when their pixel family falls back to the ordinary source texture.
    if(const auto uv=qualified_unlit_uv(material))result.transform=uv->base;
    const auto emission=[&](const Material &pass) {
        if(pass.forward_vertex!=2)return;
        for(unsigned n=0;n<3;++n)result.emission[n]+=positive(pass.forward_diffuse[n]);
    };
    emission(material);for(const auto &pass:material.world_overlays_after)if(pass)emission(*pass);
    result.color_guide=color_guide_material(material);
    return result;
}
GpuSceneUniform Scene::uniform(Vec3 environment) const {
    if(!finite(environment) || environment.x<0 || environment.y<0 || environment.z<0)
        throw FormatError("Path Tracing source environment is non-finite or negative");
    return {{std::uint32_t(nodes.size()),std::uint32_t(triangles.size()),std::uint32_t(lights.size()),emissive_count},
        {environment.x,environment.y,environment.z,std::bit_cast<float>(texture_texel_count)}};
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
Scene build_scene(std::span<const Vertex> vertices,std::span<const Material> materials,std::span<const Light> lights) {
    if(vertices.size()%3 || vertices.size()/3>maximum_triangles || materials.size()>65536 || lights.size()>65536)
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
    for(std::size_t n=0;n<materials.size();++n) {
        auto response=material_response(materials[n]);
        const auto authored_flags=response.texture[3];
        if(scene.materials[n].texture[3]&surface_flag) {
            response.texture[3]=(response.texture[3]&~8u)|surface_flag|(scene.materials[n].texture[3]&8u);
            response.base[3]=scene.materials[n].base[3];
        }
        const auto previous=scene.materials[n].texture;
        response.emission[3]=scene.materials[n].emission[3];
        response.texture[0]=previous[0];response.texture[1]=previous[1];response.texture[2]=previous[2];
        const bool resident=scene.material_texel_capacities[n]!=0;
        const bool requested=!active.empty() && active[n] && traces_material(materials[n]);
        if(!resident && !requested) {scene.materials[n]=response;continue;}
        const auto key=material_image_key(materials[n]);
        if(!resident || key!=scene.material_image_keys[n]) {
            Scene image;response.emission[3]=0;response.base[3]=1.f;response.texture[3]=authored_flags;
            append_texture(image,response,traces_material(materials[n])?current_base_texture(materials[n]):Texture{});
            append_emission(image,response,materials[n]);
            append_surface(image,response,materials[n]);
            const auto count=std::uint32_t(image.pixels.size());
            std::uint32_t offset=previous[0];
            if(count>scene.material_texel_capacities[n]) {
                if(count>maximum_texels || scene.texture_texel_count>maximum_texels-count)
                    throw FormatError("Path Tracing animated material exceeds finite atlas storage");
                offset=scene.texture_texel_count;scene.texture_texel_count+=count;
                scene.pixels.resize(scene.texture_texel_count);scene.material_texel_capacities[n]=count;
            }
            std::copy(image.pixels.begin(),image.pixels.end(),scene.pixels.begin()+offset);
            if(const auto pointer=std::bit_cast<std::uint32_t>(response.emission[3]))
                response.emission[3]=std::bit_cast<float>(offset+pointer);
            if(response.texture[3]&surface_flag)
                response.base[3]=std::bit_cast<float>(offset+std::bit_cast<std::uint32_t>(response.base[3]));
            response.texture[0]=offset;scene.material_image_keys[n]=key;
        }
        scene.materials[n]=response;
    }
    refresh_emitters(scene);
}
}
void refresh_materials(Scene &scene,std::span<const Material> materials) {
    refresh_materials_impl(scene,materials,{});
}
void refresh_materials_for_geometry(Scene &scene,std::span<const Material> materials,std::span<const Vertex> vertices) {
    if(vertices.size()%3)throw FormatError("Path Tracing material residency requires complete source triangles");
    std::vector<std::uint8_t> active(materials.size());
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
void update_geometry(Scene &scene,std::span<const Vertex> vertices,std::span<const Light> lights) {
    if(vertices.size()%3 || vertices.size()/3>maximum_triangles || lights.size()>65536)
        throw FormatError("Path Tracing scene exceeds finite geometry/light limits");
    if(scene.material_texel_capacities.size()!=scene.materials.size())
        throw FormatError("Path Tracing geometry lacks its material residency owner");
    std::vector<std::uint32_t> permutation;
    const bool reusable=reusable_hierarchy(scene,permutation);
    // Keep old nodes out of the live Scene while admitting new geometry. An
    // invalid source triangle must not leave an old tree addressing a partial
    // replacement bank if the caller catches the validation error.
    auto previous_nodes=std::move(scene.nodes);
    scene.triangles.clear();scene.lights.clear();
    scene.pixels.resize(scene.texture_texel_count);
    scene.triangles.reserve(vertices.size()/3);
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
    scene.nodes=std::move(previous_nodes);
    bool refitted=false;
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
    if(word(record)!=surface_magic || (word(record+7)!=3 && word(record+7)!=7 && word(record+7)!=15))
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
            return rgba(word(std::size_t(record)+word(at)+unsigned(y)*std::size_t(width)+unsigned(x)));
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
