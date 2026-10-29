#include "halo2/particle_material_validation.hpp"
#include "halo2/native_fragment_kernel.hpp"
#include "halo2/source_texture_filtering.hpp"
#include "halo2/texture.hpp"
#include "halo2/vertex_program.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <string>

namespace halo2 {
namespace {
void require(bool value,const char* message) {
    if(!value)throw ParticleResourceError(message);
}
bool blend_factor(unsigned value) {
    return value<=1 || (value>=0x300 && value<=0x308);
}
void texture(const Texture& image,const ShaderTextureState& sampler) {
    require(image.layers==1 && !image.depth && image.width && image.height && !image.mips.empty(),
        "Particle upload requires decoded two-dimensional mip ownership");
    require(image.format==11 || image.format==native_rgba32f,
        "Particle upload requires decoded RGBA8 or RGBA32F texels");
    require(!(sampler.color_sign&0x0fffffffu) && (!sampler.color_sign || image.format==11),
        "Particle signed texture mask has no decoded texel owner");
    require(!sampler.color_sign || image.pixels.size()%4==0,
        "Particle signed texture storage is not aligned to decoded RGBA texels");
    require(!sampler.color_key_op,"Particle color-key operation is unsupported");
    const auto filtering=source_texture_filtering(sampler);
    require(filtering.anisotropy==1,"Particle anisotropy requires an enabled device sampling owner");
    require(sampler.address_u>=1 && sampler.address_u<=5 && sampler.address_v>=1 && sampler.address_v<=5 &&
        sampler.address_w>=1 && sampler.address_w<=5,"Particle texture address mode is unsupported");
    require(sampler.mip_filter<=2 && std::isfinite(std::bit_cast<float>(sampler.lod_bias)),
        "Particle mip filter or bias is unsupported");
    require(!(sampler.alpha_kill&~4u),"Particle alpha-kill mask is unsupported");
    require(sampler.border_color==0 || sampler.border_color==0xff000000u || sampler.border_color==0xffffffffu,
        "Particle custom sampler border requires an enabled device extension");
    const std::size_t texel=image.format==11?4u:16u;
    unsigned width=image.width,height=image.height;std::size_t previous_end{};
    unsigned maximum_levels=1;
    for(auto extent=std::max(width,height);extent>1;extent/=2)++maximum_levels;
    require(image.mips.size()<=maximum_levels,"Particle mip count exceeds its image extent");
    for(const auto& mip:image.mips) {
        require(mip.width==width && mip.height==height && mip.depth==1,
            "Particle mip dimensions do not follow their decoded image");
        require(std::size_t(width)<=std::numeric_limits<std::size_t>::max()/height &&
            std::size_t(width)*height<=std::numeric_limits<std::size_t>::max()/texel,
            "Particle mip byte extent overflows its decoded storage");
        const auto bytes=std::size_t(width)*height*texel;
        require(mip.size==bytes && mip.offset>=previous_end && mip.offset%texel==0 &&
            mip.offset<=image.pixels.size() && bytes<=image.pixels.size()-mip.offset,
            "Particle mip range escapes its decoded texels");
        previous_end=mip.offset+bytes;width=std::max(1u,width/2);height=std::max(1u,height/2);
    }
}
}
ParticleMaterialValidation validate_particle_material(const ParticleMaterial& source)try {
    ParticleMaterialValidation out;out.classification=source.vertex.classification;
    require(out.classification==1 || (out.classification>=7 && out.classification<=9),
        "Particle vertex classification is unsupported");
    out.program_hash=14695981039346656037ull;
    for(auto byte:source.vertex.program)out.program_hash=(out.program_hash^std::to_integer<unsigned char>(byte))*1099511628211ull;
    const auto expected=out.classification==1?0x1f670b778798858eull:out.classification==7?
        0x2b67638b8d57aa48ull:out.classification==8?0x93c69296e450b61cull:0x3e8f2d9052454432ull;
    out.distortion=out.classification==7 && out.program_hash==0x9b39913b8d5b07a3ull;
    require(out.program_hash==expected || out.distortion,"Particle vertex program has no compiled Vulkan kernel");
    out.instructions=decode_vertex_program(source.vertex.program).count;
    out.fragment_kernel=native_fragment_kernel_path("particle",source.pass.program.words);
    require(source.pass.pixel_externs.empty() && source.pass.vertex_externs.empty() && source.pass.state_externs.empty(),
        "Particle material requires an unavailable external parameter or render-state producer");
    for(const auto& write:source.sampler_writes)
        require(write.stage<4 && write.state>=22 && write.state<=25 &&
            std::isfinite(std::bit_cast<float>(write.value)),
            "Particle bump sampler write lacks its finite authored state owner");
    const auto modes=source.pass.program.words[54];
    require(!source.textures[2] && !source.textures[3] && (modes>>10u)==0u &&
        (modes&31u)<=1u && (((modes>>5u)&31u)<=1u || ((modes>>5u)&31u)==6u),
        "Particle texture modes require an unsupported sampling path");
    bool external{};std::array<bool,2> bound_stages{};
    for(const auto& binding:source.pass.textures) {
        require(binding.stage<2 && !binding.flags,"Particle texture binding has an unsupported stage or flags");
        require(!bound_stages[binding.stage],"Particle texture stage has more than one binding");
        bound_stages[binding.stage]=true;
        if(binding.external) {
            require(binding.stage==1 && binding.external==23 && binding.parameter==255 && !external,
                "Particle external texture binding is unsupported");
            external=true;
        }
    }
    out.external_scene_texture=external;
    require(!external || (!source.textures[1] && modes==0xc1u),
        "Particle framebuffer lookup lacks its dependent-texture contract");
    require(((modes>>5u)&31u)!=6u || source.textures[1] || external,
        "Particle bump lookup lacks its second texture or framebuffer owner");
    require(((modes>>5u)&31u)!=6u || (modes&31u)==1u,
        "Particle bump lookup lacks an active preceding texture stage");
    require(source.textures[0].has_value() && !source.pass.texture_states.empty(),
        "Particle primary texture or sampler is missing");
    for(unsigned stage=0;stage<2;++stage) {
        if(stage==1 && external) {
            require(source.pass.texture_states.size()>1,"Particle framebuffer sampler is missing");
            const auto& state=source.pass.texture_states[1];
            require(!state.alpha_kill && !state.color_key_op && !state.color_sign,
                "Particle framebuffer sampler requests an unsupported texture operation");
            // Validate sampler syntax without fabricating current-surface pixels.
            Texture placeholder{1,1,11,{std::byte{},std::byte{},std::byte{},std::byte{}},{{1,1,0,4}}};
            texture(placeholder,state);continue;
        }
        const unsigned bound=source.textures[stage]?stage:0;
        require(bound<source.pass.texture_states.size(),"Particle sampler is missing");
        texture(*source.textures[bound],source.pass.texture_states[bound]);
    }
    unsigned equation=0x8006;
    for(const auto& state:source.pass.states)switch(state.index) {
    case 57:out.raster.depth_function=state.value;break;
    case 58:out.raster.alpha_function=state.value;break;
    case 59:out.blend=state.value!=0;break;
    case 60:out.raster.alpha_test=state.value;break;
    case 61:out.raster.alpha_reference=state.value;break;
    case 62:out.source_blend=state.value;break;
    case 63:out.destination_blend=state.value;break;
    case 64:out.raster.depth_write=state.value;break;
    case 67:out.raster.color_write=state.value;break;
    case 74:equation=state.value;break;
    case 143:out.raster.depth_enable=state.value;break;
    case 147:out.raster.cull=state.value;break;
    case 158:break; // Filled particle triangles do not consume SDK line width.
    default:throw ParticleResourceError("Particle raster state is unsupported: "+std::to_string(state.index));
    }
    require(equation==0x8006 && out.raster.depth_enable==2 && out.raster.depth_function>=0x200 &&
        out.raster.depth_function<=0x207 && out.raster.alpha_function>=0x200 && out.raster.alpha_function<=0x207,
        "Particle blend or depth operation is unsupported");
    require(out.raster.cull==0 || out.raster.cull==0x900 || out.raster.cull==0x901,
        "Particle cull state is unsupported");
    require(blend_factor(out.source_blend) && blend_factor(out.destination_blend),
        "Particle blend factor is unsupported");
    return out;
}catch(const ParticleResourceError&){throw;}
catch(const FormatError& error){throw ParticleResourceError(error.what());}
}
