"""Pin whole public admission passes around their shared association allocator."""
import hashlib
from audit_native_execution_ownership import unique_function
INCLUDE='#include "halo2/native_tu15_visibility_association_value.hpp"\n'
OWNERS={
 'bsp':('source_bsp_admission','258cb0d25de4cdaa41845aa5ea9568eade83cdbafd54954ee051fad6a006575d'),
 'object':('admit','d066d35c832bbcb1e6d95742653100dccb181bba423ba7401ce77b645d25c553'),
}
OLD={
 'bsp':'''                if(out.associations.size()==512){out.association_capacity_reached=true;association=0;}
                else {association=std::uint16_t(out.associations.size());out.associations.push_back({});bit(out.associations.back(),ordinal);}''',
 'object':'''            if(out.associations.size()==512){association=0;out.association_capacity_reached=true;}
            else {
                association=std::uint16_t(out.associations.size());out.associations.push_back({});
                associate(association,ordinal);
            }''',
}
NEW={kind:('                'if kind=='bsp'else'            ')+'association=native_visibility_association_detail::reserve_mask(out.associations,out.association_capacity_reached,ordinal);'for kind in OWNERS}
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def canonical_geometry(body,kind):
    if 'native_geometry_visibility_detail::geometry('in body:
        from native_geometry_visibility_public_replacements import canonical_public as canonical_geometry_policy
        return canonical_geometry_policy(body,kind)
    return body
def canonical_public(body,kind):return canonical_geometry(body.replace(NEW[kind],OLD[kind]),kind)
def replace_public(source,kind):
    owner,pin=OWNERS[kind];body=unique_function(source,owner);original=body.replace(NEW[kind],OLD[kind])
    if digest(canonical_geometry(original,kind))!=pin:raise ValueError('Complete public association owner drift '+kind)
    if original.count(OLD[kind])!=1:raise ValueError('Exact public association allocation block missing '+kind)
    expected=original.replace(OLD[kind],NEW[kind],1)
    if body!=expected:source=source.replace(body,expected,1)
    if source.count(INCLUDE)>1:raise ValueError('Duplicate public association include')
    return source if INCLUDE in source else INCLUDE+source
