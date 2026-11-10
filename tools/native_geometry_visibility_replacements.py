"""Strict complete135970 geometry admission owner authentication."""
import hashlib,re
CONTEXTS={
 'd93471c946cc6acf8f9bdcc8eb9ae4bfa7f7e583c058d6d6b5213857184d9025':{('dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981','fcd36d731dfe0e477cc47f7fb3b63973620fc1db2aab5ad558cdd10e326d2e7e')},
 '7ecf1113c04e355a04826f2fc1c357875b195287c23a5e3af8560f06a2dd541a':{('892cf508106b0c182d59e4dba45804e434a1833003cfb3c322f9a8875c54d7fb','c0e112fb90d7ea32e73e6b3b37d333b6a375456791c5f2eaf9a08906d613f6a9')},
}
PREFIX='\n    if(halo2::native_geometry_visibility_detail::bridge(s,[](auto&child,halo2::native_geometry_visibility_detail::Child which,auto&){const bool frustum=which==halo2::native_geometry_visibility_detail::Child::frustum;dispatch(child,frustum?0x167ef0u:0x166c00u,frustum?0x1359b1u:0x1359f9u,false);}))return;'
INCLUDE='#include "rendering/native_tu15_geometry_visibility.hpp"\n'
BODY=re.compile(r'static void run_135970\(WorldState &s\) \{.*?\n\}',re.S)
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete135970 geometry admission owner')
    match=matches[0];body=canonical_body(match[0]);pin=digest(body)
    if pin not in CONTEXTS:raise ValueError('Complete135970 geometry admission body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher or(digest(resolver[0]),digest(dispatcher[0]))not in CONTEXTS[pin]:raise ValueError('Complete135970 geometry admission context drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,{row[0]for row in CONTEXTS[pin]})
    return match,body
def replace_geometry_visibility(source):
    if not BODY.search(source):return source
    match,body=authenticate(source);opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
    if match[0]!=bound:source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate geometry admission include')
    return source if INCLUDE in source else INCLUDE+source
