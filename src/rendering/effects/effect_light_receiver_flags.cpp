#include "halo2/effect_light_receiver.hpp"
#include "halo2/source_object_admission.hpp"
#include <bit>

namespace halo2 {
namespace {
void write(SourceBspAdmissionRecord& row,std::uint16_t flags,SourceEffectLightFlagChanges31650& out) {
    ++out.rows;out.changed+=row.flags!=flags;row.flags=flags;
}
}
SourceEffectLightFlagChanges31650 source_effect_light_bsp_flags_31650(SourceBspAdmission& bsp,std::uint32_t flags) {
    SourceEffectLightFlagChanges31650 out;
    if(flags&0x10000u)for(auto* rows:{&bsp.clusters,&bsp.instances})for(auto& row:*rows)
        write(row,std::uint16_t((row.flags&0xdfffu)|0x1000u),out);
    return out;
}
SourceEffectLightFlagChanges31650 source_effect_light_object_flags_31650(SourceObjectAdmission& objects,
    const SourceEffectLightFlagPolicy31650& policy) {
    SourceEffectLightFlagChanges31650 out;const auto flags=policy.definition_flags;
    if(flags&0x20000u)for(auto& row:objects.objects)write(row,std::uint16_t((row.flags&0xdfffu)|0x1000u),out);
    if(!(flags&0x3800u))return out;
    const auto bits=std::uint16_t((!(flags&0x800u)||!(flags&0x2000u)?0x1000u:0u)|
        (!(flags&0x1000u)||!(flags&0x2000u)?0x2000u:0u));
    const auto edit=[&](auto owner) {
        out.owner_none_passes+=owner==0xffffffffu;
        for(auto& row:objects.objects)if(owner==0xffffffffu || row.id==owner)
            write(row,std::uint16_t((row.flags&~0x3000u)|bits),out);
    };
    edit(policy.owner);
    if(policy.root!=policy.owner){++out.root_passes;edit(policy.root);}
    return out;
}
bool source_effect_light_full_f8_31650(std::uint32_t flags,std::uint16_t cluster_count) {
    return (flags&4u)&&std::bit_cast<std::int16_t>(cluster_count)>=2;
}
}
