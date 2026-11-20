"""Exact full1373F0 render-view queue reset owner authentication and default install."""
import hashlib,re
CONTEXTS={
 '3ecb0a9ac67f906dc05f359a40d7ef55f9c3f8e88f398a7b159089556fc86e4c':{
  'dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981',
 },
 '54dcf7376f93ed547aa0bcb43c998e9d36900d3106f9f8e9a39c1884ee691b00':{
  '892cf508106b0c182d59e4dba45804e434a1833003cfb3c322f9a8875c54d7fb',
 },
}
# 1373F0 is a pure leaf: no callees.
PREFIX='\n    if(halo2::native_view_reset_detail::bridge(s))return;'
INCLUDE='#include "rendering/native_tu15_view_reset.hpp"\n'
BODY=re.compile(r'static void run_1373f0\(WorldState &s\) \{.*?\n\}',re.S)
SITES=35
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete1373F0 view reset owner')
    match=matches[0];body=canonical_body(match[0]);pin=digest(body)
    if pin not in CONTEXTS:raise ValueError('Complete1373F0 view reset body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not resolver or digest(resolver[0])not in CONTEXTS[pin]:raise ValueError('Complete1373F0 view reset context drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,CONTEXTS[pin])
    return match,body
def replace_view_reset(source):
    if not BODY.search(source):return source
    match,body=authenticate(source);opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
    if match[0]!=bound:source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate view reset include')
    return source if INCLUDE in source else INCLUDE+source
