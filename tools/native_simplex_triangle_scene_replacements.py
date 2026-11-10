"""Authenticate the actual-PC SceneConstructor copy of the whole triangle."""
import hashlib
import re

BODY=re.compile(r'static void run_30d3f0\(WorldState &s\) \{.*?\n\}',re.S)
PREFIX='\n    if(halo2::native_simplex_triangle_detail::bridge_actual(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_simplex_triangle_abi.hpp"\n'
BODY_PIN='58304c0b69420262430721364b93df0813d25a09a240df0bf261ee832c7da01e'
RESOLVER_PIN='618bc56800beb42be63e189783dd7cf9a98d7b904e56fa92de6523bf86490d7a'
DISPATCHER_PIN='1a68d583722bac0a950594a436cbb9f1b28b09624517589c9ee8ad507712f605'
PREPARED_CONTEXT=('955621384ce0f6fb97adbd8a71b455b4aff7eebd1d7234ab9ae8321ca44aa6c0',RESOLVER_PIN,'30d34d2c58b7adae1e2a3132d26e1da47e7278086c1e5ad7904fdfb58626241f')

def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    at=body.index('{')+1
    return body[:at]+body[at+len(PREFIX):]if body[at:].startswith(PREFIX)else body
def context(source):
    body=list(BODY.finditer(source))
    if len(body)!=1:raise ValueError('Missing complete Scene simplex-triangle owner')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing Scene triangle dispatch context')
    return body[0],resolver[0],dispatcher[0]
def authenticate(source):
    if not BODY.search(source):
        if PREFIX in source or INCLUDE in source:raise ValueError('Missing complete Scene simplex-triangle owner')
        return None
    match,resolver,dispatcher=context(source);body=canonical_body(match[0])
    if digest(body)!=BODY_PIN:raise ValueError('Complete Scene simplex-triangle body drift')
    if digest(resolver)!=RESOLVER_PIN or digest(dispatcher)!=DISPATCHER_PIN:raise ValueError('Complete Scene triangle resolver or dispatcher drift')
    if source.count(INCLUDE)>1 or (PREFIX in match[0])!=(INCLUDE in source):raise ValueError('Incomplete Scene triangle overlay')
    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)}
    operands={int(a,16)for a in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',body)}
    if len(operands)!=16 or any(mapping.get(value)!=value for value in operands):raise ValueError('Scene triangle identity operand mapping drift')
    return match,body
def replace_simplex_triangle_scene(source):
    found=authenticate(source)
    if not found:return source
    match,body=found;at=body.index('{')+1
    source=source[:match.start()]+body[:at]+PREFIX+body[at:]+source[match.end():]
    return source if INCLUDE in source else INCLUDE+source
def remove_simplex_triangle_scene(source):
    found=authenticate(source)
    if not found:return source
    match,body=found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    match,resolver,dispatcher=context(source)
    if source.count(INCLUDE)!=1 or match[0].count(PREFIX)!=1:raise ValueError('Missing complete Scene triangle overlay')
    if tuple(digest(v)for v in(match[0],resolver,dispatcher))!=PREPARED_CONTEXT:authenticate(source)
    return ['30d3f0']
