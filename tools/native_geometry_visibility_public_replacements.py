"""Exact public geometry-policy overlay, composed with the existing allocator."""
import hashlib
from audit_native_execution_ownership import unique_function
INCLUDE='#include "halo2/native_tu15_geometry_visibility_value.hpp"\n'
OWNERS={'bsp':'source_bsp_admission','object':'admit'}
OLD={'bsp': '            SourceClusterFrustumResult result;\n            if(in.mode==1){\n                //132E60 keeps the overlapping-sphere predicate in SSE, then\n                // uses x87 PC53 for the contained-sphere predicate.\n                const float z=i.center.z-in.sphere_center.z,x=i.center.x-in.sphere_center.x,\n                    y=i.center.y-in.sphere_center.y,r=i.radius+in.sphere_radius;\n                result.visible=!(r*r<(z*z+x*x)+y*y);\n                if(result.visible){\n                    const double cx=double(in.sphere_center.x)-i.center.x,\n                        cy=double(in.sphere_center.y)-i.center.y,cz=double(in.sphere_center.z)-i.center.z;\n                    result.contained=!(double(in.sphere_radius)<std::sqrt((cz*cz+cy*cy)+cx*cx)+i.radius);\n                }\n            }else result=source_cluster_frustum_visibility(spheres,ordinal,i.center,i.radius);\n', 'object': '        SourceClusterFrustumResult result;\n        if(in.mode==1) {\n            // TU1.5 135970: SSE overlap, then x87 PC53 containment.\n            const float z=sphere.center.z-in.sphere_center.z,\n                x=sphere.center.x-in.sphere_center.x,y=sphere.center.y-in.sphere_center.y,\n                r=sphere.radius+in.sphere_radius;\n            result.visible=!(r*r<(z*z+x*x)+y*y);\n            if(result.visible) {\n                const double dx=double(in.sphere_center.x)-sphere.center.x,\n                    dy=double(in.sphere_center.y)-sphere.center.y,\n                    dz=double(in.sphere_center.z)-sphere.center.z;\n                result.contained=!(double(in.sphere_radius)<std::sqrt((dz*dz+dy*dy)+dx*dx)+sphere.radius);\n            }\n        } else result=skip?SourceClusterFrustumResult{true,false}:\n            source_cluster_frustum_visibility(spheres,ordinal,sphere.center,sphere.radius);\n'}
PROMOTE={'bsp': '            if(in.mode==0 && result.visible && !result.contained && in.context_2a78==0 && i.radius<=2.5f)result.contained=true;\n', 'object': '        if(in.mode==0 && in.context_2a78==0 && result.visible && !result.contained && sphere.radius<=2.5f)result.contained=true;\n'}
NEW={'bsp': '            const auto result=native_geometry_visibility_detail::geometry(spheres,ordinal,i.center,i.radius,in.mode,false,in.context_2a78!=0,in.sphere_center,in.sphere_radius);\n', 'object': '        const auto result=native_geometry_visibility_detail::geometry(spheres,ordinal,sphere.center,sphere.radius,in.mode,skip,in.context_2a78!=0,in.sphere_center,in.sphere_radius);\n'}
def canonical_public(body,kind):
    if NEW[kind] in body:
        if body.count(NEW[kind])!=1:raise ValueError('Duplicate public geometry policy')
        body=body.replace(NEW[kind],OLD[kind],1)
        # The original small-sphere policy belongs after the explicit
        # visible/force gate, before its queue allocation/flag producers.
        needle='            std::uint16_t association=0xffff;'if kind=='bsp'else'        bool negative=true,positive=true;'
        if body.count(needle)!=1:raise ValueError('Missing original geometry policy continuation')
        if kind=='object':
            comment='        //132E60 only reaches the small-sphere shortcut after successful\n        // frustum admission (or the explicit sky skip), before plane flags.\n'
            if body.count(comment)!=1:raise ValueError('Missing exact public geometry policy comment')
            body=body.replace(comment,comment+PROMOTE[kind],1)
        else:body=body.replace(needle,PROMOTE[kind]+needle,1)
    return body

def replace_public(source,kind):
    import native_visibility_association_public_replacements as allocation
    owner=OWNERS[kind];body=unique_function(source,owner);original=canonical_public(body,kind)
    baseline=allocation.canonical_public(original,kind)
    if hashlib.sha256(baseline.encode()).hexdigest()!=allocation.OWNERS[kind][1]:raise ValueError('Complete public geometry admission owner drift '+kind)
    if original.count(OLD[kind])!=1 or original.count(PROMOTE[kind])!=1:raise ValueError('Missing exact original geometry policy '+kind)
    expected=original.replace(OLD[kind],NEW[kind],1).replace(PROMOTE[kind],'',1)
    if body!=expected:source=source.replace(body,expected,1)
    if source.count(INCLUDE)>1:raise ValueError('Duplicate public geometry include')
    return source if INCLUDE in source else INCLUDE+source
