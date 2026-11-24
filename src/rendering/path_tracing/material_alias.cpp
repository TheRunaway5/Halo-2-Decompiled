#include "halo2/path_tracing/material_alias.hpp"
#include "halo2/path_tracing/source_ambient.hpp"
#include "halo2/source_texture_filtering.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <map>
#include <limits>
#include <numeric>

namespace halo2::path_tracing {
namespace alias_detail {
std::array<std::uint32_t,3> append_image(Scene&,const Texture&,unsigned minimum_level);
}
namespace {
constexpr std::uint32_t maximum_texels=16'777'216;
void add(std::uint64_t& hash,std::uint64_t value) {hash^=value;hash*=1099511628211ull;}
void controls(const WorldMaterialUniform& uniform,const Material& material) {
    PixelProgram program;program.words=uniform.program;
    if(!surface_detail::controls(program,material.forward_program.words))
        throw FormatError("Path Tracing owner changed an unqualified native surface control");
}
const Texture& selected_image(const Material& material,unsigned stage) {
    if(!material.animated_textures[stage].empty())
        return material.animated_textures[stage].at(material.texture_frames[stage]);
    const Texture* images[]{&material.diffuse,&material.detail,&material.bump,&material.auxiliary};
    return *images[stage];
}
std::array<std::uint64_t,2> image_key(const Texture& image,unsigned level) {
    std::uint64_t hash=14695981039346656037ull;
    add(hash,image.width);add(hash,image.height);add(hash,image.layers);add(hash,image.depth);
    add(hash,image.format);add(hash,level);
    for(const auto& mip:image.mips) {
        add(hash,mip.width);add(hash,mip.height);add(hash,mip.depth);add(hash,mip.offset);add(hash,mip.size);
    }
    return {image.pixels.fingerprint(),hash};
}
std::array<std::uint32_t,3> resident_image(Scene& scene,const Texture& image,unsigned level) {
    if(!image.width || !image.height || image.pixels.empty() || image.layers!=1 || image.depth ||
       (image.format!=11 && (image.format<14 || image.format>16)))
        throw FormatError("Path Tracing owner selected an unqualified native image");
    const auto key=image_key(image,level);
    for(const auto& item:scene.material_alias_images)if(item.key==key)return item.image;
    const auto result=alias_detail::append_image(scene,image,level);
    scene.material_alias_images.push_back({key,result});return result;
}
void reserve_headers(Scene& scene,std::size_t slots) {
    if(slots>maximum_texels/material_alias_record_words)
        throw FormatError("Path Tracing owner headers exceed bounded atlas storage");
    const auto needed=std::uint32_t(slots*material_alias_record_words);
    if(needed<=scene.material_alias_prefix_words)return;
    std::uint32_t capacity=scene.material_alias_prefix_words?scene.material_alias_prefix_words:material_alias_record_words;
    while(capacity<needed)capacity=std::min(maximum_texels,capacity*2);
    const auto delta=capacity-scene.material_alias_prefix_words;
    if(scene.texture_texel_count>maximum_texels-delta)
        throw FormatError("Path Tracing owner headers exceed bounded atlas storage");
    scene.pixels.insert(scene.pixels.begin(),delta,0);
    scene.texture_texel_count+=delta;scene.material_alias_prefix_words=capacity;
    for(auto& material:scene.materials) {
        material.texture[0]+=delta;
        if(const auto p=std::bit_cast<std::uint32_t>(material.emission[3]))
            material.emission[3]=std::bit_cast<float>(p+delta);
        if(material.texture[3]&surface_flag)
            material.base[3]=std::bit_cast<float>(std::bit_cast<std::uint32_t>(material.base[3])+delta);
        for(unsigned record=0;record<2;++record)if(material.detail[record])material.detail[record]+=delta;
    }
    for(auto& image:scene.material_alias_images)image.image[0]+=delta;
}
void response(GpuMaterial& target,const WorldMaterialUniform* environment,const Material* source) {
    if(!environment)return;
    if(!source)throw FormatError("Path Tracing environment alias lost its source pass identity");
    controls(*environment,*source);
    const auto unit=[](float x) {return std::isfinite(x)?std::clamp(x,0.f,1.f):0.f;};
    const auto facing_color=source->forward_vertex==6?surface_detail::argb(environment->program[15]):environment->diffuse;
    const auto glancing_color=source->forward_vertex==6?surface_detail::argb(environment->program[23]):environment->specular;
    const float facing=source->forward_vertex==6?float(environment->program[17]>>24)/255.f:unit(environment->diffuse[3]);
    const float glancing=source->forward_vertex==6?float(environment->program[25]>>24)/255.f:unit(environment->specular[3]);
    target.response[0]=std::clamp(.85f-.75f*std::sqrt(std::max(facing,glancing)),.12f,.85f);
    target.response[1]=facing;target.response[2]=glancing;
    for(unsigned c=0;c<3;++c) {
        target.base[c]=unit(facing_color[c]);target.glancing_tint[c]=unit(glancing_color[c]);
    }
}
}
std::uint32_t source_material(const Scene& scene,std::uint32_t material) {
    if(material>=scene.materials.size())throw FormatError("Path Tracing material identity escaped its scene");
    return scene.material_source_roots.empty()?material:scene.material_source_roots.at(material);
}
std::size_t material_alias_cpu_bytes(const Scene& scene) {
    return scene.material_alias_keys.capacity()*sizeof(MaterialAliasKey)+
        scene.material_source_roots.capacity()*sizeof(std::uint32_t)+
        scene.material_alias_images.capacity()*sizeof(MaterialAliasImage);
}
void clear_material_aliases(Scene& scene,std::size_t root_count) {
    if(root_count>scene.materials.size())throw FormatError("Path Tracing alias root bank changed ownership");
    // Previous-frame triangles can be consulted by ordinary emitter refresh.
    // Restore their root identities before removing the frame's alias rows.
    for(auto& triangle:scene.triangles)triangle.metadata[0]=source_material(scene,triangle.metadata[0]);
    scene.materials.resize(root_count);scene.material_image_keys.resize(root_count);
    scene.material_texel_capacities.resize(root_count);scene.material_source_roots.resize(root_count);
    std::iota(scene.material_source_roots.begin(),scene.material_source_roots.end(),0u);
    scene.pixels.resize(scene.texture_texel_count);
}
std::vector<MaterialAliasBinding> refresh_material_aliases(Scene& scene,std::span<const Material> roots,
    std::span<const MaterialAliasInput> inputs) {
    if(scene.materials.size()!=roots.size())
        throw FormatError("Path Tracing aliases require a refreshed ordinary root bank");
    std::map<MaterialAliasKey,const MaterialAliasInput*> current;
    for(const auto& input:inputs) {
        if(input.key.material>=roots.size() || !input.primary || !current.emplace(input.key,&input).second)
            throw FormatError("Path Tracing alias lost its unique completed owner/root binding");
    }
    std::map<MaterialAliasKey,std::size_t> slots;
    for(std::size_t i=0;i<scene.material_alias_keys.size();++i) {
        auto& key=scene.material_alias_keys[i];
        if(current.contains(key))slots.emplace(key,i);else key={};
    }
    for(const auto& [key,input]:current)if(!slots.contains(key)) {
        const auto vacant=std::find_if(scene.material_alias_keys.begin(),scene.material_alias_keys.end(),
            [](const auto& value){return value.material==~0u;});
        const auto slot=std::size_t(vacant-scene.material_alias_keys.begin());
        if(vacant==scene.material_alias_keys.end())scene.material_alias_keys.push_back(key);else *vacant=key;
        slots.emplace(key,slot);
    }
    while(!scene.material_alias_keys.empty() && scene.material_alias_keys.back().material==~0u)
        scene.material_alias_keys.pop_back();
    scene.pixels.resize(scene.texture_texel_count);
    reserve_headers(scene,scene.material_alias_keys.size());
    const auto count=roots.size()+scene.material_alias_keys.size();
    if(count>std::numeric_limits<std::uint32_t>::max()/sizeof(GpuMaterial))
        throw FormatError("Path Tracing alias material bank exceeds GPU storage domain");
    scene.materials.resize(count);scene.material_image_keys.resize(count);scene.material_texel_capacities.resize(count);
    scene.material_source_roots.resize(count,~0u);
    std::iota(scene.material_source_roots.begin(),scene.material_source_roots.begin()+roots.size(),0u);
    for(std::size_t i=roots.size();i<count;++i) {
        scene.materials[i].texture[3]=16u;scene.material_texel_capacities[i]=0;
    }
    std::vector<MaterialAliasBinding> bindings;bindings.reserve(inputs.size());
    for(const auto& [key,input]:current) {
        const auto& root=roots[key.material];const auto source=qualified_surface(root);
        if(!source || !(scene.materials[key.material].texture[3]&surface_flag))
            throw FormatError("Path Tracing owner alias requires a resident qualified surface");
        controls(*input->primary,root);
        const auto& program=input->primary->program;
        if((source->family==SurfaceFamily::BumpLeafCoverage && ((program[13]>>24)==255 || (program[22]>>24)!=255)) ||
           (source->family==SurfaceFamily::BumpLeafCoveragePerVertex &&
            ((program[14]>>24)!=255 || (program[15]>>24)==255 || (program[17]>>24)!=255)))
            throw FormatError("Path Tracing owner changed a qualified native alpha-coverage policy");
        if((source->family==SurfaceFamily::TextureTwoChangeColor || source->family==SurfaceFamily::TextureFourChangeColor) &&
           ((input->primary->program[10]&0xffffffu)!=0xff0000u || (input->primary->program[18]&0xffffffu)!=0x00ff00u))
            throw FormatError("Path Tracing owner changed a qualified change-color selector");
        const auto uv=completed_unlit_uv(source->uv.family,input->primary->forward_transforms);
        if(!uv)throw FormatError("Path Tracing owner has nonfinite completed surface UVs");
        const auto slot=slots.at(key);const auto index=std::uint32_t(roots.size()+slot);
        const auto record=std::uint32_t(slot*material_alias_record_words);
        auto target=scene.materials[key.material];
        const auto original=std::bit_cast<std::uint32_t>(target.base[3])-1;
        if(original<scene.material_alias_prefix_words || original+surface_record_words>scene.texture_texel_count)
            throw FormatError("Path Tracing alias source header escaped shared texels");
        std::array<std::uint32_t,material_alias_record_words> header{};
        std::copy_n(scene.pixels.begin()+original,surface_record_words,header.begin());
        auto colors=surface_constants(input->primary->program,source->family);
        switch(source->family) {
        case SurfaceFamily::TerrainBlend:case SurfaceFamily::TerrainBlendThreeStage:
            colors[0]=colors[1]=0;colors[2]&=0xffffffu;colors[3]&=0xffffffu;break;
        case SurfaceFamily::LeafDetail:case SurfaceFamily::LeafDetailPerVertex:
        case SurfaceFamily::BumpLeafCoverage:case SurfaceFamily::BumpLeafCoveragePerVertex:
            colors[0]&=0xff000000u;colors[1]&=0xff000000u;colors[2]=colors[3]=0;break;
        case SurfaceFamily::DynamicDetail:colors[0]&=0xff000000u;break;
        default:for(auto& color:colors)color&=0xffffffu;break;
        }
        std::copy(colors.begin(),colors.end(),header.begin()+3);
        for(unsigned c=0;c<4;++c)header[8+c]=std::bit_cast<std::uint32_t>(uv->base[c]);
        std::uint64_t hash=14695981039346656037ull;
        for(unsigned stage=0;stage<4;++stage)if(source->stages&(1u<<stage)) {
            const auto at=12+stage*10;
            auto pixels=std::array<std::uint32_t,3>{original+header[at],header[at+1],header[at+2]};
            if(input->surface_images[stage])pixels=resident_image(scene,*input->surface_images[stage],root.texture_states[stage].max_mip);
            header[at]=pixels[0]-record;header[at+1]=pixels[1];header[at+2]=pixels[2];
            for(unsigned c=0;c<4;++c)header[at+4+c]=std::bit_cast<std::uint32_t>(uv->stages[stage][c]);
            const auto image=input->surface_images[stage]?input->surface_images[stage]:&selected_image(root,stage);
            for(auto word:image_key(*image,root.texture_states[stage].max_mip))add(hash,word);
        }
        target.base[3]=std::bit_cast<float>(record+1);target.transform=uv->base;
        response(target,input->environment,input->environment_source);
        if(input->ambient) {
            const auto* owner=source_ambient_owner(root);
            if(!owner || target.color_guide[0])throw FormatError("Path Tracing owner ambient has no qualified independent lighting child");
            controls(*input->ambient,*owner);
            const auto& sampler=owner->texture_states[2];
            const auto& image=input->ambient_image?*input->ambient_image:selected_image(*owner,2);
            const auto pixels=resident_image(scene,image,sampler.max_mip);
            std::copy(pixels.begin(),pixels.end(),header.begin()+52);
            header[55]=sampler.address_u|(sampler.address_v<<4)|(source_texture_filtering(sampler).mag_filter<<8);
            header[56]=sampler.border_color;
            for(unsigned c=0;c<4;++c) {
                const float base=input->ambient->forward_transforms[0][c],bump=input->ambient->forward_transforms[2][c];
                if(!std::isfinite(base) || !std::isfinite(bump))throw FormatError("Path Tracing owner ambient has nonfinite completed UVs");
                header[57+c]=std::bit_cast<std::uint32_t>(base);header[61+c]=std::bit_cast<std::uint32_t>(bump);
            }
            target.color_guide={3u,record+52,input->ambient->program[15]&0xffffffu,0u};
            for(auto word:image_key(image,sampler.max_mip))add(hash,word);
        }
        // Addresses move when the prefix grows; only consumed shader state
        // contributes to temporal material identity.
        for(unsigned i=0;i<header.size();++i) {
            if(i==52 || (i>=12 && i<52 && (i-12)%10==0))continue;
            add(hash,header[i]);
        }
        for(unsigned c=0;c<3;++c){add(hash,std::bit_cast<std::uint32_t>(target.base[c]));add(hash,std::bit_cast<std::uint32_t>(target.glancing_tint[c]));}
        for(float value:target.response)add(hash,std::bit_cast<std::uint32_t>(value));
        add(hash,target.texture[3]);
        add(hash,target.color_guide[0]);add(hash,target.color_guide[2]);
        if(target.color_guide[0]!=3u){add(hash,target.color_guide[1]);add(hash,target.color_guide[3]);}
        std::copy(header.begin(),header.end(),scene.pixels.begin()+record);
        scene.materials[index]=target;scene.material_image_keys[index]={hash,hash^0x616c696173ull};
        scene.material_texel_capacities[index]=material_alias_record_words;scene.material_source_roots[index]=key.material;
        bindings.push_back({key,index});
    }
    return bindings;
}
}
