#include "halo2/model_scale_reset.hpp"
#include <algorithm>
namespace halo2 {
std::vector<ModelScaleReset> Cache::load_model_scale_resets(const RenderModel &model)const {
    if(vista_)throw FormatError("Cinematic scale resets require Xbox animation graph conversion");
    std::vector<ModelScaleReset> result;
    if(model.bones.empty())return result;
    const Bytes data(cache_bytes());
    for(const auto &tag:select_tags({.group=tag_groups::model_animation_graph,.require_address=true})) {
        const auto root=metadata_offset(tag.address,0xb4);
        const auto nodes=TagAddressSpace(data,metadata_magic_).block(root+0xc,32,255);
        if(nodes.count!=model.bones.size())continue;
        bool same=true;
        for(unsigned n=0;n<nodes.count;++n) {
            const auto at=nodes.offset+n*32;
            if(data.u32(at)!=model.bones[n].name || std::int16_t(data.u16(at+8))!=model.bones[n].parent){same=false;break;}
        }
        if(!same)continue;
        const auto clips=TagAddressSpace(data,metadata_magic_).block(root+0x2c,0x6c);
        for(unsigned clip=0;clip<clips.count;++clip) {
            const auto resets=model_terminal_scale_resets(model,load_animation(tag.datum,clip));
            for(const auto &reset:resets)if(std::ranges::find(result,reset)==result.end())result.push_back(reset);
        }
    }
    return result;
}
}
