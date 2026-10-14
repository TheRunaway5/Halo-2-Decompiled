#include "halo2/source_dynamic_object.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace halo2 {
void validate_source_dynamic_objects(std::span<const SourceDynamicObject> objects) {
    if(objects.size()>2048)throw FormatError("Source dynamic object bank exceeds the original object pool");
    const auto finite=[](Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);};
    const auto identity=[](std::uint32_t id){return id!=0xffffffffu && (id>>16) && (id&65535u)<2048;};
    std::set<std::uint32_t> indices;
    for(const auto &o:objects) {
        const auto &s=o.regions.selection;const auto count=s.permutations.size();
        if(!identity(o.object) || !indices.insert(o.object&65535u).second || o.definition==0xffffffffu ||
            (o.parent!=0xffffffffu && !identity(o.parent)) || (o.type!=3 && o.type!=4 && o.type!=11) ||
            !std::isfinite(o.transform.scale) || !(o.transform.scale>0) || !finite(o.transform.position) ||
            !std::all_of(o.transform.basis.begin(),o.transform.basis.end(),finite) || !finite(o.velocity) ||
            !finite(o.sphere.center) || !std::isfinite(o.sphere.radius) || o.sphere.radius<0 ||
            count>16 || s.states.size()!=count || o.regions.looping_effects.size()!=count ||
            !s.resolved() || s.variant_index< -1 || s.variant_index>127 || o.colors.size()>4 ||
            !std::all_of(o.colors.begin(),o.colors.end(),finite))
            throw FormatError("Invalid completed source dynamic object identity, pose or model bank");
    }
}
}
