#include "halo2/source_object_primitive.hpp"

namespace halo2 {
ObjectLightSpatialPrimitive source_object_registration_primitive(
    const SourceObjectPrimitiveInput& in) {
    std::uint16_t flags=0;
    if(in.object_flags&(1u<<9)) {
        if((in.header_flags&0x10) || (in.object_flags&1))flags|=1;
        if(in.object_flags&(1u<<20))flags|=2;
        if(!(in.object_flags&(1u<<22)))flags|=4;
        if(!(in.object_flags&(1u<<31)))flags|=8;
        if(in.type==0) {
            if(in.unit_flags_10a&4)flags|=0x10;
            if(in.unit_flags_348&8)flags|=0x20;
        } else if(in.type==11 && (in.prop_flags_12c&2))flags|=0x40;
    }
    return {in.type,flags,in.primitive_sphere.center,in.primitive_sphere.radius};
}
}
