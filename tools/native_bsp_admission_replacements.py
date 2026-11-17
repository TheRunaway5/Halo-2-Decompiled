"""Exact full136EA0 BSP visibility controller owner authentication."""
import hashlib,re
CONTEXTS={
 '1fc7838d8c537d77c5935a78652f6c203ade61af00585bd084425a4b786f015d':{('dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981','fcd36d731dfe0e477cc47f7fb3b63973620fc1db2aab5ad558cdd10e326d2e7e')},
 '6811c5b82e28b056480e35315d05ccc20226c7b7c837e41d7bd9b725c89bf0e0':{('892cf508106b0c182d59e4dba45804e434a1833003cfb3c322f9a8875c54d7fb','c0e112fb90d7ea32e73e6b3b37d333b6a375456791c5f2eaf9a08906d613f6a9')},
}
PREFIX='\n    if(halo2::native_bsp_admission_detail::bridge(s,[](auto&child,halo2::native_bsp_admission_detail::Child which,auto&){using Child=halo2::native_bsp_admission_detail::Child;const auto target=which==Child::geometry?0x135970u:which==Child::association?0x136e10u:which==Child::geometry_frustum?0x167ef0u:which==Child::geometry_auxiliary?0x166c00u:0x171070u;const auto site=which==Child::geometry?0x1371abu:which==Child::association?0x1372c5u:which==Child::geometry_frustum?0x1359b1u:which==Child::geometry_auxiliary?0x1359f9u:which==Child::cluster_fog?0x137091u:which==Child::instance_fog?0x137259u:0x13739au;dispatch(child,target,site,false);}))return;'
INCLUDE='#include "rendering/native_tu15_bsp_admission.hpp"\n'
BODY=re.compile(r'static void run_136ea0\(WorldState &s\) \{.*?\n\}',re.S)
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def spellings(literal):
    selected=lambda value:f'0x{value:x}u'if literal else f'selected(0x{value:x}u,s.image)'
    shifts=(
      (f's.set(3,s.flags.shift(s.reg(3,4,0),s.reg(1,1,0),4),4,0);','halo2::native_bsp_admission_detail::retained_cluster_bit(s);'),
      (f's.set(6,s.flags.shift(s.reg(6,4,0),{selected(1)},4),4,0);','halo2::native_bsp_admission_detail::retained_double(s);'),
      ('s.set(2,s.flags.shift(s.reg(2,4,0),s.reg(1,1,0),4),4,0);','halo2::native_bsp_admission_detail::retained_association_bit(s);'))
    pairs=list(shifts)
    for pc,register,offset,width,mask,increment in((0x137100,0,4,2,0,True),(0x137312,0,4,2,0,True),(0x13738b,0,0,4,2,False),(0x1373ab,6,1,1,4,False)):
        address=f'({selected(offset)}+s.gp[{register}])';prior=f'r(s.memory,{address},{width})'
        operation=f's.flags.increment({prior},{width},false)'if increment else f's.flags.logic(({prior})|({"s.reg(2,4,0)"if mask==2 else selected(mask)}),{width})'
        old=f'w(s.memory,s.observe,0x{pc:x}u,{address},{width},{operation});'
        new=f'halo2::native_bsp_admission_detail::retained_rmw(s,0x{pc:x}u,{address},{width},{"0u"if increment else "s.reg(2,4,0)"if mask==2 else "4u"},{str(increment).lower()});'
        pairs.append((old,new))
    return tuple(pairs)
def ordered_reference(body):
    result=body
    for old,new in spellings('selected('not in body):
        if result.count(old)!=1:raise ValueError('Exact136EA0 retained primitive missing '+old)
        result=result.replace(old,new,1)
    for old,new in(('_mm_add_ss(', 'halo2::native_bsp_admission_detail::retained_add(s,'),('_mm_sub_ss(', 'halo2::native_bsp_admission_detail::retained_subtract(s,'),('_mm_mul_ss(', 'halo2::native_bsp_admission_detail::retained_multiply(s,')):
        result=result.replace(old,new)
    return result
def canonical_body(body):
    opening=body.index('{')+1
    if body[opening:].startswith(PREFIX):body=body[:opening]+body[opening+len(PREFIX):]
    literal='selected('not in body
    for old,new in spellings(literal):
        if new in body:
            if body.count(new)!=1:raise ValueError('Duplicate136EA0 fallback primitive')
            body=body.replace(new,old,1)
    for old,new in(('_mm_add_ss(', 'halo2::native_bsp_admission_detail::retained_add(s,'),('_mm_sub_ss(', 'halo2::native_bsp_admission_detail::retained_subtract(s,'),('_mm_mul_ss(', 'halo2::native_bsp_admission_detail::retained_multiply(s,')):
        body=body.replace(new,old)
    return body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete136EA0 owner')
    match=matches[0];body=canonical_body(match[0]);pin=digest(body)
    if pin not in CONTEXTS:raise ValueError('Complete136EA0 body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher or(digest(resolver[0]),digest(dispatcher[0]))not in CONTEXTS[pin]:raise ValueError('Complete136EA0 context drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,{row[0]for row in CONTEXTS[pin]})
    return match,body
def replace_bsp_admission(source):
    if not BODY.search(source):return source
    match,body=authenticate(source);ordered=ordered_reference(body);opening=ordered.index('{')+1;bound=ordered[:opening]+PREFIX+ordered[opening:]
    if match[0]!=bound:source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate136EA0 include')
    return source if INCLUDE in source else INCLUDE+source
