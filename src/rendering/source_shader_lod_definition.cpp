#include "halo2/source_shader_lod.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
SourceShaderLodDefinition Cache::load_source_shader_lod_definition(std::uint32_t datum) const {
    const auto find_tag=[&](std::uint32_t id,const char *group)->const Tag& {
        const auto it=query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group), .require_address=true});
        if(!it)throw FormatError("source shader LOD tag or template missing");
        return *it;
    };
    const Bytes bytes(cache_bytes());const auto &shader=find_tag(datum,"shad");
    const auto root=metadata_offset(shader.address,0x3e);
    SourceShaderLodDefinition result;result.bias=bytes.u16(root+0x3c);
    if(result.bias>6)throw FormatError("source shader LOD bias outside original table");
    const auto first=[&](std::size_t at,std::size_t stride) {
        const auto count=bytes.u32(at);
        if(!count || count>65536)throw FormatError("source shader LOD postprocess bank missing or invalid");
        return metadata_offset(bytes.u32(at+4),std::size_t(count)*stride);
    };
    const auto postprocess=first(root+0x20,124);
    const auto &stem=find_tag(bytes.u32(postprocess),"stem");
    const auto template_root=metadata_offset(stem.address,0x60);
    const auto template_postprocess=first(template_root+0x58,40);
    const auto count=bytes.u32(template_postprocess);
    if(count>65536)throw FormatError("source shader LOD threshold bank exceeds source count");
    if(count) {
        const auto thresholds=metadata_offset(bytes.u32(template_postprocess+4),std::size_t(count)*10);
        for(unsigned i=0;i<count;++i)result.thresholds.push_back(bytes.f32(thresholds+i*10+6));
    }
    return result;
}
}
