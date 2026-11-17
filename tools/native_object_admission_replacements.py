"""Exact full135B60 render-object admission owner authentication."""
import hashlib,re
CONTEXTS={
 'c50fe20e2266eefaa092ac7b7c7098daa0a738906defd02a5669fc2fa0c4d4cc':{('dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981','fcd36d731dfe0e477cc47f7fb3b63973620fc1db2aab5ad558cdd10e326d2e7e')},
}
PREFIX='\n    if(halo2::native_object_admission_detail::bridge(s,[](auto&child,halo2::native_object_admission_detail::Child which,auto&){using Child=halo2::native_object_admission_detail::Child;const auto target=which==Child::geometry?0x135970u:which==Child::association?0x136e10u:which==Child::geometry_frustum?0x167ef0u:0x166c00u;const auto site=which==Child::geometry?0x135c02u:which==Child::association?0x135c9bu:which==Child::geometry_frustum?0x1359b1u:0x1359f9u;dispatch(child,target,site,false);}))return;'
INCLUDE='#include "rendering/native_tu15_object_admission.hpp"\n'
BODY=re.compile(r'static void run_135b60\(WorldState &s\) \{.*?\n\}',re.S)
SITES=209
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete135B60 object admission owner')
    match=matches[0];body=canonical_body(match[0]);pin=digest(body)
    if pin not in CONTEXTS:raise ValueError('Complete135B60 object admission body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher or(digest(resolver[0]),digest(dispatcher[0]))not in CONTEXTS[pin]:raise ValueError('Complete135B60 object admission context drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,{row[0]for row in CONTEXTS[pin]})
    return match,body
def replace_object_admission(source):
    if not BODY.search(source):return source
    match,body=authenticate(source);opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
    if match[0]!=bound:source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate object admission include')
    return source if INCLUDE in source else INCLUDE+source
