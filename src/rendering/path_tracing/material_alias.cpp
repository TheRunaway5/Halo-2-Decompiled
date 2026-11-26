#include "halo2/path_tracing/material_alias.hpp"
#include "material_atlas.hpp"
#include "halo2/path_tracing/chamber_reflection.hpp"
#include "halo2/path_tracing/source_ambient.hpp"
#include "halo2/path_tracing/surface_material.hpp"
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
constexpr auto maximum_texels=material_atlas::maximum_words;
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
using ImageRequests=std::map<material_atlas::ImageKey,std::pair<const Texture*,unsigned>>;
void request_image(ImageRequests& images,const Texture& image,unsigned level) {
    if(!image.width || !image.height || image.pixels.empty() || image.layers!=1 || image.depth ||
       (image.format!=11 && (image.format<14 || image.format>16)))
        throw FormatError("Path Tracing owner selected an unqualified native image");
    images.try_emplace(image_key(image,level),&image,level);
}
auto find_image(const Scene& scene,const material_atlas::ImageKey& key) {
    return std::ranges::find(scene.material_alias_images,key,&MaterialAliasImage::key);
}
std::array<std::uint32_t,3> resident_image(const Scene& scene,const Texture& image,unsigned level) {
    const auto found=find_image(scene,image_key(image,level));
    if(found==scene.material_alias_images.end())throw FormatError("Path Tracing owner image lost its atlas allocation");
    return found->image;
}
void reserve_headers(Scene& scene,std::uint32_t capacity) {
    if(capacity<=scene.material_alias_prefix_words)return;
    const auto delta=capacity-scene.material_alias_prefix_words;
    scene.pixels.insert(scene.pixels.begin(),delta,0);
    scene.texture_texel_count+=delta;scene.material_alias_prefix_words=capacity;
    for(auto& material:scene.materials) {
        material_atlas::relocate(material,material.texture[0]+delta);
        if(material.detail[1])material.detail[1]+=delta;
    }
    for(auto& image:scene.material_alias_images)image.image[0]+=delta;
    for(auto& cube:scene.material_environment_images)cube.offset+=delta;
}
void admit_images(Scene& scene,const ImageRequests& requests,std::size_t slots) {
    if(slots>maximum_texels/material_alias_record_words)
        throw FormatError("Path Tracing owner headers exceed bounded atlas storage");
    // Before borrowing pointers from the atlas, know the exact current image
    // and header cost. Pressure can then move root and shared image blocks
    // without invalidating already-published owner records.
    Scene pending;
    for(const auto& [key,source]:requests)if(find_image(scene,key)==scene.material_alias_images.end()) {
        const auto [image,level]=source;
        pending.material_alias_images.push_back({key,alias_detail::append_image(pending,*image,level)});
    }
    const auto prefix=scene.material_alias_prefix_words;
    const auto needed=std::max(prefix,std::uint32_t(slots*material_alias_record_words));
    auto capacity=prefix?prefix:material_alias_record_words;
    while(capacity<needed)capacity=std::min(maximum_texels,capacity*2);
    if(!needed)capacity=0;
    const auto fits=[&](std::uint32_t words) {
        return scene.texture_texel_count+pending.pixels.size()+words-prefix<=maximum_texels;
    };
    if(!fits(capacity)) {
        std::vector<material_atlas::ImageKey> live;
        live.reserve(requests.size());
        for(const auto& [key,source]:requests)live.push_back(key);
        if(!material_atlas::compact(scene,pending.pixels.size()+needed-prefix,~0u,live))
            throw FormatError("Path Tracing live owner images exceed finite atlas storage");
        if(!fits(capacity))capacity=needed;
    }
    reserve_headers(scene,capacity);
    const auto start=scene.texture_texel_count;
    scene.pixels.insert(scene.pixels.end(),pending.pixels.begin(),pending.pixels.end());
    scene.texture_texel_count+=std::uint32_t(pending.pixels.size());
    for(auto& image:pending.material_alias_images) {
        image.image[0]+=start;
        scene.material_alias_images.push_back(image);
    }
}
void response(GpuMaterial& target,const WorldMaterialUniform* environment,const Material* source) {
    if(!environment)return;
    if(!source)throw FormatError("Path Tracing environment alias lost its source pass identity");
    controls(*environment,*source);
    const auto unit=[](float x) {return std::isfinite(x)?std::clamp(x,0.f,1.f):0.f;};
    auto facing_color=environment->diffuse,glancing_color=environment->specular;
    if(target.texture[3]&chamber_baked_reflection_flag) {
        const auto parameters=chamber_reflection_parameters(environment->program);
        facing_color=parameters.facing;glancing_color=parameters.glancing;
        target.glancing_tint[3]=parameters.baked_weight;
    } else if(source->forward_vertex==6) {
        facing_color=surface_detail::argb(environment->program[15]);
        glancing_color=surface_detail::argb(environment->program[23]);
        facing_color[3]=float(environment->program[17]>>24)/255.f;
        glancing_color[3]=float(environment->program[25]>>24)/255.f;
    }
    const float facing=unit(facing_color[3]),glancing=unit(glancing_color[3]);
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
        scene.material_alias_images.capacity()*sizeof(MaterialAliasImage)+
        scene.material_environment_images.capacity()*sizeof(MaterialEnvironmentImage)+
        scene.live_material_roots.capacity()*sizeof(std::uint8_t);
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
        if(!scene.live_material_roots.empty())scene.live_material_roots.at(input.key.material)=1;
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
    ImageRequests images;
    for(const auto& [key,input]:current) {
        const auto& root=roots[key.material];
        if(std::ranges::any_of(input->surface_images,[](const Texture* image){return image!=nullptr;})) {
            const auto source=qualified_surface(root);
            if(!source)throw FormatError("Path Tracing owner has no qualified source images");
            for(unsigned stage=0;stage<4;++stage)if(source->stages&(1u<<stage))
                if(const auto* image=input->surface_images[stage])
                    request_image(images,*image,root.texture_states[stage].max_mip);
        }
        if(input->ambient && !(scene.materials[key.material].texture[3]&source_flat_ambient_flag)) {
            const auto* owner=source_ambient_owner(root);
            if(!owner)throw FormatError("Path Tracing owner ambient has no qualified independent lighting child");
            const auto& image=input->ambient_image?*input->ambient_image:selected_image(*owner,2);
            request_image(images,image,owner->texture_states[2].max_mip);
        }
    }
    admit_images(scene,images,scene.material_alias_keys.size());
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
        case SurfaceFamily::DynamicDetail:case SurfaceFamily::TextureHonorGuard:
            colors[0]&=0xff000000u;break;
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
        if(input->environment && !(target.texture[3]&(16u|32u)))
            apply_surface_material_response(root,source->family,target.detail,target.response);
        if(source->default_object_ambient)
            target.color_guide[2]=source_default_ambient_rgb(program,source->family);
        if(input->ambient) {
            const auto* owner=source_ambient_owner(root);
            if(!owner || target.color_guide[0])throw FormatError("Path Tracing owner ambient has no qualified independent lighting child");
            controls(*input->ambient,*owner);
            const auto rgb=input->ambient->program[source_ambient_color_word(*owner)]&0xffffffu;
            target.color_guide={3u,0u,rgb,0u};
            // The full-resolution draw already binds the correct normal map.
            // Only slope-dependent ambient needs another copy for ray hits.
            if(!(target.texture[3]&source_flat_ambient_flag)) {
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
                target.color_guide[1]=record+52;
                for(auto word:image_key(image,sampler.max_mip))add(hash,word);
            }
        }
        // Addresses move when the prefix grows; only consumed shader state
        // contributes to temporal material identity.
        for(unsigned i=0;i<header.size();++i) {
            if(i==52 || (i>=12 && i<52 && (i-12)%10==0))continue;
            add(hash,header[i]);
        }
        for(unsigned c=0;c<3;++c){add(hash,std::bit_cast<std::uint32_t>(target.base[c]));add(hash,std::bit_cast<std::uint32_t>(target.glancing_tint[c]));}
        add(hash,std::bit_cast<std::uint32_t>(target.glancing_tint[3]));
        for(float value:target.response)add(hash,std::bit_cast<std::uint32_t>(value));
        add(hash,target.detail[2]);add(hash,target.detail[3]);
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
