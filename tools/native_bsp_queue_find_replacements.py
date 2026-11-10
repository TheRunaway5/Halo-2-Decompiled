"""Authenticate complete166C00 queue membership and public ID-search owners."""
import hashlib,re
BODY_PIN='aeb52e20044182a382593a074a53a870b244c4923389a33fe59a9173553daa16'
# Keep the complete exact resolver spelling, not a selected-pair subset.
RESOLVER_PIN='dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981'
DISPATCH_PIN='fcd36d731dfe0e477cc47f7fb3b63973620fc1db2aab5ad558cdd10e326d2e7e'
PREFIX='\n    if(halo2::native_bsp_queue_find_detail::bridge(s))return;'
INCLUDE='#include "rendering/native_tu15_bsp_queue_find.hpp"\n'
BODY=re.compile(r'static void run_166c00\(WorldState &s\) \{.*?\n\}',re.S)
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete166C00 queue-search owner')
    match=matches[0];body=canonical_body(match[0])
    if digest(body)!=BODY_PIN:raise ValueError('Complete166C00 queue-search body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatch or(digest(resolver[0]),digest(dispatch[0]))!=(RESOLVER_PIN,DISPATCH_PIN):raise ValueError('Complete166C00 selected resolver or dispatcher drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,{RESOLVER_PIN})
    if dispatch[0].count('if(target==selected(0x166c00u,s.image)){run_166c00(s);return;}')!=1:raise ValueError('Missing actual166C00 dispatch identity')
    return match,body
def replace_bsp_queue_find(source):
    if not BODY.search(source):return source
    match,body=authenticate(source)
    if match[0]==body:
        opening=body.index('{')+1;source=source[:match.start()]+body[:opening]+PREFIX+body[opening:]+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate BSP queue-search include')
    if INCLUDE not in source:source=INCLUDE+source
    return source

PUBLIC_INCLUDE='#include "halo2/native_tu15_id_search_kernel.hpp"\n'
PUBLIC={
    'instance':('SourceBspAdmission','std::size_t','instances','8544e99adf37b5f25df8bae65de64ca525b2e0c5c286e483aa175c35cfbe7fa9'),
    'object':('SourceObjectAdmission','std::uint32_t','objects','51105a096f3daccc099e0d2889478cf191635afa7d2d059382699395445790b6'),
}
def public_body(source,name):
    owner,arg,rows,pin=PUBLIC[name];matches=list(re.finditer(r'const SourceBspAdmissionRecord \*'+owner+'::'+name+r'\('+re.escape(arg)+r' id\) const \{.*?\n\}',source,re.S))
    if len(matches)!=1:raise ValueError('Expected one complete public ID-search owner '+name)
    return matches[0][0]
def replace_public(source,name):
    owner,arg,rows,pin=PUBLIC[name];body=public_body(source,name)
    replacement='const SourceBspAdmissionRecord *'+owner+'::'+name+'('+arg+' id) const {\n    return native_id_search_detail::record('+rows+',id);\n}'
    if body!=replacement:
        if digest(body)!=pin:raise ValueError('Complete public ID-search body drift '+name)
        source=source.replace(body,replacement,1)
    if source.count(PUBLIC_INCLUDE)>1:raise ValueError('Duplicate public ID-search include')
    if PUBLIC_INCLUDE not in source:source=PUBLIC_INCLUDE+source
    return source
