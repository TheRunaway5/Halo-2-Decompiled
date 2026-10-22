#include "halo2/cache.hpp"
#include <algorithm>
namespace halo2 {
PixelProgram Cache::load_menu_scene_filter() const {
    const auto effect=query_tag({.group=tag_groups::screen_effect, .name="effects\\screen_effects\\desaturate"});
    if(!effect)throw FormatError("missing original title screen effect");
    const Bytes data(cache_bytes());
    const auto at=metadata_offset(effect->address,0x48);
    if(data.u32(at+0x40)!=0x73686164)throw FormatError("title screen effect does not reference a shader");
    const auto material=load_material(data.u32(at+0x44));
    if(material.passes.size()!=1 || material.passes.front().implementation!=0)
        throw FormatError("unsupported original title screen effect implementation");
    const auto &pass=material.passes.front();
    if(pass.textures.size()!=1 || pass.textures.front().stage!=0 ||
       pass.textures.front().parameter!=255 || pass.textures.front().external!=5)
        throw FormatError("title screen effect must sample the completed framebuffer");
    return pass.program;
}
}
