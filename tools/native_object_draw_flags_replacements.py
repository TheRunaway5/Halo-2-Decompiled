"""Exact full135E00 render-object draw flags owner authentication and default install."""
import hashlib,re
CONTEXTS={
 '01e33c0f5f84ff83ea22dd97711f156fc5ea527af9d213c9237b14ed215bce2b':{('dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981','fcd36d731dfe0e477cc47f7fb3b63973620fc1db2aab5ad558cdd10e326d2e7e')},
}
PREFIX='\n    if(halo2::native_object_draw_flags_detail::bridge(s))return;'
INCLUDE='#include "rendering/native_tu15_object_draw_flags.hpp"\n'
BODY=re.compile(r'static void run_135e00\(WorldState &s\) \{.*?\n\}',re.S)
SITES=47
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete135E00 object draw flags owner')
    match=matches[0];body=canonical_body(match[0]);pin=digest(body)
    if pin not in CONTEXTS:raise ValueError('Complete135E00 object draw flags body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher or(digest(resolver[0]),digest(dispatcher[0]))not in CONTEXTS[pin]:raise ValueError('Complete135E00 object draw flags context drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,{row[0]for row in CONTEXTS[pin]})
    return match,body
def replace_object_draw_flags(source):
    if not BODY.search(source):return source
    match,body=authenticate(source);opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
    if match[0]!=bound:source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate object draw flags include')
    return source if INCLUDE in source else INCLUDE+source
