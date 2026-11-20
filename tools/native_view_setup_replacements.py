"""Exact full135690 render-view visibility setup owner authentication and default install."""
import hashlib,re
CONTEXTS={
 '237100b008c386cdf99cf2eb224a3751d56076cfbf8f85b076df31a7cae479c1':{('dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981','fcd36d731dfe0e477cc47f7fb3b63973620fc1db2aab5ad558cdd10e326d2e7e')},
}
# 11E600 is entered through the unit's own dispatcher, as the translated body does.
PREFIX='\n    if(halo2::native_view_setup_detail::bridge(s,[](auto&child,std::uint32_t target,std::uint32_t site,auto&){dispatch(child,target,site,false);}))return;'
INCLUDE='#include "rendering/native_tu15_view_setup.hpp"\n'
BODY=re.compile(r'static void run_135690\(WorldState &s\) \{.*?\n\}',re.S)
SITES=120
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete135690 view setup owner')
    match=matches[0];body=canonical_body(match[0]);pin=digest(body)
    if pin not in CONTEXTS:raise ValueError('Complete135690 view setup body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher or(digest(resolver[0]),digest(dispatcher[0]))not in CONTEXTS[pin]:raise ValueError('Complete135690 view setup context drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,{row[0]for row in CONTEXTS[pin]})
    return match,body
def replace_view_setup(source):
    if not BODY.search(source):return source
    match,body=authenticate(source);opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
    if match[0]!=bound:source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate view setup include')
    return source if INCLUDE in source else INCLUDE+source
