"""Exact full4BAE0 object fade owner authentication and default install."""
import hashlib,re
CONTEXTS={
 '14be5242fc1af001cf7fce35935bbc687a3e55bf359ea534a3cbb9fb5b5341a0':{('dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981','fcd36d731dfe0e477cc47f7fb3b63973620fc1db2aab5ad558cdd10e326d2e7e')},
}
# BCFB0 and _ftol2 (32CC0C) are entered through the unit's own dispatcher,
# as the translated body does.
PREFIX='\n    if(halo2::native_object_fade_detail::bridge(s,[](auto&child,std::uint32_t target,std::uint32_t site,auto&){dispatch(child,target,site,false);}))return;'
INCLUDE='#include "rendering/native_tu15_object_fade.hpp"\n'
BODY=re.compile(r'static void run_4bae0\(WorldState &s\) \{.*?\n\}',re.S)
SITES=127
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete4BAE0 object fade owner')
    match=matches[0];body=canonical_body(match[0]);pin=digest(body)
    if pin not in CONTEXTS:raise ValueError('Complete4BAE0 object fade body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher or(digest(resolver[0]),digest(dispatcher[0]))not in CONTEXTS[pin]:raise ValueError('Complete4BAE0 object fade context drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,{row[0]for row in CONTEXTS[pin]})
    return match,body
def replace_object_fade(source):
    if not BODY.search(source):return source
    match,body=authenticate(source);opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
    if match[0]!=bound:source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate object fade include')
    return source if INCLUDE in source else INCLUDE+source
