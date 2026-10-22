#include "halo2/source_control.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <cmath>

namespace halo2 {
bool source_device_group_set(SourceDeviceGroup &group,float requested) {
    if(!std::isfinite(requested) || !std::isfinite(group.value))
        throw FormatError("nonfinite source device group value");
    if(requested<0.f)requested=0.f;
    else if(requested>1.f)requested=1.f;
    if(group.value==requested || (group.flags&3u)==3u)return false;
    group.flags|=2u;
    group.value=requested;
    return true;
}
SourceControlActivation source_control_activate(const SourceControlDefinition &control,
                                                SourceDeviceGroup *position) {
    SourceControlActivation result;
    if(!position)return result;
    if(control.type>3 || !std::isfinite(control.call_value) ||
       !std::isfinite(position->value))
        throw FormatError("unsupported source control activation input");
    result.attempted=true;
    switch(control.type) {
    case 0:result.requested=position->value>.5f?0.f:1.f;break;
    case 1:result.requested=1.f;break;
    case 2:result.requested=0.f;break;
    case 3:result.requested=control.call_value;break;
    }
    result.changed=source_device_group_set(*position,result.requested);
    // 11BAEB compares the original requested value, before1071E0's clamp.
    result.effect=result.changed?(result.requested>.5f?control.on:control.off):control.deny;
    return result;
}
SourceControlDefinition Cache::load_source_control_definition(std::uint32_t datum)const {
    const auto metadata=load_source_tag_metadata(datum);
    if(metadata.group()!="ctrl" || metadata.size()<0x140)
        throw FormatError("source control definition missing or truncated");
    const Bytes data(metadata.bytes_at(metadata.source_address(),metadata.size()));
    if(data.u16(0)!=8)throw FormatError("source control object type mismatch");
    SourceControlDefinition result;
    result.datum=datum;result.model=data.u32(0x38);
    result.type=data.u16(0x11c);result.triggers_when=data.u16(0x11e);
    result.call_value=data.f32(0x120);result.action_string=data.u32(0x124);
    result.on=data.u32(0x12c);result.off=data.u32(0x134);result.deny=data.u32(0x13c);
    return result;
}
}
