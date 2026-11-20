"""Exact fullBBD10 object parent-hierarchy attached predicate owner authentication and default install."""
import hashlib,re
CONTEXTS={
 'd172508db4b53046a0a2fea880967003ec23ef2377ab303f2743d96ae7028b08':{
  'dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981',
  '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
 },
 '12431dc93389ec265834e077e6334e892efafcec64e89d4e12abeb4d9251b3c9':{
  '1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4',
 },
}
# BBD10 is a pure leaf: no callees.
PREFIX='\n    if(halo2::native_object_hierarchy_root_detail::bridge(s))return;'
INCLUDE='#include "rendering/native_tu15_object_hierarchy_root.hpp"\n'
BODY=re.compile(r'static void run_bbd10\(WorldState &s\) \{.*?\n\}',re.S)
SITES=19
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one completeBBD10 object hierarchy root owner')
    match=matches[0];body=canonical_body(match[0]);pin=digest(body)
    if pin not in CONTEXTS:raise ValueError('CompleteBBD10 object hierarchy root body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not resolver or digest(resolver[0])not in CONTEXTS[pin]:raise ValueError('CompleteBBD10 object hierarchy root context drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,CONTEXTS[pin])
    return match,body
def replace_object_hierarchy_root(source):
    if not BODY.search(source):return source
    match,body=authenticate(source);opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
    if match[0]!=bound:source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate object hierarchy root include')
    return source if INCLUDE in source else INCLUDE+source
