"""Exact fullC3AB0 light volume shape owner authentication and default install."""
import hashlib,re
CONTEXTS={
 '57cb71c616c429f69bc65c54d926e848d981ea4839537b97daef44ea709949a1':{
  'dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981',
  '9d38403bc71ed842b7551a91b5760b0c45fc056b3b09e724d8d82e2784a5ac13',
 },
 'f6c764bece6da8a31ddb73cf4f1c3ead40e6a4800fbdd7e419ee53d51cf761de':{
  '6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9',
 },
}
# C3AB0 is a pure leaf: no callees.
PREFIX='\n    if(halo2::native_light_shape_detail::bridge(s))return;'
INCLUDE='#include "rendering/native_tu15_light_shape.hpp"\n'
BODY=re.compile(r'static void run_c3ab0\(WorldState &s\) \{.*?\n\}',re.S)
SITES=194
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one completeC3AB0 light shape owner')
    match=matches[0];body=canonical_body(match[0]);pin=digest(body)
    if pin not in CONTEXTS:raise ValueError('CompleteC3AB0 light shape body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not resolver or digest(resolver[0])not in CONTEXTS[pin]:raise ValueError('CompleteC3AB0 light shape context drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,CONTEXTS[pin])
    return match,body
def replace_light_shape(source):
    if not BODY.search(source):return source
    match,body=authenticate(source);opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
    if match[0]!=bound:source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate light shape include')
    return source if INCLUDE in source else INCLUDE+source
