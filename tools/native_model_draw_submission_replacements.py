"""Exact full136A80 model section draw-unit submission owner authentication."""
import hashlib,re
CONTEXTS={
 'e409f2bf9b13e7f2e4cdbf1e66ed58997fab6055968a8d4a637c61686ff9407e':{('dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981','fcd36d731dfe0e477cc47f7fb3b63973620fc1db2aab5ad558cdd10e326d2e7e')},
}
# The only callee is 2CBD0 behind the State's required boundary, so the
# native route needs no fallback dispatch.
PREFIX='\n    if(halo2::native_model_draw_submission_detail::bridge(s))return;'
INCLUDE='#include "rendering/native_tu15_model_draw_submission.hpp"\n'
BODY=re.compile(r'static void run_136a80\(WorldState &s\) \{.*?\n\}',re.S)
SITES=201
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    opening=body.index('{')+1
    return body[:opening]+body[opening+len(PREFIX):]if body[opening:].startswith(PREFIX)else body
def authenticate(source):
    matches=list(BODY.finditer(source))
    if len(matches)!=1:raise ValueError('Expected one complete136A80 model draw submission owner')
    match=matches[0];body=canonical_body(match[0]);pin=digest(body)
    if pin not in CONTEXTS:raise ValueError('Complete136A80 model draw submission body drift')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher or(digest(resolver[0]),digest(dispatcher[0]))not in CONTEXTS[pin]:raise ValueError('Complete136A80 model draw submission context drift')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(source,body,{row[0]for row in CONTEXTS[pin]})
    return match,body
def replace_model_draw_submission(source):
    if not BODY.search(source):return source
    match,body=authenticate(source);opening=body.index('{')+1;bound=body[:opening]+PREFIX+body[opening:]
    if match[0]!=bound:source=source[:match.start()]+bound+source[match.end():]
    if source.count(INCLUDE)>1:raise ValueError('Duplicate model draw submission include')
    return source if INCLUDE in source else INCLUDE+source
